namespace TheLastMage.Foundation;

public sealed class RunSummaryData
{
    public string RunId { get; init; } = string.Empty;

    public string Result { get; init; } = string.Empty;

    public bool IsDebugOrTestRun { get; init; }

    public float DurationSeconds { get; init; }

    public int DayReached { get; init; }

    public int HumanityKilled { get; init; }

    public int EnemiesKilled { get; init; }

    public int TotalSpellsCast { get; init; }

    public Dictionary<string, int> SpellsUsed { get; init; } = new(StringComparer.Ordinal);

    public List<string> ItemsDiscovered { get; init; } = new();

    public string MageUsed { get; init; } = string.Empty;

    public string MageName { get; init; } = string.Empty;

    public string Cause { get; init; } = string.Empty;
}
