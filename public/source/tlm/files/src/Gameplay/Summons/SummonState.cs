using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Summons;

public sealed class SummonState
{
    public required EntityId EntityId { get; init; }

    public required ContentId SpellId { get; init; }

    public required EntityId OwnerId { get; init; }

    public Vector3 Position { get; set; }

    public float LifetimeRemaining { get; set; }

    public float AttackCooldownRemaining { get; set; }

    public float Damage { get; init; }

    public float DamageMultiplier { get; init; } = 1f;

    public int SpellChainGeneration { get; init; }

    public float AttackRange { get; init; }

    public float MoveSpeed { get; init; }

    public float AggroRange { get; init; }

    public Vector3 CurrentTargetPosition { get; set; }
}
