using TheLastMage.Foundation;

namespace TheLastMage.Debugging.Balance;

public sealed class BalanceExperimentResult
{
    public string ExperimentId { get; init; } = string.Empty;

    public string Description { get; init; } = string.Empty;

    public DateTime StartedAtUtc { get; init; } = DateTime.UtcNow;

    public DateTime FinishedAtUtc { get; set; }

    public string ReportDirectory { get; set; } = string.Empty;

    public BalanceStaticAnalysisResult StaticAnalysis { get; init; } = new();

    public BalanceMonteCarloResult MonteCarlo { get; init; } = new();

    public List<BalanceRunResult> Runs { get; } = new();

    public List<string> Warnings { get; } = new();

    public IReadOnlyList<BalanceAggregateRow> AggregateByPolicy()
    {
        return Runs
            .GroupBy(run => run.BotPolicyId, StringComparer.Ordinal)
            .Select(group => BalanceAggregateRow.FromRuns(group.Key, group))
            .OrderBy(row => row.Key, StringComparer.Ordinal)
            .ToArray();
    }

    public string BuildDeterminismKey()
    {
        var policyRows = AggregateByPolicy()
            .Select(row => $"{row.Key}:{row.RunCount}:{row.WinRatePercent:0.000}:{row.AverageDayReached:0.000}:{row.AverageDamageOutput:0.000}:{row.AveragePerformancePeakMs:0.000}");
        var monteCarlo = MonteCarlo.PolicySummaries
            .OrderBy(row => row.PolicyId, StringComparer.Ordinal)
            .Select(row => $"{row.PolicyId}:{row.Trials}:{row.OffersGenerated}:{row.Picks}:{row.PoolStarvations}:{row.DuplicateOffersPrevented}");
        return string.Join("|", policyRows.Concat(monteCarlo));
    }
}

public sealed class BalanceRunResult
{
    public string RunId { get; init; } = string.Empty;

    public int Seed { get; init; }

    public string ExperimentId { get; init; } = string.Empty;

    public string RunMode { get; init; } = string.Empty;

    public string MageId { get; init; } = string.Empty;

    public string BotPolicyId { get; init; } = string.Empty;

    public string FocusSpellId { get; init; } = "-";

    public List<string> StartingSpellIds { get; } = new();

    public List<string> ItemAcquisitionOrder { get; } = new();

    public List<string> ActivatableHistory { get; } = new();

    public int Rerolls { get; set; }

    public string ItemProfileMode { get; init; } = string.Empty;

    public int DayReached { get; set; }

    public string Outcome { get; set; } = "timeout";

    public float ElapsedSimulatedSeconds { get; set; }

    public float FinalHealth { get; set; }

    public int Humanity { get; set; }

    public int Souls { get; set; }

    public int Materials { get; set; }

    public float TotalDamageDealt { get; set; }

