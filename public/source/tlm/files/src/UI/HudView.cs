using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Commands;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Localization;

namespace TheLastMage.UI;

public partial class HudView : Control
{
    private readonly Vector3[] _barricadePositions =
    {
        new(0f, 0f, -4.75f),
        new(-1.25f, 0f, -4.5f),
        new(1.25f, 0f, -4.5f)
    };

    private readonly Vector3[] _sealPositions =
    {
        new(0f, 0f, -9f),
        new(-3.5f, 0f, -8f),
        new(3.5f, 0f, -9.8f)
    };

    private Label? _statusLabel;
    private Label? _promptLabel;
    private ProgressBar? _healthBar;
    private Label? _healthBarLabel;
    private ProgressBar[] _cooldownBars = Array.Empty<ProgressBar>();
    private Label[] _cooldownLabels = Array.Empty<Label>();
    private Button? _placeBarricadeButton;
    private Button? _placeSealButton;
    private Button? _repairButton;
    private Button? _startNextDayButton;
    private RunControllerNode? _runController;
    private RunPhase? _lastFocusedPhase;

    public override void _Ready()
    {
        SetAnchorsPreset(LayoutPreset.FullRect);
        MouseFilter = MouseFilterEnum.Ignore;

        var root = new VBoxContainer
        {
            AnchorLeft = 0.01f,
            AnchorTop = 0.01f,
            AnchorRight = 0.34f,
            AnchorBottom = 0.38f,
            MouseFilter = MouseFilterEnum.Ignore
        };
        root.AddThemeConstantOverride("separation", 6);
        AddChild(root);

        var panel = UiStyle.CreatePanel("HudStatusPanel");
        root.AddChild(panel);
        var panelMargin = UiStyle.AddMargin(panel, 10);
        var panelRoot = new VBoxContainer();
        panelRoot.AddThemeConstantOverride("separation", 6);
        panelMargin.AddChild(panelRoot);

        _statusLabel = UiStyle.CreateLabel(fontSize: 14);
        panelRoot.AddChild(_statusLabel);

        _healthBar = UiStyle.CreateBar(UiStyle.Health);
        panelRoot.AddChild(_healthBar);
        _healthBarLabel = UiStyle.CreateLabel(fontSize: 12);
        _healthBarLabel.HorizontalAlignment = HorizontalAlignment.Center;
        _healthBarLabel.VerticalAlignment = VerticalAlignment.Center;
        _healthBarLabel.SetAnchorsPreset(LayoutPreset.FullRect);
        _healthBarLabel.OffsetLeft = 0f;
        _healthBarLabel.OffsetTop = 0f;
        _healthBarLabel.OffsetRight = 0f;
        _healthBarLabel.OffsetBottom = 0f;
        _healthBar?.AddChild(_healthBarLabel);

        BuildCooldownRows(panelRoot);

        _promptLabel = UiStyle.CreateLabel(fontSize: 13, color: UiStyle.TextMuted);
        panelRoot.AddChild(_promptLabel);

        var actions = new HBoxContainer
        {
            MouseFilter = MouseFilterEnum.Stop
        };
        actions.AddThemeConstantOverride("separation", 6);
        panelRoot.AddChild(actions);

        var loc = LocalizationService.Current;
        _placeBarricadeButton = AddActionButton(actions, loc.Text("ui.hud.place_barricade"), PlaceBarricade);
        _placeSealButton = AddActionButton(actions, loc.Text("ui.hud.place_seal"), PlaceSeal);
        _repairButton = AddActionButton(actions, loc.Text("ui.hud.repair"), RepairFirstDefense);
        _startNextDayButton = AddActionButton(actions, loc.Text("ui.hud.next_day"), StartNextDay);
    }

    public override void _Process(double delta)
    {
        _runController ??= GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        var context = _runController?.Context;
        if (context == null)
        {
            Visible = false;
            return;
        }

        Visible = true;
        UpdateView(context);
    }

