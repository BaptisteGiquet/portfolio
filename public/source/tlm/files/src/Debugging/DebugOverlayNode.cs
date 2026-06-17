using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Debugging;

public partial class DebugOverlayNode : CanvasLayer
{
    private const string AccentColor = "#d2ad64";
    private const string CategoryColor = "#7fc8ff";
    private const string MutedColor = "#a8b3ad";
    private const string ValueColor = "#f1eee5";
    private const string WarningColor = "#ff9d67";

    private Control? _panel;
    private RichTextLabel? _label;
    private Control? _combatPanel;
    private Label? _combatLabel;
    private RunControllerNode? _runController;
    private bool _expanded;

    [Export] public bool StartVisible { get; set; }

    public override void _Ready()
    {
        Layer = 0;
        _panel = GetNodeOrNull<Control>("Panel");
        _label = GetNodeOrNull<RichTextLabel>("Panel/Label");
        _combatPanel = GetNodeOrNull<Control>("CombatPanel");
        _combatLabel = GetNodeOrNull<Label>("CombatPanel/Label");
        ConfigureDebugLabel();
        if (_panel != null)
        {
            SetMouseFilterRecursive(_panel, Control.MouseFilterEnum.Ignore);
            _panel.AnchorLeft = 0.58f;
            _panel.AnchorTop = 0.01f;
            _panel.AnchorRight = 0.99f;
            _panel.AnchorBottom = 0.24f;
            _panel.OffsetLeft = 0f;
            _panel.OffsetTop = 0f;
            _panel.OffsetRight = 0f;
            _panel.OffsetBottom = 0f;
        }

        if (_combatPanel != null)
        {
            _combatPanel.Visible = false;
        }

        Visible = StartVisible;
        UpdateText();
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event.IsActionPressed("debug_toggle_overlay"))
        {
            if (@event is InputEventKey key && key.ShiftPressed)
            {
                _expanded = !_expanded;
                Visible = true;
            }
            else
            {
                Visible = !Visible;
            }

            GetViewport().SetInputAsHandled();
        }
    }

    public override void _Process(double delta)
    {
        UpdateText();
    }

    private void UpdateText()
    {
        if (_label == null)
        {
            return;
        }

        _runController ??= GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        var context = _runController?.Context;
        if (context == null)
        {
            _label.Text = $"[color={AccentColor}][b]The Last Mage Debug Overlay[/b][/color]\n[color={MutedColor}]No active RunContext[/color]";
            if (_combatLabel != null)
            {
                _combatLabel.Text = string.Empty;
            }

            return;
        }

        if (_panel != null)
        {
            _panel.AnchorBottom = _expanded ? 0.68f : 0.27f;
        }

        _label.Text = _expanded ? BuildExpandedText(context) : BuildCompactText(context);
        if (_combatLabel != null)
        {
            _combatLabel.Text = string.Empty;
        }
    }

    private void ConfigureDebugLabel()
    {
        if (_label == null)
        {
            return;
        }

        _label.BbcodeEnabled = true;
        _label.FitContent = false;
        _label.ScrollActive = false;
        _label.SelectionEnabled = false;
        _label.AutowrapMode = TextServer.AutowrapMode.WordSmart;
        _label.MouseFilter = Control.MouseFilterEnum.Ignore;
        _label.AddThemeFontSizeOverride("normal_font_size", 15);
        _label.AddThemeColorOverride("default_color", new Color(0.92f, 0.92f, 0.86f));
        _label.AddThemeConstantOverride("line_separation", 3);
    }

    private static string BuildCompactText(RunContext context)
    {
        var metrics = context.State.DebugMetrics;
        var lines = new List<string>
        {
            Header("Debug Overlay"),
            Row("Runtime", Pair("frame", $"{metrics.FrameMilliseconds:0.00}ms"), Pair("AI", $"{metrics.EnemyAiMilliseconds:0.00}ms"), Pair("alloc", metrics.AllocatedBytesThisFrame.ToString())),
            Row("Actors", Pair("enemies", metrics.ActiveEnemies.ToString()), Pair("projectiles", metrics.ActiveProjectiles.ToString()), Pair("AoE", metrics.ActiveAoEs.ToString()), Pair("status", metrics.ActiveStatuses.ToString()), Pair("summons", metrics.ActiveSummons.ToString())),
            Row("Combat", Pair("damage/frame", metrics.DamageEventsThisFrame.ToString()), Pair("last", metrics.LastDamageSummary)),
            Row("Protection", Pair("blocked/frame", metrics.DamagePreventionsThisFrame.ToString()), Pair("last", metrics.LastDamagePreventionSummary)),
            Row("Spell radius", Pair("last", metrics.LastSpellRadiusSummary)),
            Row("Cast flow", Pair("last", metrics.LastCastFlowSummary)),
            Row("Run", Pair("mode", metrics.LastRunModeSummary), Pair("summary", metrics.LastRunSummary, metrics.LastRunSummary != "-")),
            Row("Mage", Pair("last hit", metrics.LastMageDamageSummary), Pair("death", metrics.DeathCauseSummary, metrics.DeathCauseSummary != "-")),
            Row("Activatable", Pair("last", metrics.LastActivatableItemSummary)),
            Row("Proc safety", Pair("used", metrics.ProcBudgetUsedThisFrame.ToString()), Pair("blocked", metrics.ProcBudgetBlockedThisFrame.ToString(), metrics.ProcBudgetBlockedThisFrame > 0)),
            Row("Feedback", Pair("requests", metrics.FeedbackRequestsThisFrame.ToString()), Pair("drops", metrics.FeedbackBudgetDropsThisFrame.ToString(), metrics.FeedbackBudgetDropsThisFrame > 0), Pair("active", metrics.ActiveFeedbackBudgetSummary)),
            Row("Queries", Pair("path", metrics.PathQueriesThisFrame.ToString()), Pair("attack", metrics.AttackChecksThisFrame.ToString()), Pair("separation", metrics.SeparationChecksThisFrame.ToString()))
        };

        return string.Join('\n', lines);
    }

    private static string BuildExpandedText(RunContext context)
    {
        var metrics = context.State.DebugMetrics;
        var lines = new List<string>
        {
            Header("Debug Details"),
            Row("Runtime", Pair("frame", $"{metrics.FrameMilliseconds:0.00}ms"), Pair("AI", $"{metrics.EnemyAiMilliseconds:0.00}ms"), Pair("alloc", metrics.AllocatedBytesThisFrame.ToString())),
            Row("Profile", Pair("summary", metrics.ProfilingSummary)),
            Row("Repro", Pair("run", metrics.ReproductionSummary)),
            Row("Actors", Pair("enemies", metrics.ActiveEnemies.ToString()), Pair("projectiles", metrics.ActiveProjectiles.ToString()), Pair("AoE", metrics.ActiveAoEs.ToString()), Pair("status", metrics.ActiveStatuses.ToString()), Pair("summons", metrics.ActiveSummons.ToString())),
            Row("Combat", Pair("damage/frame", metrics.DamageEventsThisFrame.ToString()), Pair("last", metrics.LastDamageSummary)),
            Row("Protection", Pair("blocked/frame", metrics.DamagePreventionsThisFrame.ToString()), Pair("last", metrics.LastDamagePreventionSummary)),
            Row("Spell radius", Pair("last", metrics.LastSpellRadiusSummary)),
            Row("Cast flow", Pair("last", metrics.LastCastFlowSummary)),
            Row("Run", Pair("mode", metrics.LastRunModeSummary), Pair("summary", metrics.LastRunSummary, metrics.LastRunSummary != "-")),
            Row("Mage", Pair("last hit", metrics.LastMageDamageSummary), Pair("death", metrics.DeathCauseSummary, metrics.DeathCauseSummary != "-")),
            Row("Activatable", Pair("last", metrics.LastActivatableItemSummary)),
            Row("Proc safety", Pair("used", metrics.ProcBudgetUsedThisFrame.ToString()), Pair("blocked", metrics.ProcBudgetBlockedThisFrame.ToString(), metrics.ProcBudgetBlockedThisFrame > 0)),
            Row("Feedback", Pair("requests", metrics.FeedbackRequestsThisFrame.ToString()), Pair("drops", metrics.FeedbackBudgetDropsThisFrame.ToString(), metrics.FeedbackBudgetDropsThisFrame > 0), Pair("active", metrics.ActiveFeedbackBudgetSummary)),
            Row("Queries", Pair("path", metrics.PathQueriesThisFrame.ToString()), Pair("attack", metrics.AttackChecksThisFrame.ToString()), Pair("separation", metrics.SeparationChecksThisFrame.ToString())),
            Section("World"),
            Row("Tower", Pair("state", metrics.TowerRouteState), Pair("entrance", metrics.TowerEntranceState)),
            Row("Route", Pair("path", metrics.TowerRouteSummary)),
            Row("Stages", Pair("route", metrics.TowerRouteStageSummary)),
            Row("Wave", Pair("spawned", metrics.WaveSpawnedThisDay.ToString()), Pair("last lane", metrics.LastSpawnLaneId)),
            Row("Lanes", Pair("active", metrics.SpawnLaneSummary)),
            Row("Placement", Pair("last", metrics.LastDefensePlacementValidation)),
            Section("Content"),
            Row("Audio", Pair("groups", metrics.AudioMixSummary)),
            Row("Profile", Pair("mages", $"{context.Profile.UnlockedMageIds.Count}/{context.Content.Mages.Count}"), Pair("items", $"{context.Profile.UnlockedItemIds.Count}/{context.Content.Items.Count}"), Pair("discovered", context.Profile.DiscoveredItemIds.Count.ToString()), Pair("achievements", $"{context.Profile.CompletedAchievementIds.Count}/{context.Content.Achievements.Count}")),
            Row("Run Stats", Pair("normal", context.Profile.RunStatistics.TotalNormalRuns.ToString()), Pair("wins", context.Profile.RunStatistics.Victories.ToString()), Pair("excluded", context.Profile.RunStatistics.DebugOrTestRunsExcluded.ToString())),
            Row("Reports", Pair("benchmark", metrics.BenchmarkReportPath)),
            Row("Reports", Pair("run", metrics.RunReportPath)),
            Row("Reports", Pair("balance", metrics.BalanceReportPath))
        };

        return string.Join('\n', lines);
    }

    private static string Header(string text)
    {
        return $"[font_size=17][color={AccentColor}][b]{EscapeBbcode(text)}[/b][/color][/font_size]";
    }

    private static string Section(string text)
    {
        return $"\n[color={AccentColor}][b]{EscapeBbcode(text)}[/b][/color]";
    }

    private static string Row(string category, params string[] pairs)
    {
        return $"[color={CategoryColor}][b]{EscapeBbcode(category)}[/b][/color]  {string.Join("   ", pairs)}";
    }

    private static string Pair(string label, string value, bool warning = false)
    {
        var valueColor = warning ? WarningColor : ValueColor;
        return $"[color={MutedColor}]{EscapeBbcode(label)}[/color] [color={valueColor}]{EscapeBbcode(value)}[/color]";
    }

    private static string EscapeBbcode(string value)
    {
        return string.IsNullOrEmpty(value) ? "-" : value.Replace("[", "[lb]", StringComparison.Ordinal);
    }

    private static string BuildAttributeLine(RunContext context)
    {
        var health = PlayerAttributeResolver.ResolveValue(
            context,
            PlayerAttributeResolver.Health,
            context.State.Player.BaseMaxHealth,
            recordBreakdown: false);
        var moveSpeed = PlayerAttributeResolver.ResolveValue(
            context,
            PlayerAttributeResolver.MoveSpeed,
            context.State.Player.BaseMoveSpeed,
            recordBreakdown: false);
        var damage = PlayerAttributeResolver.ResolveDamage(context, 10f, Array.Empty<TagId>(), recordBreakdown: false);
        var fireRate = PlayerAttributeResolver.ResolveFireRate(context, Array.Empty<TagId>(), recordBreakdown: false);
        var range = PlayerAttributeResolver.ResolveRange(context, 10f, Array.Empty<TagId>(), recordBreakdown: false);
        var duration = PlayerAttributeResolver.ResolveDuration(context, 10f, Array.Empty<TagId>(), recordBreakdown: false);
        var luck = PlayerAttributeResolver.ResolveValue(
            context,
            PlayerAttributeResolver.Luck,
            context.State.Player.BaseLuck,
            recordBreakdown: false);

        return $"Attributes: Health {health:0.#}  Move {moveSpeed:0.##}  Damage {damage:0.##}/10  FireRate {fireRate:0.##}x  Range {range:0.##}/10  Luck {luck:0.##}  Duration {duration:0.##}/10";
    }

    private static void SetMouseFilterRecursive(Control control, Control.MouseFilterEnum mouseFilter)
    {
        control.MouseFilter = mouseFilter;
        foreach (var child in control.GetChildren())
        {
            if (child is Control childControl)
            {
                SetMouseFilterRecursive(childControl, mouseFilter);
            }
        }
    }
}
