using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public sealed class RerollOffersCommand : ICommand
{
    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.State.PhasePermissions.CanRerollOffers)
        {
            reason = $"phase {context.State.CurrentPhase} cannot reroll offers";
            return false;
        }

        if (context.State.Market.ChoiceCompletedThisNight)
        {
            reason = "market is closed for this night";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        context.GetSystem<MarketSystem>().RerollOffers(out _);
    }
}
