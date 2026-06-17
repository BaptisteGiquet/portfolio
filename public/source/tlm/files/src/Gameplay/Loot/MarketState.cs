using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Loot;

public sealed class MarketState
{
    private readonly List<MarketOffer> _offers = new();

    public IReadOnlyList<MarketOffer> Offers => _offers;

    public int RerollCount { get; private set; }

    public bool ChoiceCompletedThisNight { get; private set; }

    public void SetOffers(IEnumerable<MarketOffer> offers)
    {
        _offers.Clear();
        _offers.AddRange(offers);
        ChoiceCompletedThisNight = false;
    }

    public bool TryGetOffer(int index, out MarketOffer? offer)
    {
        if (index < 0 || index >= _offers.Count)
        {
            offer = null;
            return false;
        }

        offer = _offers[index];
        return true;
    }

    public void RemoveOfferAt(int index)
    {
        if (index >= 0 && index < _offers.Count)
        {
            _offers.RemoveAt(index);
        }
    }

    public void CompleteChoice()
    {
        ChoiceCompletedThisNight = true;
        _offers.Clear();
    }

    public void RecordReroll()
    {
        RerollCount++;
    }

    public void Restore(IEnumerable<ContentId> offerItemIds, int rerollCount, bool choiceCompletedThisNight)
    {
        _offers.Clear();
        foreach (var itemId in offerItemIds)
        {
            if (itemId.IsValid)
            {
                _offers.Add(new MarketOffer(itemId));
            }
        }

        RerollCount = Math.Max(0, rerollCount);
        ChoiceCompletedThisNight = choiceCompletedThisNight;
    }
}
