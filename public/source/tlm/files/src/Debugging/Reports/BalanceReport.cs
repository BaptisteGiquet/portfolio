using Godot;
using System.IO;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Debugging.Reports;

public static class BalanceReport
{
    public static string Write(RunContext context)
    {
        var directory = RuntimePathResolver.GlobalizeUserPath("user://balance_reports");
        Directory.CreateDirectory(directory);
        var path = Path.Combine(directory, $"tlm_balance_{DateTime.Now:yyyyMMdd_HHmmss}.md");
        File.WriteAllText(path, Build(context));
        context.State.DebugMetrics.BalanceReportPath = path;
        return path;
    }

    public static string Build(RunContext context)
    {
        var content = context.Content;
        var productionItems = content.Items.Values
            .Where(item => !item.Id.ToString().StartsWith("regression_item_", StringComparison.Ordinal))
            .ToArray();
        var passiveItems = productionItems
            .Where(item => string.Equals(item.Kind, nameof(ItemKind.Passive), StringComparison.OrdinalIgnoreCase))
            .ToArray();
        var activatableItems = productionItems
            .Where(item => string.Equals(item.Kind, nameof(ItemKind.Activatable), StringComparison.OrdinalIgnoreCase))
            .ToArray();
        var enemyFamilies = content.Enemies.Values
            .Select(enemy => enemy.Family)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .OrderBy(value => value)
            .ToArray();
        var bossLikeEnemies = content.Enemies.Values
            .Where(enemy => GameplayTagPath.MatchesAny(TagId.From("enemy.rank.boss"), enemy.Tags)
                || GameplayTagPath.MatchesAny(TagId.From("enemy.rank.champion"), enemy.Tags))
            .ToArray();

        var lines = new List<string>
        {
            "# The Last Mage Balance Report",
            string.Empty,
            $"Run ID: `{context.State.RunId}`",
            $"Seed: `{context.Random.Seed}`",
            $"Mage: `{context.State.SelectedMageId}`",
            string.Empty,
            "## Content Counts",
            string.Empty,
            $"Spells: {content.Spells.Count}",
            $"Mages: {content.Mages.Count}",
            $"Production items: {productionItems.Length}",
            $"Passive production items: {passiveItems.Length}",
            $"Activatable production items: {activatableItems.Length}",
            $"Enemy archetypes: {content.Enemies.Count}",
            $"Enemy families: {enemyFamilies.Length} ({Format(enemyFamilies)})",
            $"Boss/champion entries: {bossLikeEnemies.Length} ({Format(bossLikeEnemies.Select(enemy => enemy.Id.ToString()))})",
            $"Waves: {content.Waves.Count}",
            $"Defenses: {content.Defenses.Count}",
            string.Empty,
            "## Sprint 13 Target Readiness",
            string.Empty,
            FormatTarget("7 scoped vertical-slice spells", content.Spells.Count, 7, 7),
            FormatTarget("20-30 production items", productionItems.Length, 20, 30),
            FormatTarget("5-6 enemy families/archetype groups", enemyFamilies.Length, 5, 6),
            FormatTarget("1-2 bosses/champions", bossLikeEnemies.Length, 1, 2),
            string.Empty,
            "## Item Modifier Coverage",
            string.Empty
        };

        AppendItemCoverage(lines, productionItems);
        lines.Add(string.Empty);
        lines.Add("## Activatable Item Coverage");
        lines.Add(string.Empty);
        AppendActivatableCoverage(lines, activatableItems);
        lines.Add(string.Empty);
        lines.Add("## Wave Schedule");
        lines.Add(string.Empty);
        foreach (var wave in content.Waves.Values.OrderBy(wave => wave.DayIndex).ThenBy(wave => wave.Id.ToString()))
        {
            lines.Add(
                $"- `{wave.Id}` day={wave.DayIndex} duration={wave.DurationSeconds:0.#}s " +
                $"target={wave.TargetActiveEnemies} intensity={wave.Intensity:0.##} enemies={Format(wave.EnemyIds.Select(id => id.ToString()))}");
        }

        lines.Add(string.Empty);
        lines.Add("## Runtime Snapshot");
        lines.Add(string.Empty);
        lines.Add($"Profiling: {context.State.DebugMetrics.ProfilingSummary}");
        lines.Add($"Death cause: {context.State.DebugMetrics.DeathCauseSummary}");
        lines.Add($"Reproduction: {context.State.DebugMetrics.ReproductionSummary}");

        return string.Join(System.Environment.NewLine, lines);
    }

    private static void AppendItemCoverage(List<string> lines, IReadOnlyList<ItemRuntimeDefinition> items)
    {
        var numeric = 0;
        var behavior = 0;
        var procCount = 0;
        var targetedStats = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        foreach (var item in items)
        {
            if (item.Modifiers.Count > 0)
            {
                numeric++;
            }

            if (item.Procs.Count > 0)
            {
                behavior++;
                procCount += item.Procs.Count;
            }

            foreach (var modifier in item.Modifiers)
            {
                var key = modifier.StatId.ToString();
                targetedStats[key] = targetedStats.GetValueOrDefault(key) + 1;
            }
        }

        lines.Add($"Numeric modifier items: {numeric}");
        lines.Add($"Behavior/proc items: {behavior}");
        lines.Add($"Proc rules: {procCount}");
        lines.Add($"Targeted stats: {Format(targetedStats.OrderBy(pair => pair.Key).Select(pair => $"{pair.Key}:{pair.Value}"))}");
        lines.Add($"Tag branches: {Format(BuildTagBranches(items).OrderBy(pair => pair.Key).Select(pair => $"{pair.Key}:{pair.Value}"))}");
    }

    private static Dictionary<string, int> BuildTagBranches(IReadOnlyList<ItemRuntimeDefinition> items)
    {
        var branches = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        foreach (var item in items)
        {
            foreach (var tag in item.Tags)
            {
                var branch = GameplayTagPath.GetParent(tag.ToString());
                if (string.IsNullOrWhiteSpace(branch))
                {
                    branch = tag.ToString();
                }

                branches[branch] = branches.GetValueOrDefault(branch) + 1;
            }
        }

        return branches;
    }

    private static void AppendActivatableCoverage(List<string> lines, IReadOnlyList<ItemRuntimeDefinition> items)
    {
        var unlimited = items.Count(item => item.HasUnlimitedActivations);
        var limited = items.Count - unlimited;
        var activeEffects = items.Sum(item => item.ActiveEffects.Count);
        var statGranting = items.Count(item => item.Modifiers.Count > 0 || item.Procs.Count > 0);

        lines.Add($"Activatable items: {items.Count}");
        lines.Add($"Limited: {limited}");
        lines.Add($"Unlimited: {unlimited}");
        lines.Add($"Active effects: {activeEffects}");
        lines.Add($"Equipped stat/proc items: {statGranting}");
    }

    private static string FormatTarget(string label, int count, int minimum, int maximum)
    {
        var status = count >= minimum && count <= maximum ? "ready" : "needs work";
        return $"- {label}: {count} ({status}; target {minimum}-{maximum})";
    }

    private static string Format(IEnumerable<string> values)
    {
        var array = values.Where(value => !string.IsNullOrWhiteSpace(value)).ToArray();
        return array.Length == 0 ? "-" : string.Join(", ", array);
    }
}
