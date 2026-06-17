using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.AoE;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public sealed class BlizzardAoEEffectExecutor : IEffectExecutor
{
    public string EffectType => "blizzard_aoe";

    public void Execute(in SpellExecutionContext context, in EffectRuntimeSpec effect)
    {
        var damage = effect.Value;
        var radius = SpellRadiusDebug.ResolveAreaRadius(context.RunContext, context.Spell, effect);
        var presentationScale = PlayerAttributeResolver.ResolveDamagePresentationScale(context.RunContext, context.Spell.Tags);
        var duration = SpellStatResolver.ResolveEffectDuration(context.RunContext, context.Spell, effect);
        var placementRange = PlayerAttributeResolver.ResolveRange(context.RunContext, 10f, context.Spell.Tags);
        var preview = SpellPlacement.BuildDisk(
            context.Origin,
            context.Direction,
            radius,
            placementRange,
            new Godot.Color(0.35f, 0.75f, 1f, 0.28f));
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
            0.65f,
            damage,
            DamageType.Frost,
            "slow",
            presentationScale: presentationScale,
            damageMultiplier: context.DamageMultiplier,
            spellChainGeneration: context.SpellChainGeneration,
            orbit: orbit);
    }
}
