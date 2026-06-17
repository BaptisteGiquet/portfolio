using TheLastMage.Gameplay.Run;

namespace TheLastMage.Debugging.Reports;

public sealed class GameplayProfilingSystem : IGameSystem
{
    private RunContext? _context;
    private float _elapsedSeconds;
    private float _summaryTimer;
    private int _samples;
    private float _totalFrameMilliseconds;
    private float _peakFrameMilliseconds;
    private float _peakEnemyAiMilliseconds;
    private long _peakAllocatedBytes;
    private int _peakEnemies;
    private int _peakProjectiles;
    private int _totalFeedbackDrops;
    private int _totalDamageEvents;

    public void Initialize(RunContext context)
    {
        _context = context;
        context.State.DebugMetrics.ProfilingSummary = "collecting";
    }

    public void Tick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        var metrics = _context.State.DebugMetrics;
        _elapsedSeconds += delta;
        _summaryTimer -= delta;
        _samples++;
        _totalFrameMilliseconds += metrics.FrameMilliseconds;
        _peakFrameMilliseconds = MathF.Max(_peakFrameMilliseconds, metrics.FrameMilliseconds);
        _peakEnemyAiMilliseconds = MathF.Max(_peakEnemyAiMilliseconds, metrics.EnemyAiMilliseconds);
        _peakAllocatedBytes = Math.Max(_peakAllocatedBytes, metrics.AllocatedBytesThisFrame);
        _peakEnemies = Math.Max(_peakEnemies, metrics.ActiveEnemies);
        _peakProjectiles = Math.Max(_peakProjectiles, metrics.ActiveProjectiles);
        _totalFeedbackDrops += metrics.FeedbackBudgetDropsThisFrame;
        _totalDamageEvents += metrics.DamageEventsThisFrame;

        if (_summaryTimer > 0f)
        {
            return;
        }

        _summaryTimer = 1f;
        var averageFrame = _samples <= 0 ? 0f : _totalFrameMilliseconds / _samples;
        metrics.ProfilingSummary =
            $"avg={averageFrame:0.00}ms peak={_peakFrameMilliseconds:0.00}ms ai_peak={_peakEnemyAiMilliseconds:0.00}ms " +
            $"enemies_peak={_peakEnemies} projectiles_peak={_peakProjectiles} damage_total={_totalDamageEvents} " +
            $"feedback_drops={_totalFeedbackDrops} alloc_peak={_peakAllocatedBytes} elapsed={_elapsedSeconds:0.0}s";
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        _context = null;
    }
}
