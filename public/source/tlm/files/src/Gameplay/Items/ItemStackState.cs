using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Items;

public sealed class ItemStackState
{
    public ItemStackState(ContentId itemId, int count)
    {
        ItemId = itemId;
        Count = count;
    }

    public ContentId ItemId { get; }

    public int Count { get; private set; }

    public void Add(int amount = 1)
    {
        Count += Math.Max(0, amount);
    }
}
