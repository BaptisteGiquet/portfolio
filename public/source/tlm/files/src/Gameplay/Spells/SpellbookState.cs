using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Spells;

public sealed class SpellbookState
{
    public const int MaxSlots = 5;

    private readonly SpellSlotState[] _slots = Enumerable.Range(0, MaxSlots).Select(_ => new SpellSlotState()).ToArray();

    public IReadOnlyList<SpellSlotState> Slots => _slots;

    public CastFlowState CastFlow { get; } = new();

    public int SelectedSlotIndex { get; private set; }

    public bool TryGetSlot(int slotIndex, out SpellSlotState? slot)
    {
        if (slotIndex < 0 || slotIndex >= _slots.Length)
        {
            slot = null;
            return false;
        }

        slot = _slots[slotIndex];
        return true;
    }

    public void SelectSlot(int slotIndex)
    {
        if (slotIndex >= 0 && slotIndex < _slots.Length)
        {
            SelectedSlotIndex = slotIndex;
        }
    }

    public void AssignSlot(int slotIndex, ContentId spellId)
    {
        if (TryGetSlot(slotIndex, out var slot) && slot != null)
        {
            slot.Assign(spellId);
        }
    }

    public void RestoreSlot(int slotIndex, ContentId spellId, float cooldownRemainingSeconds)
    {
        if (TryGetSlot(slotIndex, out var slot) && slot != null)
        {
            slot.Restore(spellId, cooldownRemainingSeconds);
        }
    }

    public void ClearSlots()
    {
        foreach (var slot in _slots)
        {
            slot.Clear();
        }

        CastFlow.Clear();
        SelectedSlotIndex = 0;
    }

    public void TickCooldowns(float delta)
    {
        foreach (var slot in _slots)
        {
            slot.Tick(delta);
        }
    }
}
