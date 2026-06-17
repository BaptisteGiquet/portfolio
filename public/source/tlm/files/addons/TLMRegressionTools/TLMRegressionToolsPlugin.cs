#if TOOLS
#nullable enable
using Godot;
using System;
using System.Collections.Generic;
using System.IO;

[Tool]
public partial class TLMRegressionToolsPlugin : EditorPlugin
{
    private const string RegressionScenePath = "res://scenes/debug/regression_runner.tscn";
    private const int MaxTransientRetries = 4;
    private const double InterScenarioDelaySeconds = 0.75;

    private static readonly RegressionScenario[] Scenarios =
    {
        new("seven_spell_combat", "Seven Spell Combat"),
        new("player_damage", "Player Damage Contract"),
        new("fire_rate", "Fire Rate Contract"),
        new("duration", "Duration Contract"),
        new("range", "Range Contract"),
        new("night_market", "Night Market Command Flow"),
        new("tooling_gate", "Gate G Tooling"),
        new("sprint_08_defense", "Sprint 08 Wave Tower Defense"),
        new("sprint_09_enemies", "Sprint 09 Enemy Variety"),
        new("sprint_10_meta", "Sprint 10 Mage Meta Progression"),
        new("sprint_11_items", "Sprint 11 Vertical Slice Items"),
        new("sprint_11_5_item_authoring", "Sprint 11.5 Item Authoring Pipeline"),
        new("sprint_12_ux", "Sprint 12 UX Feedback Readability"),
        new("sprint_13_reporting", "Sprint 13 Reporting Kickoff"),
        new("sprint_13a_activatable_items", "Sprint 13A Activatable Items"),
        new("sprint_13b_gameplay_tags", "Sprint 13B Gameplay Tags"),
        new("sprint_13c_balance_lab", "Sprint 13C Balance Simulation Lab"),
        new("sprint_15_full_run_skeleton", "Sprint 15 Full Run Skeleton"),
        new("sprint_16_localization_spine", "Sprint 16 Localization Spine"),
        new("sprint_17_controller_input", "Sprint 17 Controller Input"),
        new("sprint_18_frontend_save_stats", "Sprint 18 Front-End Save Stats"),
        new("sprint_19_options_pause", "Sprint 19 Options Pause"),
        new("item_cast_flow_behavior", "Item Cast Flow Behavior"),
        new("production_item_behavior", "Production Item Behavior")
    };

    private Button? _toolbarButton;
    private EditorDock? _dock;
    private TLMRegressionToolsDock? _dockContent;
    private bool _running;
    private bool _waitingForSceneStart;
    private bool _waitingForSceneEnd;
    private bool _retryActiveScenarioPending;
    private double _launchDelaySeconds;
    private int _scenarioIndex;
    private string _sequenceDirectory = string.Empty;
    private string _aggregateReportPath = string.Empty;
    private RegressionScenario? _activeScenario;
    private string _activeReportPath = string.Empty;
    private string _activeResultPath = string.Empty;
    private int _activeRetryCount;
    private readonly List<string> _aggregateLines = new();

    public override void _EnterTree()
    {
        CleanupStaleEditorUi();
        BuildToolbarButton();
        BuildDock();
        SetProcess(true);
    }

    public override void _ExitTree()
    {
        SetProcess(false);
        TearDownToolbarButton();
        TearDownDock();
    }

    public override void _Process(double delta)
    {
        if (!_running)
        {
            return;
        }

        var editor = EditorInterface.Singleton;
        var isPlaying = editor.IsPlayingScene();

        if (_waitingForSceneStart)
        {
            if (isPlaying)
            {
                _waitingForSceneStart = false;
                _waitingForSceneEnd = true;
                SetStatus($"Running {_activeScenario?.DisplayName ?? "scenario"}...");
            }

            return;
        }

        if (_waitingForSceneEnd)
        {
            if (isPlaying)
            {
                return;
            }

            _waitingForSceneEnd = false;
            RecordActiveScenarioResult();
            _launchDelaySeconds = InterScenarioDelaySeconds;
            return;
        }

        if (_launchDelaySeconds > 0.0)
        {
            _launchDelaySeconds -= delta;
            if (_launchDelaySeconds > 0.0)
            {
                return;
            }
        }

        if (_retryActiveScenarioPending)
        {
            _retryActiveScenarioPending = false;
            LaunchActiveScenario();
            return;
        }

        LaunchNextScenario();
    }

    public override string[] _RunScene(string sceneFile, string[] args)
    {
        if (!_running
            || _activeScenario == null
            || !string.Equals(sceneFile, RegressionScenePath, StringComparison.OrdinalIgnoreCase))
        {
            return args;
        }

        var output = new List<string>(args);
        output.Add("--");
        output.Add($"--tlm-regression-scenario={_activeScenario.Value.Id}");
        output.Add($"--tlm-regression-report={_activeReportPath}");
        output.Add($"--tlm-regression-result={_activeResultPath}");
        output.Add("--tlm-regression-env");
        output.Add("--tlm-regression-quit");
        return output.ToArray();
    }

