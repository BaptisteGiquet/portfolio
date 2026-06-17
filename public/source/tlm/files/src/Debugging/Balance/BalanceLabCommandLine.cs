using System.Globalization;
using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Debugging.Balance;

public readonly record struct BalanceLabCommandLine(
    bool RunBalanceLab,
    string ResultPath,
    BalanceExperimentDefinition Experiment)
{
    public static BalanceLabCommandLine Parse()
    {
        var runBalanceLab = false;
        var resultPath = string.Empty;
        var experiment = BalanceExperimentDefinition.Sprint13CDefault();

        foreach (var argument in OS.GetCmdlineUserArgs())
        {
            if (string.Equals(argument, "--tlm-balance-lab", StringComparison.OrdinalIgnoreCase))
            {
                runBalanceLab = true;
            }
            else if (argument.StartsWith("--tlm-balance-result=", StringComparison.OrdinalIgnoreCase))
            {
                resultPath = ReadValue(argument, "--tlm-balance-result=");
            }
            else if (argument.StartsWith("--tlm-balance-experiment=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { ExperimentId = ReadValue(argument, "--tlm-balance-experiment=") };
            }
            else if (argument.StartsWith("--tlm-balance-description=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { Description = ReadValue(argument, "--tlm-balance-description=") };
            }
            else if (argument.StartsWith("--tlm-balance-runs=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { TotalSimulationRuns = ReadInt(argument, "--tlm-balance-runs=", experiment.TotalSimulationRuns) };
            }
            else if (argument.StartsWith("--tlm-balance-seed-start=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { SeedStart = ReadInt(argument, "--tlm-balance-seed-start=", experiment.SeedStart) };
            }
            else if (argument.StartsWith("--tlm-balance-seed-count=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { SeedCount = ReadInt(argument, "--tlm-balance-seed-count=", experiment.SeedCount) };
            }
            else if (argument.StartsWith("--tlm-balance-humanity=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { StartingHumanity = ReadInt(argument, "--tlm-balance-humanity=", experiment.StartingHumanity) };
            }
            else if (argument.StartsWith("--tlm-balance-max-days=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { MaxSimulatedDays = ReadInt(argument, "--tlm-balance-max-days=", experiment.MaxSimulatedDays) };
            }
            else if (argument.StartsWith("--tlm-balance-day-seconds=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { MaxSimulatedSecondsPerDay = ReadFloat(argument, "--tlm-balance-day-seconds=", experiment.MaxSimulatedSecondsPerDay) };
            }
            else if (argument.StartsWith("--tlm-balance-monte-carlo-trials=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { MonteCarloTrials = ReadInt(argument, "--tlm-balance-monte-carlo-trials=", experiment.MonteCarloTrials) };
            }
            else if (argument.StartsWith("--tlm-balance-monte-carlo-nights=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { MonteCarloNightsPerTrial = ReadInt(argument, "--tlm-balance-monte-carlo-nights=", experiment.MonteCarloNightsPerTrial) };
            }
            else if (argument.StartsWith("--tlm-balance-run-mode=", StringComparison.OrdinalIgnoreCase)
                && Enum.TryParse<BalanceRunMode>(ReadValue(argument, "--tlm-balance-run-mode="), true, out var runMode))
            {
                experiment = experiment with { RunMode = runMode };
            }
            else if (argument.StartsWith("--tlm-balance-item-profile=", StringComparison.OrdinalIgnoreCase)
                && Enum.TryParse<BalanceItemProfileMode>(ReadValue(argument, "--tlm-balance-item-profile="), true, out var itemProfileMode))
            {
                experiment = experiment with { ItemProfileMode = itemProfileMode };
            }
            else if (argument.StartsWith("--tlm-balance-policies=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { BotPolicyIds = SplitStrings(ReadValue(argument, "--tlm-balance-policies=")) };
            }
            else if (argument.StartsWith("--tlm-balance-mages=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { MageIncludeList = SplitContentIds(ReadValue(argument, "--tlm-balance-mages=")) };
            }
            else if (argument.StartsWith("--tlm-balance-spells=", StringComparison.OrdinalIgnoreCase))
            {
                experiment = experiment with { SpellPool = SplitContentIds(ReadValue(argument, "--tlm-balance-spells=")) };
            }
        }

        return new BalanceLabCommandLine(runBalanceLab, resultPath, experiment);
    }

    private static string ReadValue(string argument, string prefix)
    {
        return argument[prefix.Length..].Trim();
    }

    private static int ReadInt(string argument, string prefix, int fallback)
    {
        return int.TryParse(ReadValue(argument, prefix), NumberStyles.Integer, CultureInfo.InvariantCulture, out var value)
            ? Math.Max(0, value)
            : fallback;
    }

    private static float ReadFloat(string argument, string prefix, float fallback)
    {
        return float.TryParse(ReadValue(argument, prefix), NumberStyles.Float, CultureInfo.InvariantCulture, out var value)
            ? MathF.Max(0f, value)
            : fallback;
    }

    private static IReadOnlyList<string> SplitStrings(string value)
    {
        return value
            .Split(new[] { ',', ';' }, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Where(entry => !string.IsNullOrWhiteSpace(entry))
            .Distinct(StringComparer.Ordinal)
            .ToArray();
    }

    private static IReadOnlyList<ContentId> SplitContentIds(string value)
    {
        return SplitStrings(value)
            .Select(ContentId.From)
            .Where(id => id.IsValid)
            .ToArray();
    }
}
