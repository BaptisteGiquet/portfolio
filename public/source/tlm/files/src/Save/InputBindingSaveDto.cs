namespace TheLastMage.Save;

public sealed class InputBindingSaveDto
{
    public string Kind { get; set; } = string.Empty;

    public string Key { get; set; } = string.Empty;

    public string MouseButton { get; set; } = string.Empty;

    public string JoyButton { get; set; } = string.Empty;

    public string JoyAxis { get; set; } = string.Empty;

    public float AxisValue { get; set; }
}
