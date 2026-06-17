namespace TheLastMage.Foundation;

public readonly record struct StatId(string Value)
{
    public bool IsValid => !string.IsNullOrWhiteSpace(Value);

    public static StatId From(string? value) => new((value ?? string.Empty).Trim());

    public override string ToString() => Value;
}
