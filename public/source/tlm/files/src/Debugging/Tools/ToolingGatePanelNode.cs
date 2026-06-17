using Godot;
using TheLastMage.Debugging.Balance;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Debugging.Tools;

public partial class ToolingGatePanelNode : Control
{
    private const int TimelineCapacity = 24;
    private const float PanelMargin = 24f;
    private static readonly Vector2 MinPanelSize = new(900f, 620f);
    private static readonly Vector2 MaxPanelSize = new(1360f, 820f);

    private readonly List<ContentId> _itemIds = new();
    private readonly List<ContentId> _spellIds = new();
    private readonly List<ContentId> _enemyIds = new();
    private readonly List<string> _timeline = new();

    private RunControllerNode? _runController;
    private GameEventBus? _subscribedEvents;
    private OptionButton? _itemOptions;
    private OptionButton? _spellOptions;
    private OptionButton? _enemyOptions;
    private SpinBox? _enemyCount;
    private SpinBox? _seed;
    private RichTextLabel? _itemInspector;
    private RichTextLabel? _enemyInspector;
    private RichTextLabel? _statInspector;
    private RichTextLabel? _lootSimulator;
    private RichTextLabel? _balanceLab;
    private RichTextLabel? _rangeInspector;
    private RichTextLabel? _eventTimeline;
    private Label? _statusLabel;
    private PanelContainer? _rootPanel;
    private CanvasLayer? _debugOverlay;
    private bool _debugOverlayHiddenByTooling;
    private bool _debugOverlayWasVisible;
    private bool _panelOpen;
    private bool _balanceLabRunning;

    [Export] public int DefaultSeed { get; set; } = 12345;

