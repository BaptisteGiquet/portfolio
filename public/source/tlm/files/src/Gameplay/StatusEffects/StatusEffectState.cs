using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.StatusEffects;

public sealed class StatusEffectState
{
    public required EntityId TargetId { get; init; }

    public required string Kind { get; init; }

    public required EntityId SourceEntityId { get; init; }

    public required ContentId SourceId { get; init; }

    public float DamageMultiplier { get; init; } = 1f;

    public int SpellChainGeneration { get; init; }

    public float Magnitude { get; set; }

    public float RemainingSeconds { get; set; }

    public float PeriodicTickRemainingSeconds { get; set; }
}
