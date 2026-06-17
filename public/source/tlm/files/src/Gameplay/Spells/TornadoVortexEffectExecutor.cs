using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.AoE;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public sealed class TornadoVortexEffectExecutor : IEffectExecutor
{
    public string EffectType => "tornado_vortex";

    public void Execute(in SpellExecutionContext context, in EffectRuntimeSpec effect)
    {
        var damage = effect.Value;
        var radius = SpellRadiusDebug.ResolveAreaRadius(context.RunContext, context.Spell, effect);
        var presentationScale = PlayerAttributeResolver.ResolveDamagePresentationScale(context.RunContext, context.Spell.Tags);
        var duration = SpellStatResolver.ResolveEffectDuration(context.RunContext, context.Spell, effect);
        var placementRange = PlayerAttributeResolver.ResolveRange(context.RunContext, 3f, context.Spell.Tags);
        var homingStrength = PlayerAttributeResolver.ResolveHomingStrength(context.RunContext, context.Spell.Tags);
        var homingAcquireRadius = PlayerAttributeResolver.ResolveHomingAcquireRadius(context.RunContext, context.Spell.Tags, 14f + homingStrength * 6f);
        var preview = SpellPlacement.BuildDisk(
            context.Origin,
            context.Direction,
            radius,
            placementRange,
            new Godot.Color(0.68f, 0.9f, 1f, 0.28f));
        var orbit = SpellOrbit.Create(
            context.RunContext,
            context.Spell.Tags,
            context.Spell.Id,
            context.CasterId,
            context.Origin,
            context.Direction,
            context.SpellChainGeneration,
            4f);
        var position = orbit.Enabled ? orbit.PositionAt(context.RunContext.State.Player.Position) : context.PlacementOverride ?? preview.Position;
        context.RunContext.GetSystem<AoESystem>().Spawn(
            context.Spell.Id,
            context.CasterId,
            position,
            context.Direction,
            preview.Radius,
            duration,
            0.5f,
            damage,
            DamageType.Physical,
            "slow",
            pullStrength: 5.5f,
            moveSpeed: 3.8f,
            presentationScale: presentationScale,
            damageMultiplier: context.DamageMultiplier,
            spellChainGeneration: context.SpellChainGeneration,
            homingStrength: homingStrength,
            homingAcquireRadius: homingAcquireRadius,
            orbit: orbit);
    }
}
