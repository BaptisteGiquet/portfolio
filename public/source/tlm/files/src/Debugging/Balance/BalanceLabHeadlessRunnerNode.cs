using Godot;
using TheLastMage.Gameplay.Run;
using TheLastMage.Save;

namespace TheLastMage.Debugging.Balance;

public partial class BalanceLabHeadlessRunnerNode : Node
{
    private RunControllerNode? _runController;

    public override async void _Ready()
    {
        var commandLine = BalanceLabCommandLine.Parse();
        if (!commandLine.RunBalanceLab)
        {
            GD.Print("Balance Lab headless runner scene loaded without --tlm-balance-lab.");
            return;
        }

        await RunHeadless(commandLine);
    }

    private async Task RunHeadless(BalanceLabCommandLine commandLine)
    {
        try
        {
            _runController = await WaitForRunController();
            _runController.SetProcess(false);
            _runController.SetPhysicsProcess(false);

            var context = _runController.Context
                ?? throw new InvalidOperationException("RunControllerNode did not create a RunContext.");

            var experiment = commandLine.Experiment;
            var startMarker = BalanceReportWriter.WriteStartMarker(experiment);
            GD.Print($"Balance Lab headless start marker: {startMarker}");
            BalanceReportWriter.WriteProgressMarker(experiment, new BalanceLabProgress
            {
                Phase = "starting",
                TotalRuns = experiment.TotalSimulationRuns
            });

            var runner = new BalanceSimulationRunner();
            var result = await runner.RunWithProgressAsync(
                experiment,
                context.Content,
                context.Profile,
                (seed, profile) => CreateBalanceRunContext(seed, profile),
                progress =>
                {
                    BalanceReportWriter.WriteProgressMarker(experiment, progress);
                    return Task.CompletedTask;
                });

            var reportDirectory = BalanceReportWriter.Write(experiment, result);
            BalanceReportWriter.WriteProgressMarker(experiment, new BalanceLabProgress
            {
                Phase = "completed",
                CompletedRuns = result.Runs.Count,
                TotalRuns = result.Runs.Count,
                Defeats = result.Runs.Count(run => string.Equals(run.Outcome, "defeat", StringComparison.OrdinalIgnoreCase)),
                Victories = result.Runs.Count(run => string.Equals(run.Outcome, "victory", StringComparison.OrdinalIgnoreCase)),
                Timeouts = result.Runs.Count(run => string.Equals(run.Outcome, "timeout", StringComparison.OrdinalIgnoreCase)),
                AverageDayReached = result.Runs.Count == 0 ? 0f : (float)result.Runs.Average(run => run.DayReached),
                AverageDamageOutput = result.Runs.Count == 0 ? 0f : (float)result.Runs.Average(run => run.TotalDamageDealt),
                AverageDamageTaken = result.Runs.Count == 0 ? 0f : (float)result.Runs.Average(run => run.PlayerDamageTaken),
                TotalSimulatedSeconds = result.Runs.Sum(run => run.ElapsedSimulatedSeconds),
                LastOutcome = "completed"
            });
            WriteResult(commandLine.ResultPath, "PASS", $"Balance Lab complete: {result.Runs.Count} runs.", reportDirectory);
            GD.Print($"Balance Lab complete: {reportDirectory}");
            GetTree().Quit(0);
        }
        catch (Exception exception)
        {
            var experiment = commandLine.Experiment;
            var failurePath = BalanceReportWriter.WriteFailureMarker(experiment, exception);
            BalanceReportWriter.WriteProgressMarker(experiment, new BalanceLabProgress
            {
                Phase = "failed",
                TotalRuns = experiment.TotalSimulationRuns,
                LastOutcome = exception.Message
            });
            WriteResult(commandLine.ResultPath, "FAIL", exception.Message, failurePath);
            GD.PushError($"Balance Lab failed: {exception}");
            await ToSignal(GetTree().CreateTimer(0.05f), SceneTreeTimer.SignalName.Timeout);
            GetTree().Quit(1);
        }
    }

    private async Task<RunControllerNode> WaitForRunController()
    {
        for (var attempt = 0; attempt < 60; attempt++)
        {
            var controller = GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
            if (controller?.Context != null)
            {
                return controller;
            }

            await ToSignal(GetTree().CreateTimer(0.05f), SceneTreeTimer.SignalName.Timeout);
        }

        throw new InvalidOperationException("Timed out waiting for RunControllerNode.");
    }

    private RunContext CreateBalanceRunContext(int seed, ProfileSaveDto profile)
    {
        if (_runController == null)
        {
            throw new InvalidOperationException("RunControllerNode is not available.");
        }

        return _runController.StartNewRun(
            RunPhase.RunSetup,
            seed,
            profileOverride: profile,
            saveProfileOverride: _ => { });
    }

    private static void WriteResult(string path, string status, string summary, string reportPath)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            return;
        }

        var directory = Path.GetDirectoryName(path);
        if (!string.IsNullOrWhiteSpace(directory))
        {
            Directory.CreateDirectory(directory);
        }

        File.WriteAllLines(path, new[]
        {
            $"status={status}",
            $"summary={summary}",
            $"report={reportPath}"
        });
    }
}
