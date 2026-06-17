using Godot;

namespace TheLastMage.UI;

public static class UiStyle
{
    public static readonly Color PanelBackground = new(0.055f, 0.066f, 0.072f, 0.9f);
    public static readonly Color PanelAccent = new(0.78f, 0.64f, 0.38f, 1f);
    public static readonly Color PanelBorder = new(0.22f, 0.26f, 0.27f, 1f);
    public static readonly Color TextPrimary = new(0.92f, 0.92f, 0.86f, 1f);
    public static readonly Color TextMuted = new(0.62f, 0.68f, 0.66f, 1f);
    public static readonly Color Health = new(0.78f, 0.16f, 0.18f, 1f);
    public static readonly Color Cooldown = new(0.2f, 0.52f, 0.78f, 1f);
    public static readonly Color Resource = new(0.79f, 0.7f, 0.38f, 1f);

    public static PanelContainer CreatePanel(string name)
    {
        var panel = new PanelContainer
        {
            Name = name,
            MouseFilter = Control.MouseFilterEnum.Stop
        };
        ApplyPanel(panel);
        return panel;
    }

    public static MarginContainer AddMargin(Control parent, int margin = 12)
    {
        var container = new MarginContainer();
        container.AddThemeConstantOverride("margin_left", margin);
        container.AddThemeConstantOverride("margin_top", margin);
        container.AddThemeConstantOverride("margin_right", margin);
        container.AddThemeConstantOverride("margin_bottom", margin);
        parent.AddChild(container);
        return container;
    }

    public static Label CreateLabel(string text = "", int fontSize = 14, Color? color = null)
    {
        var label = new Label
        {
            Text = text,
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            MouseFilter = Control.MouseFilterEnum.Ignore
        };
        label.AddThemeFontSizeOverride("font_size", fontSize);
        label.AddThemeColorOverride("font_color", color ?? TextPrimary);
        return label;
    }

    public static Button CreateButton(string text)
    {
        var button = new Button
        {
            Text = text,
            CustomMinimumSize = new Vector2(96f, 34f),
            FocusMode = Control.FocusModeEnum.All,
            MouseFilter = Control.MouseFilterEnum.Stop
        };
        ApplyButton(button);
        return button;
    }

    public static ProgressBar CreateBar(Color fillColor)
    {
        var bar = new ProgressBar
        {
            MinValue = 0,
            MaxValue = 1,
            ShowPercentage = false,
            CustomMinimumSize = new Vector2(120f, 16f),
            MouseFilter = Control.MouseFilterEnum.Ignore
        };
        bar.AddThemeStyleboxOverride("background", CreateBox(new Color(0.02f, 0.025f, 0.028f, 0.9f), PanelBorder, 3));
        bar.AddThemeStyleboxOverride("fill", CreateBox(fillColor, fillColor.Darkened(0.25f), 3));
        return bar;
    }

    public static void ApplyPanel(PanelContainer panel)
    {
        panel.AddThemeStyleboxOverride("panel", CreateBox(PanelBackground, PanelBorder, 6));
    }

    public static void ApplyButton(Button button)
    {
        button.AddThemeStyleboxOverride("normal", CreateBox(new Color(0.12f, 0.15f, 0.16f, 0.98f), PanelBorder, 4));
        button.AddThemeStyleboxOverride("hover", CreateBox(new Color(0.17f, 0.21f, 0.21f, 1f), PanelAccent, 4));
        button.AddThemeStyleboxOverride("pressed", CreateBox(new Color(0.22f, 0.2f, 0.13f, 1f), PanelAccent, 4));
        button.AddThemeStyleboxOverride("focus", CreateBox(new Color(0.12f, 0.15f, 0.16f, 0.98f), PanelAccent, 4));
        button.AddThemeColorOverride("font_color", TextPrimary);
        button.AddThemeColorOverride("font_disabled_color", TextMuted.Darkened(0.35f));
    }

    private static StyleBoxFlat CreateBox(Color background, Color border, int radius)
    {
        return new StyleBoxFlat
        {
            BgColor = background,
            BorderColor = border,
            BorderWidthLeft = 1,
            BorderWidthTop = 1,
            BorderWidthRight = 1,
            BorderWidthBottom = 1,
            CornerRadiusTopLeft = radius,
            CornerRadiusTopRight = radius,
            CornerRadiusBottomLeft = radius,
            CornerRadiusBottomRight = radius,
            ContentMarginLeft = 8,
            ContentMarginTop = 6,
            ContentMarginRight = 8,
            ContentMarginBottom = 6
        };
    }
}
