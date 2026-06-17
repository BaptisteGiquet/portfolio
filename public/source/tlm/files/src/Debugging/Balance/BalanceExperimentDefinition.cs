using TheLastMage.Foundation;

namespace TheLastMage.Debugging.Balance;

public enum BalanceRunMode
{
    ShortRun,
    VerticalSlice,
    AcceleratedLongRun,
    Stress
}

public enum BalanceItemProfileMode
{
    CurrentProfile,
    AllProductionUnlocked,
    AllProductionUnlockedAndDiscovered
}

public sealed record BalanceExperimentDefinition
{
    public string ExperimentId { get; init; } = "sprint_13c_default";

    public string Description { get; init; } = "Sprint 13C deterministic balance lab default experiment.";

    public int SeedStart { get; init; } = 130_000;

    public int SeedCount { get; init; } = 1_000;

    public int TotalSimulationRuns { get; init; }

    public IReadOnlyList<int> ExplicitSeeds { get; init; } = Array.Empty<int>();

    public BalanceRunMode RunMode { get; init; } = BalanceRunMode.VerticalSlice;

    public BalanceItemProfileMode ItemProfileMode { get; init; } = BalanceItemProfileMode.AllProductionUnlockedAndDiscovered;

    public int StartingHumanity { get; init; } = 1_000;

    public IReadOnlyList<ContentId> MageIncludeList { get; init; } = Array.Empty<ContentId>();

    public IReadOnlyList<ContentId> SpellPool { get; init; } = Array.Empty<ContentId>();

    public ContentId ItemPoolId { get; init; } = ContentId.From(ItemPoolIds.NightMarket);

    public IReadOnlyList<string> BotPolicyIds { get; init; } = new[]
    {
        BalanceBotPolicyIds.RandomBuild,
        BalanceBotPolicyIds.AggressiveDamage,
        BalanceBotPolicyIds.Survival,
        BalanceBotPolicyIds.Synergy,
        BalanceBotPolicyIds.StationaryOutput
    };

    public int MaxSimulatedDays { get; init; } = 3;

    public float MaxSimulatedSecondsPerDay { get; init; } = 20f;

    public int MonteCarloTrials { get; init; } = 10_000;

    public int MonteCarloNightsPerTrial { get; init; } = 8;

    public bool WriteMarkdown { get; init; } = true;

    public bool WriteCsv { get; init; } = true;

    public bool WriteJsonl { get; init; } = true;

    public IReadOnlyList<int> EnumerateSeeds()
    {
        if (ExplicitSeeds.Count > 0)
        {
            return ExplicitSeeds;
        }

        var seeds = new int[Math.Max(0, SeedCount)];
        for (var i = 0; i < seeds.Length; i++)
        {
            seeds[i] = SeedStart + i;
        }

        return seeds;
    }

    public BalanceExperimentDefinition WithSmokeScale(int seedCount, int monteCarloTrials)
    {
        return new BalanceExperimentDefinition
        {
            ExperimentId = $"{ExperimentId}_smoke",
            Description = Description,
            SeedStart = SeedStart,
            SeedCount = Math.Max(1, seedCount),
            TotalSimulationRuns = 0,
            ExplicitSeeds = Array.Empty<int>(),
            RunMode = BalanceRunMode.ShortRun,
            ItemProfileMode = ItemProfileMode,
            StartingHumanity = StartingHumanity,
            MageIncludeList = MageIncludeList,
            SpellPool = SpellPool,
            ItemPoolId = ItemPoolId,
            BotPolicyIds = BotPolicyIds,
            MaxSimulatedDays = 2,
            MaxSimulatedSecondsPerDay = 8f,
            MonteCarloTrials = Math.Max(1, monteCarloTrials),
            MonteCarloNightsPerTrial = Math.Min(4, Math.Max(1, MonteCarloNightsPerTrial)),
            WriteMarkdown = WriteMarkdown,
            WriteCsv = WriteCsv,
            WriteJsonl = WriteJsonl
        };
    }

    public static BalanceExperimentDefinition Sprint13CDefault()
    {
        return new BalanceExperimentDefinition
        {
            Description = "Sprint 13C deep deterministic balance lab: 1000 production-system bot runs with broad spell, mage, item, and policy coverage.",
            RunMode = BalanceRunMode.AcceleratedLongRun,
            StartingHumanity = 1_000,
            TotalSimulationRuns = 1_000,
            MaxSimulatedDays = 20,
            MaxSimulatedSecondsPerDay = 300f,
            MonteCarloTrials = 25_000,
            MonteCarloNightsPerTrial = 20
        };
    }
}
