namespace TheLastMage.Foundation;

public readonly record struct ContentId(string Value)
{
    public bool IsValid => !string.IsNullOrWhiteSpace(Value);

    public static ContentId From(string? value) => new((value ?? string.Empty).Trim());

    public override string ToString() => Value;
}
