namespace TheLastMage.Save;

public sealed class SettingsSaveDto
{
    public float MasterVolume { get; set; } = 1f;

    public float MouseSensitivity { get; set; } = 1f;

    public float GamepadSensitivity { get; set; } = 1f;

    public string DisplayMode { get; set; } = "windowed";

    public string Resolution { get; set; } = "1280x720";

    public bool VSync { get; set; } = true;

    public int FpsCap { get; set; } = 0;

    public float RenderScale { get; set; } = 1f;

    public float ScreenshakeScale { get; set; } = 0.75f;

    public float FlashScale { get; set; } = 0.7f;

    public float TextScale { get; set; } = 1f;

    public string Locale { get; set; } = "en";

    public Dictionary<string, List<InputBindingSaveDto>> InputBindings { get; set; } = new();
}
