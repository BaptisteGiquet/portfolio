using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Run;

public sealed class RunTelemetryState
{
    private readonly Dictionary<string, int> _spellCasts = new(StringComparer.Ordinal);
    private readonly List<string> _itemsDiscovered = new();
    private readonly HashSet<string> _itemsDiscoveredSet = new(StringComparer.Ordinal);

    public int EnemiesKilled { get; private set; }

    public int HumanityKilled { get; private set; }

    public int TotalSpellsCast { get; private set; }

    public IReadOnlyDictionary<string, int> SpellCasts => _spellCasts;

    public IReadOnlyList<string> ItemsDiscovered => _itemsDiscovered;

    public void RecordEnemyKilled(int humanityReduced)
    {
        EnemiesKilled++;
        HumanityKilled += Math.Max(0, humanityReduced);
    }

    public void RecordSpellCast(ContentId spellId)
    {
        if (!spellId.IsValid)
        {
            return;
        }

        TotalSpellsCast++;
        var key = spellId.ToString();
        _spellCasts[key] = _spellCasts.GetValueOrDefault(key) + 1;
    }

    public void RecordItemDiscovered(ContentId itemId)
    {
        if (!itemId.IsValid || !_itemsDiscoveredSet.Add(itemId.Value))
        {
            return;
        }

        _itemsDiscovered.Add(itemId.Value);
    }

    public void Restore(
        int enemiesKilled,
        int humanityKilled,
        int totalSpellsCast,
        IReadOnlyDictionary<string, int> spellCasts,
        IEnumerable<string> itemsDiscovered)
    {
        EnemiesKilled = Math.Max(0, enemiesKilled);
        HumanityKilled = Math.Max(0, humanityKilled);
        TotalSpellsCast = Math.Max(0, totalSpellsCast);
        _spellCasts.Clear();
        foreach (var pair in spellCasts)
        {
            if (!string.IsNullOrWhiteSpace(pair.Key) && pair.Value > 0)
            {
                _spellCasts[pair.Key] = pair.Value;
            }
        }

        _itemsDiscovered.Clear();
        _itemsDiscoveredSet.Clear();
        foreach (var itemId in itemsDiscovered)
        {
            if (!string.IsNullOrWhiteSpace(itemId) && _itemsDiscoveredSet.Add(itemId))
            {
                _itemsDiscovered.Add(itemId);
            }
        }
    }

    public RunSummaryData CreateSummary(RunState state, string result, string cause)
    {
        return new RunSummaryData
        {
            RunId = state.RunId.ToString(),
            Result = result,
            IsDebugOrTestRun = !state.CountsTowardPlayerStats,
            DurationSeconds = state.Clock.TotalElapsedSeconds,
            DayReached = state.DayIndex,
            HumanityKilled = HumanityKilled,
            EnemiesKilled = EnemiesKilled,
            TotalSpellsCast = TotalSpellsCast,
            SpellsUsed = new Dictionary<string, int>(_spellCasts, StringComparer.Ordinal),
            ItemsDiscovered = _itemsDiscovered.ToList(),
            MageUsed = state.SelectedMageId.ToString(),
            MageName = state.SelectedMageName,
            Cause = cause
        };
    }
}
