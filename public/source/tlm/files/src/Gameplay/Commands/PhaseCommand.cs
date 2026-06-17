using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public abstract class PhaseCommand : ICommand
{
    public bool CanExecute(RunContext context, out string reason)
    {
        if (!IsAllowed(context.State.PhasePermissions))
        {
            reason = $"{GetType().Name} is not allowed during {context.State.CurrentPhase}.";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public abstract void Execute(RunContext context);

    protected abstract bool IsAllowed(PhasePermissions permissions);
}
