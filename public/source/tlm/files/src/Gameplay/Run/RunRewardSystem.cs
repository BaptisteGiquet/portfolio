using Godot;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Save;

namespace TheLastMage.Gameplay.Run;

public sealed class RunRewardSystem : IGameSystem
{
    private const int SoulsPerEnemy = 2;
    private const int MaterialsPerEnemy = 3;
    private const int HumanityReductionPerEnemy = 1;

    private RunContext? _context;
    private SubscriptionToken _enemyDiedToken;
    private SubscriptionToken _spellCastToken;
    private SubscriptionToken _itemAcquiredToken;
    private SubscriptionToken _victoryToken;
    private SubscriptionToken _defeatToken;

    public void Initialize(RunContext context)
    {
        _context = context;
        _enemyDiedToken = context.Events.Subscribe<EnemyDiedEvent>(OnEnemyDied);
        _spellCastToken = context.Events.Subscribe<SpellCastEvent>(OnSpellCast);
        _itemAcquiredToken = context.Events.Subscribe<ItemAcquiredEvent>(OnItemAcquired);
        _victoryToken = context.Events.Subscribe<RunVictoryEvent>(OnRunVictory);
        _defeatToken = context.Events.Subscribe<RunDefeatEvent>(OnRunDefeat);
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
            _context.Events.Unsubscribe(_enemyDiedToken);
            _context.Events.Unsubscribe(_spellCastToken);
            _context.Events.Unsubscribe(_itemAcquiredToken);
            _context.Events.Unsubscribe(_victoryToken);
            _context.Events.Unsubscribe(_defeatToken);
        }

