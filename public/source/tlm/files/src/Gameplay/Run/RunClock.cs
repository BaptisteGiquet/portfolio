namespace TheLastMage.Gameplay.Run;

public sealed class RunClock
{
    public float TotalElapsedSeconds { get; private set; }

    public float PhaseElapsedSeconds { get; private set; }

    public float IntensitySeconds { get; private set; }

    public void Tick(float delta, bool intensityActive)
    {
        TotalElapsedSeconds += delta;
        PhaseElapsedSeconds += delta;
        if (intensityActive)
        {
            IntensitySeconds += delta;
        }
    }

    public void ResetPhase()
    {
        PhaseElapsedSeconds = 0f;
    }

    public void Restore(float totalElapsedSeconds, float phaseElapsedSeconds, float intensitySeconds)
    {
        TotalElapsedSeconds = MathF.Max(0f, totalElapsedSeconds);
        PhaseElapsedSeconds = MathF.Max(0f, phaseElapsedSeconds);
        IntensitySeconds = MathF.Max(0f, intensitySeconds);
    }
}
