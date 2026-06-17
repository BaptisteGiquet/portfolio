using System.Text.Json;
using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Debugging.Balance;

public static class BalanceReportWriter
{
    public static string Write(BalanceExperimentDefinition experiment, BalanceExperimentResult result)
    {
        var directory = Path.Combine(GetReportRoot(), $"{Sanitize(experiment.ExperimentId)}_{DateTime.Now:yyyyMMdd_HHmmss}");
        Directory.CreateDirectory(directory);
        result.ReportDirectory = directory;

        if (experiment.WriteMarkdown)
        {
            File.WriteAllText(Path.Combine(directory, "balance_summary.md"), BuildSummary(result));
        }

        if (experiment.WriteJsonl)
        {
            WriteJsonl(Path.Combine(directory, "balance_runs.jsonl"), result.Runs);
        }

        if (experiment.WriteCsv)
        {
            WriteCsv(Path.Combine(directory, "balance_item_correlations.csv"), BuildItemCorrelationRows(result));
            WriteCsv(Path.Combine(directory, "balance_spell_breakdown.csv"), BuildSpellRows(result));
            WriteCsv(Path.Combine(directory, "balance_mage_breakdown.csv"), BuildMageRows(result));
            WriteCsv(Path.Combine(directory, "balance_death_causes.csv"), BuildDeathCauseRows(result));
            WriteCsv(Path.Combine(directory, "balance_performance_peaks.csv"), BuildPerformanceRows(result));
            WriteCsv(Path.Combine(directory, "balance_outliers.csv"), BuildOutlierRows(result));
        }

        return directory;
    }

    public static string WriteStartMarker(BalanceExperimentDefinition experiment)
    {
        var root = GetReportRoot();
        var path = Path.Combine(root, "balance_lab_last_start.md");
        var lines = new[]
        {
            "# Balance Lab Last Start",
            string.Empty,
            $"Started local: `{DateTime.Now:yyyy-MM-dd HH:mm:ss}`",
            $"Experiment: `{experiment.ExperimentId}`",
            $"Run mode: `{experiment.RunMode}`",
            $"Total simulation runs: `{experiment.TotalSimulationRuns}`",
            $"Seed start: `{experiment.SeedStart}`",
            $"Seed count: `{experiment.SeedCount}`",
            $"Starting humanity: `{experiment.StartingHumanity}`",
            $"Max days: `{experiment.MaxSimulatedDays}`",
            $"Max seconds per day: `{experiment.MaxSimulatedSecondsPerDay:0.###}`",
            $"Monte Carlo trials: `{experiment.MonteCarloTrials}`",
            $"Monte Carlo nights: `{experiment.MonteCarloNightsPerTrial}`",
            string.Empty,
            "If this file is newer than the latest completed report folder, the lab stopped before report writing finished."
        };
        File.WriteAllLines(path, lines);
        return path;
    }

    public static string WriteFailureMarker(BalanceExperimentDefinition experiment, Exception exception)
    {
        var root = GetReportRoot();
        var path = Path.Combine(root, $"balance_lab_failure_{Sanitize(experiment.ExperimentId)}_{DateTime.Now:yyyyMMdd_HHmmss}.log");
        File.WriteAllText(path, exception.ToString());
        return path;
    }

    public static string WriteProgressMarker(BalanceExperimentDefinition experiment, BalanceLabProgress progress)
    {
        var root = GetReportRoot();
        var path = Path.Combine(root, "balance_lab_live_progress.md");
        var lines = new[]
        {
            "# Balance Lab Live Progress",
            string.Empty,
            $"Updated local: `{DateTime.Now:yyyy-MM-dd HH:mm:ss}`",
            $"Experiment: `{experiment.ExperimentId}`",
            $"Phase: `{progress.Phase}`",
            $"Runs: `{progress.CompletedRuns}/{progress.TotalRuns}`",
            $"Current: `seed={progress.Seed} mage={progress.MageId} policy={progress.BotPolicyId} focus={progress.FocusSpellId} day={progress.CurrentDay}`",
            $"Current run seconds: `{progress.CurrentRunSeconds:0.0}`",
            $"Total simulated seconds: `{progress.TotalSimulatedSeconds:0.0}`",
            $"Outcomes: `defeat={progress.Defeats} victory={progress.Victories} timeout={progress.Timeouts}`",
            $"Averages: `day={progress.AverageDayReached:0.00} damage={progress.AverageDamageOutput:0.0} taken={progress.AverageDamageTaken:0.0}`",
            $"Last outcome: `{progress.LastOutcome}`"
        };
        File.WriteAllLines(path, lines);
        return path;
    }