        _context = null;
    }

    private void OnEnemyDied(EnemyDiedEvent gameEvent)
    {
        if (_context == null || _context.State.CurrentPhase is RunPhase.RunVictory or RunPhase.RunDefeat)
        {
            return;
        }

        _context.State.Souls += SoulsPerEnemy;
        _context.State.Materials += MaterialsPerEnemy;
        _context.State.HumanityRemaining = Math.Max(0, _context.State.HumanityRemaining - HumanityReductionPerEnemy);
        _context.State.Telemetry.RecordEnemyKilled(HumanityReductionPerEnemy);
        _context.Events.Publish(new RunRewardsGrantedEvent(
            gameEvent.EnemyArchetypeId,
            SoulsPerEnemy,
            MaterialsPerEnemy,
            HumanityReductionPerEnemy));
    }

    private void OnSpellCast(SpellCastEvent gameEvent)
    {
        _context?.State.Telemetry.RecordSpellCast(gameEvent.SpellId);
    }

    private void OnItemAcquired(ItemAcquiredEvent gameEvent)
    {
        _context?.State.Telemetry.RecordItemDiscovered(gameEvent.ItemId);
    }

    private void OnRunVictory(RunVictoryEvent gameEvent)
    {
        CompleteRun("victory", "humanity_zero");
    }

    private void OnRunDefeat(RunDefeatEvent gameEvent)
    {
        CompleteRun("defeat", gameEvent.SourceId.IsValid ? gameEvent.SourceId.ToString() : "mage_death");
    }

    private void CompleteRun(string result, string cause)
    {
        if (_context == null)
        {
            return;
        }

        var summary = _context.State.Telemetry.CreateSummary(_context.State, result, cause);
        _context.State.StoreRunSummary(summary);
        ApplyProfileStats(summary);
        _context.Profile.SuspendedRun = null;
        _context.Events.Publish(new RunCompletedEvent(summary));
        if (summary.IsDebugOrTestRun)
        {
            _context.State.DebugMetrics.LastRunModeSummary =
                "debug/test run completed; player statistics were not changed";
        }

        _context.SaveProfile(_context.Profile);
        GD.Print(
            $"RunEnded result={result} day={summary.DayReached} " +
            $"humanity={_context.State.HumanityRemaining} humanityKilled={summary.HumanityKilled} " +
            $"kills={summary.EnemiesKilled} spells={summary.TotalSpellsCast} stats={(summary.IsDebugOrTestRun ? "excluded" : "counted")}");
    }

    private void ApplyProfileStats(RunSummaryData summary)
    {
        if (_context == null)
        {
            return;
        }

        _context.Profile.LastCompletedRun = ToSaveDto(summary);
        if (summary.IsDebugOrTestRun)
        {
            _context.Profile.RunStatistics.DebugOrTestRunsExcluded++;
            return;
        }

        var stats = _context.Profile.RunStatistics;
        stats.TotalNormalRuns++;
        if (string.Equals(summary.Result, "victory", StringComparison.OrdinalIgnoreCase))
        {
            stats.Victories++;
        }
        else if (string.Equals(summary.Result, "defeat", StringComparison.OrdinalIgnoreCase))
        {
            stats.Defeats++;
        }

        stats.TotalPlaySeconds += MathF.Max(0f, summary.DurationSeconds);
        stats.HighestDayReached = Math.Max(stats.HighestDayReached, summary.DayReached);
        stats.TotalHumanityKilled += summary.HumanityKilled;
        stats.TotalEnemiesKilled += summary.EnemiesKilled;
        stats.TotalSpellsCast += summary.TotalSpellsCast;
        stats.TotalItemsDiscovered = _context.Profile.DiscoveredItemIds.Count;
        Increment(stats.RunsByMage, summary.MageUsed, 1);
        foreach (var spell in summary.SpellsUsed)
        {
            Increment(stats.SpellCastsBySpell, spell.Key, spell.Value);
        }

        var mageStats = GetOrCreateMageStats(stats, summary.MageUsed);
        mageStats.RunsPlayed++;
        if (string.Equals(summary.Result, "victory", StringComparison.OrdinalIgnoreCase))
        {
            mageStats.Wins++;
        }
        else if (string.Equals(summary.Result, "defeat", StringComparison.OrdinalIgnoreCase))
        {
            mageStats.Deaths++;
        }

        mageStats.BestHumanityKilled = Math.Max(mageStats.BestHumanityKilled, summary.HumanityKilled);
        mageStats.BestTimeSurvivedSeconds = MathF.Max(mageStats.BestTimeSurvivedSeconds, summary.DurationSeconds);
        mageStats.TotalSpellCasts += summary.TotalSpellsCast;
        foreach (var spell in summary.SpellsUsed)
        {
            Increment(mageStats.SpellCastsBySpell, spell.Key, spell.Value);
        }
    }

    private static RunSummarySaveDto ToSaveDto(RunSummaryData summary)
    {
        return new RunSummarySaveDto
        {
            RunId = summary.RunId,
            Result = summary.Result,
            IsDebugOrTestRun = summary.IsDebugOrTestRun,
            DurationSeconds = summary.DurationSeconds,
            DayReached = summary.DayReached,
            HumanityKilled = summary.HumanityKilled,
            EnemiesKilled = summary.EnemiesKilled,
            TotalSpellsCast = summary.TotalSpellsCast,
            SpellsUsed = new Dictionary<string, int>(summary.SpellsUsed, StringComparer.Ordinal),
            ItemsDiscovered = summary.ItemsDiscovered.ToList(),
            MageUsed = summary.MageUsed,
            MageName = summary.MageName,
            Cause = summary.Cause
        };
    }

    private static void Increment(Dictionary<string, int> values, string key, int amount)
    {
        if (string.IsNullOrWhiteSpace(key) || amount <= 0)
        {
            return;
        }

        values[key] = values.GetValueOrDefault(key) + amount;
    }

    private static ProfileMageStatisticsDto GetOrCreateMageStats(ProfileRunStatisticsDto stats, string mageId)
    {
        if (stats.MageStatistics.TryGetValue(mageId, out var mageStats))
        {
            return mageStats;
        }

        mageStats = new ProfileMageStatisticsDto();
        stats.MageStatistics[mageId] = mageStats;
        return mageStats;
    }
}
