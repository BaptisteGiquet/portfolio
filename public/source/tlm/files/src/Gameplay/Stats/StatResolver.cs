using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Stats;

public sealed class StatResolver
{
    public StatBreakdown Resolve(
        StatId statId,
        float baseValue,
        IEnumerable<Modifier> modifiers,
        IReadOnlyCollection<TagId>? targetTags = null)
    {
        targetTags ??= Array.Empty<TagId>();
        var applicable = modifiers
            .Where(modifier => modifier.StatId.Equals(statId) && modifier.TargetFilter.Matches(targetTags))
            .OrderBy(modifier => modifier.Priority)
            .ThenBy(modifier => modifier.Operation)
            .ToArray();

        var value = baseValue;
        foreach (var modifier in applicable)
        {
            value = modifier.Operation switch
            {
                ModifierOp.FlatAdd => value + modifier.Value,
                ModifierOp.AdditivePercent => value + (baseValue * modifier.Value),
                ModifierOp.Multiplicative => value * modifier.Value,
                ModifierOp.MinClamp => MathF.Max(value, modifier.Value),
                ModifierOp.MaxClamp => MathF.Min(value, modifier.Value),
                ModifierOp.Override => modifier.Value,
                _ => value
            };
        }

        return new StatBreakdown(statId, baseValue, value, applicable);
    }
}
