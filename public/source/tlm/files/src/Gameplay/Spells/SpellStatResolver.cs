using TheLastMage.Foundation;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public static class SpellStatResolver
{
    public static float Resolve(RunContext context, SpellExecutionContext spellContext, string statId, float baseValue)
    {
        return PlayerAttributeResolver.ResolveValue(
            context,
            StatId.From(statId),
            baseValue,
            spellContext.Spell.Tags);
    }

    public static float Resolve(RunContext context, TheLastMage.Data.Runtime.SpellRuntimeDefinition spell, string statId, float baseValue)
    {
        return PlayerAttributeResolver.ResolveValue(
            context,
            StatId.From(statId),
            baseValue,
            spell.Tags);
    }

    public static float ResolveEffectDuration(
        RunContext context,
        TheLastMage.Data.Runtime.SpellRuntimeDefinition spell,
        TheLastMage.Data.Runtime.EffectRuntimeSpec effect)
    {
        if (!string.Equals(effect.DurationScaling, "PlayerDuration", StringComparison.OrdinalIgnoreCase))
        {
            return MathF.Max(0f, effect.DurationSeconds);
        }

        return PlayerAttributeResolver.ResolveDuration(context, effect.DurationSeconds, spell.Tags);
    }
}
