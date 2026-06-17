using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public sealed class HitscanBeamEffectExecutor : IEffectExecutor
{
    public string EffectType => "hitscan_beam_damage";

    public void Execute(in SpellExecutionContext context, in EffectRuntimeSpec effect)
    {
        var registry = context.RunContext.GetSystem<EnemySystem>().SpatialRegistry;
        var damage = effect.Value;
        var range = MathF.Max(1f, PlayerAttributeResolver.ResolveRange(context.RunContext, effect.DurationSeconds, context.Spell.Tags));
        var radius = SpellRadiusDebug.ResolveAreaRadius(context.RunContext, context.Spell, effect);
        var visualRadius = radius * PlayerAttributeResolver.ResolveDamagePresentationScale(context.RunContext, context.Spell.Tags);
        var homingStrength = PlayerAttributeResolver.ResolveHomingStrength(context.RunContext, context.Spell.Tags);
        var orbit = SpellOrbit.Create(
            context.RunContext,
            context.Spell.Tags,
            context.Spell.Id,
            context.CasterId,
            context.Origin,
            context.Direction,
            context.SpellChainGeneration,
            8f);
        if (orbit.Enabled)
        {
            ExecuteOrbitBeam(context, registry, damage, range, radius, visualRadius, orbit);
            return;
        }

        if (homingStrength > 0f)
        {
            ExecuteBentBeam(context, registry, damage, range, radius, visualRadius, homingStrength);
            return;
        }

        var beamEnd = context.Origin + NormalizeOrForward(context.Direction) * range;
        if (!registry.TryFindRayHit(context.Origin, context.Direction, range, radius, out var enemyId, out var hitPosition))
        {
            context.RunContext.Events.Publish(new BeamFiredEvent(context.Spell.Id, context.CasterId, context.Origin, beamEnd, visualRadius));
            return;
        }

        context.RunContext.Events.Publish(new BeamFiredEvent(context.Spell.Id, context.CasterId, context.Origin, hitPosition, visualRadius));
        var request = new DamageRequest(
            context.CasterId,
            enemyId,
            context.Spell.Id,
            damage,
            DamageType.Arcane,
            new HitContext(hitPosition, Vector3.Up, context.CasterId),
            DamageMultiplier: context.DamageMultiplier,
            SpellChainGeneration: context.SpellChainGeneration);
            context.RunContext.GetSystem<CombatSystem>().ApplyDamage(request);
    }

    private static void ExecuteOrbitBeam(
        in SpellExecutionContext context,
        EnemySpatialRegistry registry,
        float damage,
        float range,
        float radius,
        float visualRadius,
        SpellOrbitState orbit)
    {
        const int SegmentCount = 12;
        var combat = context.RunContext.GetSystem<CombatSystem>();
        var playerPosition = context.RunContext.State.Player.Position;
        var previous = orbit.PositionAt(playerPosition);
        var totalSeconds = Math.Clamp(range / 24f, 0.28f, 0.85f);
        var segmentSeconds = totalSeconds / SegmentCount;

        for (var segmentIndex = 0; segmentIndex < SegmentCount; segmentIndex++)
        {
            var next = orbit.Advance(playerPosition, segmentSeconds, out _);
            var segment = next - previous;
            var segmentLength = segment.Length();
            if (segmentLength <= 0.0001f)
            {
                previous = next;
                continue;
            }

            var direction = segment / segmentLength;
            if (registry.TryFindRayHit(previous, direction, segmentLength, radius, out var enemyId, out var hitPosition))
            {
                context.RunContext.Events.Publish(new BeamFiredEvent(context.Spell.Id, context.CasterId, previous, hitPosition, visualRadius));
                combat.ApplyDamage(new DamageRequest(
                    context.CasterId,
                    enemyId,
                    context.Spell.Id,
                    damage,
                    DamageType.Arcane,
                    new HitContext(hitPosition, Vector3.Up, context.CasterId),
                    DamageMultiplier: context.DamageMultiplier,
                    SpellChainGeneration: context.SpellChainGeneration));
                return;
            }

            context.RunContext.Events.Publish(new BeamFiredEvent(context.Spell.Id, context.CasterId, previous, next, visualRadius));
            previous = next;
        }
    }

    private static void ExecuteBentBeam(
        in SpellExecutionContext context,
        EnemySpatialRegistry registry,
        float damage,
        float range,
        float radius,
        float visualRadius,
        float homingStrength)
    {
        const int SegmentCount = 10;
        var segmentLength = MathF.Max(0.5f, range / SegmentCount);
        var acquireRadius = PlayerAttributeResolver.ResolveHomingAcquireRadius(context.RunContext, context.Spell.Tags, 12f + homingStrength * 8f);
        var homingConeRadians = PlayerAttributeResolver.ResolveHomingAngleRadians(context.RunContext, context.Spell.Tags, 60f);
        var position = context.Origin;
        var direction = NormalizeOrForward(context.Direction);
        var combat = context.RunContext.GetSystem<CombatSystem>();

        for (var segmentIndex = 0; segmentIndex < SegmentCount; segmentIndex++)
        {
            var remainingRange = range - segmentIndex * segmentLength;
            if (remainingRange <= 0f)
            {
                break;
            }

            if (registry.TryFindBestHomingTarget(position, direction, remainingRange, acquireRadius, homingConeRadians, out _, out var targetPosition))
            {
                targetPosition += new Godot.Vector3(0f, 0.7f, 0f);
                var desired = targetPosition - position;
                if (desired.LengthSquared() > 0.0001f)
                {
                    var turn = Math.Clamp(0.38f * homingStrength, 0.08f, 0.88f);
                    var bentDirection = direction.Lerp(desired.Normalized(), turn);
                    if (bentDirection.LengthSquared() > 0.0001f)
                    {
                        direction = bentDirection.Normalized();
                    }
                }
            }

            var currentSegmentLength = MathF.Min(segmentLength, remainingRange);
            var segmentEnd = position + direction * currentSegmentLength;
            if (registry.TryFindRayHit(position, direction, currentSegmentLength, radius, out var enemyId, out var hitPosition))
            {
                context.RunContext.Events.Publish(new BeamFiredEvent(context.Spell.Id, context.CasterId, position, hitPosition, visualRadius));
                combat.ApplyDamage(new DamageRequest(
                    context.CasterId,
                    enemyId,
                    context.Spell.Id,
                    damage,
                    DamageType.Arcane,
                    new HitContext(hitPosition, Godot.Vector3.Up, context.CasterId),
                    DamageMultiplier: context.DamageMultiplier,
                    SpellChainGeneration: context.SpellChainGeneration));
                return;
            }

            context.RunContext.Events.Publish(new BeamFiredEvent(context.Spell.Id, context.CasterId, position, segmentEnd, visualRadius));
            position = segmentEnd;
        }
    }

    private static Godot.Vector3 NormalizeOrForward(Godot.Vector3 direction)
    {
        return direction.LengthSquared() <= 0.0001f ? Godot.Vector3.Forward : direction.Normalized();
    }
}
