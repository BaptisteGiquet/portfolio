#if TOOLS
#nullable enable
using Godot;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using TheLastMage.Debugging.Balance;

namespace TheLastMage.EditorTools;

public partial class BalanceLabDock : VBoxContainer
{
    private const string HeadlessScenePath = "res://scenes/debug/balance_lab_headless.tscn";
    private readonly LineEdit _experimentId = new() { Text = "sprint_13c_default" };
    private readonly SpinBox _runs = new() { MinValue = 1, MaxValue = 100_000, Step = 1, Value = 1000 };
    private readonly SpinBox _seedStart = new() { MinValue = 1, MaxValue = int.MaxValue, Step = 1, Value = 130000 };
    private readonly SpinBox _humanity = new() { MinValue = 1, MaxValue = 1_000_000, Step = 10, Value = 1000 };
    private readonly SpinBox _maxDays = new() { MinValue = 1, MaxValue = 365, Step = 1, Value = 20 };
    private readonly SpinBox _daySeconds = new() { MinValue = 1, MaxValue = 3600, Step = 1, Value = 300 };
    private readonly SpinBox _monteCarloTrials = new() { MinValue = 1, MaxValue = 1_000_000, Step = 100, Value = 25000 };
    private readonly SpinBox _monteCarloNights = new() { MinValue = 1, MaxValue = 100, Step = 1, Value = 20 };
    private readonly OptionButton _runMode = new();
    private readonly OptionButton _itemProfileMode = new();
    private readonly LineEdit _policies = new() { Text = "random_build,aggressive_damage,survival,synergy,stationary_output" };
    private readonly LineEdit _mages = new() { PlaceholderText = "optional comma-separated mage ids" };
    private readonly LineEdit _spells = new() { PlaceholderText = "optional comma-separated spell ids" };
    private readonly Button _startButton = new() { Text = "Start Headless Lab" };
    private readonly Button _stopButton = new() { Text = "Stop" };
    private readonly Button _refreshButton = new() { Text = "Refresh Results" };
    private readonly Label _status = new() { AutowrapMode = TextServer.AutowrapMode.WordSmart };
    private readonly Label _fieldHelp = new()
    {
        AutowrapMode = TextServer.AutowrapMode.WordSmart,
        Text = BuildFieldHelpText()
    };
    private readonly RichTextLabel _results = new()
    {
        BbcodeEnabled = false,
        FitContent = false,
        ScrollActive = true,
        SelectionEnabled = true,
        SizeFlagsHorizontal = SizeFlags.ExpandFill,
        SizeFlagsVertical = SizeFlags.ExpandFill
    };

    private Process? _process;
    private string _activeResultPath = string.Empty;
    private string _activeLogPath = string.Empty;
    private double _refreshTimer;
    private bool _processExitPending;

    public BalanceLabDock()
    {
        Name = "TLMBalanceLabDockContent";
        BuildUi();
    }

    public override void _Ready()
    {
        SetProcess(true);
        RefreshResults();
        UpdateButtons();
    }

    public override void _ExitTree()
    {
        DetachProcessHandlers();
    }

    public override void _Process(double delta)
    {
        _refreshTimer -= delta;
        if (_refreshTimer <= 0.0)
        {
            _refreshTimer = 0.5;
            PollProcess();
            RefreshResults();
        }
    }

    private void BuildUi()
    {
        foreach (var child in GetChildren())
        {
            RemoveChild(child);
            child.QueueFree();
        }

        AddThemeConstantOverride("separation", 8);
        SizeFlagsHorizontal = SizeFlags.ExpandFill;
        SizeFlagsVertical = SizeFlags.ExpandFill;

        var scroll = new ScrollContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled
        };
        AddChild(scroll);

        var content = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        content.AddThemeConstantOverride("separation", 8);
        scroll.AddChild(content);

        content.AddChild(new Label { Text = "Balance Lab" });
        content.AddChild(BuildGrid());

        var buttons = new HBoxContainer();
        buttons.AddThemeConstantOverride("separation", 6);
        _startButton.Pressed += StartHeadlessLab;
        _stopButton.Pressed += StopHeadlessLab;
        _refreshButton.Pressed += RefreshResults;
        buttons.AddChild(_startButton);
        buttons.AddChild(_stopButton);
        buttons.AddChild(_refreshButton);
        content.AddChild(buttons);

        _status.Text = "Ready.";
        content.AddChild(_status);
        content.AddChild(BuildReferenceSection());

