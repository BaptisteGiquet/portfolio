using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Combat;

public readonly record struct DamageResult(
    EntityId TargetEntityId,
    float AppliedAmount,
    float RemainingHealth,
    bool TargetDied);