    public static string BuildSummary(BalanceExperimentResult result)
    {
        var lines = new List<string>
        {
            "# The Last Mage Balance Lab",
            string.Empty,
            $"Experiment: `{result.ExperimentId}`",
            $"Description: {result.Description}",
            $"Started UTC: `{result.StartedAtUtc:O}`",
            $"Finished UTC: `{result.FinishedAtUtc:O}`",
            $"Report directory: `{result.ReportDirectory}`",
            $"Run simulations: {result.Runs.Count}",
            $"Determinism key: `{result.BuildDeterminismKey()}`",
            string.Empty,
            "## Static Content",
            string.Empty,
            $"Production items: {result.StaticAnalysis.ProductionItemCount}",
            $"Spells: {result.StaticAnalysis.SpellCount}",
            $"Mages: {result.StaticAnalysis.MageCount}",
            $"Enemies: {result.StaticAnalysis.EnemyCount}",
            $"Waves: {result.StaticAnalysis.WaveCount}",
            string.Empty,
            "### Item Kinds",
            string.Empty
        };

        AppendCounts(lines, result.StaticAnalysis.ItemKinds);
        lines.Add(string.Empty);
        lines.Add("### Modifier Stats");
        lines.Add(string.Empty);
        AppendCounts(lines, result.StaticAnalysis.ModifierStats);
        lines.Add(string.Empty);
        lines.Add("### Target Tags");
        lines.Add(string.Empty);
        AppendCounts(lines, result.StaticAnalysis.TargetTags);
        lines.Add(string.Empty);
        lines.Add("## Monte Carlo");
        lines.Add(string.Empty);
        lines.Add($"Trials per policy: {result.MonteCarlo.Trials}");
        lines.Add($"Nights per trial: {result.MonteCarlo.NightsPerTrial}");
        foreach (var policy in result.MonteCarlo.PolicySummaries.OrderBy(summary => summary.PolicyId, StringComparer.Ordinal))
        {
            lines.Add($"- `{policy.PolicyId}` offers={policy.OffersGenerated} picks={policy.Picks} missed={policy.MissedOffers} starved={policy.PoolStarvations} locked_excluded={policy.LockedItemsExcluded}");
        }

        lines.Add(string.Empty);
        lines.Add("## Run Aggregates");
        lines.Add(string.Empty);
        foreach (var aggregate in result.AggregateByPolicy())
        {
            lines.Add(
                $"- `{aggregate.Key}` runs={aggregate.RunCount} win={aggregate.WinRatePercent:0.##}% " +
                $"day={aggregate.AverageDayReached:0.##} time={aggregate.AverageTimeSurvived:0.#}s " +
                $"damage={aggregate.AverageDamageOutput:0.#} taken={aggregate.AveragePlayerDamageTaken:0.#} peak={aggregate.AveragePerformancePeakMs:0.##}ms");
        }

        lines.Add(string.Empty);
        lines.Add("## Warnings And Outliers");
        lines.Add(string.Empty);
        if (result.Warnings.Count == 0)
        {
            lines.Add("- None.");
        }
        else
        {
            foreach (var warning in result.Warnings.Distinct(StringComparer.Ordinal).OrderBy(value => value, StringComparer.Ordinal))
            {
                lines.Add($"- {warning}");
            }
        }

        lines.Add(string.Empty);
        lines.Add("## Report Files");
        lines.Add(string.Empty);
        lines.Add("- `balance_runs.jsonl`");
        lines.Add("- `balance_item_correlations.csv`");
        lines.Add("- `balance_spell_breakdown.csv`");
        lines.Add("- `balance_mage_breakdown.csv`");
        lines.Add("- `balance_death_causes.csv`");
        lines.Add("- `balance_performance_peaks.csv`");
        lines.Add("- `balance_outliers.csv`");

        return string.Join(System.Environment.NewLine, lines);
    }

