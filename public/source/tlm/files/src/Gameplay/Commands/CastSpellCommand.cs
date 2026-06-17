using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Gameplay.Commands;

public sealed class CastSpellCommand : ICommand
{
    private readonly EntityId _casterId;
    private readonly int _slotIndex;
    private readonly Vector3 _origin;
    private readonly Vector3 _direction;

    public CastSpellCommand(EntityId casterId, int slotIndex, Vector3 origin, Vector3 direction)
    {
        _casterId = casterId;
        _slotIndex = slotIndex;
        _origin = origin;
        _direction = direction.Normalized();
    }

    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.State.PhasePermissions.CanCastSpells)
        {
            reason = $"phase {context.State.CurrentPhase} cannot cast spells";
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

        if (context.Content.TryGetSpell(slot.SpellId, out var spell)
            && spell != null
            && ItemBehaviorResolver.TryCreateCastFlowPlan(context, spell, out var plan)
            && plan.SuppressesNormalCasting)
        {
            reason = $"spell slot {_slotIndex + 1} requires charge/release cast flow";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        context.GetSystem<SpellSystem>().Cast(_casterId, _slotIndex, _origin, _direction);
    }
}
