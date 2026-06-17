namespace TheLastMage.Presentation.Feedback;

public sealed class FeedbackBudget
{
    private readonly Dictionary<string, int> _maxSimultaneous = new(StringComparer.OrdinalIgnoreCase)
    {
        ["cast"] = 16,
        ["projectile"] = 64,
        ["impact"] = 48,
        ["aoe"] = 24,
        ["status"] = 40,
        ["summon"] = 16,
        ["boss"] = 12,
        ["ui"] = 12,
        ["audio_horde"] = 10
    };

    public IReadOnlyDictionary<string, int> MaxSimultaneous => _maxSimultaneous;

    public bool CanRequest(string category, int activeCount)
    {
        return !_maxSimultaneous.TryGetValue(category, out var max) || activeCount < max;
    }

    public string BuildSummary()
    {
        return string.Join(", ", _maxSimultaneous.Select(pair => $"{pair.Key}:{pair.Value}"));
    }
}
