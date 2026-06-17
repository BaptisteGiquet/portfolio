using Godot;
using System.Globalization;
using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public static class SpellRadiusDebug
{
    public static float ResolveAreaRadius(
        RunContext context,
        SpellRuntimeDefinition spell,
        EffectRuntimeSpec effect,
        float minimumRadius = 0.05f)
    {
        var breakdown = PlayerAttributeResolver.ResolveBreakdown(
            context,
            PlayerAttributeResolver.AreaRadius,
            effect.Radius,
            spell.Tags,
            false);
        var resolved = MathF.Max(minimumRadius, breakdown.FinalValue);
        var damagePresentationScale = PlayerAttributeResolver.ResolveDamagePresentationScale(context, spell.Tags, false);
        var summary =
            $"{spell.Id}/{effect.EffectType} base={Format(effect.Radius)} raw={Format(breakdown.FinalValue)} final={Format(resolved)} min={Format(minimumRadius)} " +
            $"damageVisual={Format(damagePresentationScale)} visualSize={Format(resolved * damagePresentationScale)} " +
            $"mods={FormatModifiers(breakdown)} tags={FormatTags(spell.Tags)}";
        context.State.DebugMetrics.LastSpellRadiusSummary = summary;
        GD.Print($"TLM Radius Debug: {summary}");
        return resolved;
    }

    private static string FormatModifiers(StatBreakdown breakdown)
    {
        if (breakdown.AppliedModifiers.Count == 0)
        {
            return "none";
        }

        return string.Join(", ", breakdown.AppliedModifiers.Select(modifier =>
            $"{modifier.SourceId}:{modifier.Operation}:{Format(modifier.Value)}@{modifier.TargetFilter.RequiredTag}"));
    }

    private static string FormatTags(IReadOnlyList<Foundation.TagId> tags)
    {
        return tags.Count == 0 ? "none" : string.Join(",", tags.Select(tag => tag.Value));
    }

    private static string Format(float value)
    {
        return value.ToString("0.###", CultureInfo.InvariantCulture);
    }
}
