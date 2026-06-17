using TheLastMage.Foundation;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Gameplay.Commands;

public sealed class BeginCastFlowCommand : ICommand
{
    private readonly EntityId _casterId;
    private readonly int _slotIndex;

    public BeginCastFlowCommand(EntityId casterId, int slotIndex)
    {
        _casterId = casterId;
        _slotIndex = slotIndex;
    }

    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.State.PhasePermissions.CanCastSpells)
        {
            reason = $"phase {context.State.CurrentPhase} cannot cast spells";
            return false;
        }

        if (context.State.Spellbook.CastFlow.IsActive)
        {
            reason = "another cast flow is already active";
            return false;
        }

        if (!context.State.Spellbook.TryGetSlot(_slotIndex, out var slot) || slot == null || !slot.HasSpell)
        {
            reason = $"spell slot {_slotIndex + 1} is empty";
            return false;
        }

        if (!slot.IsReady)
        {
            reason = $"spell slot {_slotIndex + 1} is cooling down";
            return false;
        }

        if (!context.Content.TryGetSpell(slot.SpellId, out var spell)
            || spell == null
            || !ItemBehaviorResolver.TryCreateCastFlowPlan(context, spell, out var plan)
            || !plan.RequiresCharge)
        {
            reason = $"spell slot {_slotIndex + 1} does not use cast flow";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        context.GetSystem<SpellSystem>().BeginCastFlow(_casterId, _slotIndex);
    }
}
