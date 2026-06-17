using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Combat;

public sealed class DamagePreventionState
{
    public DamagePreventionState(
        ContentId sourceId,
        EntityId protectedEntityId,
        DamagePreventionMode mode,
        int remainingHits,
        float remainingAmount,
        TagId requiredIncomingTag)
    {
        SourceId = sourceId;
        ProtectedEntityId = protectedEntityId;
        Mode = mode;
        RemainingHits = Math.Max(0, remainingHits);
        RemainingAmount = MathF.Max(0f, remainingAmount);
        RequiredIncomingTag = requiredIncomingTag;
    }

    public ContentId SourceId { get; }

    public EntityId ProtectedEntityId { get; }

    public DamagePreventionMode Mode { get; }

    public int RemainingHits { get; private set; }

    public float RemainingAmount { get; private set; }

    public TagId RequiredIncomingTag { get; }

    public bool IsDepleted => RemainingHits <= 0 || (Mode == DamagePreventionMode.PreventAmount && RemainingAmount <= 0f);

    public float Consume(float incomingAmount)
    {
        if (incomingAmount <= 0f || IsDepleted)
        {
            return 0f;
        }

        RemainingHits--;
        if (Mode == DamagePreventionMode.PreventFullHit)
        {
            return incomingAmount;
        }

        var prevented = MathF.Min(incomingAmount, RemainingAmount);
        RemainingAmount -= prevented;
        return prevented;
    }

    public void AddHits(int count)
    {
        RemainingHits += Math.Max(0, count);
    }
}
