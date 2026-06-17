namespace TheLastMage.Foundation;

public readonly record struct EntityId(int Value)
{
    public static readonly EntityId None = new(-1);

    public bool IsValid => Value >= 0;

    public override string ToString() => Value.ToString();
}