    public Dictionary<string, float> DamageBySource { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> SpellCastCounts { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> SpellProjectileCounts { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> SpellBeamCounts { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> SpellAreaCounts { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> SpellSummonCounts { get; } = new(StringComparer.Ordinal);

    public int EnemyKills { get; set; }

    public Dictionary<string, int> KillsByEnemy { get; } = new(StringComparer.Ordinal);

    public float PlayerDamageTaken { get; set; }

    public string DeathCause { get; set; } = "-";

    public int ProcCapHits { get; set; }

    public int FeedbackDrops { get; set; }

    public float PerformancePeakMs { get; set; }

    public long AllocationPeakBytes { get; set; }

    public string ReproductionCommand { get; set; } = string.Empty;
}

public sealed class BalanceLabProgress
{
    public string Phase { get; init; } = "running";

    public int CompletedRuns { get; init; }

    public int TotalRuns { get; init; }

    public int Seed { get; init; }

    public string MageId { get; init; } = "-";

    public string BotPolicyId { get; init; } = "-";

    public string FocusSpellId { get; init; } = "-";

    public int CurrentDay { get; init; }

    public float CurrentRunSeconds { get; init; }

    public float TotalSimulatedSeconds { get; init; }

    public int Defeats { get; init; }

    public int Victories { get; init; }

    public int Timeouts { get; init; }

    public float AverageDayReached { get; init; }

    public float AverageDamageOutput { get; init; }

    public float AverageDamageTaken { get; init; }

    public string LastOutcome { get; init; } = "-";
}

public sealed class BalanceAggregateRow
{
    public string Key { get; init; } = string.Empty;

    public int RunCount { get; init; }

    public float WinRatePercent { get; init; }

    public float AverageDayReached { get; init; }

    public float AverageTimeSurvived { get; init; }

    public float AverageDamageOutput { get; init; }

    public float AveragePlayerDamageTaken { get; init; }

    public float AveragePerformancePeakMs { get; init; }

    public static BalanceAggregateRow FromRuns(string key, IEnumerable<BalanceRunResult> source)
    {
        var runs = source.ToArray();
        if (runs.Length == 0)
        {
            return new BalanceAggregateRow { Key = key };
        }

        return new BalanceAggregateRow
        {
            Key = key,
            RunCount = runs.Length,
            WinRatePercent = runs.Count(run => string.Equals(run.Outcome, "victory", StringComparison.OrdinalIgnoreCase)) * 100f / runs.Length,
            AverageDayReached = (float)runs.Average(run => run.DayReached),
            AverageTimeSurvived = (float)runs.Average(run => run.ElapsedSimulatedSeconds),
            AverageDamageOutput = (float)runs.Average(run => run.TotalDamageDealt),
            AveragePlayerDamageTaken = (float)runs.Average(run => run.PlayerDamageTaken),
            AveragePerformancePeakMs = (float)runs.Average(run => run.PerformancePeakMs)
        };
    }
}

public sealed class BalanceStaticAnalysisResult
{
    public int ProductionItemCount { get; set; }

    public int SpellCount { get; set; }

    public int MageCount { get; set; }

    public int EnemyCount { get; set; }

    public int WaveCount { get; set; }

    public Dictionary<string, int> ItemKinds { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> ItemTags { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> ItemPools { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> TargetTags { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> ModifierStats { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> DesignerEffectKinds { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> ProcKinds { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> ActiveEffectKinds { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> EnemyFamilies { get; } = new(StringComparer.Ordinal);

    public List<string> Warnings { get; } = new();
}

public sealed class BalanceMonteCarloResult
{
    public int Trials { get; set; }

    public int NightsPerTrial { get; set; }

    public int Seed { get; set; }

    public List<BalanceMonteCarloPolicySummary> PolicySummaries { get; } = new();
}

public sealed class BalanceMonteCarloPolicySummary
{
    public string PolicyId { get; init; } = string.Empty;

    public int Trials { get; set; }

    public int OffersGenerated { get; set; }

    public int Picks { get; set; }

    public int MissedOffers { get; set; }

    public int PoolStarvations { get; set; }

    public int LockedItemsExcluded { get; set; }

    public int DiscoveredOffers { get; set; }

    public int UndiscoveredOffers { get; set; }

    public int DuplicateOffersPrevented { get; set; }

    public Dictionary<string, int> OfferCounts { get; } = new(StringComparer.Ordinal);

    public Dictionary<string, int> PickCounts { get; } = new(StringComparer.Ordinal);
}

public sealed class BalanceRunEventRecorder
{
    private readonly List<Foundation.Events.SubscriptionToken> _tokens = new();
    private readonly EntityId _playerEntityId;

    public BalanceRunResult Result { get; }

    public BalanceRunEventRecorder(BalanceRunResult result, EntityId playerEntityId)
    {
        Result = result;
        _playerEntityId = playerEntityId;
    }

    public void Attach(Foundation.Events.GameEventBus events)
    {
        _tokens.Add(events.Subscribe<Foundation.Events.SpellCastEvent>(OnSpellCast));
        _tokens.Add(events.Subscribe<Foundation.Events.ProjectileSpawnedEvent>(e => Increment(Result.SpellProjectileCounts, e.SpellId.ToString())));
        _tokens.Add(events.Subscribe<Foundation.Events.BeamFiredEvent>(e => Increment(Result.SpellBeamCounts, e.SpellId.ToString())));
        _tokens.Add(events.Subscribe<Foundation.Events.AoESpawnedEvent>(e => Increment(Result.SpellAreaCounts, e.SpellId.ToString())));
        _tokens.Add(events.Subscribe<Foundation.Events.SummonSpawnedEvent>(e => Increment(Result.SpellSummonCounts, e.SpellId.ToString())));
        _tokens.Add(events.Subscribe<Foundation.Events.DamageAppliedEvent>(OnDamageApplied));
        _tokens.Add(events.Subscribe<Foundation.Events.EnemyDiedEvent>(OnEnemyDied));
        _tokens.Add(events.Subscribe<Foundation.Events.MageDiedEvent>(OnMageDied));
        _tokens.Add(events.Subscribe<Foundation.Events.ItemAcquiredEvent>(e => Result.ItemAcquisitionOrder.Add(e.ItemId.ToString())));
        _tokens.Add(events.Subscribe<Foundation.Events.ActivatableItemEquippedEvent>(e => Result.ActivatableHistory.Add($"equipped:{e.ItemId}:remaining={e.RemainingActivations}:unlimited={e.HasUnlimitedActivations}")));
        _tokens.Add(events.Subscribe<Foundation.Events.ActivatableItemUsedEvent>(e => Result.ActivatableHistory.Add($"used:{e.ItemId}:remaining={e.RemainingActivations}:depleted={e.Depleted}")));
        _tokens.Add(events.Subscribe<Foundation.Events.ActivatableItemClearedEvent>(e => Result.ActivatableHistory.Add($"cleared:{e.ItemId}")));
        _tokens.Add(events.Subscribe<Foundation.Events.ItemProcBlockedEvent>(_ => Result.ProcCapHits++));
    }

    public void Detach(Foundation.Events.GameEventBus events)
    {
        foreach (var token in _tokens)
        {
            events.Unsubscribe(token);
        }

        _tokens.Clear();
    }

    private void OnDamageApplied(Foundation.Events.DamageAppliedEvent gameEvent)
    {
        if (gameEvent.TargetId.Equals(_playerEntityId))
        {
            Result.PlayerDamageTaken += gameEvent.Amount;
            return;
        }

        Result.TotalDamageDealt += gameEvent.Amount;
        var key = gameEvent.SourceId.ToString();
        Result.DamageBySource[key] = Result.DamageBySource.GetValueOrDefault(key) + gameEvent.Amount;
    }

    private void OnSpellCast(Foundation.Events.SpellCastEvent gameEvent)
    {
        if (!gameEvent.CasterId.Equals(_playerEntityId))
        {
            return;
        }

        Increment(Result.SpellCastCounts, gameEvent.SpellId.ToString());
    }

    private void OnEnemyDied(Foundation.Events.EnemyDiedEvent gameEvent)
    {
        Result.EnemyKills++;
        var key = gameEvent.EnemyArchetypeId.ToString();
        Result.KillsByEnemy[key] = Result.KillsByEnemy.GetValueOrDefault(key) + 1;
    }

    private void OnMageDied(Foundation.Events.MageDiedEvent gameEvent)
    {
        Result.Outcome = "defeat";
        Result.DeathCause = gameEvent.SourceId.ToString();
    }

    private static void Increment(Dictionary<string, int> counts, string key)
    {
        counts[key] = counts.GetValueOrDefault(key) + 1;
    }
}
