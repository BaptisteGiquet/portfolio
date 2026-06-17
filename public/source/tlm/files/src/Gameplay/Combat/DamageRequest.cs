using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Combat;

public readonly record struct DamageRequest(
    EntityId SourceEntityId,
    EntityId TargetEntityId,
    ContentId SourceContentId,
    float Amount,
    DamageType DamageType,
    HitContext Hit,
    bool ForcePlayerOwned = false,
    float DamageMultiplier = 1f,
    int SpellChainGeneration = 0);