    public static void ExposeOnDebugMetrics(RunContext context, string reportDirectory)
    {
        context.State.DebugMetrics.BalanceReportPath = reportDirectory;
    }

    public static string GetReportRoot()
    {
        var root = Path.Combine(ProjectSettings.GlobalizePath("res://Saved"), "balance_lab");
        Directory.CreateDirectory(root);
        var ignorePath = Path.Combine(root, ".gdignore");
        if (!File.Exists(ignorePath))
        {
            File.WriteAllText(ignorePath, "Generated balance lab reports. Keep this folder out of Godot asset imports.");
        }

        return root;
    }

    private static IEnumerable<string[]> BuildItemCorrelationRows(BalanceExperimentResult result)
    {
        yield return new[] { "policy_id", "item_id", "offers", "picks", "pick_rate_percent" };
        foreach (var policy in result.MonteCarlo.PolicySummaries.OrderBy(summary => summary.PolicyId, StringComparer.Ordinal))
        {
            var itemIds = policy.OfferCounts.Keys.Concat(policy.PickCounts.Keys).Distinct(StringComparer.Ordinal).OrderBy(value => value, StringComparer.Ordinal);
            foreach (var itemId in itemIds)
            {
                var offers = policy.OfferCounts.GetValueOrDefault(itemId);
                var picks = policy.PickCounts.GetValueOrDefault(itemId);
                var rate = offers <= 0 ? 0f : picks * 100f / offers;
                yield return new[] { policy.PolicyId, itemId, offers.ToString(), picks.ToString(), $"{rate:0.###}" };
            }
        }
    }

    private static IEnumerable<string[]> BuildSpellRows(BalanceExperimentResult result)
    {
        yield return new[] { "spell_or_source_id", "focused_runs", "casts", "projectiles", "beams", "areas", "summons", "damage", "damage_per_cast", "damage_run_count" };
        var sourceIds = result.Runs
            .SelectMany(run => run.DamageBySource.Keys)
            .Concat(result.Runs.SelectMany(run => run.SpellCastCounts.Keys))
            .Concat(result.Runs.SelectMany(run => run.SpellProjectileCounts.Keys))
            .Concat(result.Runs.SelectMany(run => run.SpellBeamCounts.Keys))
            .Concat(result.Runs.SelectMany(run => run.SpellAreaCounts.Keys))
            .Concat(result.Runs.SelectMany(run => run.SpellSummonCounts.Keys))
            .Concat(result.Runs.Select(run => run.FocusSpellId))
            .Where(id => !string.IsNullOrWhiteSpace(id) && id != "-")
            .Distinct(StringComparer.Ordinal)
            .OrderBy(id => id, StringComparer.Ordinal);
        foreach (var sourceId in sourceIds)
        {
            var focusedRuns = result.Runs.Count(run => string.Equals(run.FocusSpellId, sourceId, StringComparison.Ordinal));
            var casts = result.Runs.Sum(run => run.SpellCastCounts.GetValueOrDefault(sourceId));
            var projectiles = result.Runs.Sum(run => run.SpellProjectileCounts.GetValueOrDefault(sourceId));
            var beams = result.Runs.Sum(run => run.SpellBeamCounts.GetValueOrDefault(sourceId));
            var areas = result.Runs.Sum(run => run.SpellAreaCounts.GetValueOrDefault(sourceId));
            var summons = result.Runs.Sum(run => run.SpellSummonCounts.GetValueOrDefault(sourceId));
            var damageRuns = result.Runs.Count(run => run.DamageBySource.ContainsKey(sourceId));
            var damage = result.Runs.Sum(run => run.DamageBySource.GetValueOrDefault(sourceId));
            var damagePerCast = casts <= 0 ? 0f : damage / casts;
            yield return new[]
            {
                sourceId,
                focusedRuns.ToString(),
                casts.ToString(),
                projectiles.ToString(),
                beams.ToString(),
                areas.ToString(),
                summons.ToString(),
                $"{damage:0.###}",
                $"{damagePerCast:0.###}",
                damageRuns.ToString()
            };
        }
    }

