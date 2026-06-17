using TheLastMage.Foundation;

namespace TheLastMage.Debugging.Tools;

public sealed class LootSimulationSummary
{
    private readonly Dictionary<ContentId, int> _offerCounts = new();

    public int Rolls { get; init; }

    public int Seed { get; init; }

    public int OffersGenerated { get; set; }

    public int EmptyRolls { get; set; }

    public int LockedItemsExcluded { get; init; }

    public int DiscoveredOffers { get; set; }

    public int UndiscoveredOffers { get; set; }

    public IReadOnlyDictionary<ContentId, int> OfferCounts => _offerCounts;

    public void RecordOffer(ContentId itemId, bool discovered)
    {
        OffersGenerated++;
        if (discovered)
        {
            DiscoveredOffers++;
        }
        else
        {
            UndiscoveredOffers++;
        }

        _offerCounts[itemId] = _offerCounts.TryGetValue(itemId, out var count) ? count + 1 : 1;
    }
}
