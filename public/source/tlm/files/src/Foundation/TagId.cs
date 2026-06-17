namespace TheLastMage.Foundation;

public readonly record struct TagId(string Value)
{
    public bool IsValid => !string.IsNullOrWhiteSpace(Value);

    public static TagId From(string? value) => new(GameplayTagPath.Normalize(value));

    public override string ToString() => Value;
}
