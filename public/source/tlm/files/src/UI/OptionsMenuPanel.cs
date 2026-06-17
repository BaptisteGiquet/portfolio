using Godot;
using TheLastMage.Controls;
using TheLastMage.Localization;
using TheLastMage.Save;

namespace TheLastMage.UI;

public partial class OptionsMenuPanel : Control
{
    private static readonly Vector2 RowSize = new(0f, 48f);
    private static readonly Color RowNormal = new(0.055f, 0.085f, 0.095f, 0.82f);
    private static readonly Color RowFocus = new(0.12f, 0.18f, 0.20f, 0.98f);

    private SaveServiceNode? _saveService;
    private Action? _backRequested;
    private VBoxContainer? _root;
    private HBoxContainer? _tabs;
    private VBoxContainer? _rows;
    private Label? _descriptionTitle;
    private Label? _descriptionBody;
    private Button? _backButton;
    private SettingsTab _activeTab = SettingsTab.Display;
    private string? _pendingRebindAction;
    private bool _loading;

    public void Configure(SaveServiceNode? saveService, Action? backRequested)
    {
        _saveService = saveService ?? GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        _backRequested = backRequested;
    }

    public override void _Ready()
    {
        _saveService ??= GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        SetAnchorsPreset(LayoutPreset.FullRect);
        SizeFlagsHorizontal = SizeFlags.ExpandFill;
        SizeFlagsVertical = SizeFlags.ExpandFill;
        MouseFilter = MouseFilterEnum.Stop;
        BuildUi();
    }

    public override void _Input(InputEvent @event)
    {
        if (!Visible)
        {
            return;
        }

        if (_pendingRebindAction != null)
        {
            TryFinishRebind(@event);
            return;
        }

        if (IsCategorySwitchInput(@event, JoyButton.RightShoulder))
        {
            SelectRelativeTab(1);
            GetViewport().SetInputAsHandled();
            return;
        }

        if (IsCategorySwitchInput(@event, JoyButton.LeftShoulder))
        {
            SelectRelativeTab(-1);
            GetViewport().SetInputAsHandled();
        }
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (!Visible)
        {
            return;
        }

        if (@event.IsActionPressed(InputActions.UiCancel) || @event.IsActionPressed(InputActions.CancelAction))
        {
            RequestBack();
            GetViewport().SetInputAsHandled();
        }
    }

    public void GrabInitialFocus()
    {
        var focus = FindFirstFocusable(_rows) ?? FindFirstFocusable(_tabs) ?? _backButton;
        focus?.GrabFocus();
    }

    private void BuildUi()
    {
        _root = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        _root.SetAnchorsPreset(LayoutPreset.FullRect);
        _root.AddThemeConstantOverride("separation", 12);
        AddChild(_root);

        var header = new HBoxContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        header.AddThemeConstantOverride("separation", 10);
        _root.AddChild(header);

        header.AddChild(CreateTabHint("ui.options.lb_hint"));
        _tabs = new HBoxContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        _tabs.AddThemeConstantOverride("separation", 8);
        header.AddChild(_tabs);
        header.AddChild(CreateTabHint("ui.options.rb_hint"));

        var body = new HBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        body.AddThemeConstantOverride("separation", 18);
        _root.AddChild(body);

        var scroll = new ScrollContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            FollowFocus = true,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled
        };
        scroll.CustomMinimumSize = new Vector2(560f, 0f);
        body.AddChild(scroll);

