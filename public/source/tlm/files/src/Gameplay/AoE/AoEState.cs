using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Gameplay.AoE;

public sealed class AoEState
{
    public required EntityId EntityId { get; init; }

    public required ContentId SpellId { get; init; }

    public required EntityId OwnerId { get; init; }

    public Vector3 Position { get; set; }

    public Vector3 Direction { get; set; }

    public float Radius { get; set; }

    public bool IsExpandingRing { get; init; }

    public float StartRadius { get; init; }

    public float EndRadius { get; init; }

    public float RingThickness { get; init; }

    public float LifetimeSeconds { get; init; }

    public HashSet<EntityId> HitTargets { get; } = new();

    public bool IsWall { get; init; }

    public float WallHalfLength { get; init; }

    public float WallHalfWidth { get; init; }

    public Vector3 WallRight { get; set; }

    public float LifetimeRemaining { get; set; }

    public float TickIntervalSeconds { get; init; }

    public float TickRemainingSeconds { get; set; }

    public float Damage { get; init; }

    public float DamageMultiplier { get; init; } = 1f;

    public int SpellChainGeneration { get; init; }

    public DamageType DamageType { get; init; }

    public string StatusKind { get; init; } = string.Empty;

    public float PullStrength { get; init; }

    public float MoveSpeed { get; init; }

    public float HomingStrength { get; init; }

    public float HomingAcquireRadius { get; init; }

    public float HomingTurnRateRadians { get; init; }

    public SpellOrbitState Orbit { get; set; }
}
