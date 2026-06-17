namespace TheLastMage.Foundation;

public readonly record struct GameError(string Code, string Message)
{
    public static readonly GameError None = new(string.Empty, string.Empty);

    public bool IsValid => !string.IsNullOrWhiteSpace(Code);
}
