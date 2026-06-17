using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Foundation.Random;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Loot;

public sealed class MarketSystem : IGameSystem
{
    private readonly OfferGenerator _offerGenerator = new();
    private RunContext? _context;
    private SubscriptionToken _nightStartedToken;

    public void Initialize(RunContext context)
    {
        _context = context;
        _nightStartedToken = context.Events.Subscribe<NightStartedEvent>(_ =>
        {
            if (_context?.State.CurrentPhase == RunPhase.NightMarket)
            {
                GenerateOffers();
            }
        });
    }

    public void Tick(float delta)
    {
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_nightStartedToken);
        }

        _context = null;
    }

    public void GenerateOffers()
    {
        if (_context == null)
        {
            return;
        }

        var offers = _offerGenerator.Generate(
            _context.Content,
            _context.Profile,
            _context.State.Build,
            _context.Random.Stream(RandomStreamId.Loot));
        _context.State.Market.SetOffers(offers);
        _context.Events.Publish(new MarketOffersGeneratedEvent(offers.Count));
        GD.Print($"MarketOffersGenerated count={offers.Count}");
    }

    public bool ChooseOffer(int offerIndex, out string reason)
    {
        reason = string.Empty;
        if (_context == null)
        {
            reason = "market system is not initialized";
            return false;
        }

        if (_context.State.Market.ChoiceCompletedThisNight)
        {
            reason = "an item has already been chosen this night";
            return false;
        }

        if (!_context.State.Market.TryGetOffer(offerIndex, out var offer) || offer == null)
        {
            reason = $"offer {offerIndex + 1} is not available";
            return false;
        }

        var item = _context.Content.GetItem(offer.ItemId);
        if (_context.State.Build.HasAcquiredItem(offer.ItemId))
        {
            reason = $"item {offer.ItemId} has already been chosen this run";
            return false;
        }

        AcquireItem(item);
        _context.ItemKnowledge.MarkDiscovered(offer.ItemId);
        _context.State.Market.CompleteChoice();
        _context.Events.Publish(new ItemAcquiredEvent(offer.ItemId));
        GD.Print($"ItemAcquired item={offer.ItemId}");
        _context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightDefense);
        return true;
    }

    public void AcquireItem(ItemRuntimeDefinition item)
    {
        if (_context == null)
        {
            return;
        }

        if (string.Equals(item.Kind, nameof(ItemKind.Activatable), StringComparison.OrdinalIgnoreCase))
        {
            var replaced = _context.State.Build.ActivatableItem?.ItemId ?? ContentId.From(string.Empty);
            _context.State.Build.EquipActivatableItem(item.Id, item.HasUnlimitedActivations, item.MaxActivations);
            var equipped = _context.State.Build.ActivatableItem;
            _context.State.DebugMetrics.LastActivatableItemSummary =
                $"equipped {item.Id} remaining={(item.HasUnlimitedActivations ? "unlimited" : equipped?.RemainingActivations.ToString() ?? "0")}";
            _context.Events.Publish(new ActivatableItemEquippedEvent(
                item.Id,
                replaced,
                equipped?.RemainingActivations ?? 0,
                item.HasUnlimitedActivations));
            return;
        }

        _context.State.Build.AddItem(item.Id);
    }

    public bool RerollOffers(out string reason)
    {
        reason = string.Empty;
        if (_context == null)
        {
            reason = "market system is not initialized";
            return false;
        }

        if (_context.State.Market.ChoiceCompletedThisNight)
        {
            reason = "market is closed for this night";
            return false;
        }

        _context.State.Market.RecordReroll();
        GenerateOffers();
        return true;
    }
}
