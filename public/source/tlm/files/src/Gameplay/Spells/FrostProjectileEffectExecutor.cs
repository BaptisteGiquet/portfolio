using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Projectiles;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public sealed class FrostProjectileEffectExecutor : IEffectExecutor
{
    public string EffectType => "frost_projectile_damage";

    public void Execute(in SpellExecutionContext context, in EffectRuntimeSpec effect)
    {
        var projectileSystem = context.RunContext.GetSystem<ProjectileSystem>();
        var damage = effect.Value;
        var radius = SpellRadiusDebug.ResolveAreaRadius(context.RunContext, context.Spell, effect, minimumRadius: 0.1f);
        var speed = PlayerAttributeResolver.ResolveProjectileSpeed(context.RunContext, 24f, context.Spell.Tags);
        var travelDistance = PlayerAttributeResolver.ResolveRange(context.RunContext, effect.DurationSeconds * speed, context.Spell.Tags);
        var lifetime = travelDistance / MathF.Max(0.1f, speed);
        var presentationScale = PlayerAttributeResolver.ResolveDamagePresentationScale(context.RunContext, context.Spell.Tags);
        var piercesEnemies = PlayerAttributeResolver.ResolveProjectilePierce(context.RunContext, context.Spell.Tags);
        var splitRemaining = PlayerAttributeResolver.ResolveProjectileSplitDepth(context.RunContext, context.Spell.Tags);
        var homingStrength = PlayerAttributeResolver.ResolveHomingStrength(context.RunContext, context.Spell.Tags);
        var homingConeRadians = PlayerAttributeResolver.ResolveHomingAngleRadians(context.RunContext, context.Spell.Tags, 37f);
        var homingAcquireRadius = PlayerAttributeResolver.ResolveHomingAcquireRadius(context.RunContext, context.Spell.Tags, 4.5f + homingStrength * 2.5f);
        var orbit = SpellOrbit.Create(
            context.RunContext,
            context.Spell.Tags,
            context.Spell.Id,
            context.CasterId,
            context.Origin,
            context.Direction,
            context.SpellChainGeneration,
            speed);
        ProjectileSpawnPattern.SpawnProjectiles(
            projectileSystem,
            context.Spell.Id,
            context.CasterId,
            context.Origin,
            context.Direction,
            1,
            damage,
            DamageType.Frost,
            radius,
            MathF.Max(0.1f, lifetime),
            MathF.Max(0.1f, speed),
            "slow",
            presentationScale,
            piercesEnemies,
            splitRemaining,
            context.DamageMultiplier,
            context.SpellChainGeneration,
            homingStrength,
            homingConeRadians,
            homingAcquireRadius,
            orbit);
    }
}