        _results.CustomMinimumSize = new Vector2(0f, 520f);
        content.AddChild(_results);

        FillEnumOptions(_runMode, BalanceRunMode.AcceleratedLongRun);
        FillEnumOptions(_itemProfileMode, BalanceItemProfileMode.AllProductionUnlockedAndDiscovered);
        _runMode.ItemSelected += _ => ApplyRunModePreset();
    }

    private Control BuildGrid()
    {
        var grid = new GridContainer
        {
            Columns = 2,
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        grid.AddThemeConstantOverride("h_separation", 8);
        grid.AddThemeConstantOverride("v_separation", 4);

        AddRow(grid, "Experiment", _experimentId);
        AddRow(grid, "Run mode", _runMode);
        AddRow(grid, "Runs", _runs);
        AddRow(grid, "Seed start", _seedStart);
        AddRow(grid, "Humanity", _humanity);
        AddRow(grid, "Max days", _maxDays);
        AddRow(grid, "Seconds/day", _daySeconds);
        AddRow(grid, "Monte Carlo trials", _monteCarloTrials);
        AddRow(grid, "Monte Carlo nights", _monteCarloNights);
        AddRow(grid, "Item profile", _itemProfileMode);
        AddRow(grid, "Policies", _policies);
        AddRow(grid, "Mages", _mages);
        AddRow(grid, "Spells", _spells);
        return grid;
    }

    private Control BuildReferenceSection()
    {
        var panel = new PanelContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        var margin = new MarginContainer();
        margin.AddThemeConstantOverride("margin_left", 8);
        margin.AddThemeConstantOverride("margin_top", 6);
        margin.AddThemeConstantOverride("margin_right", 8);
        margin.AddThemeConstantOverride("margin_bottom", 6);
        panel.AddChild(margin);
        margin.AddChild(_fieldHelp);
        return panel;
    }

    private static void AddRow(GridContainer grid, string label, Control control)
    {
        grid.AddChild(new Label { Text = label });
        control.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        grid.AddChild(control);
    }

    private static void FillEnumOptions<TEnum>(OptionButton options, TEnum selected)
        where TEnum : struct, Enum
    {
        options.Clear();
        var selectedIndex = 0;
        var values = Enum.GetValues<TEnum>();
        for (var i = 0; i < values.Length; i++)
        {
            var label = values[i].ToString();
            options.AddItem(label);
            if (EqualityComparer<TEnum>.Default.Equals(values[i], selected))
            {
                selectedIndex = i;
            }
        }

        options.Select(selectedIndex);
    }

    private void ApplyRunModePreset()
    {
        if (!Enum.TryParse<BalanceRunMode>(SelectedText(_runMode), true, out var mode))
        {
            return;
        }

        var preset = BalanceLabPreset.ForMode(mode);
        _runs.Value = preset.Runs;
        _humanity.Value = preset.Humanity;
        _maxDays.Value = preset.MaxDays;
        _daySeconds.Value = preset.SecondsPerDay;
        _monteCarloTrials.Value = preset.MonteCarloTrials;
        _monteCarloNights.Value = preset.MonteCarloNights;
        SetStatus($"Applied {mode} preset. You can still override individual fields before starting.");
    }

    private void StartHeadlessLab()
    {
        if (_process is { HasExited: false })
        {
            SetStatus("Balance Lab is already running.");
            return;
        }

        var root = BalanceReportWriter.GetReportRoot();
        var runDirectory = Path.Combine(root, "headless_runs", DateTime.Now.ToString("yyyyMMdd_HHmmss", CultureInfo.InvariantCulture));
        var userDataDirectory = Path.Combine(runDirectory, "user_data");
        Directory.CreateDirectory(userDataDirectory);
        _activeResultPath = Path.Combine(runDirectory, "balance_lab.result");
        _activeLogPath = Path.Combine(runDirectory, "balance_lab.process.log");

        var startInfo = new ProcessStartInfo
        {
            FileName = ResolveHeadlessExecutable(),
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };

        foreach (var argument in BuildProcessArguments(userDataDirectory, _activeResultPath))
        {
            startInfo.ArgumentList.Add(argument);
        }

        _process = new Process { StartInfo = startInfo, EnableRaisingEvents = true };
        _process.OutputDataReceived += OnProcessOutput;
        _process.ErrorDataReceived += OnProcessOutput;
        _process.Exited += OnProcessExited;
        _process.Start();
        _process.BeginOutputReadLine();
        _process.BeginErrorReadLine();

        File.WriteAllText(_activeLogPath, $"Started {DateTime.Now:O}{System.Environment.NewLine}{startInfo.FileName} {string.Join(' ', startInfo.ArgumentList)}{System.Environment.NewLine}");
        SetStatus($"Running headless Balance Lab. Result: {_activeResultPath}");
        UpdateButtons();
        RefreshResults();
    }

    private IEnumerable<string> BuildProcessArguments(string userDataDirectory, string resultPath)
    {
        yield return "--headless";
        yield return "--path";
        yield return ProjectSettings.GlobalizePath("res://");
        yield return "--scene";
        yield return HeadlessScenePath;
        yield return "--";
        yield return "--tlm-balance-lab";
        yield return $"--tlm-user-data={userDataDirectory}";
        yield return $"--tlm-balance-result={resultPath}";
        yield return $"--tlm-balance-experiment={SanitizeId(_experimentId.Text)}";
        yield return "--tlm-balance-description=Editor-launched headless Balance Lab run.";
        yield return $"--tlm-balance-run-mode={SelectedText(_runMode)}";
        yield return $"--tlm-balance-runs={(int)_runs.Value}";
        yield return $"--tlm-balance-seed-start={(int)_seedStart.Value}";
        yield return $"--tlm-balance-seed-count={(int)_runs.Value}";
        yield return $"--tlm-balance-humanity={(int)_humanity.Value}";
        yield return $"--tlm-balance-max-days={(int)_maxDays.Value}";
        yield return $"--tlm-balance-day-seconds={FormatFloat((float)_daySeconds.Value)}";
        yield return $"--tlm-balance-monte-carlo-trials={(int)_monteCarloTrials.Value}";
        yield return $"--tlm-balance-monte-carlo-nights={(int)_monteCarloNights.Value}";
        yield return $"--tlm-balance-item-profile={SelectedText(_itemProfileMode)}";
        yield return $"--tlm-balance-policies={_policies.Text.Trim()}";

        if (!string.IsNullOrWhiteSpace(_mages.Text))
        {
            yield return $"--tlm-balance-mages={_mages.Text.Trim()}";
        }

        if (!string.IsNullOrWhiteSpace(_spells.Text))
        {
            yield return $"--tlm-balance-spells={_spells.Text.Trim()}";
        }
    }

    private void StopHeadlessLab()
    {
        PollProcess();
        if (_process == null)
        {
            SetStatus("No headless Balance Lab process is running.");
            return;
        }

        if (HasPassingResult(_activeResultPath))
        {
            if (!_process.HasExited)
            {
                _process.Kill(entireProcessTree: true);
            }

            DetachProcessHandlers();
            SetStatus($"Headless Balance Lab had already completed. Result: {_activeResultPath}");
            UpdateButtons();
            RefreshResults();
            return;
        }

        if (_process is not { HasExited: false })
        {
            SetStatus("No headless Balance Lab process is running.");
            return;
        }

        _process.Kill(entireProcessTree: true);
        BalanceReportWriter.WriteProgressMarker(BuildExperimentFromFields(), new BalanceLabProgress
        {
            Phase = "interrupted",
            TotalRuns = (int)_runs.Value,
            LastOutcome = "stopped from editor dock"
        });
        SetStatus("Stopped headless Balance Lab process.");
        UpdateButtons();
        RefreshResults();
    }

    private void PollProcess()
    {
        if (_process == null)
        {
            UpdateButtons();
            return;
        }

        if (!_processExitPending && !_process.HasExited)
        {
            UpdateButtons();
            return;
        }

        var exitCode = _process.HasExited ? _process.ExitCode : 0;
        DetachProcessHandlers();
        SetStatus($"Headless Balance Lab exited with code {exitCode}. Result: {_activeResultPath}");
        UpdateButtons();
    }

    private void RefreshResults()
    {
        var text = new StringBuilder();
        AppendIfExists(text, Path.Combine(BalanceReportWriter.GetReportRoot(), "balance_lab_live_progress.md"), "Live Progress");

        if (!string.IsNullOrWhiteSpace(_activeResultPath))
        {
            AppendIfExists(text, _activeResultPath, "Active Result");
        }

        var latestSummary = FindLatestSummary();
        if (!string.IsNullOrWhiteSpace(latestSummary))
        {
            AppendIfExists(text, latestSummary, "Latest Completed Summary");
        }

        if (!string.IsNullOrWhiteSpace(_activeLogPath))
        {
            AppendTailIfExists(text, _activeLogPath, "Process Log", 80);
        }

        _results.Text = text.Length == 0
            ? $"No Balance Lab output yet.\nReport root: {BalanceReportWriter.GetReportRoot()}"
            : text.ToString();
    }

    private void UpdateButtons()
    {
        var running = _process is { HasExited: false };
        _startButton.Disabled = running;
        _stopButton.Disabled = !running;
    }

    private void SetStatus(string status)
    {
        _status.Text = status;
        GD.Print($"TLM Balance Lab: {status}");
    }

    private void OnProcessOutput(object sender, DataReceivedEventArgs e)
    {
        if (string.IsNullOrEmpty(e.Data)
            || string.IsNullOrWhiteSpace(_activeLogPath)
            || !ShouldCaptureProcessLine(e.Data))
        {
            return;
        }

        try
        {
            File.AppendAllText(_activeLogPath, e.Data + System.Environment.NewLine);
        }
        catch
        {
            // Editor log capture is best-effort; the result/progress markers remain authoritative.
        }
    }

    private void OnProcessExited(object? sender, EventArgs e)
    {
        _processExitPending = true;
    }

    private void DetachProcessHandlers()
    {
        if (_process == null)
        {
            return;
        }

        _process.OutputDataReceived -= OnProcessOutput;
        _process.ErrorDataReceived -= OnProcessOutput;
        _process.Exited -= OnProcessExited;
        _process.Dispose();
        _process = null;
        _processExitPending = false;
    }

    private BalanceExperimentDefinition BuildExperimentFromFields()
    {
        return BalanceExperimentDefinition.Sprint13CDefault() with
        {
            ExperimentId = SanitizeId(_experimentId.Text),
            RunMode = Enum.TryParse<BalanceRunMode>(SelectedText(_runMode), true, out var runMode) ? runMode : BalanceRunMode.AcceleratedLongRun,
            TotalSimulationRuns = (int)_runs.Value,
            SeedStart = (int)_seedStart.Value,
            SeedCount = (int)_runs.Value,
            StartingHumanity = (int)_humanity.Value,
            MaxSimulatedDays = (int)_maxDays.Value,
            MaxSimulatedSecondsPerDay = (float)_daySeconds.Value,
            MonteCarloTrials = (int)_monteCarloTrials.Value,
            MonteCarloNightsPerTrial = (int)_monteCarloNights.Value,
            ItemProfileMode = Enum.TryParse<BalanceItemProfileMode>(SelectedText(_itemProfileMode), true, out var itemProfileMode)
                ? itemProfileMode
                : BalanceItemProfileMode.AllProductionUnlockedAndDiscovered
        };
    }

    private static void AppendIfExists(StringBuilder text, string path, string title)
    {
        if (!File.Exists(path))
        {
            return;
        }

        text.AppendLine($"== {title} ==");
        text.AppendLine(path);
        text.AppendLine(File.ReadAllText(path));
        text.AppendLine();
    }

    private static void AppendTailIfExists(StringBuilder text, string path, string title, int maxLines)
    {
        if (!File.Exists(path))
        {
            return;
        }

        var lines = ReadLastLines(path, maxLines);
        text.AppendLine($"== {title} ==");
        text.AppendLine(path);
        foreach (var line in lines)
        {
            text.AppendLine(line);
        }

        text.AppendLine();
    }

    private static string FindLatestSummary()
    {
        var root = BalanceReportWriter.GetReportRoot();
        if (!Directory.Exists(root))
        {
            return string.Empty;
        }

        return Directory
            .EnumerateFiles(root, "balance_summary.md", SearchOption.AllDirectories)
            .OrderByDescending(File.GetLastWriteTime)
            .FirstOrDefault() ?? string.Empty;
    }

    private static string SelectedText(OptionButton options)
    {
        return options.Selected >= 0 ? options.GetItemText(options.Selected) : string.Empty;
    }

    private static string FormatFloat(float value)
    {
        return value.ToString("0.###", CultureInfo.InvariantCulture);
    }

    private static string SanitizeId(string value)
    {
        var safe = new string((value ?? string.Empty)
            .Select(character => char.IsLetterOrDigit(character) || character is '_' or '-' ? character : '_')
            .ToArray());
        return string.IsNullOrWhiteSpace(safe) ? "balance_lab" : safe;
    }

    private static string ResolveHeadlessExecutable()
    {
        var executable = OS.GetExecutablePath();
        if (!OperatingSystem.IsWindows())
        {
            return executable;
        }

        var directory = Path.GetDirectoryName(executable);
        var fileName = Path.GetFileNameWithoutExtension(executable);
        var extension = Path.GetExtension(executable);
        if (string.IsNullOrWhiteSpace(directory)
            || string.IsNullOrWhiteSpace(fileName)
            || fileName.EndsWith("_console", StringComparison.OrdinalIgnoreCase))
        {
            return executable;
        }

        var consoleExecutable = Path.Combine(directory, $"{fileName}_console{extension}");
        return File.Exists(consoleExecutable) ? consoleExecutable : executable;
    }

    private static bool HasPassingResult(string path)
    {
        if (string.IsNullOrWhiteSpace(path) || !File.Exists(path))
        {
            return false;
        }

        return File.ReadLines(path).Any(line => string.Equals(line.Trim(), "status=PASS", StringComparison.OrdinalIgnoreCase));
    }

    private static bool ShouldCaptureProcessLine(string line)
    {
        return line.Contains("Balance Lab", StringComparison.OrdinalIgnoreCase)
            || line.Contains("ERROR:", StringComparison.OrdinalIgnoreCase)
            || line.Contains("WARNING:", StringComparison.OrdinalIgnoreCase)
            || line.Contains("CrashHandlerException", StringComparison.OrdinalIgnoreCase)
            || line.Contains("status=", StringComparison.OrdinalIgnoreCase)
            || line.Contains("summary=", StringComparison.OrdinalIgnoreCase)
            || line.Contains("report=", StringComparison.OrdinalIgnoreCase);
    }

    private static IReadOnlyList<string> ReadLastLines(string path, int maxLines)
    {
        const int TailBytes = 16 * 1024;
        using var stream = File.Open(path, FileMode.Open, System.IO.FileAccess.Read, FileShare.ReadWrite);
        var start = Math.Max(0, stream.Length - TailBytes);
        stream.Seek(start, SeekOrigin.Begin);
        using var reader = new StreamReader(stream, Encoding.UTF8, detectEncodingFromByteOrderMarks: true);
        var content = reader.ReadToEnd();
        return content
            .Split(new[] { "\r\n", "\n" }, StringSplitOptions.None)
            .Where(line => !string.IsNullOrEmpty(line))
            .TakeLast(Math.Max(1, maxLines))
            .ToArray();
    }

    private static string BuildFieldHelpText()
    {
        return
            "Run modes are presets for the numeric fields. ShortRun is a quick wiring check; VerticalSlice is a moderate gameplay sample; AcceleratedLongRun is the 1000-run deep balance pass; Stress is many very short runs for repeatability/load pressure.\n\n" +
            "Humanity is the run victory budget. Each enemy death reduces it. With the Sprint 13C default, the 20 authored waves contain 1000 total enemies; clearing the configured final day is counted as a successful lab run.\n\n" +
            "Seconds/day is the simulated combat-day time limit. If a wave is still active when this limit expires, that run is recorded as a timeout. If the wave is cleared before the limit, the bot goes to Night Market and starts the next day.\n\n" +
            "Policies: random_build picks random offers/spells; aggressive_damage strongly favors damage, fire-rate, spell-count, projectile, area and proc power; survival favors health, move speed, duration, range, luck, control and active items; synergy favors items matching the current spell tags; stationary_output favors offensive/synergy picks, repeatedly uses the first ready spell, and does not use activatables.\n\n" +
            "Mages and Spells are optional comma-separated filters. Empty means rotate through all production mages/spells. A specific mage id tests only that mage. A specific spell id makes the lab focus loadouts around that spell first, then fills remaining spell slots deterministically.";
    }

    private readonly record struct BalanceLabPreset(
        int Runs,
        int Humanity,
        int MaxDays,
        int SecondsPerDay,
        int MonteCarloTrials,
        int MonteCarloNights)
    {
        public static BalanceLabPreset ForMode(BalanceRunMode mode)
        {
            return mode switch
            {
                BalanceRunMode.ShortRun => new BalanceLabPreset(50, 1000, 2, 30, 1000, 4),
                BalanceRunMode.VerticalSlice => new BalanceLabPreset(250, 1000, 5, 120, 5000, 8),
                BalanceRunMode.Stress => new BalanceLabPreset(2000, 1000, 3, 45, 10000, 8),
                _ => new BalanceLabPreset(1000, 1000, 20, 300, 25000, 20)
            };
        }
    }
}
#endif
