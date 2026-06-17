using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Achievements;
using TheLastMage.Gameplay.Commands;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.UI;

public partial class AchievementPanel : Control
{
    private static readonly Vector2 PanelSize = new(820f, 620f);
    private const float Margin = 24f;
    private const string AccentColor = "#d2ad64";
    private const string CategoryColor = "#7fc8ff";
    private const string MutedColor = "#a8b3ad";
    private const string ValueColor = "#f1eee5";

    private readonly List<ContentId> _mageIds = new();
    private readonly List<ContentId> _achievementIds = new();

    private RunControllerNode? _runController;
    private PanelContainer? _rootPanel;
    private OptionButton? _mageOptions;
    private OptionButton? _achievementOptions;
    private Label? _status;
    private RichTextLabel? _summary;
    private bool _panelOpen;

    public override void _Ready()
    {
        SetAnchorsPreset(LayoutPreset.FullRect);
        MouseFilter = MouseFilterEnum.Ignore;
        BuildUi();
        SetPanelOpen(false);
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event.IsActionPressed("debug_toggle_meta_panel"))
        {
            SetPanelOpen(!_panelOpen);
            GetViewport().SetInputAsHandled();
        }
    }

    public override void _Process(double delta)
    {
        PositionPanel();
        RefreshContext();
        RefreshSummary();
    }

    private void BuildUi()
    {
        _rootPanel = UiStyle.CreatePanel("MetaProgressionDebugPanel");
        _rootPanel.Size = PanelSize;
        _rootPanel.CustomMinimumSize = PanelSize;
        _rootPanel.MouseFilter = MouseFilterEnum.Stop;
        _rootPanel.AddThemeStyleboxOverride("panel", CreateOpaquePanelBox());
        PositionPanel();
        AddChild(_rootPanel);

        var margin = UiStyle.AddMargin(_rootPanel, 16);
        var root = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        root.AddThemeConstantOverride("separation", 10);
        margin.AddChild(root);

        root.AddChild(UiStyle.CreateLabel("Meta Progression Debug", 22, UiStyle.PanelAccent));
        _status = UiStyle.CreateLabel("", 14, UiStyle.TextMuted);
        root.AddChild(_status);

        _mageOptions = new OptionButton { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        var selectMage = UiStyle.CreateButton("Select Mage");
        selectMage.Pressed += SelectMage;
        var unlockMage = UiStyle.CreateButton("Unlock Mage");
        unlockMage.Pressed += DebugUnlockMage;

        _achievementOptions = new OptionButton { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        var completeAchievement = UiStyle.CreateButton("Complete Achievement");
        completeAchievement.Pressed += DebugCompleteAchievement;

        var controls = new GridContainer
        {
            Columns = 2,
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        controls.AddThemeConstantOverride("h_separation", 10);
        controls.AddChild(BuildGroup("Mages", _mageOptions, selectMage, unlockMage));
        controls.AddChild(BuildGroup("Achievements", _achievementOptions, completeAchievement));
        root.AddChild(controls);

        _summary = new RichTextLabel
        {
            BbcodeEnabled = true,
            FitContent = false,
            ScrollActive = true,
            SelectionEnabled = true,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            MouseFilter = MouseFilterEnum.Stop
        };
        _summary.AddThemeFontSizeOverride("normal_font_size", 15);
        _summary.AddThemeColorOverride("default_color", UiStyle.TextPrimary);
        _summary.AddThemeConstantOverride("line_separation", 4);
        root.AddChild(_summary);
    }

    private static Control BuildGroup(string title, params Control[] controls)
    {
        var panel = UiStyle.CreatePanel($"{title}DebugGroup");
        panel.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        panel.CustomMinimumSize = new Vector2(0f, 138f);

        var margin = UiStyle.AddMargin(panel, 10);
        var group = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        group.AddThemeConstantOverride("separation", 6);
        margin.AddChild(group);

        group.AddChild(UiStyle.CreateLabel(title, 16, UiStyle.PanelAccent));
        foreach (var control in controls)
        {
            control.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            group.AddChild(control);
        }

        return panel;
    }

    private void RefreshContext()
    {
        _runController ??= GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        if (_mageIds.Count != context.Content.Mages.Count)
        {
            FillOptions(_mageOptions, _mageIds, context.Content.Mages.Keys.OrderBy(id => id.ToString()));
        }

        if (_achievementIds.Count != context.Content.Achievements.Count)
        {
            FillOptions(_achievementOptions, _achievementIds, context.Content.Achievements.Keys.OrderBy(id => id.ToString()));
        }

        if (_panelOpen)
        {
            context.State.IsDebugUiInputCaptured = true;
        }
    }

    private static void FillOptions(OptionButton? options, List<ContentId> ids, IEnumerable<ContentId> source)
    {
        if (options == null)
        {
            return;
        }

        ids.Clear();
        options.Clear();
        foreach (var id in source)
        {
            ids.Add(id);
            options.AddItem(id.ToString());
        }
    }

    private void RefreshSummary()
    {
        var context = _runController?.Context;
        if (context == null || _summary == null)
        {
            return;
        }

        if (_status != null)
        {
            _status.Text = $"{context.State.SelectedMageName} ({context.State.SelectedMageId})   Mages {context.Profile.UnlockedMageIds.Count}/{context.Content.Mages.Count}   Items {context.Profile.UnlockedItemIds.Count}/{context.Content.Items.Count}   Achievements {context.Profile.CompletedAchievementIds.Count}/{context.Content.Achievements.Count}";
        }

        var lines = new List<string>
        {
            Section("Mages")
        };

        foreach (var mage in context.Content.Mages.Values.OrderBy(mage => mage.Id.ToString()))
        {
            var unlocked = context.Profile.UnlockedMageIds.Contains(mage.Id.Value, StringComparer.Ordinal) ? "unlocked" : "locked";
            var selected = mage.Id.Equals(context.State.SelectedMageId) ? " selected" : string.Empty;
            var spells = string.Join(", ", mage.StartingSpellIds.Select(id => id.ToString()));
            lines.Add(Row(
                mage.DisplayName,
                Pair("state", $"{unlocked}{selected}".Trim()),
                Pair("hp", $"{mage.MaxHealth:0.#}"),
                Pair("move", $"{mage.MoveSpeed:0.##}"),
                Pair("fire", $"{mage.FireRate:0.##}"),
                Pair("luck", $"{mage.Luck:0.##}"),
                Pair("spells", spells)));
        }

        lines.Add(string.Empty);
        lines.Add(Section("Achievements"));
        foreach (var achievement in context.Content.Achievements.Values.OrderBy(achievement => achievement.Id.ToString()))
        {
            var complete = context.Profile.CompletedAchievementIds.Contains(achievement.Id.Value, StringComparer.Ordinal) ? "complete" : "open";
            var mageRewards = achievement.UnlockMageIds.Count == 0 ? "-" : string.Join(", ", achievement.UnlockMageIds);
            var itemRewards = achievement.UnlockItemIds.Count == 0 ? "-" : string.Join(", ", achievement.UnlockItemIds);
            lines.Add(Row(
                achievement.DisplayName,
                Pair("state", complete),
                Pair("req", achievement.RequirementKey),
                Pair("mages", mageRewards),
                Pair("items", itemRewards)));
        }

        _summary.Text = string.Join('\n', lines);
    }

    private void SelectMage()
    {
        var context = _runController?.Context;
        if (context == null || !TryGetSelectedMage(out var mageId))
        {
            return;
        }

        context.Commands.TryExecute(new SelectMageCommand(mageId), context, out _);
        RefreshSummary();
    }

    private void DebugUnlockMage()
    {
        var context = _runController?.Context;
        if (context == null || !TryGetSelectedMage(out var mageId))
        {
            return;
        }

        if (!context.Profile.UnlockedMageIds.Contains(mageId.Value, StringComparer.Ordinal))
        {
            context.Profile.UnlockedMageIds.Add(mageId.Value);
            context.SaveProfile(context.Profile);
        }

        context.Commands.TryExecute(new SelectMageCommand(mageId, allowLocked: true), context, out _);
        RefreshSummary();
    }

    private void DebugCompleteAchievement()
    {
        var context = _runController?.Context;
        if (context == null || _achievementOptions == null)
        {
            return;
        }

        var index = _achievementOptions.Selected;
        if (index < 0 || index >= _achievementIds.Count)
        {
            return;
        }

        context.GetSystem<AchievementSystem>().CompleteAchievement(_achievementIds[index]);
        RefreshSummary();
    }

    private bool TryGetSelectedMage(out ContentId mageId)
    {
        mageId = default;
        if (_mageOptions == null)
        {
            return false;
        }

        var index = _mageOptions.Selected;
        if (index < 0 || index >= _mageIds.Count)
        {
            return false;
        }

        mageId = _mageIds[index];
        return true;
    }

    private void SetPanelOpen(bool open)
    {
        _panelOpen = open;
        if (_rootPanel != null)
        {
            _rootPanel.Visible = open;
        }

        MouseFilter = open ? MouseFilterEnum.Stop : MouseFilterEnum.Ignore;
        var context = _runController?.Context;
        if (context != null)
        {
            context.State.IsDebugUiInputCaptured = open;
        }

        Input.MouseMode = open
            ? Input.MouseModeEnum.Visible
            : context != null
                ? Input.MouseModeEnum.Captured
                : Input.MouseModeEnum.Visible;
    }

    private void PositionPanel()
    {
        if (_rootPanel == null)
        {
            return;
        }

        var viewportSize = GetViewportRect().Size;
        var size = new Vector2(
            MathF.Min(PanelSize.X, MathF.Max(360f, viewportSize.X - Margin * 2f)),
            MathF.Min(PanelSize.Y, MathF.Max(420f, viewportSize.Y - Margin * 2f)));
        _rootPanel.Size = size;
        _rootPanel.Position = (viewportSize - size) * 0.5f;
    }

    private static StyleBoxFlat CreateOpaquePanelBox()
    {
        return new StyleBoxFlat
        {
            BgColor = new Color(0.055f, 0.066f, 0.072f, 0.98f),
            BorderColor = UiStyle.PanelBorder,
            BorderWidthLeft = 1,
            BorderWidthTop = 1,
            BorderWidthRight = 1,
            BorderWidthBottom = 1,
            CornerRadiusTopLeft = 6,
            CornerRadiusTopRight = 6,
            CornerRadiusBottomLeft = 6,
            CornerRadiusBottomRight = 6,
            ContentMarginLeft = 8,
            ContentMarginTop = 6,
            ContentMarginRight = 8,
            ContentMarginBottom = 6
        };
    }

    private static string Section(string text)
    {
        return $"[font_size=17][color={AccentColor}][b]{EscapeBbcode(text)}[/b][/color][/font_size]";
    }

    private static string Row(string category, params string[] pairs)
    {
        return $"[color={CategoryColor}][b]{EscapeBbcode(category)}[/b][/color]  {string.Join("   ", pairs)}";
    }

    private static string Pair(string label, string value)
    {
        return $"[color={MutedColor}]{EscapeBbcode(label)}[/color] [color={ValueColor}]{EscapeBbcode(value)}[/color]";
    }

    private static string EscapeBbcode(string value)
    {
        return string.IsNullOrEmpty(value) ? "-" : value.Replace("[", "[lb]", StringComparison.Ordinal);
    }
}
