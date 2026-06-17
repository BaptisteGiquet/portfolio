using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public sealed class ChooseMarketOfferCommand : ICommand
{
    private readonly int _offerIndex;

    public ChooseMarketOfferCommand(int offerIndex)
    {
        _offerIndex = offerIndex;
    }

    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.State.PhasePermissions.CanChooseMarketOffer)
        {
            reason = $"phase {context.State.CurrentPhase} cannot choose item offers";
            return false;
        }

        if (context.State.Market.ChoiceCompletedThisNight)
        {
            reason = "an item has already been chosen this night";
            return false;
        }

        if (!context.State.Market.TryGetOffer(_offerIndex, out var offer) || offer == null)
        {
            reason = $"offer {_offerIndex + 1} is not available";
            return false;
        }

        if (context.State.Build.HasAcquiredItem(offer.ItemId))
        {
            reason = $"item {offer.ItemId} has already been chosen this run";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        context.GetSystem<MarketSystem>().ChooseOffer(_offerIndex, out _);
    }
}
