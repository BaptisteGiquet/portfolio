using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.AoE;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public sealed class FirewallHazardEffectExecutor : IEffectExecutor
{
    public string EffectType => "firewall_hazard";

    public void Execute(in SpellExecutionContext context, in EffectRuntimeSpec effect)
    {
        var damage = effect.Value;
        var halfLength = SpellRadiusDebug.ResolveAreaRadius(context.RunContext, context.Spell, effect);
        var presentationScale = PlayerAttributeResolver.ResolveDamagePresentationScale(context.RunContext, context.Spell.Tags);
        var duration = SpellStatResolver.ResolveEffectDuration(context.RunContext, context.Spell, effect);
        var placementRange = PlayerAttributeResolver.ResolveRange(context.RunContext, 7f, context.Spell.Tags);
        var preview = SpellPlacement.BuildWall(
            context.Origin,
            context.Direction,
            halfLength,
            0.55f,
            placementRange,
            new Godot.Color(1f, 0.18f, 0.04f, 0.34f));
        if (preview.Shape != SpellPlacementShape.Wall)
        {
            return;
        }

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

        context.RunContext.GetSystem<AoESystem>().SpawnWall(
            context.Spell.Id,
            context.CasterId,
            position,
            preview.WallRight,
            preview.WallHalfLength,
            preview.WallHalfWidth,
            duration,
            0.45f,
            damage,
            DamageType.Fire,
            string.Empty,
            presentationScale,
            context.DamageMultiplier,
            context.SpellChainGeneration,
            orbit);
    }
}
