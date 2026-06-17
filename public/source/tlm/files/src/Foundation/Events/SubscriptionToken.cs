namespace TheLastMage.Foundation.Events;

public readonly record struct SubscriptionToken(Guid Value)
{
    public bool IsValid => Value != Guid.Empty;
}
