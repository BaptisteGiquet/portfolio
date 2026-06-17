using TheLastMage.Foundation.Events;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Waves;

namespace TheLastMage.Gameplay.Run;

public sealed class RunPhaseStateMachine : IGameSystem
{
    private RunContext? _context;
    private SubscriptionToken _mageDiedToken;
    private ContentId _defeatSourceId = ContentId.From("unknown");

    public void Initialize(RunContext context)
    {
        _context = context;
        _mageDiedToken = context.Events.Subscribe<MageDiedEvent>(OnMageDied);
        context.Events.Publish(new RunStartedEvent(context.State.RunId));
    }

    public void Tick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        var permissions = _context.State.PhasePermissions;
        _context.State.Clock.Tick(delta, permissions.EnemySpawningActive);
        _context.State.TickTemporaryRunEffects(delta);

        if (_context.State.CurrentPhase == RunPhase.DayCombat
            && !_context.State.IsBenchmarkRun
            && _context.TryGetSystem<WaveDirectorSystem>(out var waveDirector)
            && waveDirector?.IsActiveWaveCleared == true)
        {
            if (_context.State.HumanityRemaining <= 0)
            {
                SetPhase(RunPhase.RunVictory);
                return;
            }

            SetPhase(RunPhase.NightMarket);
        }
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_mageDiedToken);
        }

        _context = null;
    }

    public void SetPhase(RunPhase phase, bool saveCheckpoint = true)
    {
        if (_context == null)
        {
            return;
        }

        _context.State.SetPhase(phase);
        if (phase == RunPhase.DayCombat)
        {
            _context.Events.Publish(new DayStartedEvent(_context.State.DayIndex));
        }
        else if (phase == RunPhase.NightMarket || phase == RunPhase.NightDefense)
        {
            _context.Events.Publish(new NightStartedEvent(_context.State.DayIndex));
        }
        else if (phase == RunPhase.RunVictory)
        {
            _context.Events.Publish(new RunVictoryEvent(_context.State.DayIndex, _context.State.HumanityRemaining));
        }
        else if (phase == RunPhase.RunDefeat)
        {
            _context.Events.Publish(new RunDefeatEvent(_context.State.DayIndex, _defeatSourceId));
            _defeatSourceId = ContentId.From("unknown");
        }

        if (saveCheckpoint)
        {
            var label = phase switch
            {
                RunPhase.DayCombat => "day_start",
                RunPhase.NightMarket => "night_market",
                RunPhase.NightDefense => "night_defense",
                _ => string.Empty
            };

            if (!string.IsNullOrEmpty(label))
            {
                RunCheckpointService.SaveStableCheckpoint(_context, label);
            }
        }
    }

    public void CompleteCurrentDayForDebug()
    {
        if (_context == null)
        {
            return;
        }

        _context.State.MarkDebugRun("forced day completion");
        if (_context.State.HumanityRemaining <= 0)
        {
            SetPhase(RunPhase.RunVictory);
            return;
        }

        SetPhase(RunPhase.NightMarket);
    }

    public void ForceVictoryForDebug()
    {
        if (_context == null)
        {
            return;
        }

        _context.State.MarkDebugRun("forced humanity zero");
        _context.State.HumanityRemaining = 0;
        SetPhase(RunPhase.RunVictory);
    }

    private void OnMageDied(MageDiedEvent gameEvent)
    {
        _defeatSourceId = gameEvent.SourceId;
        SetPhase(RunPhase.RunDefeat);
    }
}
