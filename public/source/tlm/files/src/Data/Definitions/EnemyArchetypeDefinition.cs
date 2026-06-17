using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class EnemyArchetypeDefinition : ContentDefinition
{
    [Export] public EnemyFamily Family { get; set; }

    [Export] public EnemyRole Role { get; set; }

    [Export] public float MaxHealth { get; set; } = 10f;

    [Export] public float MoveSpeed { get; set; } = 3f;

    [Export] public float AttackDamage { get; set; } = 1f;

    [Export] public float AttackRange { get; set; } = 1.5f;

    [Export] public float PreferredRange { get; set; }

    [Export] public float AbilityRange { get; set; }

    [Export] public float AbilityCooldownSeconds { get; set; }

    [Export] public float AbilityTelegraphSeconds { get; set; }

    [Export] public float AbilityProjectileSpeed { get; set; } = 14f;

    [Export] public int MaxActive { get; set; }

    [Export] public PackedScene? PresentationScene { get; set; }

    [Export] public Godot.Collections.Array<EffectSpec> Abilities { get; set; } = new();
}
