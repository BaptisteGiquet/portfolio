namespace TheLastMage.Foundation.Events;

public sealed class GameEventCounter
{
    private readonly Dictionary<string, int> _frameCounts = new();
    private readonly Dictionary<string, long> _totalCounts = new();

    public IReadOnlyDictionary<string, int> FrameCounts => _frameCounts;

    public IReadOnlyDictionary<string, long> TotalCounts => _totalCounts;

    public void Attach(GameEventBus bus)
    {
        bus.EventPublished += Record;
    }

    public void Record(Type eventType)
    {
        var key = eventType.Name;
        _frameCounts[key] = _frameCounts.GetValueOrDefault(key) + 1;
        _totalCounts[key] = _totalCounts.GetValueOrDefault(key) + 1;
    }

    public void ResetFrame()
    {
        _frameCounts.Clear();
    }
}
