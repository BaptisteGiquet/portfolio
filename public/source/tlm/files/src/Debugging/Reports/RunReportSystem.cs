using Godot;
using System.Globalization;
using System.IO;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Waves;

namespace TheLastMage.Debugging.Reports;

public sealed class RunReportSystem : IGameSystem
{
    private readonly List<SubscriptionToken> _tokens = new();
    private RunContext? _context;
    private string _reportPath = string.Empty;
    private float _elapsedSeconds;
    private float _snapshotTimer;
    private RunPhase _lastPhase = RunPhase.RunSetup;
    private int _lastDayIndex;

    public void Initialize(RunContext context)
    {
        _context = context;
        _lastPhase = context.State.CurrentPhase;
        _lastDayIndex = context.State.DayIndex;
        InitializeReport();
        context.State.DebugMetrics.ReproductionSummary =
            $"run={context.State.RunId} seed={context.Random.Seed} mage={context.State.SelectedMageId}";
        var balanceReportPath = BalanceReport.Write(context);
        Subscribe(context.Events);
        Write($"RUN START run={context.State.RunId} seed={context.Random.Seed} phase={context.State.CurrentPhase} day={context.State.DayIndex}");
        Write($"REPRODUCTION {context.State.DebugMetrics.ReproductionSummary}");
        Write($"BALANCE_REPORT path={balanceReportPath}");
    }

    public void Tick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        _elapsedSeconds += delta;
        if (_context.State.CurrentPhase != _lastPhase || _context.State.DayIndex != _lastDayIndex)
        {
            WriteSnapshot($"STATE phase/day changed from {_lastPhase}/day{_lastDayIndex}");
            _lastPhase = _context.State.CurrentPhase;
            _lastDayIndex = _context.State.DayIndex;
        }

        _snapshotTimer -= delta;
        if (_snapshotTimer <= 0f)
        {
            _snapshotTimer = 10f;
            WriteSnapshot("SNAPSHOT");
        }
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            foreach (var token in _tokens)
            {
                _context.Events.Unsubscribe(token);
            }

