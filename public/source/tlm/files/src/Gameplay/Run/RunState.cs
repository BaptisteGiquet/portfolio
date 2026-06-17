using TheLastMage.Foundation;
using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Player;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Run;

public sealed class RunState
{
    private int _nextEntityId = 1;
    private readonly List<StatBreakdown> _recentStatBreakdowns = new();
    private readonly List<ContentId> _selectedMageStartingSpellIds = new();

    public RunId RunId { get; init; } = RunId.NewRun();

    public int DayIndex { get; set; } = 1;

    public int HumanityRemaining { get; set; } = 100;

    public int Souls { get; set; }

    public int Materials { get; set; } = 40;

    public bool IsBenchmarkRun { get; set; }

    public bool IsDebugRun { get; set; }

    public bool CountsTowardPlayerStats => !IsBenchmarkRun && !IsDebugRun;

    public bool SuppressAutomaticEnemySpawns { get; set; }

    public bool IsDebugUiInputCaptured { get; set; }

    public RunPhase CurrentPhase { get; private set; } = RunPhase.RunSetup;

    public PhasePermissions PhasePermissions { get; private set; } = PhasePermissions.For(RunPhase.RunSetup);

    public RunClock Clock { get; } = new();

    public BuildState Build { get; } = new();

    public MarketState Market { get; } = new();

    public PlayerRuntimeState Player { get; } = new();

    public SpellbookState Spellbook { get; } = new();

    public RunDebugMetrics DebugMetrics { get; } = new();

    public RunTelemetryState Telemetry { get; } = new();

    public RunSummaryData? LastRunSummary { get; private set; }

    public float PlayerStasisRemainingSeconds { get; private set; }

    public ContentId PlayerStasisSourceId { get; private set; } = ContentId.From("none");

    public bool IsPlayerStasisActive => PlayerStasisRemainingSeconds > 0f;

    public TemporaryWeaponModeState? TemporaryWeaponMode { get; private set; }

    public ContentId SelectedMageId { get; private set; } = ContentId.From("mage_recluse");

    public string SelectedMageName { get; private set; } = "The Recluse";

    public IReadOnlyList<ContentId> SelectedMageStartingSpellIds => _selectedMageStartingSpellIds;

    public IReadOnlyList<StatBreakdown> RecentStatBreakdowns => _recentStatBreakdowns;

    public EntityId AllocateEntityId()
    {
        return new EntityId(_nextEntityId++);
    }

    public void SetPhase(RunPhase phase)
    {
        CurrentPhase = phase;
        PhasePermissions = PhasePermissions.For(phase);
        Clock.ResetPhase();
    }

    public void RecordStatBreakdown(StatBreakdown breakdown)
    {
        if (_recentStatBreakdowns.Count >= 24)
        {
            _recentStatBreakdowns.RemoveAt(0);
        }

        _recentStatBreakdowns.Add(breakdown);
    }

    public void ClearStatBreakdowns()
    {
        _recentStatBreakdowns.Clear();
    }

    public void SelectMage(MageRuntimeDefinition mage)
    {
        SelectedMageId = mage.Id;
        SelectedMageName = mage.DisplayName;
        Player.ApplyMage(mage.Id, mage.MaxHealth, mage.MoveSpeed, mage.FireRate, mage.Luck);
        _selectedMageStartingSpellIds.Clear();
        foreach (var spellId in mage.StartingSpellIds)
        {
            if (_selectedMageStartingSpellIds.Count >= SpellbookState.MaxSlots)
            {
                break;
            }

            _selectedMageStartingSpellIds.Add(spellId);
        }
    }

    public void MarkDebugRun(string reason)
    {
        IsDebugRun = true;
        DebugMetrics.LastRunModeSummary = string.IsNullOrWhiteSpace(reason)
            ? "debug/test run; excluded from player stats"
            : $"debug/test run; excluded from player stats ({reason})";
    }

    public void StoreRunSummary(RunSummaryData summary)
    {
        LastRunSummary = summary;
        DebugMetrics.LastRunSummary =
            $"{summary.Result} day={summary.DayReached} humanityKilled={summary.HumanityKilled} " +
            $"kills={summary.EnemiesKilled} spells={summary.TotalSpellsCast} stats={(summary.IsDebugOrTestRun ? "excluded" : "counted")} cause={summary.Cause}";
    }

    public void GrantPlayerStasis(ContentId sourceId, float durationSeconds)
    {
        if (!sourceId.IsValid || durationSeconds <= 0f)
        {
            return;
        }

        PlayerStasisSourceId = sourceId;
        PlayerStasisRemainingSeconds = MathF.Max(PlayerStasisRemainingSeconds, durationSeconds);
    }

    public void GrantTemporaryWeaponMode(ContentId sourceId, float durationSeconds, float fireRateMultiplier)
    {
        if (!sourceId.IsValid || durationSeconds <= 0f)
        {
            return;
        }

        TemporaryWeaponMode = new TemporaryWeaponModeState(sourceId, durationSeconds, fireRateMultiplier);
    }

    public void TickTemporaryRunEffects(float delta)
    {
        var step = MathF.Max(0f, delta);
        PlayerStasisRemainingSeconds = MathF.Max(0f, PlayerStasisRemainingSeconds - step);
        Build.TickTemporaryModifiers(step);
        TemporaryWeaponMode?.Tick(step);
        if (TemporaryWeaponMode?.IsActive == false)
        {
            TemporaryWeaponMode = null;
        }
    }
}
