using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Combat;

public readonly record struct DamagePreventionResult(
    ContentId SourceId,
    EntityId ProtectedEntityId,
    float PreventedAmount,
    int RemainingHits);