    private void BuildToolbarButton()
    {
        _toolbarButton = new Button
        {
            Name = "TLMRegressionToolsStartButton",
            Text = "Start Automated Tests",
            TooltipText = "Run each TLM regression scenario in a separate editor play session."
        };
        _toolbarButton.Pressed += StartAutomatedTests;
        AddControlToContainer(CustomControlContainer.Toolbar, _toolbarButton);
    }

    private void TearDownToolbarButton()
    {
        if (_toolbarButton == null)
        {
            return;
        }

        _toolbarButton.Pressed -= StartAutomatedTests;
        RemoveControlFromContainer(CustomControlContainer.Toolbar, _toolbarButton);
        _toolbarButton.QueueFree();
        _toolbarButton = null;
    }

    private void BuildDock()
    {
        _dockContent = new TLMRegressionToolsDock();

        _dock = new EditorDock
        {
            Name = "TLMRegressionToolsDock",
            Title = "TLM Tests",
            DefaultSlot = EditorDock.DockSlot.RightBl,
            LayoutKey = "tlm_regression_tools"
        };
        _dock.AddChild(_dockContent);
        AddDock(_dock);
        SetStatus("Ready.");
    }

    private void TearDownDock()
    {
        if (_dock != null)
        {
            RemoveDock(_dock);
            _dock.QueueFree();
        }

        _dockContent = null;
        _dock = null;
    }

    private static void CleanupStaleEditorUi()
    {
        var baseControl = EditorInterface.Singleton.GetBaseControl();
        if (baseControl == null)
        {
            return;
        }

        RemoveStaleNodesRecursive(baseControl);
    }

    private static void RemoveStaleNodesRecursive(Node node)
    {
        foreach (var child in node.GetChildren())
        {
            if (child is Button button
                && (button.Name == "TLMRegressionToolsStartButton"
                    || string.Equals(button.Text, "Start Automated Tests", StringComparison.Ordinal)))
            {
                button.QueueFree();
                continue;
            }

            if (child.Name == "TLMRegressionToolsDock"
                || child.Name == "TLMRegressionToolsDockContent")
            {
                child.QueueFree();
                continue;
            }

            RemoveStaleNodesRecursive(child);
        }
    }

    private void StartAutomatedTests()
    {
        if (_running)
        {
            SetStatus("Already running automated tests.");
            return;
        }

        if (EditorInterface.Singleton.IsPlayingScene())
        {
            SetStatus("Stop the current play session before starting automated tests.");
            return;
        }

        var root = ProjectSettings.GlobalizePath("res://Saved/regression_reports/editor_sequences");
        _sequenceDirectory = Path.Combine(root, DateTime.Now.ToString("yyyyMMdd_HHmmss"));
        Directory.CreateDirectory(_sequenceDirectory);
        _aggregateReportPath = Path.Combine(_sequenceDirectory, "summary.log");
        _aggregateLines.Clear();
        _scenarioIndex = 0;
        _running = true;
        _waitingForSceneStart = false;
        _waitingForSceneEnd = false;
        _retryActiveScenarioPending = false;
        _launchDelaySeconds = 0.0;
        _activeScenario = null;
        _activeReportPath = string.Empty;
        _activeResultPath = string.Empty;
        _activeRetryCount = 0;
        SetButtonsDisabled(true);
        AppendAggregate($"TLM automated regression sequence started: {_sequenceDirectory}");
        SetStatus("Starting automated tests...");
        LaunchNextScenario();
    }

    private void LaunchNextScenario()
    {
        if (_scenarioIndex >= Scenarios.Length)
        {
            FinishSequence();
            return;
        }

        _activeScenario = Scenarios[_scenarioIndex];
        _scenarioIndex++;
        _activeReportPath = Path.Combine(_sequenceDirectory, $"{_scenarioIndex:00}_{_activeScenario.Value.Id}.log");
        _activeResultPath = Path.Combine(_sequenceDirectory, $"{_scenarioIndex:00}_{_activeScenario.Value.Id}.result");
        DeleteIfExists(_activeReportPath);
        DeleteIfExists(_activeResultPath);
        _activeRetryCount = 0;
        _retryActiveScenarioPending = false;

        LaunchActiveScenario();
    }

    private void RecordActiveScenarioResult()
    {
        if (_activeScenario == null)
        {
            return;
        }

        var result = ReadResultFile(_activeResultPath);
        if (IsTransientRegressionResult(result) && _activeRetryCount < MaxTransientRetries)
        {
            _activeRetryCount++;
            AppendAggregate($"RETRY transient {_activeScenario.Value.DisplayName}: {result.Status}");
            DeleteIfExists(_activeResultPath);
            SetStatus($"Retrying {_activeScenario.Value.DisplayName} after transient {result.Status}...");
            _retryActiveScenarioPending = true;
            return;
        }

        AppendAggregate($"{result.Status} {_activeScenario.Value.DisplayName}: {result.Summary}");
        AppendAggregate($"  report={result.ReportPath}");
        SetStatus($"{result.Status} {_activeScenario.Value.DisplayName}");
    }

