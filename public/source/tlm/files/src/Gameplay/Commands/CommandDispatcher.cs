using Godot;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public sealed class CommandDispatcher
{
    private readonly Queue<string> _debugLog = new();

    public IReadOnlyCollection<string> DebugLog => _debugLog;

    public bool TryExecute(ICommand command, RunContext context, out string reason)
    {
        if (!command.CanExecute(context, out reason))
        {
            AppendDebug($"Rejected {command.GetType().Name}: {reason}");
            return false;
        }

        command.Execute(context);
        reason = string.Empty;
        AppendDebug($"Executed {command.GetType().Name}");
        return true;
    }

    private void AppendDebug(string message)
    {
        if (_debugLog.Count >= 64)
        {
            _debugLog.Dequeue();
        }

        _debugLog.Enqueue(message);
        GD.Print(message);
    }
}
