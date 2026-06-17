using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Gameplay.Commands;

public sealed class ReleaseCastFlowCommand : ICommand
{
    private readonly Vector3 _origin;
    private readonly Vector3 _direction;

    public ReleaseCastFlowCommand(Vector3 origin, Vector3 direction)
    {
        _origin = origin;
        _direction = direction.Normalized();
    }

    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.State.Spellbook.CastFlow.IsActive)
        {
            reason = "no active cast flow";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        context.GetSystem<SpellSystem>().ReleaseCastFlow(_origin, _direction);
    }
}
