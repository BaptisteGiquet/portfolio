using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Gameplay.Projectiles;

public struct ProjectileState
{
    public EntityId EntityId;
    public ContentId SpellId;
    public EntityId OwnerId;
    public Vector3 Position;
    public Vector3 PreviousPosition;
    public Vector3 Velocity;
    public float Radius;
    public float Damage;
    public DamageType DamageType;
    public float LifetimeRemaining;
    public bool PiercesEnemies;
    public int SplitRemaining;
    public float DamageMultiplier;
    public int SpellChainGeneration;
    public List<EntityId>? HitTargets;
    public EntityId DirectTargetId;
    public Vector3 DirectTargetPosition;
    public float DirectTargetRadius;
    public bool IsDirectTargetProjectile;
    public float HomingStrength;
    public EntityId HomingTargetId;
    public float HomingAcquireRange;
    public float HomingAcquireRadius;
    public float HomingConeRadians;
    public float HomingTurnRateRadians;
    public float HomingRetargetRemaining;
    public SpellOrbitState Orbit;
    public string StatusKind;
    public bool Active;
}