    private void UpdateView(RunContext context)
    {
        var viewModel = RunViewModels.BuildHud(context);

        if (_statusLabel != null)
        {
            var loc = LocalizationService.Current;
            _statusLabel.Text = IsTerminalPhase(viewModel.Phase)
                ? loc.Text(
                    "ui.hud.status_terminal",
                    viewModel.Phase,
                    viewModel.Day,
                    viewModel.Souls,
                    viewModel.Materials,
                    viewModel.Humanity,
                    context.State.DebugMetrics.LastRunSummary)
                : loc.Text(
                    "ui.hud.status_active",
                    viewModel.Phase,
                    viewModel.Day,
                    viewModel.Souls,
                    viewModel.Materials,
                    viewModel.Humanity,
                    viewModel.ActivatableItemName,
                    viewModel.ActivatableUsesText,
                    viewModel.WaveState,
                    viewModel.ActiveEnemies,
                    viewModel.TargetEnemies,
                    viewModel.ActiveDefenses);
        }

        if (_healthBar != null)
        {
            _healthBar.Value = viewModel.HealthMax <= 0f ? 0f : Math.Clamp(viewModel.HealthCurrent / viewModel.HealthMax, 0f, 1f);
        }

        if (_healthBarLabel != null)
        {
            _healthBarLabel.Text = LocalizationService.Current.Text("ui.hud.health", viewModel.HealthCurrent, viewModel.HealthMax);
        }

        UpdateCooldownRows(viewModel);
        ApplyTextScale(context.Settings.TextScale);

        if (_promptLabel != null)
        {
            _promptLabel.Text = LocalizationService.Current.Text(
                "ui.hud.prompt_with_readability",
                viewModel.Prompt,
                viewModel.ReadabilitySummary);
        }

        var canBuild = context.State.PhasePermissions.CanPlaceDefense;
        var canRepair = context.State.PhasePermissions.CanRepairDefense;
        var canStart = context.State.PhasePermissions.CanStartNextDay;
        SetButtonState(_placeBarricadeButton, canBuild);
        SetButtonState(_placeSealButton, canBuild);
        SetButtonState(_repairButton, canRepair && HasDamagedDefense(context));
        SetButtonState(_startNextDayButton, canStart);
        UpdateFocusForPhase(context.State.CurrentPhase);
    }

    private static bool IsTerminalPhase(RunPhase phase)
    {
        return phase is RunPhase.RunVictory or RunPhase.RunDefeat;
    }

    private static Button AddActionButton(Container parent, string text, Action action)
    {
        var button = UiStyle.CreateButton(text);
        button.CustomMinimumSize = new Vector2(92f, 32f);
        button.Pressed += action;
        parent.AddChild(button);
        return button;
    }

    private static void SetButtonState(Button? button, bool enabled)
    {
        if (button != null)
        {
            button.Disabled = !enabled;
            button.Visible = enabled;
        }
    }

    private void UpdateFocusForPhase(RunPhase phase)
    {
        if (_lastFocusedPhase == phase)
        {
            return;
        }

        _lastFocusedPhase = phase;
        if (phase == RunPhase.NightDefense)
        {
            CallDeferred(nameof(GrabNightDefenseFocus));
        }
    }

    private void GrabNightDefenseFocus()
    {
        if (_startNextDayButton is { Visible: true, Disabled: false })
        {
            _startNextDayButton.GrabFocus();
            return;
        }

        foreach (var button in new[] { _placeBarricadeButton, _placeSealButton, _repairButton })
        {
            if (button is { Visible: true, Disabled: false })
            {
                button.GrabFocus();
                return;
            }
        }
    }

