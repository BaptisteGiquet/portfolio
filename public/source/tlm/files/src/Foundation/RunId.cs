namespace TheLastMage.Foundation;

public readonly record struct RunId(Guid Value)
{
    public static RunId NewRun() => new(Guid.NewGuid());

    public static RunId FromString(string value)
    {
        return Guid.TryParse(value, out var guid) ? new RunId(guid) : NewRun();
    }

    public bool IsValid => Value != Guid.Empty;

    public override string ToString() => Value.ToString("N");
}
