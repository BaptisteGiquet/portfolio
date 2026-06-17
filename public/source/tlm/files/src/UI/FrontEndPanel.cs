using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;
using TheLastMage.Gameplay.Run;
using TheLastMage.Localization;
using TheLastMage.Save;

namespace TheLastMage.UI;

public partial class FrontEndPanel : Control
{
    private const string CatalogPath = "res://data/catalogs/content_catalog.tres";
    private const string RunRootScene = "res://scenes/run/run_root.tscn";

    private readonly List<Button> _focusButtons = new();
    private SaveServiceNode? _saveService;
    private ContentRegistry _content = new();
    private PanelContainer? _panel;
    private VBoxContainer? _contentRoot;
    private string _view = "slots";
    private int _selectedSlot = 1;
    private bool _deleteSlotMode;

    public override void _Ready()
    {
        SetAnchorsPreset(LayoutPreset.FullRect);
        MouseFilter = MouseFilterEnum.Stop;
        _saveService = GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        LoadContent();
        BuildShell();
        ShowSlotPicker();
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event.IsActionPressed("ui_cancel"))
        {
            if (_view == "slots")
            {
                if (_deleteSlotMode)
                {
                    _deleteSlotMode = false;
                    ShowSlotPicker();
                    GetViewport().SetInputAsHandled();
                }

                return;
            }

            if (_view == "main")
            {
                return;
            }

            ShowMainMenu();
            GetViewport().SetInputAsHandled();
        }
    }

    public override void _Process(double delta)
    {
        CenterPanel();
    }

    private void LoadContent()
    {
        var catalog = ResourceLoader.Load<ContentCatalog>(CatalogPath) ?? new ContentCatalog();
        LocalizationService.Current.LoadEnglish(catalog, _saveService?.Settings.Locale ?? LocalizationService.DefaultLocale);
        _content = new ContentRegistry();
        var validation = _content.Load(catalog);
        foreach (var issue in validation.Issues)
        {
            if (issue.Severity == ValidationSeverity.Error)
            {
                GD.PushError($"{issue.Code}: {issue.Message}");
            }
            else
            {
                GD.PushWarning($"{issue.Code}: {issue.Message}");
            }
        }
    }

    private void BuildShell()
    {
        _panel = UiStyle.CreatePanel("FrontEndPanel");
        _panel.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        _panel.SizeFlagsVertical = SizeFlags.ExpandFill;
        AddChild(_panel);
        CenterPanel();

        var margin = UiStyle.AddMargin(_panel, 22);
        _contentRoot = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        _contentRoot.AddThemeConstantOverride("separation", 12);
        margin.AddChild(_contentRoot);
    }

    private void ShowMainMenu()
    {
        _view = "main";
        BeginView("ui.frontend.title", "ui.frontend.main_subtitle");
        AddRoot(UiStyle.CreateLabel(Format("ui.frontend.active_slot", _selectedSlot), 15, UiStyle.TextMuted));

        if (RunCheckpointService.CanUseCheckpoint(CurrentProfile().SuspendedRun))
        {
            AddRoot(UiStyle.CreateLabel(
                Format("ui.frontend.suspended_run_summary", RunCheckpointService.Describe(CurrentProfile().SuspendedRun!)),
                15,
                UiStyle.TextMuted));
            AddMenuButton("ui.frontend.continue", ContinueSuspendedRun);
        }
        else
        {
            AddMenuButton("ui.frontend.new_run", ShowMageSelection);
        }

        AddMenuButton("ui.frontend.stats", ShowStats);
        AddMenuButton("ui.frontend.options", ShowOptions);
        AddMenuButton("ui.frontend.credits", ShowCredits);
        AddMenuButton("ui.frontend.feedback", ShowFeedback);
        AddMenuButton("ui.frontend.quit", () => GetTree().Quit());
        FocusFirstButton();
    }

    private void ShowMageSelection()
    {
        _view = "mages";
        BeginView("ui.frontend.mage_selection", "ui.frontend.mage_selection_subtitle");

        var scroll = CreateScroll();
        var list = new VBoxContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        list.AddThemeConstantOverride("separation", 8);
        scroll.AddChild(list);
        AddRoot(scroll);

        var profile = CurrentProfile();
        foreach (var mage in FrontEndViewModels.BuildMageSelection(_content, profile))
        {
            var button = UiStyle.CreateButton(Format("ui.frontend.mage_card",
                mage.DisplayName,
                mage.IsUnlocked ? Loc("ui.frontend.unlocked") : Loc("ui.frontend.locked"),
                mage.StartingSpell,
                mage.MaxHealth,
                mage.MoveSpeed,
                mage.FireRate,
                mage.Luck,
                mage.Stats.RunsPlayed,
                mage.Stats.Wins,
                mage.Stats.Deaths,
                mage.Stats.BestHumanityKilled,
                FormatTime(mage.Stats.BestTimeSurvivedSeconds),
                mage.IsUnlocked ? mage.Description : Format("ui.frontend.unlock_requirement", mage.UnlockRequirement)));
            button.Disabled = !mage.IsUnlocked;
            var mageId = mage.MageId;
            button.Pressed += () => StartRunWithMage(mageId);
            list.AddChild(button);
            _focusButtons.Add(button);
        }

        AddBackButton();
        FocusFirstButton();
    }

    private void ShowSlotPicker()
    {
        _view = "slots";
        BeginView("ui.frontend.choose_save", _deleteSlotMode ? "ui.frontend.delete_slot_subtitle" : "ui.frontend.choose_save_subtitle");

        var summaries = _saveService?.LoadSlotSummaries() ?? Array.Empty<ProfileSlotSummaryDto>();
        var grid = new GridContainer
        {
            Columns = 1,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        grid.AddThemeConstantOverride("v_separation", 8);
        AddRoot(grid);

        foreach (var summary in summaries)
        {
            var button = UiStyle.CreateButton(Format("ui.frontend.slot_card",
                summary.SlotIndex,
                summary.Exists ? Loc("ui.frontend.slot_in_use") : Loc("ui.frontend.slot_empty"),
                summary.RunsPlayed,
                summary.Wins,
                summary.Deaths,
                summary.HighestDayReached,
                summary.HumanityKilled,
                summary.EnemiesKilled,
                summary.ItemsDiscovered,
                summary.AchievementsCompleted));
            var slotIndex = summary.SlotIndex;
            button.Pressed += () => ChooseOrDeleteSlot(slotIndex);
            grid.AddChild(button);
            _focusButtons.Add(button);
        }

        AddSaveSelectionActions();
        FocusFirstButton();
    }

    private void ChooseOrDeleteSlot(int slotIndex)
    {
        _selectedSlot = slotIndex;
        if (_deleteSlotMode)
        {
            var summary = _saveService?.LoadSlotSummary(slotIndex);
            GD.Print($"FrontEndSlotDeleteChoose slot={slotIndex} exists={summary?.Exists == true}");
            if (summary?.Exists == true)
            {
                _saveService?.DeleteProfileSlot(slotIndex);
            }

            _deleteSlotMode = false;
            ShowSlotPicker();
            return;
        }

        GD.Print($"FrontEndSlotChoose slot={slotIndex}");
        _saveService?.LoadProfileSlot(slotIndex);
        ShowMainMenu();
    }

    private void AddSaveSelectionActions()
    {
        var row = new HBoxContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        row.AddThemeConstantOverride("separation", 8);
        AddRoot(row);

        if (_deleteSlotMode)
        {
            var cancel = UiStyle.CreateButton(Loc("ui.frontend.cancel"));
            cancel.Pressed += () =>
            {
                GD.Print("FrontEndSlotDeleteModeCancel");
                _deleteSlotMode = false;
                ShowSlotPicker();
            };
            row.AddChild(cancel);
            _focusButtons.Add(cancel);
            return;
        }

        var delete = UiStyle.CreateButton(Loc("ui.frontend.delete_save"));
        delete.Pressed += () =>
        {
            GD.Print("FrontEndSlotDeleteModeStart");
            _deleteSlotMode = true;
            ShowSlotPicker();
        };
        row.AddChild(delete);
        _focusButtons.Add(delete);
    }

    private void ShowStats()
    {
        _view = "stats";
        BeginView("ui.frontend.stats", "ui.frontend.stats_subtitle");

        var profile = CurrentProfile();
        var stats = FrontEndViewModels.BuildProfileStats(_content, profile);
        AddRoot(UiStyle.CreateLabel(Format("ui.frontend.player_stats",
            stats.RunsPlayed,
            stats.Wins,
            stats.Deaths,
            FormatTime(stats.TimeSurvivedSeconds),
            stats.EnemiesKilled,
            stats.HumanityKilled,
            stats.ItemsDiscovered,
            stats.MagesUnlocked,
            stats.TotalSpellCasts), 16));

        var spellText = stats.SpellUsage.Count == 0
            ? Loc("ui.frontend.no_spell_usage")
            : string.Join('\n', stats.SpellUsage.Select(pair => Format("ui.frontend.spell_usage_line", ResolveSpellName(pair.Key), pair.Value)));
        AddRoot(UiStyle.CreateLabel(spellText, 15, UiStyle.TextMuted));

        var achievements = new RichTextLabel
        {
            BbcodeEnabled = true,
            FitContent = false,
            ScrollActive = true,
            SelectionEnabled = false,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            MouseFilter = MouseFilterEnum.Stop
        };
        achievements.AddThemeFontSizeOverride("normal_font_size", 15);
        achievements.AddThemeColorOverride("default_color", UiStyle.TextPrimary);
        achievements.Text = string.Join('\n', FrontEndViewModels.BuildAchievements(_content, profile).Select(FormatAchievement));
        AddRoot(achievements);

        AddBackButton();
        FocusFirstButton();
    }

    private void ShowOptions()
    {
        _view = "options";
        BeginView("ui.frontend.options", "ui.frontend.options_subtitle");
        var options = new OptionsMenuPanel();
        options.Configure(_saveService, ShowMainMenu);
        AddRoot(options);
        options.CallDeferred(nameof(OptionsMenuPanel.GrabInitialFocus));
    }

    private void AddSettingButton(string key, Action pressed)
    {
        AddMenuButton(key, () =>
        {
            pressed();
            ShowOptions();
        });
    }

    private void CycleDisplayMode()
    {
        var settings = CurrentSettings();
        settings.DisplayMode = NextValue(settings.DisplayMode, new[] { "windowed", "fullscreen", "borderless" });
        SaveSettings(settings);
    }

    private void CycleResolution()
    {
        var settings = CurrentSettings();
        settings.Resolution = NextValue(settings.Resolution, new[] { "1280x720", "1600x900", "1920x1080", "2560x1440", "3840x2160" });
        SaveSettings(settings);
    }

    private void ToggleVSync()
    {
        var settings = CurrentSettings();
        settings.VSync = !settings.VSync;
        SaveSettings(settings);
    }

    private void CycleFpsCap()
    {
        var settings = CurrentSettings();
        settings.FpsCap = NextValue(settings.FpsCap, new[] { 0, 30, 60, 90, 120, 144, 240 });
        SaveSettings(settings);
    }

    private void CycleRenderScale()
    {
        var settings = CurrentSettings();
        settings.RenderScale = NextValue(settings.RenderScale, new[] { 0.5f, 0.65f, 0.8f, 1f, 1.25f });
        SaveSettings(settings);
    }

    private void CycleMasterVolume()
    {
        var settings = CurrentSettings();
        settings.MasterVolume = NextValue(settings.MasterVolume, new[] { 0f, 0.25f, 0.5f, 0.75f, 1f });
        SaveSettings(settings);
    }

    private void CycleMouseSensitivity()
    {
        var settings = CurrentSettings();
        settings.MouseSensitivity = NextValue(settings.MouseSensitivity, new[] { 0.5f, 0.75f, 1f, 1.25f, 1.5f, 2f });
        SaveSettings(settings);
    }

    private void CycleGamepadSensitivity()
    {
        var settings = CurrentSettings();
        settings.GamepadSensitivity = NextValue(settings.GamepadSensitivity, new[] { 0.5f, 0.75f, 1f, 1.25f, 1.5f, 2f });
        SaveSettings(settings);
    }

    private void ShowCredits()
    {
        _view = "credits";
        BeginView("ui.frontend.credits", "ui.frontend.credits_subtitle");
        AddRoot(UiStyle.CreateLabel(Loc("ui.frontend.credits_body"), 16));
        AddBackButton();
        FocusFirstButton();
    }

    private void ShowFeedback()
    {
        _view = "feedback";
        BeginView("ui.frontend.feedback", "ui.frontend.feedback_subtitle");
        AddRoot(UiStyle.CreateLabel(Loc("ui.frontend.feedback_body"), 16));
        AddBackButton();
        FocusFirstButton();
    }

    private void StartRunWithMage(ContentId mageId)
    {
        var saveService = _saveService;
        if (saveService == null)
        {
            return;
        }

        var profile = saveService.Profile;
        if (!profile.UnlockedMageIds.Contains(mageId.Value, StringComparer.Ordinal))
        {
            return;
        }

        profile.SuspendedRun = null;
        profile.PreferredMageId = mageId.Value;
        saveService.SaveProfile(profile);
        GetTree().ChangeSceneToFile(RunRootScene);
    }

    private void ContinueSuspendedRun()
    {
        if (!RunCheckpointService.CanUseCheckpoint(CurrentProfile().SuspendedRun))
        {
            ShowMageSelection();
            return;
        }

        GetTree().ChangeSceneToFile(RunRootScene);
    }

    private void BeginView(string titleKey, string subtitleKey)
    {
        ClearContent();
        AddRoot(UiStyle.CreateLabel(Loc(titleKey), 28, UiStyle.PanelAccent));
        AddRoot(UiStyle.CreateLabel(Loc(subtitleKey), 15, UiStyle.TextMuted));
    }

    private void AddMenuButton(string key, Action pressed)
    {
        var button = UiStyle.CreateButton(Loc(key));
        button.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        button.Pressed += pressed;
        AddRoot(button);
        _focusButtons.Add(button);
    }

    private void AddBackButton()
    {
        AddMenuButton("ui.frontend.back", ShowMainMenu);
    }

    private void AddRoot(Control control)
    {
        _contentRoot?.AddChild(control);
    }

    private ScrollContainer CreateScroll()
    {
        return new ScrollContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            FollowFocus = true,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled
        };
    }

    private void ClearContent()
    {
        _focusButtons.Clear();
        if (_contentRoot == null)
        {
            return;
        }

        foreach (var child in _contentRoot.GetChildren())
        {
            _contentRoot.RemoveChild(child);
            child.QueueFree();
        }
    }

    private void FocusFirstButton()
    {
        if (_focusButtons.Count > 0)
        {
            _focusButtons[0].GrabFocus();
        }
    }

    private void CenterPanel()
    {
        if (_panel == null)
        {
            return;
        }

        var viewport = GetViewportRect().Size;
        _panel.Size = viewport;
        _panel.CustomMinimumSize = viewport;
        _panel.Position = Vector2.Zero;
    }

    private ProfileSaveDto CurrentProfile()
    {
        return _saveService?.Profile ?? new ProfileSaveDto();
    }

    private SettingsSaveDto CurrentSettings()
    {
        return _saveService?.Settings ?? new SettingsSaveDto();
    }

    private void SaveSettings(SettingsSaveDto settings)
    {
        _saveService?.SaveSettings(settings);
    }

    private static string NextValue(string current, IReadOnlyList<string> values)
    {
        var index = values
            .Select((value, valueIndex) => new { value, valueIndex })
            .FirstOrDefault(pair => string.Equals(pair.value, current, StringComparison.OrdinalIgnoreCase))
            ?.valueIndex ?? -1;
        return values[(index + 1) % values.Count];
    }

    private static int NextValue(int current, IReadOnlyList<int> values)
    {
        var index = -1;
        for (var i = 0; i < values.Count; i++)
        {
            if (values[i] == current)
            {
                index = i;
                break;
            }
        }

        return values[(index + 1) % values.Count];
    }

    private static float NextValue(float current, IReadOnlyList<float> values)
    {
        var index = -1;
        for (var i = 0; i < values.Count; i++)
        {
            if (Math.Abs(values[i] - current) < 0.01f)
            {
                index = i;
                break;
            }
        }

        return values[(index + 1) % values.Count];
    }

    private string ResolveSpellName(string spellId)
    {
        var id = ContentId.From(spellId);
        return _content.Spells.TryGetValue(id, out var spell) ? spell.DisplayName : spellId;
    }

    private static string FormatAchievement(FrontEndAchievementViewModel achievement)
    {
        var state = achievement.IsComplete ? "complete" : "open";
        return $"[b]{EscapeBbcode(achievement.DisplayName)}[/b]  {EscapeBbcode(state)}  {EscapeBbcode(achievement.Requirement)}  {EscapeBbcode(achievement.Unlocks)}";
    }

    private static string FormatTime(float seconds)
    {
        var span = TimeSpan.FromSeconds(Math.Max(0f, seconds));
        return span.TotalHours >= 1.0
            ? $"{(int)span.TotalHours:0}:{span.Minutes:00}:{span.Seconds:00}"
            : $"{span.Minutes:0}:{span.Seconds:00}";
    }

    private static string Loc(string key)
    {
        return LocalizationService.Current.Text(key);
    }

    private static string Format(string key, params object?[] args)
    {
        return LocalizationService.Current.Text(key, args);
    }

    private static string EscapeBbcode(string value)
    {
        return value.Replace("[", "[lb]", StringComparison.Ordinal).Replace("]", "[rb]", StringComparison.Ordinal);
    }
}