    private void BuildCooldownRows(Container parent)
    {
        _cooldownBars = new ProgressBar[SpellbookState.MaxSlots];
        _cooldownLabels = new Label[SpellbookState.MaxSlots];
        for (var index = 0; index < SpellbookState.MaxSlots; index++)
        {
            var row = new HBoxContainer();
            row.AddThemeConstantOverride("separation", 6);
            parent.AddChild(row);
            _cooldownLabels[index] = UiStyle.CreateLabel("-", 12, UiStyle.TextPrimary);
            _cooldownLabels[index].CustomMinimumSize = new Vector2(128f, 0f);
            row.AddChild(_cooldownLabels[index]);
            _cooldownBars[index] = UiStyle.CreateBar(UiStyle.Cooldown);
            _cooldownBars[index].SizeFlagsHorizontal = SizeFlags.ExpandFill;
            row.AddChild(_cooldownBars[index]);
        }
    }

    private void UpdateCooldownRows(HudViewModel viewModel)
    {
        for (var index = 0; index < _cooldownBars.Length && index < viewModel.Spells.Count; index++)
        {
            var spell = viewModel.Spells[index];
            _cooldownLabels[index].Text = LocalizationService.Current.Text(
                "ui.hud.cooldown_slot",
                spell.IsSelected ? "*" : " ",
                spell.SlotNumber,
                spell.DisplayName);
            if (!spell.HasSpell)
            {
                _cooldownBars[index].Value = 0f;
                continue;
            }

            var ready = spell.IsReady || spell.CooldownTotalSeconds <= 0f;
            _cooldownBars[index].Value = ready
                ? 1f
                : Math.Clamp(1f - (spell.CooldownRemainingSeconds / spell.CooldownTotalSeconds), 0f, 1f);
        }
    }

    private void ApplyTextScale(float textScale)
    {
        var scale = Math.Clamp(textScale, 0.85f, 1.35f);
        _statusLabel?.AddThemeFontSizeOverride("font_size", Mathf.RoundToInt(14f * scale));
        _promptLabel?.AddThemeFontSizeOverride("font_size", Mathf.RoundToInt(13f * scale));
        _healthBarLabel?.AddThemeFontSizeOverride("font_size", Mathf.RoundToInt(12f * scale));
        foreach (var label in _cooldownLabels)
        {
            label.AddThemeFontSizeOverride("font_size", Mathf.RoundToInt(12f * scale));
        }
    }

    private void PlaceBarricade()
    {
        ExecuteDefensePlacement(ContentId.From("barricade"), _barricadePositions);
    }

    private void PlaceSeal()
    {
        ExecuteDefensePlacement(ContentId.From("explosive_seal"), _sealPositions);
    }

    private void ExecuteDefensePlacement(ContentId defenseId, IReadOnlyList<Vector3> positions)
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        var index = context.GetSystem<DefenseSystem>().ActiveCount % positions.Count;
        context.Commands.TryExecute(new PlaceDefenseCommand(defenseId, positions[index]), context, out _);
    }

    private void RepairFirstDefense()
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        var combat = context.GetSystem<CombatSystem>();
        foreach (var defense in context.GetSystem<DefenseSystem>().Defenses)
        {
            if (combat.TryGetHealth(defense.EntityId, out var health)
                && health != null
                && !health.IsDead
                && health.CurrentHealth < health.MaxHealth)
            {
                context.Commands.TryExecute(new RepairDefenseCommand(defense.EntityId, health.MaxHealth), context, out _);
                return;
            }
        }
    }

    private void StartNextDay()
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        context.Commands.TryExecute(new StartNextDayCommand(), context, out _);
    }

    private static bool HasDamagedDefense(RunContext context)
    {
        var combat = context.GetSystem<CombatSystem>();
        foreach (var defense in context.GetSystem<DefenseSystem>().Defenses)
        {
            if (combat.TryGetHealth(defense.EntityId, out var health)
                && health != null
                && !health.IsDead
                && health.CurrentHealth < health.MaxHealth)
            {
                return true;
            }
        }

        return false;
    }
}
