using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Items;

public sealed class ActivatableItemState
{
    public ActivatableItemState(ContentId itemId, bool hasUnlimitedActivations, int remainingActivations, int maxActivations)
    {
        ItemId = itemId;
        HasUnlimitedActivations = hasUnlimitedActivations;
        RemainingActivations = remainingActivations;
        MaxActivations = maxActivations;
    }

    public ContentId ItemId { get; }

    public bool HasUnlimitedActivations { get; }

    public int RemainingActivations { get; private set; }

    public int MaxActivations { get; }

    public bool HasUsesRemaining => HasUnlimitedActivations || RemainingActivations > 0;

    public bool TryConsumeUse()
    {
        if (HasUnlimitedActivations)
        {
            return true;
        }

        if (RemainingActivations <= 0)
        {
            return false;
        }

        RemainingActivations--;
        return true;
    }
}
