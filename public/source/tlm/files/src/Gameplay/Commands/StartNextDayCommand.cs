using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public sealed class StartNextDayCommand : PhaseCommand
{
    public override void Execute(RunContext context)
    {
        context.State.DayIndex++;
        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
    }

    protected override bool IsAllowed(PhasePermissions permissions)
    {
        return permissions.CanStartNextDay;
    }
}