        _rows = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ShrinkBegin
        };
        _rows.AddThemeConstantOverride("separation", 4);
        scroll.AddChild(_rows);

        var details = UiStyle.CreatePanel("OptionsDetails");
        details.CustomMinimumSize = new Vector2(330f, 0f);
        details.SizeFlagsHorizontal = SizeFlags.Fill;
        details.SizeFlagsVertical = SizeFlags.ExpandFill;
        body.AddChild(details);

        var detailsMargin = UiStyle.AddMargin(details, 18);
        var detailsRoot = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        detailsRoot.AddThemeConstantOverride("separation", 10);
        detailsMargin.AddChild(detailsRoot);

        _descriptionTitle = UiStyle.CreateLabel("", 20, UiStyle.PanelAccent);
        detailsRoot.AddChild(_descriptionTitle);
        _descriptionBody = UiStyle.CreateLabel("", 15, UiStyle.TextPrimary);
        detailsRoot.AddChild(_descriptionBody);

        var spacer = new Control { SizeFlagsVertical = SizeFlags.ExpandFill };
        detailsRoot.AddChild(spacer);
        detailsRoot.AddChild(UiStyle.CreateLabel(LocalizationService.Current.Text("ui.options.footer_hint"), 13, UiStyle.TextMuted));

        _backButton = UiStyle.CreateButton(LocalizationService.Current.Text("ui.frontend.back"));
        _backButton.SizeFlagsHorizontal = SizeFlags.Fill;
        _backButton.Pressed += RequestBack;
        _root.AddChild(_backButton);

        RebuildTabs();
        RebuildRows();
    }

    private void RebuildTabs()
    {
        if (_tabs == null)
        {
            return;
        }

        ClearChildren(_tabs);
        foreach (var tab in Tabs)
        {
            var button = UiStyle.CreateButton(LocalizationService.Current.Text(TabLabelKey(tab)));
            button.ToggleMode = true;
            button.ButtonPressed = tab == _activeTab;
            button.FocusMode = FocusModeEnum.None;
            button.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            var selected = tab;
            button.Pressed += () => SelectTab(selected);
            _tabs.AddChild(button);
        }
    }

    private static Label CreateTabHint(string key)
    {
        var label = UiStyle.CreateLabel(LocalizationService.Current.Text(key), 13, UiStyle.TextMuted);
        label.CustomMinimumSize = new Vector2(34f, 0f);
        label.HorizontalAlignment = HorizontalAlignment.Center;
        return label;
    }

    private static bool IsCategorySwitchInput(InputEvent @event, JoyButton button)
    {
        return @event is InputEventJoypadButton { Pressed: true } joypadButton && joypadButton.ButtonIndex == button;
    }

    private void RebuildRows()
    {
        if (_rows == null)
        {
            return;
        }

        _loading = true;
        ClearChildren(_rows);
        switch (_activeTab)
        {
            case SettingsTab.Display:
                AddDropdownRow("ui.options.display_mode", "ui.options.display_mode_desc", DisplayModes, CurrentSettings().DisplayMode, value => UpdateSettings(settings => settings.DisplayMode = value));
                AddDropdownRow("ui.options.resolution", "ui.options.resolution_desc", Resolutions, CurrentSettings().Resolution, value => UpdateSettings(settings => settings.Resolution = value));
                AddChoiceRow("ui.options.vsync", "ui.options.vsync_desc", BoolChoices, CurrentSettings().VSync ? "on" : "off", value => UpdateSettings(settings => settings.VSync = value == "on"));
                AddChoiceRow("ui.options.fps_cap", "ui.options.fps_cap_desc", FpsChoices, FpsToValue(CurrentSettings().FpsCap), value => UpdateSettings(settings => settings.FpsCap = ValueToFps(value)));
                break;
            case SettingsTab.Graphics:
                AddChoiceRow("ui.options.render_scale", "ui.options.render_scale_desc", RenderScaleChoices, FloatToValue(CurrentSettings().RenderScale), value => UpdateSettings(settings => settings.RenderScale = ValueToFloat(value, settings.RenderScale)));
                break;
            case SettingsTab.Gameplay:
                AddChoiceRow("ui.options.mouse_sensitivity", "ui.options.mouse_sensitivity_desc", SensitivityChoices, FloatToValue(CurrentSettings().MouseSensitivity), value => UpdateSettings(settings => settings.MouseSensitivity = ValueToFloat(value, settings.MouseSensitivity)));
                AddChoiceRow("ui.options.gamepad_sensitivity", "ui.options.gamepad_sensitivity_desc", SensitivityChoices, FloatToValue(CurrentSettings().GamepadSensitivity), value => UpdateSettings(settings => settings.GamepadSensitivity = ValueToFloat(value, settings.GamepadSensitivity)));
                break;
            case SettingsTab.Sound:
                AddChoiceRow("ui.options.master_volume", "ui.options.master_volume_desc", VolumeChoices, FloatToValue(CurrentSettings().MasterVolume), value => UpdateSettings(settings => settings.MasterVolume = ValueToFloat(value, settings.MasterVolume)));
                break;
            case SettingsTab.Language:
                AddDropdownRow("ui.options.language", "ui.options.language_desc", BuildLocaleChoices(), CurrentSettings().Locale, value => UpdateSettings(settings => settings.Locale = LocalizationSpreadsheet.NormalizeLocale(value), rebuildAfterSave: true));
                break;
            case SettingsTab.Controls:
                foreach (var binding in PlayerInputBindings)
                {
                    AddRebindRow(binding.LabelKey, "ui.options.rebind_desc", binding.ActionName);
                }
                break;
        }

        _loading = false;
        SelectFirstRowDescription();
    }

    private void AddDropdownRow(string labelKey, string descriptionKey, IReadOnlyList<OptionChoice> choices, string selectedValue, Action<string> changed)
    {
        var selector = new OptionButton
        {
            FocusMode = FocusModeEnum.All,
            CustomMinimumSize = new Vector2(230f, 34f),
            SizeFlagsHorizontal = SizeFlags.Fill
        };

        var selectedIndex = 0;
        for (var index = 0; index < choices.Count; index++)
        {
            selector.AddItem(LocalizationService.Current.Text(choices[index].LabelKey));
            selector.SetItemMetadata(index, choices[index].Value);
            if (string.Equals(choices[index].Value, selectedValue, StringComparison.OrdinalIgnoreCase))
            {
                selectedIndex = index;
            }
        }

        selector.Select(selectedIndex);
        selector.ItemSelected += index =>
        {
            if (_loading)
            {
                return;
            }

            changed(selector.GetItemMetadata((int)index).AsString());
        };

        AddSettingRow(labelKey, descriptionKey, selector);
    }

    private void AddChoiceRow(string labelKey, string descriptionKey, IReadOnlyList<OptionChoice> choices, string selectedValue, Action<string> changed)
    {
        var row = new HBoxContainer
        {
            CustomMinimumSize = new Vector2(250f, 34f),
            SizeFlagsHorizontal = SizeFlags.Fill
        };
        row.AddThemeConstantOverride("separation", 6);

        var previous = UiStyle.CreateButton("<");
        previous.CustomMinimumSize = new Vector2(38f, 34f);
        row.AddChild(previous);

        var valueLabel = UiStyle.CreateLabel("", 15, UiStyle.TextPrimary);
        valueLabel.HorizontalAlignment = HorizontalAlignment.Center;
        valueLabel.CustomMinimumSize = new Vector2(150f, 34f);
        valueLabel.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        row.AddChild(valueLabel);

        var next = UiStyle.CreateButton(">");
        next.CustomMinimumSize = new Vector2(38f, 34f);
        row.AddChild(next);

        var selectedIndex = FindChoiceIndex(choices, selectedValue);
        RefreshValueLabel(valueLabel, choices[selectedIndex]);

        void SelectDelta(int delta)
        {
            selectedIndex = Wrap(selectedIndex + delta, choices.Count);
            RefreshValueLabel(valueLabel, choices[selectedIndex]);
            changed(choices[selectedIndex].Value);
        }

        previous.Pressed += () => SelectDelta(-1);
        next.Pressed += () => SelectDelta(1);
        AddSettingRow(labelKey, descriptionKey, row);
    }

    private void AddRebindRow(string labelKey, string descriptionKey, string actionName)
    {
        var row = new HBoxContainer
        {
            CustomMinimumSize = new Vector2(360f, 34f),
            SizeFlagsHorizontal = SizeFlags.Fill
        };
        row.AddThemeConstantOverride("separation", 8);

        var value = UiStyle.CreateLabel(BindingSummary(actionName), 13, UiStyle.TextMuted);
        value.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        row.AddChild(value);

        var rebind = UiStyle.CreateButton(LocalizationService.Current.Text("ui.pause.rebind"));
        rebind.CustomMinimumSize = new Vector2(100f, 34f);
        rebind.Pressed += () =>
        {
            _pendingRebindAction = actionName;
            rebind.Text = LocalizationService.Current.Text("ui.pause.rebind_waiting");
            ShowDescription(labelKey, "ui.options.rebind_waiting_desc");
        };
        row.AddChild(rebind);

        AddSettingRow(labelKey, descriptionKey, row);
    }

    private void AddSettingRow(string labelKey, string descriptionKey, Control valueControl)
    {
        if (_rows == null)
        {
            return;
        }

        var panel = new PanelContainer
        {
            CustomMinimumSize = RowSize,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            MouseFilter = MouseFilterEnum.Stop
        };
        ApplyRowStyle(panel, focused: false);
        panel.MouseEntered += () => ShowDescription(labelKey, descriptionKey);
        _rows.AddChild(panel);

        var margin = UiStyle.AddMargin(panel, 10);
        var row = new HBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        row.AddThemeConstantOverride("separation", 12);
        margin.AddChild(row);

        var label = UiStyle.CreateLabel(LocalizationService.Current.Text(labelKey), 15, UiStyle.TextPrimary);
        label.VerticalAlignment = VerticalAlignment.Center;
        label.CustomMinimumSize = new Vector2(220f, 0f);
        label.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        row.AddChild(label);

        valueControl.FocusEntered += () =>
        {
            ApplyRowStyle(panel, focused: true);
            ShowDescription(labelKey, descriptionKey);
        };
        valueControl.FocusExited += () => ApplyRowStyle(panel, focused: false);
        row.AddChild(valueControl);
    }

    private void SelectTab(SettingsTab tab)
    {
        if (_activeTab == tab)
        {
            return;
        }

        _activeTab = tab;
        RebuildTabs();
        RebuildRows();
        GrabInitialFocus();
    }

    private void SelectRelativeTab(int delta)
    {
        var current = Array.IndexOf(Tabs, _activeTab);
        _activeTab = Tabs[Wrap(current + delta, Tabs.Length)];
        RebuildTabs();
        RebuildRows();
        GrabInitialFocus();
    }

    private void UpdateSettings(Action<SettingsSaveDto> apply, bool rebuildAfterSave = false)
    {
        if (_saveService == null)
        {
            return;
        }

        var settings = _saveService.Settings;
        apply(settings);
        _saveService.SaveSettings(settings);
        if (rebuildAfterSave)
        {
            CallDeferred(nameof(RebuildAfterLocaleChange));
        }
    }

    private void RebuildAfterLocaleChange()
    {
        RebuildTabs();
        RebuildRows();
    }

    private void TryFinishRebind(InputEvent inputEvent)
    {
        if (_pendingRebindAction == null || _saveService == null)
        {
            _pendingRebindAction = null;
            RebuildRows();
            return;
        }

        if (!InputBindingService.TryRebindAction(_saveService.Settings, _pendingRebindAction, inputEvent, out _))
        {
            return;
        }

        _saveService.SaveSettings(_saveService.Settings);
        _pendingRebindAction = null;
        RebuildRows();
        GetViewport().SetInputAsHandled();
    }

    private void RequestBack()
    {
        _pendingRebindAction = null;
        _backRequested?.Invoke();
    }

    private SettingsSaveDto CurrentSettings()
    {
        return _saveService?.Settings ?? new SettingsSaveDto();
    }

    private IReadOnlyList<OptionChoice> BuildLocaleChoices()
    {
        var locales = new List<string> { LocalizationService.DefaultLocale };
        foreach (var locale in LocalizationService.Current.AvailableLocales)
        {
            var normalized = LocalizationSpreadsheet.NormalizeLocale(locale);
            if (!locales.Contains(normalized, StringComparer.OrdinalIgnoreCase))
            {
                locales.Add(normalized);
            }
        }

        var current = LocalizationSpreadsheet.NormalizeLocale(CurrentSettings().Locale);
        if (!locales.Contains(current, StringComparer.OrdinalIgnoreCase))
        {
            locales.Add(current);
        }

        return locales.Select(locale => new OptionChoice(locale, locale)).ToList();
    }

    private static Control? FindFirstFocusable(Node? node)
    {
        if (node == null)
        {
            return null;
        }

        foreach (var child in node.GetChildren())
        {
            if (child is Control { FocusMode: FocusModeEnum.All, Visible: true } control && !control.DisabledAsButton())
            {
                return control;
            }

            var nested = FindFirstFocusable(child);
            if (nested != null)
            {
                return nested;
            }
        }

        return null;
    }

    private static void ClearChildren(Node node)
    {
        foreach (var child in node.GetChildren())
        {
            node.RemoveChild(child);
            child.QueueFree();
        }
    }

    private void SelectFirstRowDescription()
    {
        var row = CurrentTabRows().FirstOrDefault();
        if (row.LabelKey != null)
        {
            ShowDescription(row.LabelKey, row.DescriptionKey);
        }
    }

    private IEnumerable<(string LabelKey, string DescriptionKey)> CurrentTabRows()
    {
        return _activeTab switch
        {
            SettingsTab.Display => new[]
            {
                ("ui.options.display_mode", "ui.options.display_mode_desc"),
                ("ui.options.resolution", "ui.options.resolution_desc"),
                ("ui.options.vsync", "ui.options.vsync_desc"),
                ("ui.options.fps_cap", "ui.options.fps_cap_desc")
            },
            SettingsTab.Graphics => new[] { ("ui.options.render_scale", "ui.options.render_scale_desc") },
            SettingsTab.Gameplay => new[]
            {
                ("ui.options.mouse_sensitivity", "ui.options.mouse_sensitivity_desc"),
                ("ui.options.gamepad_sensitivity", "ui.options.gamepad_sensitivity_desc")
            },
            SettingsTab.Sound => new[] { ("ui.options.master_volume", "ui.options.master_volume_desc") },
            SettingsTab.Language => new[] { ("ui.options.language", "ui.options.language_desc") },
            SettingsTab.Controls => PlayerInputBindings.Select(binding => (binding.LabelKey, "ui.options.rebind_desc")),
            _ => Array.Empty<(string LabelKey, string DescriptionKey)>()
        };
    }

    private void ShowDescription(string labelKey, string descriptionKey)
    {
        if (_descriptionTitle != null)
        {
            _descriptionTitle.Text = LocalizationService.Current.Text(labelKey);
        }

        if (_descriptionBody != null)
        {
            _descriptionBody.Text = LocalizationService.Current.Text(descriptionKey);
        }
    }

    private static void ApplyRowStyle(PanelContainer panel, bool focused)
    {
        panel.AddThemeStyleboxOverride("panel", CreateBox(focused ? RowFocus : RowNormal, focused ? UiStyle.PanelAccent : UiStyle.PanelBorder));
    }

    private static StyleBoxFlat CreateBox(Color background, Color border)
    {
        return new StyleBoxFlat
        {
            BgColor = background,
            BorderColor = border,
            BorderWidthLeft = 1,
            BorderWidthTop = 1,
            BorderWidthRight = 1,
            BorderWidthBottom = 1,
            CornerRadiusTopLeft = 2,
            CornerRadiusTopRight = 2,
            CornerRadiusBottomLeft = 2,
            CornerRadiusBottomRight = 2,
            ContentMarginLeft = 8,
            ContentMarginTop = 6,
            ContentMarginRight = 8,
            ContentMarginBottom = 6
        };
    }

    private static void RefreshValueLabel(Label label, OptionChoice choice)
    {
        label.Text = LocalizationService.Current.Text(choice.LabelKey);
    }

    private static int FindChoiceIndex(IReadOnlyList<OptionChoice> choices, string value)
    {
        for (var index = 0; index < choices.Count; index++)
        {
            if (string.Equals(choices[index].Value, value, StringComparison.OrdinalIgnoreCase))
            {
                return index;
            }
        }

        return 0;
    }

    private static int Wrap(int value, int count)
    {
        return ((value % count) + count) % count;
    }

    private static string FpsToValue(int fps)
    {
        return fps <= 0 ? "0" : fps.ToString();
    }

    private static int ValueToFps(string value)
    {
        return int.TryParse(value, out var fps) ? Math.Max(0, fps) : 0;
    }

    private static string FloatToValue(float value)
    {
        return value.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture);
    }

    private static float ValueToFloat(string value, float fallback)
    {
        return float.TryParse(value, System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out var parsed)
            ? parsed
            : fallback;
    }

    private static string BindingSummary(string actionName)
    {
        return LocalizationService.Current.Text(
            "ui.pause.binding_summary",
            InputPromptService.ActionLabel(actionName, InputDeviceKind.KeyboardMouse),
            InputPromptService.ActionLabel(actionName, InputDeviceKind.Controller));
    }

    private static string TabLabelKey(SettingsTab tab)
    {
        return tab switch
        {
            SettingsTab.Display => "ui.options.tab_display",
            SettingsTab.Graphics => "ui.options.tab_graphics",
            SettingsTab.Gameplay => "ui.options.tab_gameplay",
            SettingsTab.Sound => "ui.options.tab_sound",
            SettingsTab.Language => "ui.options.tab_language",
            SettingsTab.Controls => "ui.options.tab_controls",
            _ => "ui.options.tab_display"
        };
    }

    private static readonly SettingsTab[] Tabs =
    {
        SettingsTab.Display,
        SettingsTab.Graphics,
        SettingsTab.Gameplay,
        SettingsTab.Sound,
        SettingsTab.Language,
        SettingsTab.Controls
    };

    private static readonly OptionChoice[] DisplayModes =
    {
        new("windowed", "ui.options.value_windowed"),
        new("fullscreen", "ui.options.value_fullscreen"),
        new("borderless", "ui.options.value_borderless")
    };

    private static readonly OptionChoice[] Resolutions =
    {
        new("1280x720", "ui.options.value_1280x720"),
        new("1600x900", "ui.options.value_1600x900"),
        new("1920x1080", "ui.options.value_1920x1080"),
        new("2560x1440", "ui.options.value_2560x1440"),
        new("3840x2160", "ui.options.value_3840x2160")
    };

    private static readonly OptionChoice[] BoolChoices =
    {
        new("off", "ui.options.value_off"),
        new("on", "ui.options.value_on")
    };

    private static readonly OptionChoice[] FpsChoices =
    {
        new("0", "ui.options.value_uncapped"),
        new("30", "ui.options.value_30"),
        new("60", "ui.options.value_60"),
        new("90", "ui.options.value_90"),
        new("120", "ui.options.value_120"),
        new("144", "ui.options.value_144"),
        new("240", "ui.options.value_240")
    };

    private static readonly OptionChoice[] RenderScaleChoices =
    {
        new("0.5", "ui.options.value_50_percent"),
        new("0.65", "ui.options.value_65_percent"),
        new("0.8", "ui.options.value_80_percent"),
        new("1", "ui.options.value_100_percent"),
        new("1.25", "ui.options.value_125_percent")
    };

    private static readonly OptionChoice[] VolumeChoices =
    {
        new("0", "ui.options.value_0_percent"),
        new("0.25", "ui.options.value_25_percent"),
        new("0.5", "ui.options.value_50_percent"),
        new("0.75", "ui.options.value_75_percent"),
        new("1", "ui.options.value_100_percent")
    };

    private static readonly OptionChoice[] SensitivityChoices =
    {
        new("0.5", "ui.options.value_50_percent"),
        new("0.75", "ui.options.value_75_percent"),
        new("1", "ui.options.value_100_percent"),
        new("1.25", "ui.options.value_125_percent"),
        new("1.5", "ui.options.value_150_percent"),
        new("2", "ui.options.value_200_percent"),
        new("3", "ui.options.value_300_percent")
    };

    private static readonly InputBindingRow[] PlayerInputBindings =
    {
        new(InputActions.CastPrimary, "ui.pause.binding_cast"),
        new(InputActions.UseActivatableItem, "ui.pause.binding_active_item"),
        new(InputActions.SpellPrevious, "ui.pause.binding_spell_previous"),
        new(InputActions.SpellNext, "ui.pause.binding_spell_next"),
        new(InputActions.UiAccept, "ui.pause.binding_confirm"),
        new(InputActions.UiCancel, "ui.pause.binding_back"),
        new(InputActions.Pause, "ui.pause.binding_pause")
    };

    private enum SettingsTab
    {
        Display,
        Graphics,
        Gameplay,
        Sound,
        Language,
        Controls
    }

    private readonly record struct OptionChoice(string Value, string LabelKey);

    private readonly record struct InputBindingRow(string ActionName, string LabelKey);
}

internal static class OptionsMenuControlExtensions
{
    public static bool DisabledAsButton(this Control control)
    {
        return control is BaseButton { Disabled: true };
    }
}
