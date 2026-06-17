using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Defenses;

public sealed class DefenseState
{
    public required EntityId EntityId { get; init; }

    public required ContentId DefenseId { get; init; }

    public required Vector3 Position { get; init; }

    public required string PlacementRule { get; init; }

    public required float PlacementRadius { get; init; }

    public required float BlockRadius { get; init; }

    public required float TriggerRadius { get; init; }

    public required float ExplosionRadius { get; init; }

    public required float FuseSeconds { get; init; }

    public required float RechargeSeconds { get; init; }

    public required int MaxCharges { get; init; }

    public required float Damage { get; init; }

    public bool IsArmed { get; set; }

    public float FuseRemaining { get; set; }

    public float RechargeRemaining { get; set; }

    public int ChargesRemaining { get; set; }

    public EntityId ArmedByEnemyId { get; set; } = EntityId.None;

    public Vector3 ArmedByEnemyPosition { get; set; }

    public bool HasTriggered { get; set; }

    public bool IsBarricade => DefenseId.Value.Contains("barricade", StringComparison.OrdinalIgnoreCase);

    public bool IsExplosiveSeal => DefenseId.Value.Contains("explosive_seal", StringComparison.OrdinalIgnoreCase)
        || DefenseId.Value.Contains("seal", StringComparison.OrdinalIgnoreCase);
}