    private void FinishSequence()
    {
        _running = false;
        SetButtonsDisabled(false);
        var failed = 0;
        foreach (var line in _aggregateLines)
        {
            if (line.Contains(" FAIL ", StringComparison.OrdinalIgnoreCase)
                || line.Contains(" MISSING_RESULT ", StringComparison.OrdinalIgnoreCase))
            {
                failed++;
            }
        }

        var passed = Scenarios.Length - failed;
        AppendAggregate($"COMPLETE passed={passed} failed={failed}");
        SetStatus($"Automated tests complete: {passed} passed, {failed} failed. Summary: {_aggregateReportPath}");
    }

    private RegressionResult ReadResultFile(string resultPath)
    {
        if (!File.Exists(resultPath))
        {
            return new RegressionResult("MISSING_RESULT", "Result file was not written by the play session.", _activeReportPath);
        }

        var status = "UNKNOWN";
        var summary = string.Empty;
        var report = _activeReportPath;
        foreach (var line in File.ReadAllLines(resultPath))
        {
            if (line.StartsWith("status=", StringComparison.OrdinalIgnoreCase))
            {
                status = line["status=".Length..].Trim();
            }
            else if (line.StartsWith("summary=", StringComparison.OrdinalIgnoreCase))
            {
                summary = line["summary=".Length..].Trim();
            }
            else if (line.StartsWith("report=", StringComparison.OrdinalIgnoreCase))
            {
                report = line["report=".Length..].Trim();
            }
        }

        if (string.IsNullOrWhiteSpace(summary))
        {
            summary = "No summary was written.";
        }

        return new RegressionResult(status, summary, report);
    }

    private void LaunchActiveScenario()
    {
        if (_activeScenario == null)
        {
            return;
        }

        var retrySuffix = _activeRetryCount > 0 ? $" retry {_activeRetryCount}" : string.Empty;
        AppendAggregate($"START {_scenarioIndex}/{Scenarios.Length}: {_activeScenario.Value.DisplayName}{retrySuffix}");
        SetStatus($"Launching {_scenarioIndex}/{Scenarios.Length}: {_activeScenario.Value.DisplayName}{retrySuffix}");
        _waitingForSceneStart = true;
        EditorInterface.Singleton.PlayCustomScene(RegressionScenePath);
    }

    private static bool IsTransientRegressionResult(RegressionResult result)
    {
        return string.Equals(result.Status, "MISSING_RESULT", StringComparison.OrdinalIgnoreCase)
            || result.Status.StartsWith("PROCESS_EXIT_", StringComparison.OrdinalIgnoreCase);
    }

    private void AppendAggregate(string line)
    {
        var formattedLine = $"{DateTime.Now:HH:mm:ss} {line}";
        _aggregateLines.Add(formattedLine);
        if (!string.IsNullOrWhiteSpace(_aggregateReportPath))
        {
            File.AppendAllLines(_aggregateReportPath, new[] { formattedLine });
        }

        _dockContent?.SetLog(string.Join('\n', _aggregateLines));
    }

    private void SetStatus(string text)
    {
        _dockContent?.SetStatus(text);
        GD.Print($"TLM Regression Tools: {text}");
    }

    private void SetButtonsDisabled(bool disabled)
    {
        if (_toolbarButton != null)
        {
            _toolbarButton.Disabled = disabled;
        }

        _dockContent?.SetStartDisabled(disabled);
    }

    private static void DeleteIfExists(string path)
    {
        if (File.Exists(path))
        {
            File.Delete(path);
        }
    }

    private readonly record struct RegressionScenario(string Id, string DisplayName);

    private readonly record struct RegressionResult(string Status, string Summary, string ReportPath);
}

internal partial class TLMRegressionToolsDock : VBoxContainer
{
    private readonly Label _statusLabel;
    private readonly Label _logLabel;

    public TLMRegressionToolsDock()
    {
        Name = "TLMRegressionToolsDockContent";

        AddChild(new Label { Text = "The Last Mage Tests" });

        _statusLabel = new Label
        {
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            Text = "Ready."
        };
        AddChild(_statusLabel);

        _logLabel = new Label
        {
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            Text = string.Empty
        };
        AddChild(_logLabel);
    }

    public void SetStatus(string status)
    {
        _statusLabel.Text = status;
    }

    public void SetLog(string log)
    {
        _logLabel.Text = log;
    }

    public void SetStartDisabled(bool disabled)
    {
    }
}
#endif
