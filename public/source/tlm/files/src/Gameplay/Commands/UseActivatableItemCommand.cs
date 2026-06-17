using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public sealed class UseActivatableItemCommand : ICommand
{
    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.State.PhasePermissions.CanCastSpells)
        {
            reason = $"phase {context.State.CurrentPhase} cannot use activatable items";
            return false;
        }

        if (context.State.Build.ActivatableItem == null)
        {
            reason = "no activatable item is equipped";
            return false;
        }

        if (!context.State.Build.ActivatableItem.HasUsesRemaining)
        {
            reason = "equipped activatable item has no remaining activations";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        context.GetSystem<ActivatableItemSystem>().UseEquippedItem(out _);
    }
}
