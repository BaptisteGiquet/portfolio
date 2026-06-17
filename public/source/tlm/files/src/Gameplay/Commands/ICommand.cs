using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public interface ICommand
{
    bool CanExecute(RunContext context, out string reason);

    void Execute(RunContext context);
}
