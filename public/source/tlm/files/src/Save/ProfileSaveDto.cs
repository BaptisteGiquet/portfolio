namespace TheLastMage.Save;

public sealed class ProfileSaveDto
{
    public string PreferredMageId { get; set; } = ProfileDefaultsService.DefaultMageId;

    public List<string> UnlockedMageIds { get; set; } = new();

    public List<string> UnlockedSpellIds { get; set; } = new();

    public List<string> UnlockedItemIds { get; set; } = new();

    public List<string> DiscoveredItemIds { get; set; } = new();

    public List<string> CompletedAchievementIds { get; set; } = new();

    public ProfileRunStatisticsDto RunStatistics { get; set; } = new();

    public RunSummarySaveDto? LastCompletedRun { get; set; }

    public RunCheckpointSaveDto? SuspendedRun { get; set; }
}

public sealed class ProfileRunStatisticsDto
{
    public int TotalNormalRuns { get; set; }

    public int Victories { get; set; }

    public int Defeats { get; set; }

    public int DebugOrTestRunsExcluded { get; set; }

    public float TotalPlaySeconds { get; set; }

    public int HighestDayReached { get; set; }

    public int TotalHumanityKilled { get; set; }

    public int TotalEnemiesKilled { get; set; }

    public int TotalSpellsCast { get; set; }

    public int TotalItemsDiscovered { get; set; }

    public Dictionary<string, int> RunsByMage { get; set; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> SpellCastsBySpell { get; set; } = new(StringComparer.Ordinal);

    public Dictionary<string, ProfileMageStatisticsDto> MageStatistics { get; set; } = new(StringComparer.Ordinal);
}

public sealed class ProfileMageStatisticsDto
{
    public int RunsPlayed { get; set; }

    public int Wins { get; set; }

    public int Deaths { get; set; }

    public int BestHumanityKilled { get; set; }

    public float BestTimeSurvivedSeconds { get; set; }

    public int TotalSpellCasts { get; set; }

    public Dictionary<string, int> SpellCastsBySpell { get; set; } = new(StringComparer.Ordinal);
}

public sealed class RunSummarySaveDto
{
    public string RunId { get; set; } = string.Empty;

    public string Result { get; set; } = string.Empty;

    public bool IsDebugOrTestRun { get; set; }

    public float DurationSeconds { get; set; }

    public int DayReached { get; set; }

    public int HumanityKilled { get; set; }

    public int EnemiesKilled { get; set; }

    public int TotalSpellsCast { get; set; }

    public Dictionary<string, int> SpellsUsed { get; set; } = new(StringComparer.Ordinal);

    public List<string> ItemsDiscovered { get; set; } = new();

    public string MageUsed { get; set; } = string.Empty;

    public string MageName { get; set; } = string.Empty;

    public string Cause { get; set; } = string.Empty;
}

public sealed class RunCheckpointSaveDto
{
    public string RunId { get; set; } = string.Empty;

    public int Seed { get; set; }

    public string Phase { get; set; } = string.Empty;

    public string CheckpointLabel { get; set; } = string.Empty;

    public int DayIndex { get; set; } = 1;

    public int HumanityRemaining { get; set; } = 100;

    public int Souls { get; set; }

    public int Materials { get; set; } = 40;

    public string MageId { get; set; } = ProfileDefaultsService.DefaultMageId;

    public float TotalElapsedSeconds { get; set; }

    public float PhaseElapsedSeconds { get; set; }

    public float IntensitySeconds { get; set; }

    public List<RunSpellSlotSaveDto> SpellSlots { get; set; } = new();

    public int SelectedSpellSlotIndex { get; set; }

    public List<RunItemStackSaveDto> Items { get; set; } = new();

    public List<string> AcquiredItemIds { get; set; } = new();

    public RunActivatableItemSaveDto? ActivatableItem { get; set; }

    public List<string> MarketOfferItemIds { get; set; } = new();

    public int MarketRerollCount { get; set; }

    public bool MarketChoiceCompletedThisNight { get; set; }

    public int EnemiesKilled { get; set; }

    public int HumanityKilled { get; set; }

    public int TotalSpellsCast { get; set; }

    public Dictionary<string, int> SpellsUsed { get; set; } = new(StringComparer.Ordinal);

    public List<string> ItemsDiscoveredThisRun { get; set; } = new();
}

public sealed class RunSpellSlotSaveDto
{
    public int SlotIndex { get; set; }

    public string SpellId { get; set; } = string.Empty;

    public float CooldownRemainingSeconds { get; set; }
}

public sealed class RunItemStackSaveDto
{
    public string ItemId { get; set; } = string.Empty;

    public int Count { get; set; }
}

public sealed class RunActivatableItemSaveDto
{
    public string ItemId { get; set; } = string.Empty;

    public bool HasUnlimitedActivations { get; set; }

    public int RemainingActivations { get; set; }

    public int MaxActivations { get; set; }
}
