using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;
using TheLastMage.Gameplay.Summons;

namespace TheLastMage.Gameplay.Spells;

public sealed class SummonEffectExecutor : IEffectExecutor
{
    public string EffectType => "raise_dead_summon";

    public void Execute(in SpellExecutionContext context, in EffectRuntimeSpec effect)
    {
        var damage = effect.Value;
        var baseSearchRadius = SpellRadiusDebug.ResolveAreaRadius(context.RunContext, context.Spell, effect);
        var searchRadius = PlayerAttributeResolver.ResolveRange(context.RunContext, baseSearchRadius, context.Spell.Tags);
        var lifetime = SpellStatResolver.ResolveEffectDuration(context.RunContext, context.Spell, effect);
        if (HasPersistentSummons(context.RunContext))
        {
            lifetime = float.PositiveInfinity;
        }

        var aggroRange = PlayerAttributeResolver.ResolveRange(context.RunContext, 26f, context.Spell.Tags);
        var presentationScale = PlayerAttributeResolver.ResolveDamagePresentationScale(context.RunContext, context.Spell.Tags);
        var preview = SpellPlacement.BuildDisk(
            context.Origin,
            context.Direction,
            searchRadius,
            MathF.Max(2f, searchRadius * 0.5f),
            new Godot.Color(0.25f, 1f, 0.55f, 0.28f));
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
        var summonId = context.RunContext.GetSystem<SummonSystem>().TryRaiseFromCorpse(
            context.Spell.Id,
            context.CasterId,
            position,
            preview.Radius,
            lifetime,
            damage,
            aggroRange,
            presentationScale,
            context.DamageMultiplier,
            context.SpellChainGeneration);

        if (!summonId.IsValid)
        {
            GD.Print($"RaiseDeadNoCorpse spell={context.Spell.Id} searchRadius={searchRadius:0.##}");
        }
    }

    private static bool HasPersistentSummons(RunContext context)
    {
        foreach (var stack in context.State.Build.Items)
        {
            var item = context.Content.GetItem(stack.ItemId);
            if (item.Effects.Any(effect => string.Equals(effect.Kind, "PersistentSummons", StringComparison.Ordinal)))
            {
                return true;
            }
        }

        return false;
    }
}
