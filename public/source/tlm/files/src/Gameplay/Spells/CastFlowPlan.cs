using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Spells;

public readonly record struct CastFlowPlan(
    CastFlowMode Mode,
    ContentId SourceItemId,
    float RequiredChargeSeconds,
    int BonusSpellInstances,
    CastFlowReleaseKind ReleaseKind = CastFlowReleaseKind.ExtraSpellInstances,
    float DamageMultiplier = 0f,
    float BaseRadius = 0f,
    float BaseDurationSeconds = 0f,
    float StartRadius = 0f,
    float RingThickness = 0f,
    float TickIntervalSeconds = 0f)
{
    public bool RequiresCharge => Mode is CastFlowMode.ChargeThenRelease or CastFlowMode.SustainedCastThenRelease;

    public bool AllowsConcurrentCasting => Mode == CastFlowMode.SustainedCastThenRelease;

    public bool SuppressesNormalCasting => Mode == CastFlowMode.ChargeThenRelease;
}