            WriteSnapshot("RUN SHUTDOWN");
        }

        _tokens.Clear();
        _context = null;
    }

    private void InitializeReport()
    {
        var directory = RuntimePathResolver.GlobalizeUserPath("user://run_reports");
        Directory.CreateDirectory(directory);
        _reportPath = Path.Combine(directory, $"tlm_run_{DateTime.Now:yyyyMMdd_HHmmss}.log");
        _context!.State.DebugMetrics.RunReportPath = _reportPath;
        File.WriteAllText(_reportPath, "The Last Mage run report\n");
    }

    private void Subscribe(GameEventBus events)
    {
        _tokens.Add(events.Subscribe<RunStartedEvent>(e => Write($"EVENT RunStarted run={e.RunId}")));
        _tokens.Add(events.Subscribe<DayStartedEvent>(e => WriteSnapshot($"EVENT DayStarted day={e.DayIndex}")));
        _tokens.Add(events.Subscribe<NightStartedEvent>(e => WriteSnapshot($"EVENT NightStarted day={e.DayIndex} phase={_context?.State.CurrentPhase}")));
        _tokens.Add(events.Subscribe<RunVictoryEvent>(e => WriteSnapshot($"EVENT RunVictory day={e.DayIndex} humanity={e.HumanityRemaining}")));
        _tokens.Add(events.Subscribe<RunDefeatEvent>(e => WriteSnapshot($"EVENT RunDefeat day={e.DayIndex} source={e.SourceId} cause={_context?.State.DebugMetrics.DeathCauseSummary}")));
        _tokens.Add(events.Subscribe<RunCompletedEvent>(e => WriteSnapshot($"EVENT RunCompleted result={e.Summary.Result} day={e.Summary.DayReached} stats={(e.Summary.IsDebugOrTestRun ? "excluded" : "counted")} cause={e.Summary.Cause}")));
        _tokens.Add(events.Subscribe<DefensePlacedEvent>(e => Write($"EVENT DefensePlaced defense={e.DefenseId} entity={e.DefenseEntityId} pos={Format(e.Position)}")));
        _tokens.Add(events.Subscribe<DefenseDestroyedEvent>(e => WriteSnapshot($"EVENT DefenseDestroyed defense={e.DefenseEntityId} archetype={e.DefenseId} source={e.SourceId}")));
        _tokens.Add(events.Subscribe<WaveSpawnedEnemyEvent>(e => Write($"EVENT WaveSpawnedEnemy wave={e.WaveId} enemy={e.EnemyArchetypeId} entity={e.EnemyId}")));
        _tokens.Add(events.Subscribe<EnemyDiedEvent>(e => Write($"EVENT EnemyDied enemy={e.EnemyId} archetype={e.EnemyArchetypeId} source={e.SourceId}")));
        _tokens.Add(events.Subscribe<MageDiedEvent>(e => WriteSnapshot($"EVENT MageDied mage={e.MageId} source={e.SourceId}")));
        _tokens.Add(events.Subscribe<RunRewardsGrantedEvent>(e => Write($"EVENT Rewards source={e.SourceId} souls={e.Souls} materials={e.Materials} humanityReduced={e.HumanityReduced}")));
        _tokens.Add(events.Subscribe<MarketOffersGeneratedEvent>(e => WriteSnapshot($"EVENT MarketOffersGenerated count={e.OfferCount}")));
        _tokens.Add(events.Subscribe<ItemAcquiredEvent>(e => WriteSnapshot($"EVENT ItemAcquired item={e.ItemId}")));
        _tokens.Add(events.Subscribe<ActivatableItemEquippedEvent>(e => WriteSnapshot($"EVENT ActivatableEquipped item={e.ItemId} replaced={e.ReplacedItemId} remaining={e.RemainingActivations} unlimited={e.HasUnlimitedActivations}")));
        _tokens.Add(events.Subscribe<ActivatableItemUsedEvent>(e => WriteSnapshot($"EVENT ActivatableUsed item={e.ItemId} remaining={e.RemainingActivations} depleted={e.Depleted}")));
        _tokens.Add(events.Subscribe<ActivatableItemClearedEvent>(e => WriteSnapshot($"EVENT ActivatableCleared item={e.ItemId}")));
        _tokens.Add(events.Subscribe<AchievementCompletedEvent>(e => Write($"EVENT AchievementCompleted achievement={e.AchievementId}")));
    }

    private void WriteSnapshot(string label)
    {
        if (_context == null)
        {
            return;
        }

        var state = _context.State;
        var enemies = _context.TryGetSystem<EnemySystem>(out var enemySystem) ? enemySystem : null;
        var defenses = _context.TryGetSystem<DefenseSystem>(out var defenseSystem) ? defenseSystem : null;
        var waves = _context.TryGetSystem<WaveDirectorSystem>(out var waveSystem) ? waveSystem : null;
        var combat = _context.TryGetSystem<CombatSystem>(out var combatSystem) ? combatSystem : null;
        var playerHealth = "-";
        if (combat?.TryGetHealth(state.Player.EntityId, out var health) == true && health != null)
        {
            playerHealth = $"{health.CurrentHealth:0.#}/{health.MaxHealth:0.#}";
        }

        Write(
            $"{label} phase={state.CurrentPhase} day={state.DayIndex} time={state.Clock.PhaseElapsedSeconds:0.0}s " +
            $"player={Format(state.Player.Position)} hp={playerHealth} souls={state.Souls} materials={state.Materials} humanity={state.HumanityRemaining} " +
            $"enemies={enemies?.ActiveCount ?? 0} defenses={defenses?.ActiveCount ?? 0} waveActive={waves?.HasActiveWave ?? false} " +
            $"waveTarget={state.DebugMetrics.WaveTargetActiveEnemies} waveSpawned={state.DebugMetrics.WaveSpawnedThisDay} " +
            $"tower={state.DebugMetrics.TowerRouteState}/{state.DebugMetrics.TowerEntranceState} routeStages={state.DebugMetrics.TowerRouteStageSummary} " +
            $"offers={state.Market.Offers.Count} items={state.Build.Items.Count} profiling=\"{state.DebugMetrics.ProfilingSummary}\" " +
            $"activatable=\"{state.DebugMetrics.LastActivatableItemSummary}\" " +
            $"lastMageDamage=\"{state.DebugMetrics.LastMageDamageSummary}\" death=\"{state.DebugMetrics.DeathCauseSummary}\"");
    }

    private void Write(string line)
    {
        if (string.IsNullOrWhiteSpace(_reportPath))
        {
            return;
        }

        File.AppendAllLines(_reportPath, new[] { $"{_elapsedSeconds:0000.0}s {line}" });
    }

    private static string Format(Vector3 value)
    {
        var culture = CultureInfo.InvariantCulture;
        return string.Create(culture, $"({value.X:0.##},{value.Y:0.##},{value.Z:0.##})");
    }
}