    private static IEnumerable<string[]> BuildMageRows(BalanceExperimentResult result)
    {
        yield return new[] { "mage_id", "runs", "win_rate_percent", "average_day", "average_damage" };
        foreach (var group in result.Runs.GroupBy(run => run.MageId, StringComparer.Ordinal).OrderBy(group => group.Key, StringComparer.Ordinal))
        {
            var runs = group.ToArray();
            yield return new[]
            {
                group.Key,
                runs.Length.ToString(),
                $"{runs.Count(run => run.Outcome == "victory") * 100f / runs.Length:0.###}",
                $"{runs.Average(run => run.DayReached):0.###}",
                $"{runs.Average(run => run.TotalDamageDealt):0.###}"
            };
        }
    }

    private static IEnumerable<string[]> BuildDeathCauseRows(BalanceExperimentResult result)
    {
        yield return new[] { "death_cause", "count" };
        foreach (var group in result.Runs
            .Where(run => string.Equals(run.Outcome, "defeat", StringComparison.OrdinalIgnoreCase))
            .GroupBy(run => string.IsNullOrWhiteSpace(run.DeathCause) ? "-" : run.DeathCause, StringComparer.Ordinal)
            .OrderByDescending(group => group.Count()))
        {
            yield return new[] { group.Key, group.Count().ToString() };
        }
    }

    private static IEnumerable<string[]> BuildPerformanceRows(BalanceExperimentResult result)
    {
        yield return new[] { "run_id", "seed", "policy_id", "mage_id", "peak_ms", "allocation_peak_bytes", "proc_cap_hits", "feedback_drops" };
        foreach (var run in result.Runs.OrderByDescending(run => run.PerformancePeakMs).ThenByDescending(run => run.AllocationPeakBytes))
        {
            yield return new[]
            {
                run.RunId,
                run.Seed.ToString(),
                run.BotPolicyId,
                run.MageId,
                $"{run.PerformancePeakMs:0.###}",
                run.AllocationPeakBytes.ToString(),
                run.ProcCapHits.ToString(),
                run.FeedbackDrops.ToString()
            };
        }
    }

    private static IEnumerable<string[]> BuildOutlierRows(BalanceExperimentResult result)
    {
        yield return new[] { "kind", "detail" };
        foreach (var warning in result.Warnings.Distinct(StringComparer.Ordinal).OrderBy(value => value, StringComparer.Ordinal))
        {
            yield return new[] { "warning", warning };
        }
    }

    private static void AppendCounts(List<string> lines, IReadOnlyDictionary<string, int> counts)
    {
        if (counts.Count == 0)
        {
            lines.Add("- None.");
            return;
        }

        foreach (var pair in counts.OrderBy(pair => pair.Key, StringComparer.Ordinal))
        {
            lines.Add($"- `{pair.Key}`: {pair.Value}");
        }
    }

    private static void WriteJsonl(string path, IEnumerable<BalanceRunResult> runs)
    {
        using var writer = new StreamWriter(path);
        foreach (var run in runs)
        {
            writer.WriteLine(JsonSerializer.Serialize(run));
        }
    }

    private static void WriteCsv(string path, IEnumerable<string[]> rows)
    {
        File.WriteAllLines(path, rows.Select(row => string.Join(",", row.Select(EscapeCsv))));
    }

    private static string EscapeCsv(string value)
    {
        value ??= string.Empty;
        return value.Contains(',', StringComparison.Ordinal) || value.Contains('"', StringComparison.Ordinal) || value.Contains('\n', StringComparison.Ordinal)
            ? $"\"{value.Replace("\"", "\"\"", StringComparison.Ordinal)}\""
            : value;
    }

    private static string Sanitize(string value)
    {
        var safe = new string(value.Select(character => char.IsLetterOrDigit(character) || character is '_' or '-' ? character : '_').ToArray());
        return string.IsNullOrWhiteSpace(safe) ? "experiment" : safe;
    }
}
