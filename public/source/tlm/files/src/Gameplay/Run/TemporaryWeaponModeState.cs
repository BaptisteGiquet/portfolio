using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Run;

public sealed class TemporaryWeaponModeState
{
    public TemporaryWeaponModeState(ContentId sourceId, float remainingSeconds, float fireRateMultiplier)
    {
        SourceId = sourceId;
        RemainingSeconds = MathF.Max(0f, remainingSeconds);
        FireRateMultiplier = MathF.Max(0.05f, fireRateMultiplier);
    }

    public ContentId SourceId { get; }

    public float RemainingSeconds { get; private set; }

    public float FireRateMultiplier { get; }

    public float CooldownRemainingSeconds { get; private set; }

    public bool IsActive => RemainingSeconds > 0f;

    public bool CanFire => IsActive && CooldownRemainingSeconds <= 0f;

    public void StartCooldown(float cooldownSeconds)
    {
        CooldownRemainingSeconds = MathF.Max(0.01f, cooldownSeconds);
    }

    public void Tick(float delta)
    {
        var step = MathF.Max(0f, delta);
        RemainingSeconds = MathF.Max(0f, RemainingSeconds - step);
        CooldownRemainingSeconds = MathF.Max(0f, CooldownRemainingSeconds - step);
    }
}
