using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;

namespace TheLastMage.Debugging.Balance;

public static class BalanceStaticAnalyzer
{
    public static BalanceStaticAnalysisResult Analyze(ContentRegistry content)
    {
        var result = new BalanceStaticAnalysisResult
        {
            SpellCount = content.Spells.Count,
            MageCount = content.Mages.Count,
            EnemyCount = content.Enemies.Count,
            WaveCount = content.Waves.Count
        };

        var productionItems = content.Items.Values
            .Where(item => !IsRegressionItem(item.Id))
            .ToArray();
        result.ProductionItemCount = productionItems.Length;

        foreach (var item in productionItems)
        {
            Increment(result.ItemKinds, item.Kind);
            foreach (var tag in item.Tags)
            {
                Increment(result.ItemTags, tag.ToString());
            }

            foreach (var pool in item.PoolWeights)
            {
                Increment(result.ItemPools, pool.PoolId.ToString());
            }

            foreach (var effect in item.Effects)
            {
                Increment(result.DesignerEffectKinds, effect.Kind);
                RecordTarget(result, content, item.Id, effect.TargetTag);
                if (effect.StatId.IsValid)
                {
                    Increment(result.ModifierStats, effect.StatId.ToString());
                }
            }

            foreach (var modifier in item.Modifiers)
            {
                Increment(result.ModifierStats, modifier.StatId.ToString());
                RecordTarget(result, content, item.Id, modifier.TargetTag);
            }

            foreach (var proc in item.Procs)
            {
                Increment(result.ProcKinds, $"{proc.Trigger}:{proc.EffectType}");
                RecordTarget(result, content, item.Id, proc.TargetTag);
            }

            foreach (var activeEffect in item.ActiveEffects)
            {
                Increment(result.ActiveEffectKinds, activeEffect.EffectType);
            }

            if (item.Effects.Count == 0 && item.Modifiers.Count == 0 && item.Procs.Count == 0 && item.ActiveEffects.Count == 0 && !item.IsFlavorOnly)
            {
                result.Warnings.Add($"Item `{item.Id}` has no gameplay effects and is not marked flavor-only.");
            }
        }

        foreach (var enemy in content.Enemies.Values)
        {
            Increment(result.EnemyFamilies, string.IsNullOrWhiteSpace(enemy.Family) ? "unknown" : enemy.Family);
        }

        if (result.ProductionItemCount < 50)
        {
            result.Warnings.Add($"Production item count is {result.ProductionItemCount}; Sprint 13C expects at least 50.");
        }

        if (content.Spells.Count == 0)
        {
            result.Warnings.Add("No spells are registered for balance analysis.");
        }

        return result;
    }

    private static void RecordTarget(
        BalanceStaticAnalysisResult result,
        ContentRegistry content,
        ContentId itemId,
        TagId targetTag)
    {
        if (!targetTag.IsValid)
        {
            Increment(result.TargetTags, "<all>");
            result.Warnings.Add($"Item `{itemId}` has a broad all-content target.");
            return;
        }

        var target = targetTag.ToString();
        Increment(result.TargetTags, target);
        var matchesSpell = content.Spells.Values.Any(spell => GameplayTagPath.MatchesAny(targetTag, spell.Tags));
        if (!matchesSpell && target.StartsWith("spell.", StringComparison.OrdinalIgnoreCase))
        {
            result.Warnings.Add($"Item `{itemId}` targets `{target}`, which matches no current spell.");
        }
    }

    private static bool IsRegressionItem(ContentId itemId)
    {
        return itemId.ToString().StartsWith("regression_item_", StringComparison.Ordinal);
    }

    private static void Increment(Dictionary<string, int> counts, string key)
    {
        if (string.IsNullOrWhiteSpace(key))
        {
            key = "<none>";
        }

        counts[key] = counts.GetValueOrDefault(key) + 1;
    }
}