    public override void _Ready()
    {
        MouseFilter = MouseFilterEnum.Pass;
        BuildUi();
        SetPanelOpen(false);
        RefreshContext();
        RefreshAllReports();
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event.IsActionPressed("debug_toggle_tooling_panel"))
        {
            SetPanelOpen(!_panelOpen);
            GetViewport().SetInputAsHandled();
        }
    }

    public override void _Process(double delta)
    {
        PositionPanel();
        RefreshContext();
        RefreshStatsAndRanges();
    }

    public override void _ExitTree()
    {
        RestoreDebugOverlay();
        DetachTimeline();
    }

    private void BuildUi()
    {
        var root = new PanelContainer
        {
            CustomMinimumSize = MinPanelSize,
            MouseFilter = MouseFilterEnum.Stop
        };
        root.AddThemeStyleboxOverride("panel", CreatePanelBox());
        _rootPanel = root;
        AddChild(root);
        PositionPanel();

        var margin = new MarginContainer();
        margin.AddThemeConstantOverride("margin_left", 14);
        margin.AddThemeConstantOverride("margin_top", 12);
        margin.AddThemeConstantOverride("margin_right", 14);
        margin.AddThemeConstantOverride("margin_bottom", 14);
        root.AddChild(margin);

        var main = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        main.AddThemeConstantOverride("separation", 8);
        margin.AddChild(main);

        var header = new HBoxContainer();
        var title = new Label
        {
            Text = "Tooling Dashboard",
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        title.AddThemeFontSizeOverride("font_size", 20);
        header.AddChild(title);
        _statusLabel = new Label
        {
            Text = "F4 close  |  F3 overlay  |  B build  |  F6 meta"
        };
        _statusLabel.AddThemeFontSizeOverride("font_size", 13);
        _statusLabel.AddThemeColorOverride("font_color", new Color(0.68f, 0.72f, 0.7f));
        header.AddChild(_statusLabel);
        main.AddChild(header);

        main.AddChild(BuildActionStrip());

        var tabs = new TabContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        main.AddChild(tabs);

        _itemInspector = BuildReportLabel();
        _enemyInspector = BuildReportLabel();
        _statInspector = BuildReportLabel();
        _lootSimulator = BuildReportLabel();
        _balanceLab = BuildReportLabel();
        _rangeInspector = BuildReportLabel();
        _eventTimeline = BuildReportLabel();

        var playerSplit = new HSplitContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            SplitOffsets = new[] { 620 }
        };
        playerSplit.AddChild(BuildReportSection("Player Stats", _statInspector, 220f));
        playerSplit.AddChild(BuildReportSection("Range / Hitbox Data", _rangeInspector, 220f));
        tabs.AddChild(BuildTabPage("Player", playerSplit));

        var contentSplit = new HSplitContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            SplitOffsets = new[] { 620 }
        };
        contentSplit.AddChild(BuildReportSection("Item Inspector", _itemInspector, 220f));
        contentSplit.AddChild(BuildReportSection("Enemy Inspector", _enemyInspector, 220f));
        tabs.AddChild(BuildTabPage("Content", contentSplit));

        tabs.AddChild(BuildTabPage("Loot", BuildReportSection("Loot Simulation", _lootSimulator, 360f)));
        tabs.AddChild(BuildTabPage("Balance", BuildReportSection("Balance Lab", _balanceLab, 360f)));
        tabs.AddChild(BuildTabPage("Events", BuildReportSection("Event Timeline", _eventTimeline, 360f)));
    }

    private Control BuildActionStrip()
    {
        var strip = new GridContainer
        {
            Columns = 6,
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        strip.AddThemeConstantOverride("h_separation", 8);

        _seed = new SpinBox { MinValue = 1, MaxValue = int.MaxValue, Value = DefaultSeed, Step = 1 };
        var restart = new Button { Text = "Restart Seed" };
        restart.Pressed += RestartWithSeed;
        strip.AddChild(BuildActionGroup("Run", _seed, restart));

        _enemyOptions = new OptionButton();
        _enemyOptions.ItemSelected += _ => RefreshEnemyInspector();
        _enemyCount = new SpinBox { MinValue = 1, MaxValue = 300, Value = 8, Step = 1 };
        var spawn = new Button { Text = "Spawn Enemies" };
        spawn.Pressed += SpawnEnemies;
        strip.AddChild(BuildActionGroup("Enemy", _enemyOptions, _enemyCount, spawn));

        _spellOptions = new OptionButton();
        var select = new Button { Text = "Select Spell" };
        select.Pressed += SelectSpell;
        strip.AddChild(BuildActionGroup("Spell", _spellOptions, select));

        _itemOptions = new OptionButton();
        _itemOptions.ItemSelected += _ => RefreshItemInspector();
        var grant = new Button { Text = "Grant Item" };
        grant.Pressed += GrantItem;
        var runLoot = new Button { Text = "Run 10k Rolls" };
        runLoot.Pressed += RunLootSimulation;
        strip.AddChild(BuildActionGroup("Item / Loot", _itemOptions, grant, runLoot));

        var refreshBalance = new Button { Text = "Refresh Reports" };
        refreshBalance.Pressed += RefreshBalanceLab;
        strip.AddChild(BuildActionGroup("Balance", refreshBalance));

        var forceNight = new Button { Text = "Force Night" };
        forceNight.Pressed += ForceNight;
        var forceVictory = new Button { Text = "Force Victory" };
        forceVictory.Pressed += ForceVictory;
        strip.AddChild(BuildActionGroup("Dev Run", forceNight, forceVictory));

        return strip;
    }

    private static Control BuildActionGroup(string title, params Control[] controls)
    {
        var box = new VBoxContainer
        {
            CustomMinimumSize = new Vector2(0f, 86f),
            SizeFlagsHorizontal = SizeFlags.ExpandFill
        };
        box.AddThemeConstantOverride("separation", 2);
        var label = new Label { Text = title };
        label.AddThemeFontSizeOverride("font_size", 15);
        label.AddThemeColorOverride("font_color", new Color(0.95f, 0.92f, 0.84f));
        box.AddChild(label);
        foreach (var control in controls)
        {
            control.SizeFlagsHorizontal = SizeFlags.ExpandFill;
            box.AddChild(control);
        }

        return box;
    }

    private static Control BuildTabPage(string title, Control content)
    {
        var page = new MarginContainer
        {
            Name = title,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        page.AddThemeConstantOverride("margin_left", 0);
        page.AddThemeConstantOverride("margin_top", 8);
        page.AddThemeConstantOverride("margin_right", 0);
        page.AddThemeConstantOverride("margin_bottom", 0);
        content.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        content.SizeFlagsVertical = SizeFlags.ExpandFill;
        page.AddChild(content);
        return page;
    }

    private static Control BuildReportSection(string title, RichTextLabel report, float minHeight)
    {
        var panel = new PanelContainer
        {
            CustomMinimumSize = new Vector2(0f, minHeight),
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };

        var margin = new MarginContainer();
        margin.AddThemeConstantOverride("margin_left", 12);
        margin.AddThemeConstantOverride("margin_top", 10);
        margin.AddThemeConstantOverride("margin_right", 12);
        margin.AddThemeConstantOverride("margin_bottom", 10);
        panel.AddChild(margin);

        var box = new VBoxContainer
        {
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        box.AddThemeConstantOverride("separation", 8);
        margin.AddChild(box);

        var titleLabel = new Label { Text = title };
        titleLabel.AddThemeFontSizeOverride("font_size", 18);
        titleLabel.AddThemeColorOverride("font_color", new Color(1f, 0.94f, 0.78f));
        box.AddChild(titleLabel);

        report.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        report.SizeFlagsVertical = SizeFlags.ExpandFill;
        box.AddChild(report);

        return panel;
    }

    private static StyleBoxFlat CreatePanelBox()
    {
        return new StyleBoxFlat
        {
            BgColor = new Color(0.055f, 0.06f, 0.064f, 0.97f),
            BorderColor = new Color(0.22f, 0.27f, 0.29f, 1f),
            BorderWidthLeft = 1,
            BorderWidthTop = 1,
            BorderWidthRight = 1,
            BorderWidthBottom = 1,
            CornerRadiusTopLeft = 6,
            CornerRadiusTopRight = 6,
            CornerRadiusBottomLeft = 6,
            CornerRadiusBottomRight = 6
        };
    }

    private void PositionPanel()
    {
        if (_rootPanel == null)
        {
            return;
        }

        var viewportSize = GetViewportRect().Size;
        var size = new Vector2(
            MathF.Min(MaxPanelSize.X, MathF.Max(MinPanelSize.X, viewportSize.X - PanelMargin * 2f)),
            MathF.Min(MaxPanelSize.Y, MathF.Max(MinPanelSize.Y, viewportSize.Y - PanelMargin * 2f)));
        _rootPanel.Size = size;
        _rootPanel.Position = (viewportSize - size) * 0.5f;
    }

    private static RichTextLabel BuildReportLabel()
    {
        var label = new RichTextLabel
        {
            BbcodeEnabled = true,
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            ContextMenuEnabled = true,
            FitContent = false,
            ScrollActive = true,
            SelectionEnabled = true,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill
        };
        label.AddThemeFontSizeOverride("font_size", 15);
        return label;
    }

    private void RefreshContext()
    {
        _runController ??= GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        if (_panelOpen)
        {
            context.State.IsDebugUiInputCaptured = true;
        }

        if (!ReferenceEquals(_subscribedEvents, context.Events))
        {
            DetachTimeline();
            _subscribedEvents = context.Events;
            _subscribedEvents.EventPublished += OnEventPublished;
            RefreshOptions(context);
        }
    }

    private void DetachTimeline()
    {
        if (_subscribedEvents != null)
        {
            _subscribedEvents.EventPublished -= OnEventPublished;
            _subscribedEvents = null;
        }
    }

    private void RefreshOptions(RunContext context)
    {
        FillItemOptions(
            _itemOptions,
            _itemIds,
            OrderItemsForManualTesting(context.Content.Items.Values));
        FillOptions(_spellOptions, _spellIds, context.Content.Spells.Keys.OrderBy(id => id.ToString()));
        FillOptions(_enemyOptions, _enemyIds, context.Content.Enemies.Keys.OrderBy(id => id.ToString()));
        RefreshItemInspector();
        RefreshEnemyInspector();
    }

    public static IEnumerable<ItemRuntimeDefinition> OrderItemsForManualTesting(IEnumerable<ItemRuntimeDefinition> items)
    {
        return items
            .OrderBy(item => item.ItemNumber > 0 ? item.ItemNumber : int.MaxValue)
            .ThenBy(item => item.DisplayName, StringComparer.OrdinalIgnoreCase)
            .ThenBy(item => item.Id.ToString(), StringComparer.Ordinal);
    }

    private static void FillItemOptions(OptionButton? options, List<ContentId> ids, IEnumerable<ItemRuntimeDefinition> source)
    {
        if (options == null)
        {
            return;
        }

        ids.Clear();
        options.Clear();
        foreach (var item in source)
        {
            ids.Add(item.Id);
            var label = string.IsNullOrWhiteSpace(item.DisplayName) ? item.Id.ToString() : item.DisplayName;
            if (item.ItemNumber > 0)
            {
                label = $"{label} #{item.ItemNumber:000}";
            }

            options.AddItem(label);
            options.SetItemTooltip(options.ItemCount - 1, item.Id.ToString());
        }
    }

    private static void FillOptions(OptionButton? options, List<ContentId> ids, IEnumerable<ContentId> source)
    {
        if (options == null)
        {
            return;
        }

        ids.Clear();
        options.Clear();
        foreach (var id in source)
        {
            ids.Add(id);
            options.AddItem(id.ToString());
        }
    }

    private void RefreshAllReports()
    {
        RefreshItemInspector();
        RefreshEnemyInspector();
        RefreshStatsAndRanges();
        RunLootSimulation();
        RefreshBalanceLab();
        RefreshTimelineLabel();
    }

    private void RefreshStatsAndRanges()
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        if (_statInspector != null)
        {
            _statInspector.Text = ToolingGateReport.BuildStatInspection(context);
        }

        if (_rangeInspector != null)
        {
            _rangeInspector.Text = ToolingGateReport.BuildRangeInspection(context);
        }
    }

    private void RefreshItemInspector()
    {
        var context = _runController?.Context;
        if (context == null || _itemInspector == null || _itemOptions == null)
        {
            return;
        }

        var index = _itemOptions.Selected;
        if (index < 0 || index >= _itemIds.Count)
        {
            _itemInspector.Text = "No item selected.";
            return;
        }

        var itemId = _itemIds[index];
        _itemInspector.Text = context.Content.Items.TryGetValue(itemId, out var item)
            ? ToolingGateReport.BuildItemInspection(item)
            : $"Missing item {itemId}.";
    }

    private void RefreshEnemyInspector()
    {
        var context = _runController?.Context;
        if (context == null || _enemyInspector == null || _enemyOptions == null)
        {
            return;
        }

        var index = _enemyOptions.Selected;
        if (index < 0 || index >= _enemyIds.Count)
        {
            _enemyInspector.Text = "No enemy selected.";
            return;
        }

        var enemyId = _enemyIds[index];
        _enemyInspector.Text = context.Content.Enemies.TryGetValue(enemyId, out var enemy)
            ? ToolingGateReport.BuildEnemyInspection(enemy, ToolingGateReport.ValidateCatalogResource())
            : $"Missing enemy {enemyId}.";
    }

    private void RunLootSimulation()
    {
        var context = _runController?.Context;
        if (context == null || _lootSimulator == null)
        {
            return;
        }

        var seed = _seed == null ? DefaultSeed : (int)_seed.Value;
        _lootSimulator.Text = ToolingGateReport.BuildLootSimulationText(
            context.Content,
            context.Profile,
            ToolingGateReport.DefaultSimulationRolls,
            seed);
    }

    private void RefreshBalanceLab()
    {
        if (_balanceLab == null)
        {
            return;
        }

        var context = _runController?.Context;
        _balanceLab.Text = context == null
            ? "No active RunContext."
            : $"Balance lab reports are written by the editor headless dock.\nProduction items: {context.Content.Items.Values.Count(item => !item.Id.ToString().StartsWith("regression_item_", StringComparison.Ordinal))}\nReports: {Godot.ProjectSettings.GlobalizePath("res://Saved/balance_lab")}\nLatest report: {context.State.DebugMetrics.BalanceReportPath}";
    }

    private async void RunBalanceSmoke()
    {
        await Task.CompletedTask;
        if (_balanceLab != null)
        {
            _balanceLab.Text = "Deep Balance Lab runs from the TLM Balance editor dock as a separate headless process.";
        }
    }

    private async Task ShowBalanceProgress(BalanceExperimentDefinition experiment, BalanceLabProgress progress)
    {
        if (_balanceLab != null)
        {
            var percent = progress.TotalRuns <= 0 ? 0f : progress.CompletedRuns * 100f / progress.TotalRuns;
            _balanceLab.Text =
                $"Balance lab running\n" +
                $"Phase: {progress.Phase}\n" +
                $"Runs: {progress.CompletedRuns}/{progress.TotalRuns} ({percent:0.0}%)\n" +
                $"Current: seed={progress.Seed} mage={progress.MageId} policy={progress.BotPolicyId} focus={progress.FocusSpellId} day={progress.CurrentDay}\n" +
                $"Outcomes: defeat={progress.Defeats} victory={progress.Victories} timeout={progress.Timeouts}\n" +
                $"Averages: day={progress.AverageDayReached:0.00} damage={progress.AverageDamageOutput:0.0} taken={progress.AverageDamageTaken:0.0}\n" +
                $"Sim seconds: current={progress.CurrentRunSeconds:0.0} total={progress.TotalSimulatedSeconds:0.0}\n" +
                $"Last: {progress.LastOutcome}\n" +
                $"Live marker: {BalanceReportWriter.WriteProgressMarker(experiment, progress)}";
        }

        await ToSignal(GetTree().CreateTimer(0.01f), SceneTreeTimer.SignalName.Timeout);
    }

    private void RestartWithSeed()
    {
        if (_runController == null)
        {
            return;
        }

        var seed = _seed == null ? DefaultSeed : (int)_seed.Value;
        _runController.StartNewRun(_runController.InitialPhase, seed);
        if (_runController.Context != null)
        {
            _runController.Context.State.MarkDebugRun("tooling restart");
            _runController.Context.State.IsDebugUiInputCaptured = _panelOpen;
        }

        _timeline.Clear();
        RefreshContext();
        RefreshAllReports();
    }

    private void SpawnEnemies()
    {
        var context = _runController?.Context;
        if (context == null || _enemyOptions == null || _enemyOptions.Selected < 0 || _enemyOptions.Selected >= _enemyIds.Count)
        {
            return;
        }

        var enemySystem = context.GetSystem<EnemySystem>();
        context.State.MarkDebugRun("tooling enemy spawn");
        var enemyId = _enemyIds[_enemyOptions.Selected];
        var count = _enemyCount == null ? 1 : (int)_enemyCount.Value;
        var center = context.State.Player.Position;
        for (var i = 0; i < count; i++)
        {
            var angle = i * Mathf.Tau / Math.Max(1, count);
            var ring = 6f + i % 5;
            enemySystem.Spawn(enemyId, center + new Vector3(MathF.Sin(angle) * ring, 0f, -MathF.Cos(angle) * ring));
        }

        RefreshAllReports();
    }

    private void GrantItem()
    {
        var context = _runController?.Context;
        if (context == null || _itemOptions == null || _itemOptions.Selected < 0 || _itemOptions.Selected >= _itemIds.Count)
        {
            return;
        }

        var itemId = _itemIds[_itemOptions.Selected];
        context.State.MarkDebugRun("tooling item grant");
        if (!context.Content.Items.TryGetValue(itemId, out var item))
        {
            return;
        }

        if (context.State.Build.HasAcquiredItem(itemId))
        {
            context.State.DebugMetrics.LastActivatableItemSummary = $"debug grant skipped duplicate {itemId}";
            RefreshAllReports();
            return;
        }

        context.GetSystem<MarketSystem>().AcquireItem(item);
        context.ItemKnowledge.MarkDiscovered(itemId);
        context.Events.Publish(new ItemAcquiredEvent(itemId));
        RefreshAllReports();
    }

    private void SelectSpell()
    {
        var context = _runController?.Context;
        if (context == null || _spellOptions == null || _spellOptions.Selected < 0 || _spellOptions.Selected >= _spellIds.Count)
        {
            return;
        }

        context.State.Spellbook.AssignSlot(0, _spellIds[_spellOptions.Selected]);
        context.State.MarkDebugRun("tooling spell selection");
        context.State.Spellbook.SelectSlot(0);
        RefreshAllReports();
    }

    private void ForceNight()
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        context.GetSystem<RunPhaseStateMachine>().CompleteCurrentDayForDebug();
        RefreshAllReports();
    }

    private void ForceVictory()
    {
        var context = _runController?.Context;
        if (context == null)
        {
            return;
        }

        context.GetSystem<RunPhaseStateMachine>().ForceVictoryForDebug();
        RefreshAllReports();
    }

    private void OnEventPublished(Type eventType)
    {
        _timeline.Add($"{Time.GetTicksMsec() / 1000.0:0000.000} {eventType.Name}");
        if (_timeline.Count > TimelineCapacity)
        {
            _timeline.RemoveAt(0);
        }

        RefreshTimelineLabel();
    }

    private void RefreshTimelineLabel()
    {
        if (_eventTimeline != null)
        {
            _eventTimeline.Text = _timeline.Count == 0
                ? "No events observed yet."
                : string.Join('\n', _timeline);
        }
    }

    private void SetPanelOpen(bool open)
    {
        _panelOpen = open;
        if (_rootPanel != null)
        {
            _rootPanel.Visible = open;
        }

        SetDebugOverlayHidden(open);

        MouseFilter = open ? MouseFilterEnum.Stop : MouseFilterEnum.Ignore;
        var context = _runController?.Context;
        if (context != null)
        {
            context.State.IsDebugUiInputCaptured = open;
        }

        Input.MouseMode = open
            ? Input.MouseModeEnum.Visible
            : context != null
                ? Input.MouseModeEnum.Captured
                : Input.MouseModeEnum.Visible;
    }

    private void SetDebugOverlayHidden(bool hidden)
    {
        _debugOverlay ??= GetTree().Root.FindChild("DebugOverlay", true, false) as CanvasLayer;
        if (_debugOverlay == null)
        {
            return;
        }

        if (hidden)
        {
            if (_debugOverlayHiddenByTooling)
            {
                return;
            }

            _debugOverlayWasVisible = _debugOverlay.Visible;
            _debugOverlay.Visible = false;
            _debugOverlayHiddenByTooling = true;
            return;
        }

        RestoreDebugOverlay();
    }

    private void RestoreDebugOverlay()
    {
        if (_debugOverlay == null || !_debugOverlayHiddenByTooling)
        {
            return;
        }

        _debugOverlay.Visible = _debugOverlayWasVisible;
        _debugOverlayHiddenByTooling = false;
    }
}
