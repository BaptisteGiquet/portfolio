using Godot;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.UI;

public partial class BuildInspectorPanel : Control
{
    private static readonly Vector2 PanelSize = new(560f, 520f);
    private const float Margin = 24f;

    private RunControllerNode? _runController;
    private PanelContainer? _panel;
    private Label? _mageLabel;
    private RichTextLabel? _summary;
    private bool _open;

    public override void _Ready()
    {
        SetAnchorsPreset(LayoutPreset.FullRect);
        MouseFilter = MouseFilterEnum.Ignore;
        BuildUi();
        SetOpen(false);
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event.IsActionPressed("open_build_inspector"))
        {
            SetOpen(!_open);
            GetViewport().SetInputAsHandled();
        }
    }

    public override void _Process(double delta)
    {
        _runController ??= RunUiLocator.FindRunController(this);
        PositionPanel();
        var context = _runController?.Context;
        if (context == null || !_open || _summary == null || _mageLabel == null)
        {
            return;
        }

        var viewModel = RunViewModels.BuildInspector(context);
        _mageLabel.Text = $"Build Inspector - {viewModel.MageText}";
        _summary.Text = string.Join('\n', new[]
        {
            "[b]Effective Stats[/b]",
            viewModel.AttributeText,
            string.Empty,
            "[b]Relics And Modifiers[/b]",
            viewModel.ItemText,
            string.Empty,
            "[b]Proc Safety[/b]",
            viewModel.ProcText,
            string.Empty,
            "[b]Feedback Budgets[/b]",
            viewModel.FeedbackText
        });
    }

    private void BuildUi()
    {
        _panel = UiStyle.CreatePanel("BuildInspectorPanel");
        _panel.SetAnchorsPreset(LayoutPreset.TopLeft);
        _panel.Size = PanelSize;
        _panel.CustomMinimumSize = PanelSize;
        PositionPanel();
        AddChild(_panel);

        var margin = UiStyle.AddMargin(_panel, 12);
        var root = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        root.AddThemeConstantOverride("separation", 8);
        margin.AddChild(root);

        _mageLabel = UiStyle.CreateLabel("Build Inspector", 18, UiStyle.PanelAccent);
        root.AddChild(_mageLabel);

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
        _summary.AddThemeColorOverride("default_color", UiStyle.TextPrimary);
        _summary.AddThemeFontSizeOverride("normal_font_size", 14);
        root.AddChild(_summary);
    }

    private void SetOpen(bool open)
    {
        _open = open;
        PositionPanel();
        if (_panel != null)
        {
            _panel.Visible = open;
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
        if (_panel == null)
        {
            return;
        }

        var viewportSize = GetViewportRect().Size;
        var size = new Vector2(
            MathF.Min(PanelSize.X, MathF.Max(320f, viewportSize.X - Margin * 2f)),
            MathF.Min(PanelSize.Y, MathF.Max(360f, viewportSize.Y - Margin * 2f)));
        _panel.Size = size;
        _panel.Position = new Vector2(
            MathF.Max(Margin, viewportSize.X - size.X - Margin),
            MathF.Max(Margin, viewportSize.Y - size.Y - Margin));
    }
}
