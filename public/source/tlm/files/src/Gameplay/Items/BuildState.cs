using TheLastMage.Foundation;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Items;

public sealed class BuildState
{
    private readonly Dictionary<ContentId, ItemStackState> _items = new();
    private readonly HashSet<ContentId> _acquiredItemIds = new();
    private readonly List<TemporaryModifierState> _temporaryModifiers = new();

    public IReadOnlyCollection<ItemStackState> Items => _items.Values;

    public IReadOnlyCollection<ContentId> AcquiredItemIds => _acquiredItemIds;

    public IReadOnlyList<TemporaryModifierState> TemporaryModifiers => _temporaryModifiers;

    public ActivatableItemState? ActivatableItem { get; private set; }

    public bool HasItem(ContentId itemId)
    {
        return _items.ContainsKey(itemId);
    }

    public bool HasAcquiredItem(ContentId itemId)
    {
        return _acquiredItemIds.Contains(itemId);
    }

    public void AddItem(ContentId itemId)
    {
        _acquiredItemIds.Add(itemId);
        if (_items.TryGetValue(itemId, out var stack))
        {
            stack.Add();
            return;
        }

        _items.Add(itemId, new ItemStackState(itemId, 1));
    }

    public void RestoreItemStack(ContentId itemId, int count)
    {
        if (!itemId.IsValid || count <= 0)
        {
            return;
        }

        _acquiredItemIds.Add(itemId);
        _items[itemId] = new ItemStackState(itemId, count);
    }

    public void RestoreAcquiredItem(ContentId itemId)
    {
        if (itemId.IsValid)
        {
            _acquiredItemIds.Add(itemId);
        }
    }

    public void EquipActivatableItem(ContentId itemId, bool hasUnlimitedActivations, int maxActivations)
    {
        _acquiredItemIds.Add(itemId);
        ActivatableItem = new ActivatableItemState(
            itemId,
            hasUnlimitedActivations,
            hasUnlimitedActivations ? 0 : Math.Max(0, maxActivations),
            Math.Max(0, maxActivations));
    }

    public void RestoreActivatableItem(
        ContentId itemId,
        bool hasUnlimitedActivations,
        int remainingActivations,
        int maxActivations)
    {
        if (!itemId.IsValid)
        {
            ActivatableItem = null;
            return;
        }

        _acquiredItemIds.Add(itemId);
        ActivatableItem = new ActivatableItemState(
            itemId,
            hasUnlimitedActivations,
            hasUnlimitedActivations ? 0 : Math.Max(0, remainingActivations),
            Math.Max(0, maxActivations));
    }

    public void ClearForRestore()
    {
        _items.Clear();
        _acquiredItemIds.Clear();
        _temporaryModifiers.Clear();
        ActivatableItem = null;
    }

    public bool TryUseActivatableItem(out bool depleted)
    {
        depleted = false;
        if (ActivatableItem == null || !ActivatableItem.TryConsumeUse())
        {
            return false;
        }

        depleted = !ActivatableItem.HasUsesRemaining;
        if (depleted)
        {
            ClearActivatableItem();
        }

        return true;
    }

    public void ClearActivatableItem()
    {
        ActivatableItem = null;
    }

    public void GrantTemporaryModifier(
        ContentId sourceId,
        StatId statId,
        ModifierOp operation,
        float value,
        int priority,
        TagId targetTag,
        float durationSeconds)
    {
        if (!sourceId.IsValid || !statId.IsValid || durationSeconds <= 0f)
        {
            return;
        }

        _temporaryModifiers.Add(new TemporaryModifierState(
            sourceId,
            statId,
            operation,
            value,
            priority,
            targetTag,
            durationSeconds));
    }

    public void TickTemporaryModifiers(float delta)
    {
        for (var i = _temporaryModifiers.Count - 1; i >= 0; i--)
        {
            _temporaryModifiers[i].Tick(delta);
            if (_temporaryModifiers[i].IsExpired)
            {
                _temporaryModifiers.RemoveAt(i);
            }
        }
    }
}
