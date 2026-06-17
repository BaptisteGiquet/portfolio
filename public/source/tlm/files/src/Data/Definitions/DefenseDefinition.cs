using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class DefenseDefinition : ContentDefinition
{
    [Export] public DefensePlacementRule PlacementRule { get; set; }

    [Export] public float MaxHealth { get; set; } = 50f;

    [Export] public float Cost { get; set; } = 10f;

    [Export] public float RepairCost { get; set; } = 4f;

    [Export] public float PlacementRadius { get; set; } = 1.4f;

    [Export] public float BlockRadius { get; set; } = 4f;

    [Export] public float TriggerRadius { get; set; } = 2.5f;

    [Export] public float ExplosionRadius { get; set; } = 4.5f;

    [Export] public float FuseSeconds { get; set; } = 0.45f;

    [Export] public float RechargeSeconds { get; set; }

    [Export] public int MaxCharges { get; set; } = 1;

    [Export] public float Damage { get; set; } = 35f;
}
