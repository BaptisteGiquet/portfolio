using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Navigation;

namespace TheLastMage.Gameplay.Enemies;

public sealed class EnemyState
{
    public required EntityId EntityId { get; init; }

    public required ContentId ArchetypeId { get; init; }

    public required string Role { get; init; }

    public IReadOnlyList<TagId> Tags { get; init; } = GameplayTagSet.Empty;

    public Vector3 Position { get; set; }

    public float MoveSpeed { get; init; }

    public float AttackDamage { get; init; }

    public float AttackRange { get; init; }

    public float AttackCooldownRemaining { get; set; }

    public float PreferredRange { get; init; }

    public float AbilityRange { get; init; }

    public float AbilityCooldownSeconds { get; init; }

    public float AbilityCooldownRemaining { get; set; }

    public float AbilityTelegraphSeconds { get; init; }

    public float AbilityTelegraphRemaining { get; set; }

    public float AbilityProjectileSpeed { get; init; }

    public IReadOnlyList<EnemyAbilityRuntimeSpec> Abilities { get; init; } = Array.Empty<EnemyAbilityRuntimeSpec>();

    public int PendingAbilityIndex { get; set; } = -1;

    public EntityId PendingAbilityTargetId { get; set; } = EntityId.None;

    public Vector3 PendingAbilityTargetPosition { get; set; }

    public Vector3 CurrentGoal { get; set; }

    public Vector3 CurrentSlotGoal { get; set; }

    public Vector3 SteeringVelocity { get; set; }

    public TowerRouteStep TowerRouteStep { get; set; } = TowerRouteStep.Entrance;

    public string RouteDecision { get; set; } = string.Empty;

    public string TemporaryFormId { get; set; } = string.Empty;

    public float TemporaryFormRemainingSeconds { get; set; }

    public bool TemporaryFormDisablesAttacks { get; set; }

    public bool HasTemporaryFormHealthSnapshot { get; set; }

    public float TemporaryFormPreviousMaxHealth { get; set; }

    public float TemporaryFormPreviousCurrentHealth { get; set; }
}
