using Godot;
using TheLastMage.Controls;
using TheLastMage.Gameplay.Run;
using TheLastMage.Localization;
using TheLastMage.Save;

namespace TheLastMage.UI;

public partial class PauseSettingsPanel : Control
{
    private const string MainMenuScene = "res://scenes/app/game_root.tscn";
    private static readonly DebugShortcut[] DebugShortcuts =
    {
        new("debug_toggle_overlay", "ui.pause.shortcut_debug_overlay"),
        new("debug_toggle_overlay", "ui.pause.shortcut_debug_overlay_details", true),
        new("open_build_inspector", "ui.pause.shortcut_build_inspector"),
        new("debug_toggle_tooling_panel", "ui.pause.shortcut_tooling_panel"),
        new("debug_toggle_meta_panel", "ui.pause.shortcut_meta_panel"),
        new("debug_start_run", "ui.pause.shortcut_load_run_scene")
    };

    private SaveServiceNode? _saveService;
    private RunControllerNode? _runController;
    private PanelContainer? _panel;
    private VBoxContainer? _contentRoot;
    private Button? _resumeButton;
    private Button? _optionsButton;
    private Button? _returnMainMenuButton;
    private Button? _cancelReturnButton;
    private Label? _returnCheckpointLabel;
    private RichTextLabel? _debugKeybinds;
    private CanvasLayer? _debugOverlay;
    private bool _debugOverlayHiddenByPause;
    private bool _debugOverlayWasVisible;
    private bool _open;
    private bool _showingOptions;
    private bool _confirmReturnToMenu;
    private bool _treePausedBeforePauseMenu;

    public override void _Ready()
    {
        ProcessMode = ProcessModeEnum.Always;
        SetAnchorsPreset(LayoutPreset.FullRect);
        MouseFilter = MouseFilterEnum.Ignore;
        _saveService = GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        _runController = RunUiLocator.FindRunController(this);
        BuildShell();
        BuildPauseMenu();
        SetOpen(false);
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event.IsActionPressed(InputActions.Pause))
        {
            SetOpen(!_open);
            GetViewport().SetInputAsHandled();
            return;
        }

        if (!_open)
        {
            return;
        }

        if (@event.IsActionPressed(InputActions.CancelAction) || @event.IsActionPressed(InputActions.UiCancel))
        {
            if (_showingOptions)
            {
                BuildPauseMenu();
            }
            else
            {
                SetOpen(false);
            }

            GetViewport().SetInputAsHandled();
        }
    }

    public override void _Process(double delta)
    {
        _runController ??= RunUiLocator.FindRunController(this);
        CenterPanel();
        if (!_open || _runController?.Context == null)
        {
            return;
        }

        _runController.Context.State.IsDebugUiInputCaptured = true;
        if (Input.MouseMode != Input.MouseModeEnum.Visible)
        {
            Input.MouseMode = Input.MouseModeEnum.Visible;
        }
    }

    private void BuildShell()
    {
        _panel = UiStyle.CreatePanel("PauseSettingsPanel");
        _panel.SetAnchorsPreset(LayoutPreset.TopLeft);
        AddChild(_panel);

        var margin = UiStyle.AddMargin(_panel, 20);
        _contentRoot = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        _contentRoot.AddThemeConstantOverride("separation", 10);
        margin.AddChild(_contentRoot);
        CenterPanel();
    }

    private void BuildPauseMenu()
    {
        _showingOptions = false;
        _confirmReturnToMenu = false;
        ClearContent();
        CenterPanel();

        AddRoot(UiStyle.CreateLabel(LocalizationService.Current.Text("ui.pause.title"), 26, UiStyle.PanelAccent));
        AddRoot(UiStyle.CreateLabel(LocalizationService.Current.Text("ui.pause.subtitle"), 14, UiStyle.TextMuted));

        _resumeButton = AddActionButton("ui.pause.resume", () => SetOpen(false));
        _optionsButton = AddActionButton("ui.frontend.options", OpenOptions);
        _returnMainMenuButton = AddActionButton("ui.pause.main_menu", BeginReturnToMainMenu);
        AddActionButton("ui.pause.feedback", () => GD.Print("PauseFeedbackRequested"));
        AddActionButton("ui.pause.quit", () =>
        {
            GetTree().Paused = false;
            GetTree().Quit();
        });

        _returnCheckpointLabel = UiStyle.CreateLabel("", 13, UiStyle.TextMuted);
        AddRoot(_returnCheckpointLabel);

        _cancelReturnButton = UiStyle.CreateButton(LocalizationService.Current.Text("ui.frontend.cancel"));
        _cancelReturnButton.Pressed += CancelReturnToMainMenu;
        AddRoot(_cancelReturnButton);

        AddRoot(UiStyle.CreateLabel(LocalizationService.Current.Text("ui.pause.debug_shortcuts"), 18, UiStyle.PanelAccent));
        _debugKeybinds = new RichTextLabel
        {
            BbcodeEnabled = true,
            FitContent = true,
            ScrollActive = false,
            SelectionEnabled = false,
            CustomMinimumSize = new Vector2(0f, 110f),
            MouseFilter = MouseFilterEnum.Ignore
        };
        _debugKeybinds.AddThemeFontSizeOverride("normal_font_size", 15);
        _debugKeybinds.AddThemeColorOverride("default_color", UiStyle.TextPrimary);
        _debugKeybinds.AddThemeConstantOverride("line_separation", 4);
        AddRoot(_debugKeybinds);

        RefreshDebugKeybinds();
        RefreshReturnToMainMenuState();
        _resumeButton?.GrabFocus();
    }

    private void OpenOptions()
    {
        _showingOptions = true;
        _confirmReturnToMenu = false;
        ClearContent();
        CenterPanel();

        AddRoot(UiStyle.CreateLabel(LocalizationService.Current.Text("ui.frontend.options"), 26, UiStyle.PanelAccent));
        AddRoot(UiStyle.CreateLabel(LocalizationService.Current.Text("ui.frontend.options_subtitle"), 14, UiStyle.TextMuted));

        var options = new OptionsMenuPanel
        {
            ProcessMode = ProcessModeEnum.Always
        };
        options.Configure(_saveService, BuildPauseMenu);
        AddRoot(options);
        options.CallDeferred(nameof(OptionsMenuPanel.GrabInitialFocus));
    }

    private Button AddActionButton(string key, Action pressed)
    {
        var button = UiStyle.CreateButton(LocalizationService.Current.Text(key));
        button.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        button.Pressed += pressed;
        AddRoot(button);
        return button;
    }

    private void SetOpen(bool open)
    {
        var changed = _open != open;
        _open = open;
        _runController ??= RunUiLocator.FindRunController(this);
        if (_panel != null)
        {
            _panel.Visible = open;
        }

        MouseFilter = open ? MouseFilterEnum.Stop : MouseFilterEnum.Ignore;
        if (!changed)
        {
            return;
        }

        if (open)
        {
            _treePausedBeforePauseMenu = GetTree().Paused;
            GetTree().Paused = true;
            BuildPauseMenu();
        }
        else
        {
            _showingOptions = false;
            _confirmReturnToMenu = false;
            GetTree().Paused = _treePausedBeforePauseMenu;
        }

        SetDebugOverlayHidden(open);
        if (_runController?.Context != null)
        {
            _runController.Context.State.IsDebugUiInputCaptured = open;
        }

        Input.MouseMode = open
            ? Input.MouseModeEnum.Visible
            : _runController?.Context != null
                ? Input.MouseModeEnum.Captured
                : Input.MouseModeEnum.Visible;
    }

    private void BeginReturnToMainMenu()
    {
        if (!_confirmReturnToMenu)
        {
            _confirmReturnToMenu = true;
            RefreshReturnToMainMenuState();
            _returnMainMenuButton?.GrabFocus();
            return;
        }

        GetTree().Paused = false;
        GetTree().ChangeSceneToFile(MainMenuScene);
    }

    private void CancelReturnToMainMenu()
    {
        _confirmReturnToMenu = false;
        RefreshReturnToMainMenuState();
        _resumeButton?.GrabFocus();
    }

    private void RefreshReturnToMainMenuState()
    {
        if (_returnCheckpointLabel != null)
        {
            var checkpoint = _saveService?.Profile.SuspendedRun;
            _returnCheckpointLabel.Text = _confirmReturnToMenu
                ? LocalizationService.Current.Text(
                    "ui.pause.main_menu_checkpoint_warning",
                    checkpoint != null
                        ? RunCheckpointService.Describe(checkpoint)
                        : LocalizationService.Current.Text("ui.common.none"))
                : string.Empty;
        }

        if (_returnMainMenuButton != null)
        {
            _returnMainMenuButton.Text = LocalizationService.Current.Text(
                _confirmReturnToMenu ? "ui.pause.confirm_main_menu" : "ui.pause.main_menu");
        }

        if (_cancelReturnButton != null)
        {
            _cancelReturnButton.Visible = _confirmReturnToMenu;
        }
    }

    private void RefreshDebugKeybinds()
    {
        if (_debugKeybinds == null)
        {
            return;
        }

        _debugKeybinds.Text = string.Join('\n', DebugShortcuts.Select(FormatShortcut));
    }

    private static string FormatShortcut(DebugShortcut shortcut)
    {
        var binding = InputPromptService.ActionLabel(shortcut.ActionName, InputDeviceKind.KeyboardMouse);
        var unbound = LocalizationService.Current.Text("ui.pause.unbound");
        if (shortcut.RequiresShift && binding != unbound)
        {
            binding = $"Shift+{binding}";
        }

        return $"[color=#d2ad64][b]{EscapeBbcode(binding)}[/b][/color]  [color=#ecece0]{EscapeBbcode(LocalizationService.Current.Text(shortcut.LabelKey))}[/color]";
    }

    private static string EscapeBbcode(string value)
    {
        return string.IsNullOrEmpty(value) ? "-" : value.Replace("[", "[lb]", StringComparison.Ordinal);
    }

    private void SetDebugOverlayHidden(bool hidden)
    {
        _debugOverlay ??= GetTree().Root.FindChild("DebugOverlay", true, false) as CanvasLayer;
        if (_debugOverlay == null)
        {
            return;
        }

        if (hidden)
        {
            if (_debugOverlayHiddenByPause)
            {
                return;
            }

            _debugOverlayWasVisible = _debugOverlay.Visible;
            _debugOverlay.Visible = false;
            _debugOverlayHiddenByPause = true;
            return;
        }

        if (!_debugOverlayHiddenByPause)
        {
            return;
        }

        _debugOverlay.Visible = _debugOverlayWasVisible;
        _debugOverlayHiddenByPause = false;
    }

    private void AddRoot(Control control)
    {
        _contentRoot?.AddChild(control);
    }

    private void ClearContent()
    {
        if (_contentRoot == null)
        {
            return;
        }

        foreach (var child in _contentRoot.GetChildren())
        {
            _contentRoot.RemoveChild(child);
            child.QueueFree();
        }

        _resumeButton = null;
        _optionsButton = null;
        _returnMainMenuButton = null;
        _cancelReturnButton = null;
        _returnCheckpointLabel = null;
        _debugKeybinds = null;
    }

    private void CenterPanel()
    {
        if (_panel == null)
        {
            return;
        }

        var viewportSize = GetViewportRect().Size;
        _panel.Size = viewportSize;
        _panel.CustomMinimumSize = viewportSize;
        _panel.Position = Vector2.Zero;
    }

    private readonly record struct DebugShortcut(string ActionName, string LabelKey, bool RequiresShift = false);
}
