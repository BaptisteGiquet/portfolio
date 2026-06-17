using Godot;
using TheLastMage.Debugging.Balance;
using TheLastMage.Debugging.Reports;
using TheLastMage.Controls;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Data.Tags;
using TheLastMage.Debugging.Tools;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Achievements;
using TheLastMage.Gameplay.AoE;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Commands;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Projectiles;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.Stats;
using TheLastMage.Gameplay.StatusEffects;
using TheLastMage.Save;
using TheLastMage.Gameplay.Summons;
using TheLastMage.Gameplay.Waves;
using TheLastMage.Localization;
using TheLastMage.UI;

namespace TheLastMage.Debugging.Regression;

public partial class RegressionRunnerNode : Node3D
{
    private const string ScenarioSevenSpellCombat = "seven_spell_combat";
    private const string ScenarioPlayerDamage = "player_damage";
    private const string ScenarioFireRate = "fire_rate";
    private const string ScenarioDuration = "duration";
    private const string ScenarioRange = "range";
    private const string ScenarioNightMarket = "night_market";
    private const string ScenarioToolingGate = "tooling_gate";
    private const string ScenarioSprint08Defense = "sprint_08_defense";
    private const string ScenarioSprint09Enemies = "sprint_09_enemies";
    private const string ScenarioSprint10Meta = "sprint_10_meta";
    private const string ScenarioSprint11Items = "sprint_11_items";
    private const string ScenarioSprint115ItemAuthoring = "sprint_11_5_item_authoring";
    private const string ScenarioSprint12Ux = "sprint_12_ux";
    private const string ScenarioSprint13Reporting = "sprint_13_reporting";
    private const string ScenarioSprint13AActivatableItems = "sprint_13a_activatable_items";
    private const string ScenarioSprint13BGameplayTags = "sprint_13b_gameplay_tags";
    private const string ScenarioSprint13CBalanceLab = "sprint_13c_balance_lab";
    private const string ScenarioSprint15FullRunSkeleton = "sprint_15_full_run_skeleton";
    private const string ScenarioSprint16LocalizationSpine = "sprint_16_localization_spine";
    private const string ScenarioSprint17ControllerInput = "sprint_17_controller_input";
    private const string ScenarioSprint18FrontEndSaveStats = "sprint_18_frontend_save_stats";
    private const string ScenarioSprint19OptionsPause = "sprint_19_options_pause";
    private const string ScenarioItemCastFlowBehavior = "item_cast_flow_behavior";
    private const string ScenarioProductionItemBehavior = "production_item_behavior";
    private const string RegressionDamageItem = "regression_item_damage";
    private const string RegressionRangeItem = "regression_item_range";
    private const string RegressionProjectileItem = "regression_item_projectile";
    private const string RegressionStatusItem = "regression_item_status";
    private const string RegressionFireStatusItem = "regression_item_fire_status";
    private const string RegressionBurningGroundItem = "regression_item_burning_ground";
    private const string RegressionKillItem = "regression_item_kill";
    private const string RegressionLockedItem = "regression_item_locked";
    private const string RegressionActivatableLimitedItem = "regression_item_active_limited";
    private const string RegressionActivatableUnlimitedItem = "regression_item_active_unlimited";
    private const string RegressionActivatableReplacementItem = "regression_item_active_replacement";
    private const string RegressionKeepItInItem = "regression_item_keep_it_in";
    private const string RegressionAbyssalRingItem = "regression_item_abyssal_ring";
    private const string RegressionFaultyFocusItem = "regression_item_faulty_focus";
    private const string RegressionUnlockAchievement = "regression_unlock_item";

    private readonly RegressionScenario[] _scenarios;
    private readonly List<string> _lines = new();
    private readonly List<MeshInstance3D> _markers = new();
    private readonly List<string> _reportLines = new();

    private EventLog? _events;
    private RunControllerNode? _runController;
    private Label? _label;
    private RunContext? _context;
    private string _currentScenario = "Bootstrap";
    private string _reportPath = string.Empty;
    private string _resultPath = string.Empty;
    private bool _includeEnvironmentDiagnostics;
    private Vector3 _scenarioOrigin = Vector3.Zero;
    private int _passed;
    private int _failed;
    private int _scenarioIndex;

    [Export] public bool AutoStart { get; set; }

    [Export] public bool QuitWhenDone { get; set; }

    public RegressionRunnerNode()
    {
        _scenarios = new[]
        {
            new RegressionScenario(ScenarioSevenSpellCombat, "Seven Spell Combat", SevenSpellCombatScenario),
            new RegressionScenario(ScenarioPlayerDamage, "Player Damage Contract", PlayerDamageScenario),
            new RegressionScenario(ScenarioFireRate, "Fire Rate Contract", FireRateScenario),
            new RegressionScenario(ScenarioDuration, "Duration Contract", DurationScenario),
            new RegressionScenario(ScenarioRange, "Range Contract", RangeScenario),
            new RegressionScenario(ScenarioNightMarket, "Night Market Command Flow", NightMarketScenario),
            new RegressionScenario(ScenarioToolingGate, "Gate G Tooling", ToolingGateScenario),
            new RegressionScenario(ScenarioSprint08Defense, "Sprint 08 Wave Tower Defense", Sprint08DefenseScenario),
            new RegressionScenario(ScenarioSprint09Enemies, "Sprint 09 Enemy Variety", Sprint09EnemiesScenario),
            new RegressionScenario(ScenarioSprint10Meta, "Sprint 10 Mage Meta Progression", Sprint10MetaScenario),
            new RegressionScenario(ScenarioSprint11Items, "Sprint 11 Vertical Slice Items", Sprint11ItemsScenario),
            new RegressionScenario(ScenarioSprint115ItemAuthoring, "Sprint 11.5 Item Authoring Pipeline", Sprint115ItemAuthoringScenario),
            new RegressionScenario(ScenarioSprint12Ux, "Sprint 12 UX Feedback Readability", Sprint12UxScenario),
            new RegressionScenario(ScenarioSprint13Reporting, "Sprint 13 Reporting Kickoff", Sprint13ReportingScenario),
            new RegressionScenario(ScenarioSprint13AActivatableItems, "Sprint 13A Activatable Items", Sprint13AActivatableItemsScenario),
            new RegressionScenario(ScenarioSprint13BGameplayTags, "Sprint 13B Gameplay Tags", Sprint13BGameplayTagsScenario),
            new RegressionScenario(ScenarioSprint13CBalanceLab, "Sprint 13C Balance Simulation Lab", Sprint13CBalanceLabScenario),
            new RegressionScenario(ScenarioSprint15FullRunSkeleton, "Sprint 15 Full Run Skeleton", Sprint15FullRunSkeletonScenario),
            new RegressionScenario(ScenarioSprint16LocalizationSpine, "Sprint 16 Localization Spine", Sprint16LocalizationSpineScenario),
            new RegressionScenario(ScenarioSprint17ControllerInput, "Sprint 17 Controller Input", Sprint17ControllerInputScenario),
            new RegressionScenario(ScenarioSprint18FrontEndSaveStats, "Sprint 18 Front-End Save Stats", Sprint18FrontEndSaveStatsScenario),
            new RegressionScenario(ScenarioSprint19OptionsPause, "Sprint 19 Options Pause", Sprint19OptionsPauseScenario),
            new RegressionScenario(ScenarioItemCastFlowBehavior, "Item Cast Flow Behavior", ItemCastFlowBehaviorScenario),
            new RegressionScenario(ScenarioProductionItemBehavior, "Production Item Behavior", ProductionItemBehaviorScenario)
        };
    }

    public override async void _Ready()
    {
        BuildUi();
        FrameRegressionCamera();
        SetStatus("Waiting for RunContext...");
        await WaitSeconds(0.1f);

        _runController = GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        _context = _runController?.Context;
        if (_context == null)
        {
            Fail("Bootstrap", "RunContext was not created.");
            return;
        }

        _events = new EventLog(Report);
        var commandLine = RegressionCommandLine.Parse();
        _includeEnvironmentDiagnostics = commandLine.IncludeEnvironmentDiagnostics;
        if (!string.IsNullOrWhiteSpace(commandLine.ReportPath))
        {
            _reportPath = commandLine.ReportPath;
        }

        if (!string.IsNullOrWhiteSpace(commandLine.ResultPath))
        {
            _resultPath = commandLine.ResultPath;
        }

        SetStatus("Ready. Use the editor automated test button to start scenario play sessions.");

        if (commandLine.RunAllScenarios)
        {
            await RunAllScenarios(commandLine.QuitWhenDone);
            return;
        }

        if (!string.IsNullOrWhiteSpace(commandLine.ScenarioId))
        {
            await RunSingleScenario(commandLine.ScenarioId, quitWhenDone: commandLine.QuitWhenDone);
            return;
        }

        if (AutoStart)
        {
            await RunAllScenarios();
        }
    }

    public override void _ExitTree()
    {
        if (_context != null && _events != null)
        {
            _events.Detach(_context.Events);
        }
    }

    private async Task RunAllScenarios(bool quitWhenDone = false)
    {
        ResetRunState();
        InitializeReport();
        if (_includeEnvironmentDiagnostics)
        {
            ReportEnvironmentDiagnostics("all");
        }

        Report("Regression run started.");
        foreach (var scenario in _scenarios)
        {
            await RunScenario(scenario.DisplayName, scenario.Run);
        }

        var summary = $"Regression complete: {_passed} passed, {_failed} failed.";
        AddLine(_failed == 0 ? $"PASS {summary}" : $"FAIL {summary}");
        Report(summary);
        WriteResult(summary);
        FlushReport();
        GD.Print(summary);

        if (QuitWhenDone || quitWhenDone)
        {
            await WaitSeconds(0.1f);
            GetTree().Quit(_failed == 0 ? 0 : 1);
        }
    }

    private async Task RunSingleScenario(string scenarioId, bool quitWhenDone)
    {
        ResetRunState();
        InitializeReport();
        if (_includeEnvironmentDiagnostics)
        {
            ReportEnvironmentDiagnostics(scenarioId);
        }

        var scenario = FindScenario(scenarioId);
        if (scenario == null)
        {
            _failed++;
            var message = $"Unknown regression scenario '{scenarioId}'.";
            AddLine($"FAIL {message}");
            Report(message);
            WriteResult(message);
            FlushReport();
            if (quitWhenDone)
            {
                GetTree().Quit(1);
            }

            return;
        }

        var selectedScenario = scenario.Value;
        Report($"Regression single scenario started: {selectedScenario.Id}.");
        await RunScenario(selectedScenario.DisplayName, selectedScenario.Run);
        var summary = $"Regression scenario {selectedScenario.Id} complete: {_passed} passed, {_failed} failed.";
        AddLine(_failed == 0 ? $"PASS {summary}" : $"FAIL {summary}");
        Report(summary);
        WriteResult(summary);
        FlushReport();

        if (quitWhenDone)
        {
            await WaitSeconds(0.1f);
            GetTree().Quit(_failed == 0 ? 0 : 1);
        }
    }

    private async Task RunScenario(string name, Func<Task> scenario)
    {
        _currentScenario = name;
        _scenarioOrigin = new Vector3(_scenarioIndex * 18f, 0f, 0f);
        _scenarioIndex++;
        SetStatus($"Running: {name}");
        var context = StartFreshScenarioRun();
        context.State.Player.Position = _scenarioOrigin;
        Events.Clear();
        Report($"SCENARIO START {name} run={context.State.RunId} origin={Format(_scenarioOrigin)}");
        ReportPlayerAttributes();
        try
        {
            await scenario();
            _passed++;
            AddLine($"PASS {name}");
            Report($"SCENARIO PASS {name}");
        }
        catch (Exception exception)
        {
            _failed++;
            AddLine($"FAIL {name}: {exception.Message}");
            Report($"SCENARIO FAIL {name}: {exception}");
            GD.PushError($"Regression scenario '{name}' failed: {exception}");
        }
        finally
        {
            FlushReport();
        }

        await WaitSeconds(0.35f);
    }

    private async Task SevenSpellCombatScenario()
    {
        var context = RequireContext();
        var combat = context.GetSystem<CombatSystem>();
        var enemies = context.GetSystem<EnemySystem>();

        await CastAtEnemy(ContentId.From("fireball"), new Vector3(0f, 0f, -8f), 0.75f);
        Assert(Events.DamageBySource(ContentId.From("fireball")) > 0f, "Fireball did not damage an enemy.");

        await CastAtEnemy(ContentId.From("frost_bolt"), new Vector3(1f, 0f, -8f), 0.75f);
        Assert(Events.DamageBySource(ContentId.From("frost_bolt")) > 0f, "Frost Bolt did not damage an enemy.");
        Assert(Events.StatusCount("slow") > 0, "Frost Bolt did not apply slow.");

        await CastAtEnemy(ContentId.From("arcane_beam"), new Vector3(0f, 0f, -10f), 0.15f);
        Assert(Events.BeamCount(ContentId.From("arcane_beam")) > 0, "Arcane Beam did not fire.");
        Assert(Events.DamageBySource(ContentId.From("arcane_beam")) > 0f, "Arcane Beam did not damage an enemy.");

        await CastAtEnemy(ContentId.From("firewall"), new Vector3(0f, 0f, -7f), 0.8f);
        Assert(Events.AoECount(ContentId.From("firewall")) > 0, "Firewall did not spawn an AoE.");
        Assert(Events.DamageBySource(ContentId.From("firewall")) > 0f, "Firewall did not tick damage.");
        Assert(Events.StatusCountFromSource("burn", ContentId.From("firewall")) == 0, "Firewall applied burn without an immolate item.");

        await CastAtEnemy(ContentId.From("blizzard"), new Vector3(0f, 0f, -10f), 0.9f);
        Assert(Events.AoECount(ContentId.From("blizzard")) > 0, "Blizzard did not spawn an AoE.");
        Assert(Events.DamageBySource(ContentId.From("blizzard")) > 0f, "Blizzard did not tick damage.");
        Assert(Events.StatusCount("slow") > 1, "Blizzard did not apply slow.");

        await CastAtEnemy(ContentId.From("tornado"), new Vector3(0f, 0f, -3f), 0.8f);
        Assert(Events.AoECount(ContentId.From("tornado")) > 0, "Tornado did not spawn an AoE.");
        Assert(Events.DamageBySource(ContentId.From("tornado")) > 0f, "Tornado did not tick damage.");

        var corpseEnemy = enemies.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(1.5f, 0f, -4f)));
        combat.ApplyDamage(new DamageRequest(
            context.State.Player.EntityId,
            corpseEnemy,
            ContentId.From("regression_setup"),
            999f,
            DamageType.Physical,
            new HitContext(_scenarioOrigin, Vector3.Up, context.State.Player.EntityId)));
        await WaitSeconds(0.1f);
        await CastSpell(ContentId.From("raise_dead"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.25f);
        Assert(Events.SummonCount(ContentId.From("raise_dead")) > 0, "Raise Dead did not spawn a summon from a corpse.");
    }

    private async Task PlayerDamageScenario()
    {
        var context = RequireContext();
        var baseline = await CastAtEnemy(ContentId.From("fireball"), new Vector3(-2f, 0f, -8f), 0.75f);
        AddItem(ContentId.From(RegressionDamageItem));
        var boosted = await CastAtEnemy(ContentId.From("fireball"), new Vector3(-2f, 0f, -8f), 0.75f);
        Assert(boosted > baseline + 3.5f, $"Expected editor-authored +damage to increase Fireball from {baseline:0.##}, got {boosted:0.##}.");

        Events.Clear();
        await CastAtEnemy(ContentId.From("firewall"), new Vector3(-3f, 0f, -7f), 0.8f);
        Assert(Events.DamageBySource(ContentId.From("firewall")) > 0f, $"Firewall damage did not use global player damage. Observed {Events.DamageBySource(ContentId.From("firewall")):0.##}.");

        await AssertSummonDamageUsesPlayerDamage();
        await AssertDefenseDamageUsesPlayerDamage();
        await AssertEnemyDamageDoesNotUsePlayerDamage();
    }

    private async Task FireRateScenario()
    {
        var baseline = CastAndGetCooldown(ContentId.From("fireball"));
        Assert(baseline > 0f, $"Fireball cooldown was not established: baseline={baseline:0.###}.");

        await CastSpell(ContentId.From("firewall"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.1f);
        var firewall = LastArea(ContentId.From("firewall"));
        Assert(Mathf.IsEqualApprox(firewall.TickIntervalSeconds, 0.45f), "Fire rate changed Firewall tick interval.");
    }

    private async Task DurationScenario()
    {
        await CastSpell(ContentId.From("firewall"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.1f);
        var baselineFirewallLifetime = LastArea(ContentId.From("firewall")).LifetimeRemaining;

        Assert(baselineFirewallLifetime > 0f, "Firewall lifetime was not established.");

        await CastSpell(ContentId.From("fireball"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.05f);
        var projectileLifetime = LastProjectile(ContentId.From("fireball")).LifetimeRemaining;
        Assert(projectileLifetime < 2.7f, "Duration changed Fireball projectile travel lifetime.");

        var summon = await SpawnSummonFromCorpse();
        Assert(summon.LifetimeRemaining > 0f, "Raise Dead summon lifetime was not established.");

        await CastAtEnemy(ContentId.From("frost_bolt"), new Vector3(3f, 0f, -8f), 0.75f);
        var slowDuration = Events.LastStatusDuration("slow");
        Assert(slowDuration > 0f, $"Slow status duration was not established. Observed event duration {slowDuration:0.##}.");
    }

    private async Task RangeScenario()
    {
        Events.Clear();
        await CastSpell(ContentId.From("arcane_beam"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.1f);
        var baselineBeam = Events.LastBeamDistance(ContentId.From("arcane_beam"));

        Assert(baselineBeam > 0f, "Arcane Beam range was not established.");

        await CastSpell(ContentId.From("fireball"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.05f);
        var projectileLifetime = LastProjectile(ContentId.From("fireball")).LifetimeRemaining;
        Assert(projectileLifetime > 0f, "Fireball travel lifetime was not established.");

        AddItem(ContentId.From(RegressionRangeItem));
        await CastSpell(ContentId.From("blizzard"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.1f);
        var blizzard = LastArea(ContentId.From("blizzard"));
        Assert(blizzard.Position.DistanceTo(_scenarioOrigin) > 12f, "Range did not increase AoE placement distance.");
        Assert(blizzard.Radius < 7.2f, "Range incorrectly increased AoE radius.");

        var summon = await SpawnSummonFromCorpse();
        Assert(summon.AggroRange > 31f, "Range did not increase summon aggro/search distance.");
    }

    private async Task NightMarketScenario()
    {
        var context = RequireContext();
        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightMarket);
        Report($"NIGHT MARKET phase={context.State.CurrentPhase} offers={context.State.Market.Offers.Count}");

        Assert(context.State.Market.Offers.Count > 0, "Night market did not generate offers.");
        var offeredItemId = context.State.Market.Offers[0].ItemId;
        var chosen = context.Commands.TryExecute(new ChooseMarketOfferCommand(0), context, out var reason);
        Report($"NIGHT MARKET choose item={offeredItemId} chosen={chosen} reason={reason} phase={context.State.CurrentPhase}");
        Assert(chosen, $"Could not choose market offer: {reason}");
        Assert(context.State.Build.HasAcquiredItem(offeredItemId), "Chosen offer was not recorded in run-level acquired item tracking.");
        Assert(
            context.State.Build.HasItem(offeredItemId)
            || context.State.Build.ActivatableItem?.ItemId.Equals(offeredItemId) == true,
            "Chosen offer was neither added as a passive stack nor equipped as an activatable item.");
        Assert(context.State.CurrentPhase == RunPhase.NightDefense, "Choosing an item did not advance to NightDefense.");
    }

    private async Task ToolingGateScenario()
    {
        var context = RequireContext();
        var catalog = ResourceLoader.Load<ContentCatalog>("res://data/catalogs/content_catalog.tres")
            ?? throw new InvalidOperationException("Known-good content catalog could not be loaded.");

        var goodValidation = ToolingGateReport.ValidateKnownGoodCatalog(catalog);
        Report("TOOLING VALIDATION GOOD");
        Report(ToolingGateReport.BuildValidationText(goodValidation));
        Assert(!goodValidation.HasErrors, "Known-good catalog has validator errors.");

        var brokenValidation = ToolingGateReport.ValidateBrokenDebugCatalog();
        Report("TOOLING VALIDATION BROKEN");
        Report(ToolingGateReport.BuildValidationText(brokenValidation));
        Assert(brokenValidation.HasErrors, "Broken debug catalog did not produce validator errors.");
        Assert(
            brokenValidation.Issues.Any(issue => issue.Code is "item.invalid_effect_stat" or "item.invalid_effect_chance" or "item.recursive_proc_risk"),
            "Broken item data did not exercise Gate G item validation.");
        Assert(
            brokenValidation.Issues.Any(issue => issue.Code == "spell.invalid_duration_scaling"),
            "Broken spell duration scaling was not rejected.");

        var lootSummary = new LootSimulator().Run(
            context.Content,
            context.Profile,
            ToolingGateReport.DefaultSimulationRolls,
            606);
        Report("TOOLING LOOT SIMULATION");
        Report(ToolingGateReport.BuildLootSimulationText(context.Content, context.Profile, ToolingGateReport.DefaultSimulationRolls, 606));
        Assert(lootSummary.Rolls == ToolingGateReport.DefaultSimulationRolls, "Loot simulator did not run the requested roll count.");
        Assert(lootSummary.OffersGenerated > 0, "Loot simulator generated no offers.");
        Assert(lootSummary.LockedItemsExcluded > 0, "Loot simulator did not report locked item state.");

        var orderedItems = ToolingGatePanelNode.OrderItemsForManualTesting(context.Content.Items.Values).ToArray();
        var numberedItems = orderedItems.Where(item => item.ItemNumber > 0).ToArray();
        Assert(
            numberedItems.SequenceEqual(numberedItems.OrderBy(item => item.ItemNumber)),
            "Tooling item dropdown is not sorted by item number.");

        var enemyCountBefore = context.GetSystem<EnemySystem>().ActiveCount;
        context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(6f, 0f, -6f)));
        Assert(context.GetSystem<EnemySystem>().ActiveCount == enemyCountBefore + 1, "Combat sandbox enemy spawn control path failed.");

        context.State.Build.AddItem(ContentId.From(RegressionDamageItem));
        context.ItemKnowledge.MarkDiscovered(ContentId.From(RegressionDamageItem));
        Assert(context.State.Build.HasItem(ContentId.From(RegressionDamageItem)), "Combat sandbox item grant path failed.");

        var statText = ToolingGateReport.BuildStatInspection(context);
        var rangeText = ToolingGateReport.BuildRangeInspection(context);
        Report(statText);
        Report(rangeText);
        Assert(statText.Contains("Canonical Player Attributes", StringComparison.Ordinal), "Stat inspector did not report canonical attributes.");
        Assert(statText.Contains("Broader Modifier Stats", StringComparison.Ordinal), "Stat inspector did not separate broader modifier stats.");
        Assert(rangeText.Contains("projectile", StringComparison.OrdinalIgnoreCase)
            && rangeText.Contains("beam", StringComparison.OrdinalIgnoreCase)
            && rangeText.Contains("summon", StringComparison.OrdinalIgnoreCase)
            && rangeText.Contains("duration_scaling", StringComparison.OrdinalIgnoreCase),
            "Range visualizer report is missing projectile/beam/summon/duration-scaling coverage.");

        await WaitSeconds(0.1f);
    }

    private async Task Sprint08DefenseScenario()
    {
        var context = RequireContext();
        var report = WaveAuthoringReport.Build(context.Content);
        Report(report);
        Assert(report.Contains("north_road", StringComparison.Ordinal), "Wave authoring report does not include authored spawn lanes.");
        Assert(report.Contains("human_brute", StringComparison.Ordinal), "Wave authoring report does not include brute spawn groups.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightDefense);
        context.State.Materials = 100;
        var invalid = context.Commands.TryExecute(
            new PlaceDefenseCommand(ContentId.From("barricade"), new Vector3(16f, 0f, 16f)),
            context,
            out var invalidReason);
        Assert(!invalid, $"Invalid barricade placement was accepted: {invalidReason}");

        var valid = context.Commands.TryExecute(
            new PlaceDefenseCommand(ContentId.From("barricade"), new Vector3(0f, 0f, -5f)),
            context,
            out var validReason);
        Assert(valid, $"Valid barricade placement was rejected: {validReason}");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        context.State.Player.Position = new Vector3(0f, 4.5f, 5f);
        await WaitSeconds(0.2f);
        var brute = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_brute"), new Vector3(0f, 0f, -8f));
        Assert(brute.IsValid, "Could not spawn Sprint 8 brute.");
        await WaitSeconds(2.0f);
        Assert(Events.DamageBySource(ContentId.From("human_brute")) > 0f, "Brute did not pressure the barricade through CombatSystem.");

        Events.Clear();
        var seal = context.GetSystem<DefenseSystem>().PlaceDefense(ContentId.From("explosive_seal"), new Vector3(4f, 0f, -2f));
        Assert(seal.IsValid, "Could not place explosive seal for Sprint 8 validation.");
        context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), new Vector3(4f, 0f, -2.5f));
        await WaitSeconds(1.3f);
        Assert(Events.DamageBySource(ContentId.From("explosive_seal")) > 0f, "Explosive Seal did not trigger through proximity/fuse rules.");

        context.State.DayIndex = 2;
        context.Events.Publish(new DayStartedEvent(2));
        context.GetSystem<WaveDirectorSystem>().SpawnUntilTarget();
        Assert(context.State.DebugMetrics.SpawnLaneSummary.Contains("north_road", StringComparison.Ordinal), "Spawn lane summary is missing authored lanes.");
        Assert(context.State.DebugMetrics.LastSpawnLaneId != "-", "Wave spawning did not record a lane.");
    }

    private async Task Sprint09EnemiesScenario()
    {
        var context = RequireContext();
        var validation = ToolingGateReport.ValidateCatalogResource();
        Report(ToolingGateReport.BuildValidationText(validation));
        Assert(!validation.HasErrors, "Sprint 9 catalog validation has errors.");

        var required = new[]
        {
            ContentId.From("human_grunt"),
            ContentId.From("human_soldier"),
            ContentId.From("human_archer"),
            ContentId.From("human_brute"),
            ContentId.From("human_acolyte"),
            ContentId.From("human_champion")
        };
        foreach (var enemyId in required)
        {
            Assert(context.Content.Enemies.ContainsKey(enemyId), $"Missing Sprint 9 enemy archetype '{enemyId}'.");
        }

        var archerInspection = ToolingGateReport.BuildEnemyInspection(context.Content.GetEnemy(ContentId.From("human_archer")), validation);
        Report(archerInspection);
        Assert(archerInspection.Contains("Role:", StringComparison.Ordinal), "Enemy inspector does not show role.");
        Assert(archerInspection.Contains("Abilities:", StringComparison.Ordinal), "Enemy inspector does not show abilities.");
        Assert(archerInspection.Contains("Validation Issues:", StringComparison.Ordinal), "Enemy inspector does not show validation issues.");

        var enemySystem = context.GetSystem<EnemySystem>();
        var acolyteA = enemySystem.Spawn(ContentId.From("human_acolyte"), ScenarioPoint(new Vector3(-28f, 0f, -28f)));
        var acolyteB = enemySystem.Spawn(ContentId.From("human_acolyte"), ScenarioPoint(new Vector3(-30f, 0f, -28f)));
        var acolyteC = enemySystem.Spawn(ContentId.From("human_acolyte"), ScenarioPoint(new Vector3(-32f, 0f, -28f)));
        Assert(acolyteA.IsValid && acolyteB.IsValid && !acolyteC.IsValid, "Specialist MaxActive limit was not enforced.");

        Events.Clear();
        enemySystem.Spawn(ContentId.From("human_archer"), ScenarioPoint(new Vector3(0f, 0f, -9f)));
        await WaitSeconds(1.4f);
        Assert(Events.DamageBySource(ContentId.From("human_archer")) > 0f, "Archer did not damage through the ranged projectile path.");
        Assert(Events.EnemyAbilityTelegraphs > 0, "Ranged enemy ability was not telegraphed.");

        Events.Clear();
        var championA = enemySystem.Spawn(ContentId.From("human_champion"), ScenarioPoint(new Vector3(2f, 0f, -2f)));
        var championB = enemySystem.Spawn(ContentId.From("human_champion"), ScenarioPoint(new Vector3(4f, 0f, -2f)));
        Assert(championA.IsValid && !championB.IsValid, "Champion MaxActive limit was not enforced.");
        await WaitSeconds(1.8f);
        Assert(Events.DamageBySource(ContentId.From("human_champion")) > 0f, "Champion did not reuse CombatSystem damage.");
        Assert(Events.EnemyAbilityExecutions > 0, "Champion ability execution event was not published.");

        var authoredEnemyIds = context.Content.Waves.Values
            .SelectMany(wave => wave.EnemyIds.Concat(wave.SpawnGroups.Select(group => group.EnemyId)))
            .ToHashSet();
        Assert(required.All(authoredEnemyIds.Contains), "Authored waves do not cover the Sprint 9 mixed enemy set.");
        Assert(
            context.Content.Waves.Values.Any(wave => wave.SpawnGroups.Any(group => group.EnemyId.Equals(ContentId.From("human_champion")))),
            "Authored waves do not include the first champion spawn group.");
    }

    private async Task Sprint10MetaScenario()
    {
        var context = RequireContext();
        var validation = ToolingGateReport.ValidateCatalogResource();
        Report(ToolingGateReport.BuildValidationText(validation));
        Assert(!validation.HasErrors, "Sprint 10 catalog validation has errors.");

        var requiredMages = new[]
        {
            ContentId.From("mage_recluse"),
            ContentId.From("mage_vessel"),
            ContentId.From("mage_guardian")
        };
        foreach (var mageId in requiredMages)
        {
            Assert(context.Content.Mages.ContainsKey(mageId), $"Missing Sprint 10 mage '{mageId}'.");
        }

        var profileWithCurrentDefaults = new ProfileSaveDto
        {
            PreferredMageId = string.Empty,
            UnlockedItemIds = new List<string> { RegressionDamageItem },
            DiscoveredItemIds = new List<string> { RegressionProjectileItem },
            CompletedAchievementIds = new List<string> { "test_achievement" }
        };
        var normalized = ProfileDefaultsService.EnsureCurrent(profileWithCurrentDefaults);
        Assert(normalized, "Profile defaults did not report changes for an incomplete current-profile save.");
        Assert(profileWithCurrentDefaults.UnlockedMageIds.Contains(ProfileDefaultsService.DefaultMageId), "Profile defaults did not unlock the default mage.");
        Assert(profileWithCurrentDefaults.UnlockedItemIds.Contains(RegressionDamageItem), "Profile defaults did not preserve unlocked items.");
        Assert(profileWithCurrentDefaults.DiscoveredItemIds.Contains(RegressionProjectileItem), "Profile defaults did not preserve item discovery.");
        Assert(profileWithCurrentDefaults.CompletedAchievementIds.Contains("test_achievement"), "Profile defaults did not preserve completed achievements.");

        Assert(context.Profile.UnlockedMageIds.Contains("mage_recluse"), "Recluse is not unlocked by default.");
        Assert(context.State.SelectedMageId.Equals(ContentId.From("mage_recluse")), "Recluse is not selected for a fresh profile.");
        Assert(context.State.Spellbook.Slots.Any(slot => slot.HasSpell && slot.SpellId.Equals(ContentId.From("fireball"))), "Recluse starting spell was not applied.");

        var blocked = context.Commands.TryExecute(new SelectMageCommand(ContentId.From("mage_vessel")), context, out _);
        Assert(!blocked, "Locked mage selection was allowed without an unlock.");

        var achievementSystem = context.GetSystem<AchievementSystem>();
        Assert(achievementSystem.CompleteAchievement(ContentId.From("defeat_first_champion")), "Debug achievement completion failed.");
        Assert(context.Profile.CompletedAchievementIds.Contains("defeat_first_champion"), "Completed achievement did not persist to profile state.");
        Assert(context.Profile.UnlockedMageIds.Contains("mage_vessel"), "Vessel was not unlocked by achievement reward.");

        var selected = context.Commands.TryExecute(new SelectMageCommand(ContentId.From("mage_vessel")), context, out var selectReason);
        Assert(selected, $"Unlocked mage selection failed: {selectReason}");
        Assert(context.State.SelectedMageId.Equals(ContentId.From("mage_vessel")), "Selected mage did not update run state.");
        Assert(Math.Abs(context.State.Player.BaseMaxHealth - 80f) < 0.01f, "Vessel max health was not applied.");
        Assert(
            context.GetSystem<CombatSystem>().TryGetHealth(context.State.Player.EntityId, out var playerHealth)
            && playerHealth != null
            && Math.Abs(playerHealth.MaxHealth - 80f) < 0.01f,
            "Vessel combat max health was not applied.");
        Assert(context.State.Spellbook.Slots.Any(slot => slot.HasSpell && slot.SpellId.Equals(ContentId.From("arcane_beam"))), "Vessel starting spell was not applied.");

        var lockedBefore = new LootSimulator().Run(context.Content, new ProfileSaveDto(), 64, 101);
        var unlockedProfile = new ProfileSaveDto();
        ProfileDefaultsService.EnsureCurrent(unlockedProfile);
        unlockedProfile.UnlockedItemIds.Add(RegressionLockedItem);
        var unlockedAfter = new LootSimulator().Run(context.Content, unlockedProfile, 64, 101);
        Assert(lockedBefore.LockedItemsExcluded > unlockedAfter.LockedItemsExcluded, "Unlocked items did not enter future loot simulation pools.");

        await WaitSeconds(0.1f);
    }

    private async Task Sprint11ItemsScenario()
    {
        var context = RequireContext();
        var validation = ToolingGateReport.ValidateCatalogResource();
        Report(ToolingGateReport.BuildValidationText(validation));
        Assert(!validation.HasErrors, "Editor-authored item catalog validation has errors.");
        var editorItemResourceCount = CountEditorItemResources();
        var productionItemCount = context.Content.Items.Values.Count(item => !IsRegressionItem(item.Id));
        Assert(
            productionItemCount == editorItemResourceCount,
            $"Expected gameplay catalog to contain all {editorItemResourceCount} product-owner editor-created item Resources, found {productionItemCount}.");
        Assert(context.Content.Items.ContainsKey(ContentId.From(RegressionDamageItem)), "Regression numeric item fixture is missing.");
        Assert(context.Content.Items.ContainsKey(ContentId.From(RegressionStatusItem)), "Regression status proc item fixture is missing.");
        Assert(context.Content.Items.ContainsKey(ContentId.From(RegressionFireStatusItem)), "Regression fire status proc item fixture is missing.");
        Assert(context.Content.Items.ContainsKey(ContentId.From(RegressionBurningGroundItem)), "Regression burning ground item fixture is missing.");
        Assert(context.Content.Items.ContainsKey(ContentId.From(RegressionKillItem)), "Regression kill proc item fixture is missing.");

        var batchReport = ToolingGateReport.BuildItemBatchReport(context.Content, validation);
        Report(batchReport);
        Assert(batchReport.Contains("Synergy targets", StringComparison.Ordinal), "Sprint 11 item batch report does not include synergy targets.");
        Assert(batchReport.Contains("Proc safety", StringComparison.Ordinal), "Sprint 11 item batch report does not include proc safety.");

        var lootSummary = new LootSimulator().Run(context.Content, context.Profile, ToolingGateReport.DefaultSimulationRolls, 11011);
        Assert(lootSummary.OffersGenerated > 0, "Sprint 11 loot simulator generated no offers.");
        Assert(lootSummary.OfferCounts.Count >= 3, "Loot simulator did not see the editor-created item pool.");

        await AssertSprint11NumericItems();
        await AssertSprint11OnHitProcItems();
        await AssertSprint11OnKillProcItems();
    }

    private async Task Sprint115ItemAuthoringScenario()
    {
        var context = RequireContext();
        var validation = ToolingGateReport.ValidateCatalogResource();
        Report(ToolingGateReport.BuildValidationText(validation));
        Assert(!validation.HasErrors, "Sprint 11.5 catalog validation has errors.");

        Assert(context.Content.Items.ContainsKey(ContentId.From(RegressionProjectileItem)), "Regression projectile item is missing.");
        Assert(
            Godot.FileAccess.FileExists("res://addons/the_last_mage_tools/plugin.cfg"),
            "The Last Mage editor tools plugin.cfg is missing.");

        AssertAllDesignerEffectsCompile();
        AssertAuthoringDropdownPreservesUnknowns();
        await AssertAdditionalProjectileRuntime();
        await AssertFreezeAndImmolateRuntime();

        var authoringItem = new ItemDefinition
        {
            Id = "regression_authoring_item",
            DisplayName = "Regression Authoring Item",
            HiddenDescription = "Hidden.",
            RevealedStatText = "+2 Spell Count",
            RevealedBehaviorText = "Adds two extra spell casts.",
            RevealedEffectText = ItemDefinition.CombineRevealedText("+2 Spell Count", "Adds two extra spell casts.")
        };
        authoringItem.PoolWeights.Add(new ItemPoolWeightDefinition { PoolId = ItemPoolIds.NightMarket, Weight = 2.5f });
        authoringItem.Effects.Add(new ItemEffectSpec
        {
            Kind = ItemEffectKind.AddSpellCount,
            Value = 2f,
            TargetTag = string.Empty
        });
        var authoringModifiers = ItemEffectCompiler.BuildModifierSpecs(authoringItem.Effects);
        var authoringProcs = ItemEffectCompiler.BuildProcSpecs(authoringItem.Effects);
        Assert(authoringModifiers.Count == 1, "Designer effect did not generate a modifier spec.");
        Assert(authoringModifiers[0].StatId == "spell_count", "AddSpellCount did not compile to spell_count.");
        Assert(Mathf.IsEqualApprox(authoringModifiers[0].Value, 2f), "AddSpellCount did not preserve the authored count value.");
        Assert(authoringProcs.Count == 0, "AddSpellCount unexpectedly generated a proc.");
        Assert(
            authoringItem.GetCombinedRevealedText().Contains("Stats:", StringComparison.Ordinal)
            && authoringItem.GetCombinedRevealedText().Contains("Behavior:", StringComparison.Ordinal),
            "Split revealed item text did not preserve stats and behavior sections.");

        var itemText = ToolingGateReport.BuildItemInspection(context.Content.GetItem(ContentId.From(RegressionProjectileItem)));
        Report(itemText);
        Assert(itemText.Contains("Pools:", StringComparison.Ordinal), "Item inspection does not include pool metadata.");
        Assert(itemText.Contains("Telemetry:", StringComparison.Ordinal), "Item inspection does not include read-only telemetry metadata.");
    }

    private void AssertAuthoringDropdownPreservesUnknowns()
    {
        var statOptions = AuthoringDropdownOptions.BuildPreservingSelected(
            new[] { "damage", "fire_rate", "damage" },
            "future_stat");
        var unresolvedStat = statOptions.LastOrDefault();
        Assert(
            unresolvedStat is { Value: "future_stat", IsUnresolved: true }
            && unresolvedStat.Label.Contains(AuthoringDropdownOptions.UnresolvedSuffix, StringComparison.Ordinal),
            "Authoring dropdown options did not preserve an unknown selected stat value.");

        var knownOptions = AuthoringDropdownOptions.BuildPreservingSelected(
            new[] { "damage", "fire_rate" },
            "damage");
        Assert(
            knownOptions.Count(option => string.Equals(option.Value, "damage", StringComparison.OrdinalIgnoreCase)) == 1
            && knownOptions.All(option => !option.IsUnresolved),
            "Authoring dropdown options incorrectly marked a known selected value as unresolved.");
    }

    private async Task Sprint12UxScenario()
    {
        var context = RequireContext();
        context.Settings.ScreenshakeScale = 0.45f;
        context.Settings.FlashScale = 0.35f;
        context.Settings.TextScale = 1.15f;

        var hud = RunViewModels.BuildHud(context);
        Report($"SPRINT12 HUD phase={hud.Phase} health={hud.HealthCurrent:0.#}/{hud.HealthMax:0.#} spells={hud.Spells.Count} readability={hud.ReadabilitySummary}");
        Assert(hud.Spells.Count == SpellbookState.MaxSlots, "HUD view model does not expose all spell slots.");
        Assert(hud.Prompt.Contains("Fight", StringComparison.OrdinalIgnoreCase), "HUD prompt did not describe the active combat phase.");

        AddItem(ContentId.From(RegressionDamageItem));
        var inspector = RunViewModels.BuildInspector(context);
        Report($"SPRINT12 INSPECTOR\n{inspector.AttributeText}\n{inspector.ItemText}\n{inspector.FeedbackText}");
        Assert(inspector.AttributeText.Contains("Damage", StringComparison.Ordinal), "Build inspector does not expose effective damage.");
        Assert(inspector.ItemText.Contains("Regression Damage Item", StringComparison.Ordinal), "Build inspector does not expose acquired item sources.");
        Assert(inspector.FeedbackText.Contains("impact", StringComparison.OrdinalIgnoreCase), "Build inspector does not expose feedback budgets.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightMarket);
        await WaitSeconds(0.25f);
        var offers = RunViewModels.BuildMarketOffers(context);
        Assert(offers.Count == 3, "Market view model does not provide three offer slots.");
        Assert(offers.Any(offer => offer.CanChoose), "Market view model has no choosable offer.");
        Assert(offers.Any(offer => !offer.Revealed && offer.DisplayName != "Unknown Relic"), "Undiscovered offer state should keep item names visible.");
        Assert(offers.Any(offer => !offer.Revealed && offer.BodyText.Contains("hidden", StringComparison.OrdinalIgnoreCase)), "Undiscovered offer state did not hide effect text.");

        Events.Clear();
        var beforeRequests = context.State.DebugMetrics.FeedbackRequestsThisFrame;
        context.Events.Publish(new DamageAppliedEvent(context.State.Player.EntityId, 2f, ContentId.From("sprint_12_feedback")));
        Assert(context.State.DebugMetrics.FeedbackRequestsThisFrame > beforeRequests, "Feedback budget metrics did not count event-driven feedback.");
        Assert(context.State.DebugMetrics.ActiveFeedbackBudgetSummary.Contains("impact", StringComparison.OrdinalIgnoreCase), "Feedback budget summary is missing impact category.");
        Assert(context.State.DebugMetrics.AudioMixSummary.Contains("impacts", StringComparison.OrdinalIgnoreCase), "Audio grouping summary is missing impact group.");
        Assert(context.State.DebugMetrics.ReadabilitySummary.Contains("shake 0.45", StringComparison.Ordinal), "Readability settings were not reflected in debug metrics.");
    }

    private async Task Sprint13ReportingScenario()
    {
        var context = RequireContext();
        await WaitSeconds(1.1f);

        Assert(
            context.State.DebugMetrics.ProfilingSummary.Contains("avg=", StringComparison.Ordinal),
            "Gameplay profiling summary was not updated.");
        Assert(
            context.State.DebugMetrics.ReproductionSummary.Contains($"seed={context.Random.Seed}", StringComparison.Ordinal),
            "Reproduction summary does not include the run seed.");

        var balanceReport = BalanceReport.Build(context);
        Report($"SPRINT13 BALANCE\n{balanceReport}");
        Assert(balanceReport.Contains("Sprint 13 Target Readiness", StringComparison.Ordinal), "Balance report is missing Sprint 13 readiness.");
        Assert(balanceReport.Contains("20-30 production items", StringComparison.Ordinal), "Balance report does not expose the vertical-slice item count target.");
        Assert(balanceReport.Contains("needs work", StringComparison.Ordinal), "Balance report should mark unmet content targets while product-owner item content is below target.");

        var reportPath = BalanceReport.Write(context);
        Assert(!string.IsNullOrWhiteSpace(reportPath), "Balance report did not write a path.");
        Assert(context.State.DebugMetrics.BalanceReportPath == reportPath, "Balance report path was not exposed in debug metrics.");

        var combat = context.GetSystem<CombatSystem>();
        combat.ApplyDamage(new DamageRequest(
            EntityId.None,
            context.State.Player.EntityId,
            ContentId.From("sprint_13_death_test"),
            999f,
            DamageType.Physical,
            new HitContext(context.State.Player.Position, Vector3.Up, EntityId.None)));
        await WaitSeconds(0.1f);
        Assert(
            context.State.DebugMetrics.DeathCauseSummary.Contains("sprint_13_death_test", StringComparison.Ordinal),
            "Death cause summary did not record the killing source.");
        Assert(context.State.CurrentPhase == RunPhase.RunDefeat, "Mage death did not transition the run to defeat.");
    }

    private Task Sprint16LocalizationSpineScenario()
    {
        var context = RequireContext();
        var catalog = ResourceLoader.Load<ContentCatalog>("res://data/catalogs/content_catalog.tres") ?? new ContentCatalog();
        var validation = LocalizationValidator.Validate(catalog);
        Report($"SPRINT16 localization validation issues={validation.Issues.Count} errors={validation.Issues.Count(issue => issue.Severity == Foundation.Validation.ValidationSeverity.Error)}");
        foreach (var issue in validation.Issues)
        {
            Report($"SPRINT16 {issue.Severity} {issue.Code} {issue.Message}");
        }

        Assert(!validation.HasErrors, "Localization validation has errors.");

        var brokenEntries = EnglishLocalizationSource.Build(catalog);
        brokenEntries.Remove(LocalizationKeys.ContentName("spell", ContentId.From("fireball")));
        var brokenValidation = LocalizationValidator.Validate(catalog, brokenEntries, includeRawStringScan: false);
        Assert(
            brokenValidation.Issues.Any(issue => issue.Code == "localization.missing_key"),
            "Localization validator did not catch a removed content key.");

        var testSpreadsheetPath = "user://localization_regression.csv";
        var spreadsheet = LocalizationSpreadsheet.FromSource(catalog);
        spreadsheet.SetTranslation("ui.hud.health", "fr", "Vie {0:0}/{1:0}");
        spreadsheet.SetTranslation("ui.market.choose", "fr", "Choisir");
        spreadsheet.SetTranslation(LocalizationKeys.ContentName("spell", ContentId.From("fireball")), "fr", "Boule de feu");
        spreadsheet.Save(testSpreadsheetPath);

        var importedSpreadsheet = LocalizationSpreadsheet.ImportAndNormalize(catalog, testSpreadsheetPath, "user://localization_regression_normalized.csv");
        var spreadsheetValidation = importedSpreadsheet.ValidateAgainst(catalog);
        Report($"SPRINT16 spreadsheet validation issues={spreadsheetValidation.Issues.Count} errors={spreadsheetValidation.Issues.Count(issue => issue.Severity == Foundation.Validation.ValidationSeverity.Error)} locales={string.Join(",", importedSpreadsheet.LocaleColumns)}");
        Assert(!spreadsheetValidation.HasErrors, "Localization spreadsheet import/normalize validation has errors.");

        LocalizationService.Current.LoadEnglish(catalog, "fr", "user://localization_regression_normalized.csv");
        Assert(
            LocalizationService.Current.Text("ui.hud.health", 4f, 9f).StartsWith("Vie", StringComparison.Ordinal),
            "French spreadsheet translation did not override HUD health text.");
        Assert(
            string.Equals(LocalizationService.Current.Text("ui.market.choose"), "Choisir", StringComparison.Ordinal),
            "French spreadsheet translation did not override market button text.");
        Assert(
            string.Equals(LocalizationService.Current.ContentName("spell", ContentId.From("fireball")), "Boule de feu", StringComparison.Ordinal),
            "French spreadsheet translation did not override content name.");
        Assert(
            string.Equals(LocalizationService.Current.ContentName("spell", ContentId.From("arcane_beam")), "Arcane Beam", StringComparison.Ordinal),
            "Missing French content translation did not fall back to English.");

        var brokenSpreadsheet = LocalizationSpreadsheet.FromSource(catalog);
        brokenSpreadsheet.SetTranslation("ui.hud.health", "fr", "Vie");
        var brokenSpreadsheetValidation = brokenSpreadsheet.ValidateAgainst(catalog);
        Assert(
            brokenSpreadsheetValidation.Issues.Any(issue => issue.Code == "localization.placeholder_mismatch"),
            "Localization spreadsheet validation did not catch placeholder mismatch.");

        var hud = RunViewModels.BuildHud(context);
        Assert(!ContainsMissingMarker(hud.Prompt), "HUD prompt contains a missing localization marker.");
        Assert(hud.Spells.All(spell => !ContainsMissingMarker(spell.DisplayName)), "HUD spell labels contain a missing localization marker.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightMarket);
        context.State.Market.SetOffers(new[]
        {
            new MarketOffer(ContentId.From("survivalist_belt")),
            new MarketOffer(ContentId.From("bent_wand")),
            new MarketOffer(ContentId.From("jelly_ring"))
        });
        var offers = RunViewModels.BuildMarketOffers(context);
        Assert(offers.All(offer => !ContainsMissingMarker(offer.DisplayName)), "Market offer names contain a missing localization marker.");
        Assert(offers.All(offer => !ContainsMissingMarker(offer.BodyText)), "Market offer body text contains a missing localization marker.");

        LocalizationService.Current.LoadEnglish(catalog, LocalizationService.DefaultLocale);
        return Task.CompletedTask;
    }

    private Task Sprint17ControllerInputScenario()
    {
        var context = RequireContext();
        InputBindingService.EnsureDefaultActions();

        AssertActionHasDevices(InputActions.MoveForward);
        AssertActionHasDevices(InputActions.MoveBack);
        AssertActionHasDevices(InputActions.MoveLeft);
        AssertActionHasDevices(InputActions.MoveRight);
        AssertActionHasDevices(InputActions.CastPrimary);
        AssertActionHasDevices(InputActions.UseActivatableItem);
        AssertActionHasDevices(InputActions.SpellPrevious);
        AssertActionHasDevices(InputActions.SpellNext);
        AssertActionHasDevices(InputActions.UiAccept);
        AssertActionHasDevices(InputActions.UiCancel);
        AssertActionHasDevices(InputActions.Pause);

        Assert(InputBindingService.GetActionEvents(InputActions.LookLeft).Any(inputEvent => InputPromptService.GetDeviceKind(inputEvent) == InputDeviceKind.Controller), "Controller look-left binding is missing.");
        Assert(InputBindingService.GetActionEvents(InputActions.LookRight).Any(inputEvent => InputPromptService.GetDeviceKind(inputEvent) == InputDeviceKind.Controller), "Controller look-right binding is missing.");
        Assert(InputBindingService.GetActionEvents(InputActions.LookUp).Any(inputEvent => InputPromptService.GetDeviceKind(inputEvent) == InputDeviceKind.Controller), "Controller look-up binding is missing.");
        Assert(InputBindingService.GetActionEvents(InputActions.LookDown).Any(inputEvent => InputPromptService.GetDeviceKind(inputEvent) == InputDeviceKind.Controller), "Controller look-down binding is missing.");

        InputPromptService.ObserveInputEvent(new InputEventJoypadButton { ButtonIndex = JoyButton.A, Pressed = true });
        Assert(InputPromptService.LastDeviceKind == InputDeviceKind.Controller, "Last input device did not switch to controller.");
        var controllerHud = RunViewModels.BuildHud(context);
        Report($"SPRINT17 controller prompt: {controllerHud.Prompt}");
        Assert(controllerHud.Prompt.Contains("RT", StringComparison.Ordinal), "Controller HUD prompt did not expose the Xbox-style cast binding.");
        Assert(controllerHud.Prompt.Contains("Menu", StringComparison.Ordinal), "Controller HUD prompt did not expose the Xbox-style pause binding.");

        InputPromptService.ObserveInputEvent(new InputEventKey { Keycode = Key.Z, Pressed = true });
        Assert(InputPromptService.LastDeviceKind == InputDeviceKind.KeyboardMouse, "Last input device did not switch back to keyboard/mouse.");
        var keyboardHud = RunViewModels.BuildHud(context);
        Report($"SPRINT17 keyboard prompt: {keyboardHud.Prompt}");
        Assert(keyboardHud.Prompt.Contains("LMB", StringComparison.Ordinal), "Keyboard/mouse HUD prompt did not expose the current mouse cast binding.");

        var settings = new SettingsSaveDto();
        Assert(
            InputBindingService.TryRebindAction(
                settings,
                InputActions.CastPrimary,
                new InputEventKey { Keycode = Key.F, Pressed = true },
                out var rebindReason),
            $"Keyboard rebind failed: {rebindReason}");
        Assert(
            InputPromptService.ActionLabel(InputActions.CastPrimary, InputDeviceKind.KeyboardMouse).Contains("F", StringComparison.Ordinal),
            "Prompt did not update immediately after keyboard rebinding.");
        Assert(
            InputPromptService.ActionLabel(InputActions.CastPrimary, InputDeviceKind.Controller).Contains("RT", StringComparison.Ordinal),
            "Keyboard rebinding should preserve the controller cast binding.");

        var saveService = GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        Assert(saveService != null, "SaveService autoload is missing for input persistence validation.");
        saveService!.SaveSettings(settings);
        var loaded = saveService.LoadSettings();
        Assert(
            loaded.InputBindings.TryGetValue(InputActions.CastPrimary, out var savedBindings)
            && savedBindings.Any(binding => string.Equals(binding.Key, nameof(Key.F), StringComparison.Ordinal)),
            "Input rebind was not persisted in settings.");

        InputBindingService.ResetActionToDefault(loaded, InputActions.CastPrimary);
        saveService.SaveSettings(loaded);
        Assert(
            InputPromptService.ActionLabel(InputActions.CastPrimary, InputDeviceKind.KeyboardMouse).Contains("LMB", StringComparison.Ordinal),
            "Resetting the cast binding did not restore the default keyboard/mouse prompt.");

        return Task.CompletedTask;
    }

    private async Task Sprint18FrontEndSaveStatsScenario()
    {
        var context = RequireContext();
        var freshProfile = new ProfileSaveDto();
        ProfileDefaultsService.EnsureCurrent(freshProfile);
        var mages = FrontEndViewModels.BuildMageSelection(context.Content, freshProfile);
        Assert(mages.Count == context.Content.Mages.Count, "Front-end mage selection did not expose every authored mage.");
        Assert(mages.Any(mage => mage.MageId.Equals(ContentId.From("mage_recluse")) && mage.IsUnlocked), "Front-end did not show Recluse as unlocked.");
        Assert(mages.Any(mage => mage.MageId.Equals(ContentId.From("mage_vessel")) && !mage.IsUnlocked && mage.UnlockRequirement != "-"), "Front-end did not show locked Vessel with an unlock requirement.");

        var vesselProfile = new ProfileSaveDto();
        vesselProfile.UnlockedMageIds.Add("mage_vessel");
        vesselProfile.PreferredMageId = "mage_vessel";
        ProfileDefaultsService.EnsureCurrent(vesselProfile);
        var vesselContext = _runController!.StartNewRun(profileOverride: vesselProfile, saveProfileOverride: _ => { });
        _context = vesselContext;
        Assert(vesselContext.State.SelectedMageId.Equals(ContentId.From("mage_vessel")), "Preferred mage was not applied when starting a run from the front-end profile.");

        var defeatContext = StartFreshScenarioRun();
        defeatContext.State.IsBenchmarkRun = false;
        defeatContext.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        KillMage(defeatContext, ContentId.From("sprint_18_defeat_smoke"));
        await WaitSeconds(0.1f);
        Assert(defeatContext.Profile.RunStatistics.TotalNormalRuns == 1, "Normal failed run did not update profile run stats.");
        Assert(defeatContext.Profile.RunStatistics.MageStatistics.TryGetValue("mage_recluse", out var recluseStats), "Normal failed run did not create per-mage stats.");
        Assert(recluseStats!.RunsPlayed == 1 && recluseStats.Deaths == 1, "Per-mage runs/deaths did not update for normal failed run.");

        var debugContext = StartFreshScenarioRun();
        debugContext.State.IsBenchmarkRun = false;
        debugContext.State.MarkDebugRun("sprint 18 stats exclusion smoke");
        debugContext.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        KillMage(debugContext, ContentId.From("sprint_18_debug_defeat_smoke"));
        await WaitSeconds(0.1f);
        Assert(debugContext.Profile.RunStatistics.TotalNormalRuns == 0, "Debug/test failed run changed normal player stats.");
        Assert(debugContext.Profile.RunStatistics.DebugOrTestRunsExcluded == 1, "Debug/test failed run was not counted as excluded.");

        var saveService = GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        Assert(saveService != null, "SaveService autoload is missing for Sprint 18 slot validation.");
        var slotPaths = Enumerable.Range(1, SaveServiceNode.ProfileSlotCount)
            .ToDictionary(slot => slot, slot => RuntimePathResolver.GlobalizeUserPath($"user://profiles/slot_{slot:00}.json"));
        var legacyProfilePath = RuntimePathResolver.GlobalizeUserPath("user://profile.json");
        var backups = slotPaths.Values
            .Append(legacyProfilePath)
            .ToDictionary(path => path, path => (Exists: File.Exists(path), Text: File.Exists(path) ? File.ReadAllText(path) : string.Empty));
        try
        {
            ResetRegressionProfileFiles();

            var emptySummaries = saveService!.LoadSlotSummaries();
            Assert(emptySummaries.Count == SaveServiceNode.ProfileSlotCount, "Save slot summary count does not match the configured slot count.");
            Assert(emptySummaries.All(summary => !summary.Exists), "Fresh save-slot summary did not report every slot as empty.");

            Directory.CreateDirectory(Path.GetDirectoryName(legacyProfilePath)!);
            File.WriteAllText(legacyProfilePath, "{\"PreferredMageId\":\"mage_vessel\",\"RunStatistics\":{\"TotalNormalRuns\":99,\"Victories\":99}}");
            var freshSlotOne = saveService.LoadProfileSlot(1);
            Assert(saveService.ActiveProfileSlot == 1, "Choosing an empty slot did not mark it active.");
            Assert(freshSlotOne.RunStatistics.TotalNormalRuns == 0, "Empty slot imported obsolete profile stats.");
            Assert(saveService.Profile.RunStatistics.TotalNormalRuns == 0, "Active profile imported obsolete profile stats.");
            Assert(File.Exists(slotPaths[1]), "Choosing an empty slot did not create a current-format slot file.");
            Assert(saveService.LoadSlotSummary(1).Exists, "Newly chosen empty slot did not report as in use.");
            Assert(freshSlotOne.UnlockedMageIds.Contains(ProfileDefaultsService.DefaultMageId), "Fresh slot did not receive current profile defaults.");

            freshSlotOne = BuildSaveRegressionProfile(3, 1, 2, "mage_recluse", "fireball", "survivalist_belt", "survive_first_day");
            saveService.SaveProfile(freshSlotOne);
            AssertSummary(saveService.LoadSlotSummary(1), 1, true, 3, 1, 2, 42, 63, 1, 1);
            var reloadedSlotOne = saveService.LoadProfileSlot(1);
            Assert(saveService.Profile.RunStatistics.TotalNormalRuns == 3, "Loading an existing slot did not update SaveService.Profile.");
            Assert(reloadedSlotOne.RunStatistics.SpellCastsBySpell.GetValueOrDefault("fireball") == 12, "Reloaded slot did not preserve spell usage stats.");
            Assert(reloadedSlotOne.RunStatistics.MageStatistics.TryGetValue("mage_recluse", out var reloadedMageStats) && reloadedMageStats.Deaths == 2, "Reloaded slot did not preserve per-mage stats.");
            Assert(reloadedSlotOne.LastCompletedRun?.MageUsed == "mage_recluse", "Reloaded slot did not preserve last completed run data.");

            saveService.SaveProfileSlot(2, BuildSaveRegressionProfile(7, 2, 5, "mage_vessel", "arcane_beam", RegressionDamageItem, "defeat_first_champion"));
            var loadedSlotTwo = saveService.LoadProfileSlot(2);
            Assert(saveService.ActiveProfileSlot == 2, "Loading slot 2 did not set the active slot.");
            Assert(saveService.Profile.RunStatistics.TotalNormalRuns == 7, "Loading an existing slot did not replace the active profile stats.");
            saveService.SaveProfile(BuildSaveRegressionProfile(8, 3, 5, "mage_vessel", "arcane_beam", RegressionDamageItem, "defeat_first_champion"));
            AssertSummary(saveService.LoadSlotSummary(2), 2, true, 8, 3, 5, 112, 168, 1, 1);
            AssertSummary(saveService.LoadSlotSummary(1), 1, true, 3, 1, 2, 42, 63, 1, 1);

            saveService.DeleteProfileSlot(1);
            Assert(saveService.ActiveProfileSlot == 2, "Deleting an inactive slot cleared the active slot.");
            Assert(saveService.Profile.RunStatistics.TotalNormalRuns == 8, "Deleting an inactive slot changed the active profile.");
            Assert(!saveService.LoadSlotSummary(1).Exists, "Deleted inactive slot still reported as in use.");

            saveService.DeleteProfileSlot(2);
            Assert(saveService.ActiveProfileSlot == 0, "Deleting the active slot did not clear the active slot index.");
            Assert(saveService.Profile.RunStatistics.TotalNormalRuns == 0, "Deleting the active slot did not reset the in-memory profile.");
            Assert(saveService.Profile.UnlockedMageIds.Contains(ProfileDefaultsService.DefaultMageId), "Deleting the active slot did not restore current profile defaults.");
            var recreatedSlotTwo = saveService.LoadProfileSlot(2);
            Assert(recreatedSlotTwo.RunStatistics.TotalNormalRuns == 0, "Reloading a deleted slot reused deleted stats.");
            saveService.DeleteProfileSlot(2);

            ResetRegressionProfileFiles();
            for (var slotIndex = 1; slotIndex <= SaveServiceNode.ProfileSlotCount; slotIndex++)
            {
                var mageId = slotIndex % 2 == 0 ? "mage_vessel" : "mage_recluse";
                var spellId = slotIndex % 2 == 0 ? "arcane_beam" : "fireball";
                saveService.SaveProfileSlot(slotIndex, BuildSaveRegressionProfile(slotIndex, slotIndex / 2, slotIndex - (slotIndex / 2), mageId, spellId, $"regression_item_slot_{slotIndex}", $"achievement_slot_{slotIndex}"));
            }

            var populatedSummaries = saveService.LoadSlotSummaries();
            Assert(populatedSummaries.Count == SaveServiceNode.ProfileSlotCount, "Populated save-slot summary count does not match the configured slot count.");
            foreach (var summary in populatedSummaries)
            {
                AssertSummary(summary, summary.SlotIndex, true, summary.SlotIndex, summary.SlotIndex / 2, summary.SlotIndex - (summary.SlotIndex / 2), 14 * summary.SlotIndex, 21 * summary.SlotIndex, 1, 1);
            }

            saveService.DeleteProfileSlot(4);
            var afterMiddleDelete = saveService.LoadSlotSummaries();
            Assert(!afterMiddleDelete.Single(summary => summary.SlotIndex == 4).Exists, "Deleted middle slot still reported as in use.");
            Assert(afterMiddleDelete.Where(summary => summary.SlotIndex != 4).All(summary => summary.Exists), "Deleting one slot affected another slot summary.");
            ResetRegressionProfileFiles();
        }
        finally
        {
            foreach (var (path, backup) in backups)
            {
                var directory = Path.GetDirectoryName(path);
                if (!string.IsNullOrWhiteSpace(directory))
                {
                    Directory.CreateDirectory(directory);
                }

                if (backup.Exists)
                {
                    File.WriteAllText(path, backup.Text);
                }
                else if (File.Exists(path))
                {
                    File.Delete(path);
                }
            }
        }

        void ResetRegressionProfileFiles()
        {
            foreach (var slotIndex in Enumerable.Range(1, SaveServiceNode.ProfileSlotCount))
            {
                saveService!.DeleteProfileSlot(slotIndex);
            }

            if (File.Exists(legacyProfilePath))
            {
                File.Delete(legacyProfilePath);
            }
        }

        static ProfileSaveDto BuildSaveRegressionProfile(int runs, int wins, int deaths, string mageId, string spellId, string itemId, string achievementId)
        {
            var profile = new ProfileSaveDto
            {
                PreferredMageId = mageId,
                LastCompletedRun = new RunSummarySaveDto
                {
                    RunId = $"save-regression-{runs}",
                    Result = deaths > wins ? "defeat" : "victory",
                    DurationSeconds = 60f + runs,
                    DayReached = runs,
                    HumanityKilled = 14 * runs,
                    EnemiesKilled = 21 * runs,
                    TotalSpellsCast = 4 * runs,
                    MageUsed = mageId,
                    MageName = mageId,
                    Cause = "save regression",
                    SpellsUsed = new Dictionary<string, int>(StringComparer.Ordinal) { [spellId] = 4 * runs },
                    ItemsDiscovered = new List<string> { itemId }
                }
            };
            profile.UnlockedMageIds.Add(mageId);
            profile.DiscoveredItemIds.Add(itemId);
            profile.CompletedAchievementIds.Add(achievementId);
            profile.RunStatistics.TotalNormalRuns = runs;
            profile.RunStatistics.Victories = wins;
            profile.RunStatistics.Defeats = deaths;
            profile.RunStatistics.TotalPlaySeconds = 60f + runs;
            profile.RunStatistics.HighestDayReached = runs;
            profile.RunStatistics.TotalHumanityKilled = 14 * runs;
            profile.RunStatistics.TotalEnemiesKilled = 21 * runs;
            profile.RunStatistics.TotalSpellsCast = 4 * runs;
            profile.RunStatistics.TotalItemsDiscovered = profile.DiscoveredItemIds.Count;
            profile.RunStatistics.RunsByMage[mageId] = runs;
            profile.RunStatistics.SpellCastsBySpell[spellId] = 4 * runs;
            profile.RunStatistics.MageStatistics[mageId] = new ProfileMageStatisticsDto
            {
                RunsPlayed = runs,
                Wins = wins,
                Deaths = deaths,
                BestHumanityKilled = 14 * runs,
                BestTimeSurvivedSeconds = 60f + runs,
                TotalSpellCasts = 4 * runs,
                SpellCastsBySpell = new Dictionary<string, int>(StringComparer.Ordinal) { [spellId] = 4 * runs }
            };
            ProfileDefaultsService.EnsureCurrent(profile);
            return profile;
        }

        void AssertSummary(ProfileSlotSummaryDto summary, int slotIndex, bool exists, int runs, int wins, int deaths, int humanity, int enemies, int items, int achievements)
        {
            Assert(summary.SlotIndex == slotIndex, $"Slot summary index mismatch for slot {slotIndex}.");
            Assert(summary.Exists == exists, $"Slot {slotIndex} existence mismatch.");
            Assert(summary.RunsPlayed == runs, $"Slot {slotIndex} run count mismatch.");
            Assert(summary.Wins == wins, $"Slot {slotIndex} win count mismatch.");
            Assert(summary.Deaths == deaths, $"Slot {slotIndex} death count mismatch.");
            Assert(summary.HumanityKilled == humanity, $"Slot {slotIndex} humanity count mismatch.");
            Assert(summary.EnemiesKilled == enemies, $"Slot {slotIndex} enemy count mismatch.");
            Assert(summary.ItemsDiscovered == items, $"Slot {slotIndex} item discovery count mismatch.");
            Assert(summary.AchievementsCompleted == achievements, $"Slot {slotIndex} achievement count mismatch.");
        }
    }

    private void AssertActionHasDevices(string actionName)
    {
        var events = InputBindingService.GetActionEvents(actionName);
        Assert(events.Any(inputEvent => InputPromptService.GetDeviceKind(inputEvent) == InputDeviceKind.KeyboardMouse), $"Action '{actionName}' is missing keyboard/mouse binding.");
        Assert(events.Any(inputEvent => InputPromptService.GetDeviceKind(inputEvent) == InputDeviceKind.Controller), $"Action '{actionName}' is missing controller binding.");
    }

    private Task Sprint19OptionsPauseScenario()
    {
        if (_runController == null)
        {
            throw new InvalidOperationException("RunControllerNode is not available.");
        }

        var profile = CreateFreshRegressionProfile();
        var context = _runController.StartNewRun(
            RunPhase.RunSetup,
            19019,
            profileOverride: profile,
            saveProfileOverride: _ => { });
        _context = context;
        PrepareRunContext(context);
        context.State.IsBenchmarkRun = false;

        context.State.Souls = 12;
        context.State.Materials = 34;
        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        Assert(profile.SuspendedRun != null, "Day start did not create a suspended-run checkpoint.");
        Assert(string.Equals(profile.SuspendedRun!.CheckpointLabel, "day_start", StringComparison.Ordinal), "Day checkpoint label is wrong.");
        Assert(profile.SuspendedRun.Souls == 12 && profile.SuspendedRun.Materials == 34, "Day checkpoint did not capture run resources.");

        context.State.Souls = 999;
        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightMarket);
        Assert(profile.SuspendedRun != null, "Night market did not update the suspended-run checkpoint.");
        Assert(string.Equals(profile.SuspendedRun!.CheckpointLabel, "night_market", StringComparison.Ordinal), "Night market checkpoint label is wrong.");
        Assert(profile.SuspendedRun.MarketOfferItemIds.Count > 0, "Night market checkpoint did not preserve market offers.");

        var firstOffer = profile.SuspendedRun.MarketOfferItemIds[0];
        Assert(context.GetSystem<MarketSystem>().ChooseOffer(0, out var chooseReason), $"Choosing checkpoint market offer failed: {chooseReason}");
        Assert(profile.SuspendedRun != null, "Night defense did not keep a suspended-run checkpoint.");
        Assert(string.Equals(profile.SuspendedRun!.CheckpointLabel, "night_defense", StringComparison.Ordinal), "Night defense checkpoint label is wrong.");
        Assert(profile.SuspendedRun.AcquiredItemIds.Contains(firstOffer), "Night defense checkpoint did not capture the chosen item.");

        var checkpoint = profile.SuspendedRun;
        var resumed = _runController.StartFromCheckpoint(checkpoint, profileOverride: profile, saveProfileOverride: _ => { });
        _context = resumed;
        Assert(resumed.State.CurrentPhase == RunPhase.NightDefense, "Resumed run did not restore the checkpoint phase.");
        Assert(resumed.State.Souls == checkpoint.Souls, "Resumed run did not restore checkpoint souls.");
        Assert(resumed.State.Build.HasAcquiredItem(ContentId.From(firstOffer)), "Resumed run did not restore acquired item state.");
        Assert(resumed.State.Spellbook.Slots.Any(slot => slot.SpellId.Equals(ContentId.From("fireball"))), "Resumed run did not restore spell slots.");

        resumed.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.RunVictory);
        Assert(profile.SuspendedRun == null, "Completed run did not clear the suspended-run checkpoint.");

        var saveService = GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        Assert(saveService != null, "SaveService autoload is missing for Sprint 19 settings validation.");
        var settings = saveService!.Settings;
        settings.DisplayMode = "windowed";
        settings.Resolution = "1600x900";
        settings.VSync = false;
        settings.FpsCap = 120;
        settings.RenderScale = 0.8f;
        settings.MasterVolume = 0.5f;
        settings.MouseSensitivity = 1.5f;
        settings.GamepadSensitivity = 1.25f;
        saveService.SaveSettings(settings);
        var loaded = saveService.LoadSettings();
        Assert(string.Equals(loaded.Resolution, "1600x900", StringComparison.Ordinal), "Resolution setting did not persist.");
        Assert(!loaded.VSync && loaded.FpsCap == 120, "Video performance settings did not persist.");
        Assert(Math.Abs(loaded.RenderScale - 0.8f) < 0.01f, "Render scale setting did not persist.");
        Assert(Math.Abs(loaded.MasterVolume - 0.5f) < 0.01f, "Master volume setting did not persist.");
        Assert(Math.Abs(loaded.MouseSensitivity - 1.5f) < 0.01f, "Mouse sensitivity setting did not persist.");
        Assert(Math.Abs(loaded.GamepadSensitivity - 1.25f) < 0.01f, "Gamepad sensitivity setting did not persist.");
        saveService.SaveSettings(new SettingsSaveDto());

        var pausePanel = GetNodeOrNull<PauseSettingsPanel>("/root/DebugServices/DebugUiLayer/PauseSettingsPanel");
        Assert(pausePanel != null, "Pause settings panel is missing from DebugServices.");
        GetTree().Paused = false;
        pausePanel!._UnhandledInput(new InputEventAction { Action = InputActions.Pause, Pressed = true });
        Assert(GetTree().Paused, "Opening the pause menu did not pause the scene tree.");
        pausePanel._UnhandledInput(new InputEventAction { Action = InputActions.Pause, Pressed = true });
        Assert(!GetTree().Paused, "Closing the pause menu did not restore scene tree processing.");

        return Task.CompletedTask;
    }

    private async Task Sprint13AActivatableItemsScenario()
    {
        var context = RequireContext();
        var validation = ToolingGateReport.ValidateCatalogResource();
        Report(ToolingGateReport.BuildValidationText(validation));
        Assert(!validation.HasErrors, "Sprint 13A catalog validation has errors.");
        Assert(context.Content.Items.ContainsKey(ContentId.From(RegressionActivatableLimitedItem)), "Limited activatable regression fixture is missing.");
        Assert(context.Content.Items.ContainsKey(ContentId.From(RegressionActivatableUnlimitedItem)), "Unlimited activatable regression fixture is missing.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightMarket);
        context.State.Market.SetOffers(new[] { new MarketOffer(ContentId.From(RegressionActivatableLimitedItem)) });
        var choseLimited = context.Commands.TryExecute(new ChooseMarketOfferCommand(0), context, out var limitedReason);
        Assert(choseLimited, $"Could not choose limited activatable item: {limitedReason}");
        Assert(context.State.Build.Items.Count == 0, "Activatable item was incorrectly added to passive item stacks.");
        Assert(context.State.Build.ActivatableItem?.ItemId.Equals(ContentId.From(RegressionActivatableLimitedItem)) == true, "Limited activatable item was not equipped.");
        var limitedState = context.State.Build.ActivatableItem ?? throw new InvalidOperationException("Limited activatable state missing after equip.");
        Assert(limitedState.RemainingActivations == 2, "Limited activatable remaining uses did not initialize to max activations.");
        Assert(context.State.CurrentPhase == RunPhase.NightDefense, "Choosing activatable item did not advance to NightDefense.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        context.State.Player.Position = _scenarioOrigin;
        context.State.Player.AimDirection = new Vector3(0f, 0f, -1f);
        Events.Clear();
        Assert(context.Commands.TryExecute(new UseActivatableItemCommand(), context, out var useReason), $"Limited activatable use failed: {useReason}");
        await WaitSeconds(0.05f);
        Assert(Events.ActivatableUses == 1, "Using the limited activatable did not publish an activatable use event.");
        Assert(Events.TeleportRequests == 1, "Limited activatable teleport active effect did not publish debug visibility.");
        Assert(context.State.Build.ActivatableItem?.RemainingActivations == 1, "Limited activatable did not decrement remaining activations.");
        Assert(context.State.Player.Position.DistanceTo(_scenarioOrigin) > 5f, "Teleport active effect did not move runtime player state.");

        Assert(context.Commands.TryExecute(new UseActivatableItemCommand(), context, out var secondUseReason), $"Second limited activatable use failed: {secondUseReason}");
        await WaitSeconds(0.05f);
        Assert(context.State.Build.ActivatableItem == null, "Limited activatable slot was not cleared after depletion.");
        Assert(Events.ActivatableClears == 1, "Limited activatable depletion did not publish a clear event.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightMarket);
        context.State.Market.SetOffers(new[] { new MarketOffer(ContentId.From(RegressionActivatableLimitedItem)) });
        Assert(
            !context.Commands.TryExecute(new ChooseMarketOfferCommand(0), context, out _),
            "Previously acquired depleted activatable item could be chosen again.");

        context.State.Market.SetOffers(new[] { new MarketOffer(ContentId.From(RegressionActivatableUnlimitedItem)) });
        Assert(context.Commands.TryExecute(new ChooseMarketOfferCommand(0), context, out var unlimitedReason), $"Could not choose unlimited activatable item: {unlimitedReason}");
        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        Assert(context.Commands.TryExecute(new UseActivatableItemCommand(), context, out var unlimitedUseReason), $"Unlimited activatable use failed: {unlimitedUseReason}");
        await WaitSeconds(0.05f);
        Assert(context.State.Build.ActivatableItem?.ItemId.Equals(ContentId.From(RegressionActivatableUnlimitedItem)) == true, "Unlimited activatable did not remain equipped after use.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightMarket);
        context.State.Market.SetOffers(new[] { new MarketOffer(ContentId.From(RegressionActivatableUnlimitedItem)) });
        Assert(
            !context.Commands.TryExecute(new ChooseMarketOfferCommand(0), context, out _),
            "Previously acquired unlimited activatable item could be chosen again.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        AddItem(ContentId.From(RegressionDamageItem));
        var damageWithPassiveAndActive = PlayerAttributeResolver.ResolveDamage(context, 10f, Array.Empty<TagId>(), false);
        Assert(damageWithPassiveAndActive >= 15.9f, $"Equipped activatable modifier did not share stat resolution. Damage={damageWithPassiveAndActive:0.###}.");

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.NightMarket);
        context.State.Market.SetOffers(new[] { new MarketOffer(ContentId.From(RegressionActivatableReplacementItem)) });
        Assert(context.Commands.TryExecute(new ChooseMarketOfferCommand(0), context, out var replacementReason), $"Could not choose replacement activatable item: {replacementReason}");
        Assert(context.State.Build.ActivatableItem?.ItemId.Equals(ContentId.From(RegressionActivatableReplacementItem)) == true, "New activatable item did not replace the old one.");
        Assert(context.State.Build.Items.Any(stack => stack.ItemId.Equals(ContentId.From(RegressionDamageItem))), "Passive item behavior changed during activatable replacement.");

        var report = BalanceReport.Build(context);
        Report($"SPRINT13A BALANCE\n{report}");
        Assert(report.Contains("Activatable Item Coverage", StringComparison.Ordinal), "Balance report does not separate activatable item coverage.");
        var inspector = RunViewModels.BuildInspector(context);
        Assert(inspector.ItemText.Contains("Passive Items", StringComparison.Ordinal), "Build inspector does not separate passive items.");
        Assert(inspector.ItemText.Contains("Activatable Item", StringComparison.Ordinal), "Build inspector does not separate activatable item slot.");
    }

    private async Task Sprint13BGameplayTagsScenario()
    {
        var context = RequireContext();
        var contentCatalog = ResourceLoader.Load<ContentCatalog>("res://data/catalogs/content_catalog.tres", cacheMode: ResourceLoader.CacheMode.Ignore)
            ?? new ContentCatalog();
        var tagCatalog = GameplayTagCatalogUtility.LoadDefaultCatalog();
        var tagValidation = GameplayTagCatalogUtility.ValidateCatalog(tagCatalog);
        Report($"TAG CATALOG VALIDATION issues={tagValidation.Issues.Count}");
        Assert(!tagValidation.HasErrors, "Gameplay tag catalog has validation errors.");

        var imported = GameplayTagCatalogUtility.BuildImportedCatalog(contentCatalog, new GameplayTagCatalog());
        Assert(imported.Contains("spell"), "Import did not create parent tag 'spell'.");
        Assert(imported.Contains("spell.element.fire"), "Import did not include existing spell tag 'spell.element.fire'.");
        Assert(imported.Contains("enemy.race.human"), "Import did not include existing enemy tag 'enemy.race.human'.");
        Assert(imported.Contains("enemy.race.human.entity.acolyte"), "Import did not include existing race-scoped enemy entity tag.");

        var hierarchy = new GameplayTagCatalog();
        hierarchy.EnsureTag("combat.damage.fire");
        Assert(hierarchy.Contains("combat"), "EnsureTag did not create root parent.");
        Assert(hierarchy.Contains("combat.damage"), "EnsureTag did not create intermediate parent.");
        Assert(hierarchy.Contains("combat.damage.fire"), "EnsureTag did not create child tag.");

        Assert(GameplayTagPath.IsSameOrParentOf("combat.damage", "combat.damage.fire"), "Parent tag 'combat.damage' did not match child 'combat.damage.fire'.");
        Assert(GameplayTagPath.IsSameOrParentOf("spell.delivery", "spell.delivery.projectile"), "Parent tag 'spell.delivery' did not match child 'spell.delivery.projectile'.");
        Assert(!GameplayTagPath.IsSameOrParentOf("spell.delivery.projectile", "spell.delivery"), "Child target incorrectly matched parent source.");
        Assert(GameplayTagPath.MatchesAny(TagId.From(string.Empty), new[] { TagId.From("anything") }), "Empty target tag did not match all.");

        var resolver = new StatResolver();
        var modifier = new Modifier(
            ContentId.From("regression_tag_modifier"),
            StatId.From("damage"),
            ModifierOp.Multiplicative,
            1.5f,
            0,
            new TargetFilter(TagId.From("combat.damage")));
        var breakdown = resolver.Resolve(
            StatId.From("damage"),
            10f,
            new[] { modifier },
            new[] { TagId.From("combat.damage.fire") });
        Assert(Mathf.IsEqualApprox(breakdown.FinalValue, 15f), $"Hierarchical modifier target did not apply. Final={breakdown.FinalValue:0.###}.");

        var deprecated = new GameplayTagCatalog();
        deprecated.EnsureTag("status.burn");
        var burn = deprecated.Find("status.burn");
        Assert(burn != null, "Deprecated test tag was not created.");
        burn!.IsDeprecated = true;
        Assert(GameplayTagCatalogUtility.IsDeprecated(deprecated, "status.burn"), "Deprecated tag lookup failed.");

        var migrationCatalog = new ContentCatalog();
        var item = new ItemDefinition { Id = "tag_migration_item", DisplayName = "Tag Migration Item", RevealedEffectText = "Test" };
        item.PoolWeights.Add(new ItemPoolWeightDefinition { PoolId = ItemPoolIds.NightMarket, Weight = 1f });
        item.Tags.Add("combat.damage.fire");
        item.Tags.Add("combat.damage.fire.ignite");
        item.Effects.Add(new ItemEffectSpec
        {
            Kind = ItemEffectKind.AddMultiplyStat,
            StatId = "damage",
            Value = 1.1f,
            TargetTag = "combat.damage.fire"
        });
        item.Effects.Add(new ItemEffectSpec
        {
            Kind = ItemEffectKind.AddMultiplyStat,
            StatId = "damage",
            Value = 1.2f,
            TargetTag = "combat.damage.fire.burning_ground"
        });
        migrationCatalog.Items.Add(item);
        var migrated = GameplayTagReferenceMigration.RenameInCatalog(migrationCatalog, "combat.damage.fire", "combat.damage.arcane");
        Assert(migrated == 4, $"Expected tag rename migration to update 4 references, updated {migrated}.");
        Assert(item.Tags.Contains("combat.damage.arcane"), "Tag rename migration did not update item tags.");
        Assert(item.Tags.Contains("combat.damage.arcane.ignite"), "Tag rename migration did not update child item tags.");
        Assert(string.Equals(item.Effects[0].TargetTag, "combat.damage.arcane", StringComparison.Ordinal), "Tag rename migration did not update item effect target.");
        Assert(string.Equals(item.Effects[1].TargetTag, "combat.damage.arcane.burning_ground", StringComparison.Ordinal), "Tag rename migration did not update child item effect target.");

        var validation = ToolingGateReport.ValidateCatalogResource();
        Report(ToolingGateReport.BuildValidationText(validation));
        Assert(!validation.HasErrors, "Catalog validation failed after gameplay tag migration.");
        await WaitSeconds(0.05f);
    }

    private async Task Sprint13CBalanceLabScenario()
    {
        var context = RequireContext();
        var productionItemCount = context.Content.Items.Values.Count(item => !IsRegressionItem(item.Id));
        Assert(productionItemCount >= 50, $"Sprint 13C requires at least 50 production items; found {productionItemCount}.");

        var experiment = new BalanceExperimentDefinition
        {
            ExperimentId = "sprint_13c_regression",
            Description = "Regression smoke for Sprint 13C balance lab.",
            SeedStart = 131_300,
            SeedCount = 2,
            RunMode = BalanceRunMode.ShortRun,
            MaxSimulatedDays = 2,
            MaxSimulatedSecondsPerDay = 4f,
            MonteCarloTrials = 96,
            MonteCarloNightsPerTrial = 4,
            BotPolicyIds = new[]
            {
                BalanceBotPolicyIds.RandomBuild,
                BalanceBotPolicyIds.AggressiveDamage
            }
        };

        Events.Detach(context.Events);
        var runner = new BalanceSimulationRunner();
        var first = runner.Run(experiment, context.Content, context.Profile, CreateBalanceRunContext);
        var second = runner.Run(experiment, context.Content, context.Profile, CreateBalanceRunContext);

        Report($"SPRINT13C BALANCE\n{BalanceReportWriter.BuildSummary(first)}");
        Assert(first.Runs.Count > 0, "Balance lab did not produce run results.");
        Assert(first.StaticAnalysis.ProductionItemCount >= 50, "Static analysis did not see the 50-item production pool.");
        Assert(first.MonteCarlo.PolicySummaries.All(summary => summary.OffersGenerated > 0), "Monte Carlo did not generate offers for every policy.");
        Assert(
            first.MonteCarlo.PolicySummaries.Select(summary => string.Join(';', summary.PickCounts.OrderBy(pair => pair.Key).Select(pair => $"{pair.Key}:{pair.Value}"))).Distinct(StringComparer.Ordinal).Count() > 1,
            "Different bot policies did not produce different Monte Carlo pick behavior.");
        Assert(first.BuildDeterminismKey() == second.BuildDeterminismKey(), "Balance lab deterministic repeat key changed for the same experiment and seeds.");

        var replaySource = first.Runs[0];
        var replay = runner.Replay(
            experiment,
            replaySource.Seed,
            ContentId.From(replaySource.MageId),
            replaySource.BotPolicyId,
            context.Profile,
            context.Content,
            CreateBalanceRunContext,
            ContentId.From(replaySource.FocusSpellId));
        Assert(replay.Seed == replaySource.Seed && replay.BotPolicyId == replaySource.BotPolicyId, "Replay did not preserve seed and policy.");

        var reportDirectory = BalanceReportWriter.Write(experiment, first);
        _context = _runController?.Context;
        if (_context != null)
        {
            BalanceReportWriter.ExposeOnDebugMetrics(_context, reportDirectory);
            Events.Attach(_context.Events);
        }

        Report($"SPRINT13C REPORT {reportDirectory}");
        Assert(File.Exists(Path.Combine(reportDirectory, "balance_summary.md")), "Balance summary report was not written.");
        Assert(File.Exists(Path.Combine(reportDirectory, "balance_runs.jsonl")), "Balance run JSONL report was not written.");
        Assert(File.Exists(Path.Combine(reportDirectory, "balance_item_correlations.csv")), "Balance item correlation CSV was not written.");

        var brokenValidation = ToolingGateReport.ValidateBrokenDebugCatalog();
        Assert(brokenValidation.HasErrors, "Broken content validation did not fail before simulation.");
        await WaitSeconds(0.1f);
    }

    private async Task Sprint15FullRunSkeletonScenario()
    {
        var context = StartFreshScenarioRun();
        context.State.IsBenchmarkRun = false;
        context.State.HumanityRemaining = 5;
        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        KillMage(context, ContentId.From("sprint_15_defeat_smoke"));
        await WaitSeconds(0.1f);

        Assert(context.State.CurrentPhase == RunPhase.RunDefeat, "Mage death did not produce run defeat.");
        Assert(context.State.LastRunSummary != null, "Defeat did not create run summary data.");
        Assert(string.Equals(context.State.LastRunSummary!.Result, "defeat", StringComparison.Ordinal), "Defeat summary result is wrong.");
        Assert(context.State.LastRunSummary.DayReached == context.State.DayIndex, "Defeat summary did not record day reached.");
        Assert(context.State.LastRunSummary.MageUsed == context.State.SelectedMageId.ToString(), "Defeat summary did not record mage used.");
        Assert(context.Profile.RunStatistics.TotalNormalRuns == 1, "Normal defeat was not counted in player stats.");
        Assert(context.Profile.RunStatistics.Defeats == 1, "Normal defeat did not increment defeat stats.");
        Assert(context.Profile.LastCompletedRun?.IsDebugOrTestRun == false, "Normal defeat was marked as debug/test.");

        context = StartFreshScenarioRun();
        context.State.IsBenchmarkRun = false;
        context.State.MarkDebugRun("sprint 15 accelerated victory smoke");
        context.State.HumanityRemaining = 1;
        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
        context.GetSystem<WaveDirectorSystem>().SpawnUntilTarget();
        KillAllActiveEnemies(context, ContentId.From("sprint_15_victory_smoke"));
        await WaitSeconds(0.2f);

        Assert(context.State.HumanityRemaining == 0, "Accelerated victory smoke did not reduce humanity to zero.");
        Assert(context.State.CurrentPhase == RunPhase.RunVictory, "Humanity zero did not produce run victory.");
        Assert(context.State.LastRunSummary != null, "Victory did not create run summary data.");
        Assert(string.Equals(context.State.LastRunSummary!.Result, "victory", StringComparison.Ordinal), "Victory summary result is wrong.");
        Assert(context.State.LastRunSummary.HumanityKilled == 1, "Victory summary did not record humanity killed.");
        Assert(context.State.LastRunSummary.EnemiesKilled == 1, "Victory summary did not record enemies killed.");
        Assert(context.State.LastRunSummary.IsDebugOrTestRun, "Accelerated victory summary was not marked debug/test.");
        Assert(context.Profile.RunStatistics.TotalNormalRuns == 0, "Debug victory changed normal player run stats.");
        Assert(context.Profile.RunStatistics.DebugOrTestRunsExcluded == 1, "Debug victory was not counted as an excluded debug/test run.");
        Assert(context.Profile.LastCompletedRun?.IsDebugOrTestRun == true, "Debug victory save summary did not mark debug/test.");
        await WaitSeconds(0.05f);
    }

    private async Task ItemCastFlowBehaviorScenario()
    {
        var context = StartFreshScenarioRun();
        AddItem(ContentId.From(RegressionKeepItInItem));
        var spellId = ContentId.From("fireball");
        context.State.Spellbook.AssignSlot(0, spellId);
        context.State.Spellbook.SelectSlot(0);
        Events.Clear();

        var origin = _scenarioOrigin;
        var direction = new Vector3(0f, 0f, -1f);
        var direct = new CastSpellCommand(context.State.Player.EntityId, 0, origin, direction);
        Assert(!context.Commands.TryExecute(direct, context, out var directReason), "Keep It In did not block direct spell casting.");
        Assert(directReason.Contains("requires charge", StringComparison.OrdinalIgnoreCase), $"Unexpected direct-cast rejection reason: {directReason}");

        Assert(
            context.Commands.TryExecute(new BeginCastFlowCommand(context.State.Player.EntityId, 0), context, out var beginReason),
            $"Could not begin Keep It In charge: {beginReason}");
        Assert(context.State.Spellbook.CastFlow.IsActive, "Keep It In did not create an active cast flow.");
        Assert(context.State.Spellbook.CastFlow.RequiredChargeSeconds < 0.5f, "Keep It In charge time did not scale down from fire rate.");
        Assert(context.State.Spellbook.TryGetSlot(0, out var slot) && slot != null && slot.CooldownRemainingSeconds <= 0f, "Keep It In started cooldown before release.");

        await WaitSeconds(0.05f);
        context.Commands.TryExecute(new ReleaseCastFlowCommand(origin, direction), context, out _);
        await WaitSeconds(0.05f);
        Assert(Events.ProjectileCount(spellId) == 0, "Incomplete Keep It In release spawned spell instances.");
        Assert(slot!.CooldownRemainingSeconds <= 0f, "Incomplete Keep It In release started cooldown.");

        Assert(
            context.Commands.TryExecute(new BeginCastFlowCommand(context.State.Player.EntityId, 0), context, out beginReason),
            $"Could not begin second Keep It In charge: {beginReason}");
        await WaitSeconds(context.State.Spellbook.CastFlow.RequiredChargeSeconds + 0.1f);
        Assert(context.State.Spellbook.CastFlow.IsReady, "Keep It In charge did not become ready.");
        Assert(
            context.Commands.TryExecute(new ReleaseCastFlowCommand(origin, direction), context, out var releaseReason),
            $"Could not release charged Keep It In cast: {releaseReason}");
        await WaitSeconds(0.05f);
        Assert(Events.ProjectileCount(spellId) >= 4, $"Charged Keep It In release spawned {Events.ProjectileCount(spellId)} Fireball instance(s), expected at least 4.");
        Assert(slot.CooldownRemainingSeconds > 0f, "Charged Keep It In release did not start cooldown.");
        Assert(context.State.DebugMetrics.LastCastFlowSummary.Contains("released", StringComparison.OrdinalIgnoreCase), "Cast flow debug summary did not record release.");

        context = StartFreshScenarioRun();
        AddItem(ContentId.From(RegressionAbyssalRingItem));
        context.State.Spellbook.AssignSlot(0, spellId);
        context.State.Spellbook.SelectSlot(0);
        Events.Clear();

        Assert(
            context.Commands.TryExecute(new BeginCastFlowCommand(context.State.Player.EntityId, 0), context, out beginReason),
            $"Could not begin Abyssal Ring charge: {beginReason}");
        Assert(context.State.Spellbook.CastFlow.AllowsConcurrentCasting, "Abyssal Ring should allow normal held spell casts during charge.");
        Assert(context.State.Spellbook.CastFlow.RequiredChargeSeconds < 0.7f, "Abyssal Ring charge time did not scale down from fire rate.");
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, origin, direction), context, out directReason),
            $"Abyssal Ring incorrectly blocked normal held casting: {directReason}");
        await WaitSeconds(0.05f);
        Assert(Events.ProjectileCount(spellId) > 0, "Abyssal Ring did not allow normal spell firing while charging.");

        var enemy = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(4f, 0f, 0f)));
        Assert(enemy.IsValid, "Abyssal Ring regression could not spawn a ring target.");
        await WaitSeconds(context.State.Spellbook.CastFlow.RequiredChargeSeconds + 0.05f);
        Assert(
            context.Commands.TryExecute(new ReleaseCastFlowCommand(origin, direction), context, out releaseReason),
            $"Could not release charged Abyssal Ring: {releaseReason}");
        var ring = context.GetSystem<AoESystem>().ActiveAreas.LastOrDefault(area => area.IsExpandingRing);
        if (ring == null)
        {
            Assert(false, "Abyssal Ring release did not spawn an expanding ring area.");
            return;
        }

        Assert(ring.Damage >= 390f, $"Abyssal Ring damage did not scale from player damage and multiplier. Damage={ring.Damage:0.###}.");
        Assert(ring.EndRadius > 12f, $"Abyssal Ring radius did not scale from player radius. EndRadius={ring.EndRadius:0.###}.");
        Assert(ring.LifetimeSeconds > 1.5f, $"Abyssal Ring duration did not scale from player duration. Lifetime={ring.LifetimeSeconds:0.###}.");
        await WaitSeconds(0.75f);
        Assert(Events.DamageBySource(ContentId.From(RegressionAbyssalRingItem)) > 0f, "Abyssal Ring did not damage enemies through CombatSystem.");

        context = StartFreshScenarioRun();
        AddItem(ContentId.From(RegressionFaultyFocusItem));
        context.State.Spellbook.AssignSlot(0, spellId);
        context.State.Spellbook.SelectSlot(0);
        Events.Clear();
        var focusTarget = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(0f, 0f, -4f)));
        Assert(focusTarget.IsValid, "Faulty Focus regression could not spawn a target.");
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, origin, direction), context, out directReason),
            $"Faulty Focus blocked normal spell casting: {directReason}");
        await WaitSeconds(0.35f);
        Assert(Events.ItemProcCount(ContentId.From(RegressionFaultyFocusItem), "faulty_focus_split_spell") > 0, "Faulty Focus did not trigger from a spell hit.");
        Assert(Events.ProjectileCount(spellId) >= 4, "Faulty Focus did not spawn child Fireball projectiles.");
        Assert(
            context.GetSystem<ProjectileSystem>().ActiveProjectiles.Any(projectile => projectile.SpellChainGeneration == 1 && Math.Abs(projectile.DamageMultiplier - 0.5f) < 0.001f),
            "Faulty Focus child projectiles did not preserve generation and 50% damage scaling.");
    }

    private async Task ProductionItemBehaviorScenario()
    {
        var context = StartFreshScenarioRun();
        var itemIds = context.Content.Items.Values
            .Where(item => !IsRegressionItem(item.Id))
            .OrderBy(item => item.ItemNumber)
            .ThenBy(item => item.Id.Value, StringComparer.Ordinal)
            .Select(item => item.Id)
            .ToArray();
        var editorItemResourceCount = CountEditorItemResources();
        Assert(itemIds.Length == editorItemResourceCount, $"Production item behavior test expected {editorItemResourceCount} item Resources, found {itemIds.Length} runtime items.");
        Assert(itemIds.Length > 0, "Production item behavior test found no production items.");

        foreach (var itemId in itemIds)
        {
            await AssertProductionItemBehavior(itemId);
        }
    }

    private async Task AssertProductionItemBehavior(ContentId itemId)
    {
        var context = StartFreshScenarioRun();
        var item = context.Content.GetItem(itemId);
        Report($"PRODUCTION ITEM START id={item.Id} number={item.ItemNumber} name={item.DisplayName} effects={item.Effects.Count} modifiers={item.Modifiers.Count} procs={item.Procs.Count}");
        if (item.IsFlavorOnly)
        {
            Report($"PRODUCTION ITEM SKIP flavor-only id={item.Id}");
            return;
        }

        var modifierChecks = item.Modifiers
            .Select(modifier => new ModifierRuntimeCheck(modifier, ResolveModifierValue(context, modifier)))
            .ToArray();
        var spellModifierChecks = item.Modifiers
            .SelectMany(modifier => BuildSpellModifierChecks(context, modifier))
            .ToArray();

        AddItem(item.Id);
        Assert(context.State.Build.HasAcquiredItem(item.Id), $"Production item '{item.Id}' was not recorded as acquired.");
        Assert(item.Kind == nameof(ItemKind.Activatable) || context.State.Build.HasItem(item.Id), $"Production passive item '{item.Id}' was not added to passive build state.");

        foreach (var check in modifierChecks)
        {
            AssertModifierChanged(
                item,
                check,
                ResolveModifierValue(context, check.Modifier),
                HasOpposingModifier(item.Modifiers, check.Modifier));
        }

        foreach (var check in spellModifierChecks)
        {
            AssertSpellModifierChanged(
                item,
                check,
                ResolveModifierValueForTags(context, check.Modifier, check.SpellTags),
                HasOpposingModifier(item.Modifiers, check.Modifier));
        }

        foreach (var effect in item.Effects)
        {
            StartFreshScenarioRun();
            AddItem(item.Id);
            await AssertProductionEffectBehavior(item, effect);
        }

        foreach (var effect in item.ActiveEffects)
        {
            StartFreshScenarioRun();
            await AssertProductionActiveEffectBehavior(item, effect);
        }

        if (item.ActiveEffects.Count == 0 && item.Kind == nameof(ItemKind.Activatable))
        {
            StartFreshScenarioRun();
            await AssertProductionEmptyActivatableBehavior(item);
        }

        Report($"PRODUCTION ITEM PASS id={item.Id}");
    }

    private async Task AssertProductionEffectBehavior(ItemRuntimeDefinition item, ItemEffectRuntimeSpec effect)
    {
        Report($"PRODUCTION ITEM EFFECT id={item.Id} kind={effect.Kind}");
        if (!Enum.TryParse<ItemEffectKind>(effect.Kind, out var kind))
        {
            Assert(false, $"Production item '{item.Id}' has unknown effect kind '{effect.Kind}'.");
            return;
        }

        switch (kind)
        {
            case ItemEffectKind.AddMultiplyStat:
            case ItemEffectKind.AddFlatStat:
            case ItemEffectKind.RemoveMultiplyStat:
            case ItemEffectKind.RemoveFlatStat:
                Assert(
                    item.Modifiers.Count > 0,
                    $"Production item '{item.Id}' effect '{kind}' did not compile to any runtime modifier.");
                if (effect.StatId.Equals(PlayerAttributeResolver.AreaRadius))
                {
                    await AssertProductionSpellRadiusMatrix(
                        item.Id,
                        effect.TargetTag,
                        kind is ItemEffectKind.RemoveMultiplyStat or ItemEffectKind.RemoveFlatStat
                            ? RadiusExpectation.Decrease
                            : RadiusExpectation.Increase);
                }

                break;
            case ItemEffectKind.HomingSpells:
                await AssertProductionHomingSpells(item.Id);
                break;
            case ItemEffectKind.OrbitingSpells:
                await AssertProductionOrbitingSpells(item.Id);
                break;
            case ItemEffectKind.AddProjectilePierce:
                await AssertProductionProjectilePierce(item.Id, effect.TargetTag);
                break;
            case ItemEffectKind.SplitProjectile:
                await AssertProductionProjectileSplit(item.Id);
                break;
            case ItemEffectKind.AddSpellCount:
                await AssertProductionSpellCount(item.Id, effect);
                break;
            case ItemEffectKind.FreezeWithFrostDamage:
                await AssertHitProc(
                    item.Id,
                    ContentId.From("frost_bolt"),
                    DamageType.Frost,
                    "apply_freeze",
                    () => Assert(Events.StatusCountFromSource("freeze", item.Id) > 0, $"Production item '{item.Id}' did not apply freeze."));
                break;
            case ItemEffectKind.ImmolateWithFireDamage:
                await AssertHitProc(
                    item.Id,
                    ContentId.From("fireball"),
                    DamageType.Fire,
                    "apply_burn",
                    () => Assert(Events.StatusCountFromSource("burn", item.Id) > 0, $"Production item '{item.Id}' did not make fire damage apply burn."));
                break;
            case ItemEffectKind.EnemiesExplodeOnDeath:
                await AssertKillProcAoEForMatchingSpells(
                    item.Id,
                    DamageType.Fire,
                    effect.TargetTag,
                    "kill_explosion",
                    $"Production item '{item.Id}' did not spawn a kill explosion.");
                break;
            case ItemEffectKind.BurningGroundOnEnemyDeath:
                await AssertKillProcAoEForMatchingSpells(
                    item.Id,
                    DamageType.Fire,
                    effect.TargetTag,
                    "burning_ground",
                    $"Production item '{item.Id}' did not spawn burning ground.");
                break;
            case ItemEffectKind.KeepItIn:
                await AssertProductionKeepItIn(item.Id, effect);
                break;
            case ItemEffectKind.AbyssalRing:
                await AssertProductionAbyssalRing(item.Id, effect);
                break;
            case ItemEffectKind.FaultyFocus:
                await AssertProductionFaultyFocus(item.Id, effect);
                break;
            case ItemEffectKind.PreventNextPlayerHealthLoss:
                await AssertProductionDamagePrevention(item.Id, effect);
                break;
            case ItemEffectKind.PersistentSummons:
                await AssertProductionPersistentSummons(item.Id);
                break;
            default:
                Assert(false, $"Production item '{item.Id}' effect '{kind}' has no production behavior regression coverage.");
                break;
        }
    }

    private async Task AssertProductionActiveEffectBehavior(ItemRuntimeDefinition item, EffectRuntimeSpec effect)
    {
        var context = RequireContext();
        var runtimeItem = context.Content.GetItem(item.Id);
        Report($"PRODUCTION ITEM ACTIVE id={item.Id} effect={effect.EffectType}");
        context.GetSystem<MarketSystem>().AcquireItem(runtimeItem);
        context.Events.Publish(new ItemAcquiredEvent(item.Id));
        Assert(
            context.State.Build.ActivatableItem?.ItemId.Equals(item.Id) == true,
            $"Production activatable item '{item.Id}' was not equipped before active effect test.");

        switch (effect.EffectType)
        {
            case ActiveEffectIds.MagicBall:
            {
                var beforePosition = context.State.Player.Position;
                var beforeSouls = context.State.Souls;
                var beforeMaterials = context.State.Materials;
                AssertUseActivatable(context);
                await WaitSeconds(0.05f);
                Assert(context.State.Player.Position.DistanceTo(beforePosition) < 0.001f, $"Production activatable item '{item.Id}' moved the player despite magic_ball.");
                Assert(context.State.Souls == beforeSouls, $"Production activatable item '{item.Id}' changed souls despite magic_ball.");
                Assert(context.State.Materials == beforeMaterials, $"Production activatable item '{item.Id}' changed materials despite magic_ball.");
                break;
            }
            case ActiveEffectIds.TeleportPlayer:
            {
                var before = context.State.Player.Position;
                AssertUseActivatable(context);
                await WaitSeconds(0.05f);
                Assert(context.State.Player.Position.DistanceTo(before) > 0.1f, $"Production activatable item '{item.Id}' did not move the player.");
                break;
            }
            case ActiveEffectIds.CooldownTick:
            {
                context.State.Spellbook.AssignSlot(0, ContentId.From("fireball"));
                Assert(context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, Vector3.Forward), context, out var castReason), $"Cooldown active setup cast failed: {castReason}");
                Assert(context.State.Spellbook.TryGetSlot(0, out var slot) && slot != null, "Cooldown active setup missing spell slot.");
                var before = slot!.CooldownRemainingSeconds;
                AssertUseActivatable(context);
                await WaitSeconds(0.05f);
                Assert(slot.CooldownRemainingSeconds < before, $"Production activatable item '{item.Id}' did not reduce cooldown.");
                break;
            }
            case ActiveEffectIds.GrantSouls:
            {
                var before = context.State.Souls;
                AssertUseActivatable(context);
                await WaitSeconds(0.05f);
                Assert(context.State.Souls > before, $"Production activatable item '{item.Id}' did not grant souls.");
                break;
            }
            case ActiveEffectIds.GrantMaterials:
            {
                var before = context.State.Materials;
                AssertUseActivatable(context);
                await WaitSeconds(0.05f);
                Assert(context.State.Materials > before, $"Production activatable item '{item.Id}' did not grant materials.");
                break;
            }
            case ActiveEffectIds.HealPlayer:
            {
                var combat = context.GetSystem<CombatSystem>();
                combat.ApplyDamage(new DamageRequest(
                    EntityId.None,
                    context.State.Player.EntityId,
                    ContentId.From("regression_damage"),
                    1f,
                    DamageType.Physical,
                    new HitContext(context.State.Player.Position, Vector3.Up, EntityId.None)));
                Assert(combat.TryGetHealth(context.State.Player.EntityId, out var health) && health != null, "Player health missing for heal active test.");
                var before = health!.CurrentHealth;
                AssertUseActivatable(context);
                await WaitSeconds(0.05f);
                Assert(health.CurrentHealth > before, $"Production activatable item '{item.Id}' did not heal the player.");
                break;
            }
            case ActiveEffectIds.GrantHitProtection:
            {
                AssertUseActivatable(context);
                context.GetSystem<CombatSystem>().ApplyDamage(new DamageRequest(
                    EntityId.None,
                    context.State.Player.EntityId,
                    ContentId.From("regression_damage"),
                    1f,
                    DamageType.Physical,
                    new HitContext(context.State.Player.Position, Vector3.Up, EntityId.None)));
                await WaitSeconds(0.05f);
                Assert(Events.DamagePreventionCount(item.Id) > 0, $"Production activatable item '{item.Id}' did not prevent a hit.");
                break;
            }
            case ActiveEffectIds.SleepNearbyEnemies:
            {
                var enemySystem = context.GetSystem<EnemySystem>();
                var enemy = enemySystem.Spawn(ContentId.From("human_grunt"), context.State.Player.Position + new Vector3(1f, 0f, 0f));
                Assert(enemy.IsValid, "Sleep active test could not spawn a nearby enemy.");
                AssertUseActivatable(context);
                await WaitSeconds(0.05f);
                Assert(Events.StatusCountFromSource(StatusEffectRules.Sleep, item.Id) > 0, $"Production activatable item '{item.Id}' did not apply sleep.");
                Assert(context.GetSystem<StatusEffectSystem>().BlocksActions(enemy), $"Production activatable item '{item.Id}' sleep did not block enemy actions.");
                break;
            }
            case ActiveEffectIds.TransformEnemiesToForm:
            {
                var enemySystem = context.GetSystem<EnemySystem>();
                var enemy = enemySystem.Spawn(ContentId.From("human_grunt"), context.State.Player.Position + new Vector3(2f, 0f, 0f));
                Assert(enemy.IsValid, "Transform active test could not spawn an enemy.");
                AssertUseActivatable(context);
                await WaitSeconds(0.05f);
                var state = enemySystem.ActiveEnemies.FirstOrDefault(active => active.EntityId.Equals(enemy));
                Assert(state != null, "Transform active test enemy disappeared unexpectedly.");
                Assert(string.Equals(state!.TemporaryFormId, "among_us", StringComparison.Ordinal), $"Production activatable item '{item.Id}' did not apply the among_us temporary form.");
                Assert(state.TemporaryFormDisablesAttacks, $"Production activatable item '{item.Id}' temporary form does not suppress attacks.");
                Assert(context.GetSystem<CombatSystem>().TryGetHealth(enemy, out var health) && health != null && health.MaxHealth <= 1.001f, $"Production activatable item '{item.Id}' did not set transformed enemy HP to 1.");
                break;
            }
            case ActiveEffectIds.PlayerStasis:
            {
                AssertUseActivatable(context);
                Assert(context.State.IsPlayerStasisActive, $"Production activatable item '{item.Id}' did not start player stasis.");
                var combat = context.GetSystem<CombatSystem>();
                Assert(combat.TryGetHealth(context.State.Player.EntityId, out var health) && health != null, "Player health missing for stasis active test.");
                var before = health!.CurrentHealth;
                combat.ApplyDamage(new DamageRequest(
                    EntityId.None,
                    context.State.Player.EntityId,
                    ContentId.From("regression_damage"),
                    1f,
                    DamageType.Physical,
                    new HitContext(context.State.Player.Position, Vector3.Up, EntityId.None)));
                await WaitSeconds(0.05f);
                Assert(Math.Abs(health.CurrentHealth - before) < 0.001f, $"Production activatable item '{item.Id}' stasis did not prevent damage.");
                Assert(Events.DamagePreventionCount(item.Id) > 0, $"Production activatable item '{item.Id}' stasis did not publish damage prevention.");
                break;
            }
            case ActiveEffectIds.TemporaryStatModifier:
            {
                var statId = StatId.From(effect.TargetStatId.Value);
                var before = PlayerAttributeResolver.ResolveValue(context, statId, 10f, recordBreakdown: false);
                AssertUseActivatable(context);
                var after = PlayerAttributeResolver.ResolveValue(context, statId, 10f, recordBreakdown: false);
                Assert(after > before, $"Production activatable item '{item.Id}' did not apply temporary stat modifier for '{effect.TargetStatId}'.");
                break;
            }
            case ActiveEffectIds.AssaultRifleMode:
            {
                AssertUseActivatable(context);
                Assert(context.State.TemporaryWeaponMode?.IsActive == true, $"Production activatable item '{item.Id}' did not enter assault rifle mode.");
                Assert(context.GetSystem<SpellSystem>().TryFireTemporaryWeapon(
                    context.State.Player.EntityId,
                    _scenarioOrigin,
                    new Vector3(0f, 0f, -1f)), $"Production activatable item '{item.Id}' assault rifle mode could not fire.");
                await WaitSeconds(0.05f);
                Assert(Events.ProjectileCount(item.Id) > 0, $"Production activatable item '{item.Id}' assault rifle mode did not spawn a projectile.");
                break;
            }
            case ActiveEffectIds.ThrowTeleportProjectile:
            {
                var before = context.State.Player.Position;
                AssertUseActivatable(context);
                await WaitSeconds(MathF.Max(0.1f, effect.DurationSeconds + 0.1f));
                Assert(context.State.Player.Position.DistanceTo(before) > 0.1f, $"Production activatable item '{item.Id}' teleport projectile did not move the player after landing.");
                Assert(Events.TeleportRequests > 0, $"Production activatable item '{item.Id}' teleport projectile did not publish a teleport request.");
                break;
            }
            default:
                Assert(false, $"Production item '{item.Id}' active effect '{effect.EffectType}' has no production behavior regression coverage.");
                break;
        }

        Assert(Events.ActivatableUses > 0, $"Production activatable item '{item.Id}' did not publish a use event.");
    }

    private async Task AssertProductionEmptyActivatableBehavior(ItemRuntimeDefinition item)
    {
        var context = RequireContext();
        Report($"PRODUCTION ITEM ACTIVE id={item.Id} effect=<none>");
        context.GetSystem<MarketSystem>().AcquireItem(item);
        context.Events.Publish(new ItemAcquiredEvent(item.Id));
        Assert(
            context.State.Build.ActivatableItem?.ItemId.Equals(item.Id) == true,
            $"Production activatable item '{item.Id}' was not equipped before empty active effect test.");

        var beforePosition = context.State.Player.Position;
        AssertUseActivatable(context);
        await WaitSeconds(0.05f);
        Assert(context.State.Player.Position.DistanceTo(beforePosition) < 0.001f, $"Production activatable item '{item.Id}' with no active effects moved the player.");
        Assert(Events.ActivatableUses > 0, $"Production activatable item '{item.Id}' with no active effects did not publish a use event.");
    }

    private void AssertUseActivatable(RunContext context)
    {
        Assert(context.Commands.TryExecute(new UseActivatableItemCommand(), context, out var reason), $"Activatable item use failed: {reason}");
    }

    private void AssertModifierChanged(ItemRuntimeDefinition item, ModifierRuntimeCheck check, float after, bool hasOpposingModifier)
    {
        var modifier = check.Modifier;
        var before = check.BeforeValue;
        var operation = Enum.TryParse<ModifierOp>(modifier.Operation, out var parsed)
            ? parsed
            : ModifierOp.FlatAdd;
        var changed = Math.Abs(after - before) > 0.001f;
        Assert(changed, $"Production item '{item.Id}' modifier '{modifier.StatId}' did not change resolved value. Before={before:0.###}, after={after:0.###}.");
        if (hasOpposingModifier)
        {
            Report($"PRODUCTION MODIFIER MIXED item={item.Id} stat={modifier.StatId} before={before:0.###} after={after:0.###} target={modifier.TargetTag}");
            return;
        }

        if (operation == ModifierOp.FlatAdd && modifier.Value > 0f)
        {
            Assert(after > before, $"Production item '{item.Id}' expected '{modifier.StatId}' to increase. Before={before:0.###}, after={after:0.###}.");
        }
        else if (operation == ModifierOp.FlatAdd && modifier.Value < 0f)
        {
            Assert(after < before, $"Production item '{item.Id}' expected '{modifier.StatId}' to decrease. Before={before:0.###}, after={after:0.###}.");
        }
        else if (operation == ModifierOp.Multiplicative && modifier.Value > 1f)
        {
            Assert(after > before, $"Production item '{item.Id}' expected multiplicative '{modifier.StatId}' to increase. Before={before:0.###}, after={after:0.###}.");
        }
        else if (operation == ModifierOp.Multiplicative && modifier.Value < 1f)
        {
            Assert(after < before, $"Production item '{item.Id}' expected multiplicative '{modifier.StatId}' to decrease. Before={before:0.###}, after={after:0.###}.");
        }
    }

    private void AssertSpellModifierChanged(ItemRuntimeDefinition item, SpellModifierRuntimeCheck check, float after, bool hasOpposingModifier)
    {
        var modifier = check.Modifier;
        var before = check.BeforeValue;
        var operation = Enum.TryParse<ModifierOp>(modifier.Operation, out var parsed)
            ? parsed
            : ModifierOp.FlatAdd;
        var changed = Math.Abs(after - before) > 0.001f;
        Assert(
            changed,
            $"Production item '{item.Id}' modifier '{modifier.StatId}' did not change resolved value for spell '{check.SpellId}'. Before={before:0.###}, after={after:0.###}.");

        Report($"PRODUCTION SPELL MODIFIER item={item.Id} spell={check.SpellId} stat={modifier.StatId} before={before:0.###} after={after:0.###} target={modifier.TargetTag}");
        if (hasOpposingModifier)
        {
            Report($"PRODUCTION SPELL MODIFIER MIXED item={item.Id} spell={check.SpellId} stat={modifier.StatId} before={before:0.###} after={after:0.###} target={modifier.TargetTag}");
            return;
        }

        if (operation == ModifierOp.FlatAdd && modifier.Value > 0f)
        {
            Assert(after > before, $"Production item '{item.Id}' expected '{modifier.StatId}' to increase for spell '{check.SpellId}'. Before={before:0.###}, after={after:0.###}.");
        }
        else if (operation == ModifierOp.FlatAdd && modifier.Value < 0f)
        {
            Assert(after < before, $"Production item '{item.Id}' expected '{modifier.StatId}' to decrease for spell '{check.SpellId}'. Before={before:0.###}, after={after:0.###}.");
        }
        else if (operation == ModifierOp.Multiplicative && modifier.Value > 1f)
        {
            Assert(after > before, $"Production item '{item.Id}' expected multiplicative '{modifier.StatId}' to increase for spell '{check.SpellId}'. Before={before:0.###}, after={after:0.###}.");
        }
        else if (operation == ModifierOp.Multiplicative && modifier.Value < 1f)
        {
            Assert(after < before, $"Production item '{item.Id}' expected multiplicative '{modifier.StatId}' to decrease for spell '{check.SpellId}'. Before={before:0.###}, after={after:0.###}.");
        }
    }

    private static bool HasOpposingModifier(IEnumerable<ModifierRuntimeSpec> modifiers, ModifierRuntimeSpec modifier)
    {
        var direction = ModifierDirection(modifier);
        if (direction == 0)
        {
            return false;
        }

        return modifiers.Any(other =>
            other.StatId == modifier.StatId
            && other.TargetTag == modifier.TargetTag
            && ModifierDirection(other) == -direction);
    }

    private static int ModifierDirection(ModifierRuntimeSpec modifier)
    {
        var operation = Enum.TryParse<ModifierOp>(modifier.Operation, out var parsed)
            ? parsed
            : ModifierOp.FlatAdd;

        if (operation == ModifierOp.FlatAdd)
        {
            return modifier.Value > 0f ? 1 : modifier.Value < 0f ? -1 : 0;
        }

        if (operation == ModifierOp.Multiplicative)
        {
            return modifier.Value > 1f ? 1 : modifier.Value < 1f ? -1 : 0;
        }

        return 0;
    }

    private IEnumerable<SpellModifierRuntimeCheck> BuildSpellModifierChecks(RunContext context, ModifierRuntimeSpec modifier)
    {
        foreach (var spell in EnumerateSpellsMatchingTarget(context, modifier.TargetTag))
        {
            yield return new SpellModifierRuntimeCheck(
                modifier,
                spell.Id,
                spell.Tags,
                ResolveModifierValueForTags(context, modifier, spell.Tags));
        }
    }

    private float ResolveModifierValue(RunContext context, ModifierRuntimeSpec modifier)
    {
        return ResolveModifierValueForTags(context, modifier, TagsForModifier(modifier));
    }

    private float ResolveModifierValueForTags(RunContext context, ModifierRuntimeSpec modifier, IReadOnlyCollection<TagId> tags)
    {
        var stat = modifier.StatId;
        if (stat.Equals(PlayerAttributeResolver.Damage))
        {
            return PlayerAttributeResolver.ResolveDamage(context, 10f, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.FireRate))
        {
            return PlayerAttributeResolver.ResolveFireRate(context, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.MoveSpeed))
        {
            return PlayerAttributeResolver.ResolveValue(
                context,
                PlayerAttributeResolver.MoveSpeed,
                context.State.Player.BaseMoveSpeed,
                tags,
                false);
        }

        if (stat.Equals(PlayerAttributeResolver.Health))
        {
            return PlayerAttributeResolver.ResolveValue(
                context,
                PlayerAttributeResolver.Health,
                context.State.Player.BaseMaxHealth,
                tags,
                false);
        }

        if (stat.Equals(PlayerAttributeResolver.Range))
        {
            return PlayerAttributeResolver.ResolveRange(context, 10f, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.Duration))
        {
            return PlayerAttributeResolver.ResolveDuration(context, 5f, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.AreaRadius))
        {
            return PlayerAttributeResolver.ResolveAreaRadius(context, 3f, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.ProjectileSpeed))
        {
            return PlayerAttributeResolver.ResolveProjectileSpeed(context, 18f, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.Pierce))
        {
            return PlayerAttributeResolver.ResolveProjectilePierce(context, tags, false) ? 1f : 0f;
        }

        if (stat.Equals(PlayerAttributeResolver.SpellCount))
        {
            return PlayerAttributeResolver.ResolveAdditionalSpellCount(context, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.ProjectileSplit))
        {
            return PlayerAttributeResolver.ResolveProjectileSplitDepth(context, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.HomingStrength))
        {
            return PlayerAttributeResolver.ResolveHomingStrength(context, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.HomingAngleDegrees))
        {
            return PlayerAttributeResolver.ResolveValue(context, stat, 0f, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.HomingAcquireRadius))
        {
            return PlayerAttributeResolver.ResolveValue(context, stat, 0f, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.SpellOrbitStrength))
        {
            return PlayerAttributeResolver.ResolveSpellOrbitStrength(context, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.SpellOrbitRadius))
        {
            return PlayerAttributeResolver.ResolveValue(context, stat, 0f, tags, false);
        }

        if (stat.Equals(PlayerAttributeResolver.SpellOrbitAngularSpeed))
        {
            return PlayerAttributeResolver.ResolveValue(context, stat, 0f, tags, false);
        }

        return PlayerAttributeResolver.ResolveValue(context, stat, 10f, tags, false);
    }

    private IEnumerable<SpellRuntimeDefinition> EnumerateSpellsMatchingTarget(RunContext context, TagId targetTag)
    {
        return context.Content.Spells.Values
            .Where(spell => GameplayTagPath.MatchesAny(targetTag, spell.Tags))
            .OrderBy(spell => spell.Id.Value, StringComparer.Ordinal);
    }

    private static IReadOnlyCollection<TagId> TagsForModifier(ModifierRuntimeSpec modifier)
    {
        return modifier.TargetTag.IsValid ? new[] { modifier.TargetTag } : Array.Empty<TagId>();
    }

    private async Task AssertProductionSpellCount(ContentId itemId, ItemEffectRuntimeSpec effect)
    {
        var context = RequireContext();
        var expectedExtra = Math.Clamp((int)MathF.Floor(MathF.Max(0f, effect.Value) + 0.001f), 0, 12);
        var extra = PlayerAttributeResolver.ResolveAdditionalSpellCount(context, Array.Empty<TagId>(), false);
        Assert(extra >= expectedExtra, $"Production item '{itemId}' did not resolve expected extra spell count. Expected>={expectedExtra}, observed={extra}.");
        Events.Clear();
        context.State.Spellbook.AssignSlot(0, ContentId.From("fireball"));
        context.State.Spellbook.SelectSlot(0);
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var reason),
            $"Production item '{itemId}' could not cast Fireball for spell-count validation: {reason}");
        await WaitSeconds(0.05f);
        Assert(Events.ProjectileCount(ContentId.From("fireball")) >= 1 + expectedExtra, $"Production item '{itemId}' did not spawn expected repeated Fireballs.");
    }

    private async Task AssertProductionProjectilePierce(ContentId itemId, TagId targetTag)
    {
        var context = RequireContext();
        var projectileSpells = EnumerateSpellsMatchingTarget(context, targetTag)
            .Where(HasProjectileRadiusOutput)
            .Select(spell => spell.Id)
            .ToArray();
        Assert(projectileSpells.Length > 0, $"Production item '{itemId}' projectile pierce target matched no projectile spells.");

        foreach (var spellId in projectileSpells)
        {
            context = StartFreshScenarioRun();
            AddItem(itemId);
            var tags = context.Content.Spells[spellId].Tags;
            Assert(PlayerAttributeResolver.ResolveProjectilePierce(context, tags, false), $"Production item '{itemId}' did not enable projectile pierce for spell '{spellId}'.");
            Events.Clear();
            context.State.Spellbook.AssignSlot(0, spellId);
            context.State.Spellbook.SelectSlot(0);
            var enemies = context.GetSystem<EnemySystem>();
            var first = enemies.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(0f, 0f, -4f)));
            var second = enemies.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(0f, 0f, -6f)));
            Assert(first.IsValid && second.IsValid, $"Production item '{itemId}' could not spawn projectile pierce targets for spell '{spellId}'.");
            Assert(
                context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var reason),
                $"Production item '{itemId}' could not cast {spellId} for pierce validation: {reason}");
            await WaitSeconds(0.5f);
            Assert(Events.LastDamageToTarget(first) > 0f && Events.LastDamageToTarget(second) > 0f, $"Production item '{itemId}' projectile spell '{spellId}' did not pierce through two enemies.");
        }
    }

    private async Task AssertProductionProjectileSplit(ContentId itemId)
    {
        var context = RequireContext();
        Assert(PlayerAttributeResolver.ResolveProjectileSplitDepth(context, new[] { TagId.From("spell.delivery.projectile") }, false) > 0, $"Production item '{itemId}' did not enable projectile split depth.");
        Events.Clear();
        context.State.Spellbook.AssignSlot(0, ContentId.From("fireball"));
        context.State.Spellbook.SelectSlot(0);
        var target = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(0f, 0f, -4f)));
        Assert(target.IsValid, $"Production item '{itemId}' could not spawn projectile split target.");
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var reason),
            $"Production item '{itemId}' could not cast Fireball for split validation: {reason}");
        await WaitSeconds(0.35f);
        Assert(context.GetSystem<ProjectileSystem>().ActiveProjectiles.Count > 1, $"Production item '{itemId}' did not leave split child projectiles active after impact.");
    }

    private async Task AssertProductionSpellRadiusMatrix(ContentId itemId, TagId targetTag, RadiusExpectation expectation)
    {
        var context = RequireContext();
        Assert(
            context.Content.GetItem(itemId).Modifiers.Any(modifier => modifier.StatId.Equals(PlayerAttributeResolver.AreaRadius)),
            $"Production item '{itemId}' radius effect did not compile to a radius modifier.");

        var spellIds = EnumerateSpellsMatchingTarget(context, targetTag)
            .Where(HasMeasurableRadiusOutput)
            .Select(spell => spell.Id)
            .ToArray();
        Assert(spellIds.Length > 0, $"Production item '{itemId}' radius target '{targetTag}' matched no measurable spells.");

        foreach (var spellId in spellIds)
        {
            var before = await MeasureSpellRadiusOutput(spellId, itemId, grantItem: false);
            var after = await MeasureSpellRadiusOutput(spellId, itemId, grantItem: true);
            Report($"PRODUCTION SPELL RADIUS item={itemId} spell={spellId} before={before:0.###} after={after:0.###} expectation={expectation}");
            if (expectation == RadiusExpectation.Increase)
            {
                Assert(after > before + 0.001f, $"Production item '{itemId}' did not increase spell '{spellId}' radius. Before={before:0.###}, after={after:0.###}.");
            }
            else
            {
                Assert(after < before - 0.001f, $"Production item '{itemId}' did not decrease spell '{spellId}' radius. Before={before:0.###}, after={after:0.###}.");
            }
        }
    }

    private async Task<float> MeasureSpellRadiusOutput(ContentId spellId, ContentId itemId, bool grantItem)
    {
        var context = StartFreshScenarioRun();
        if (grantItem)
        {
            AddItem(itemId);
        }

        var spell = context.Content.Spells[spellId];
        var effect = spell.Effects.FirstOrDefault();
        Assert(!string.IsNullOrWhiteSpace(effect.EffectType), $"Spell '{spellId}' has no effect to measure radius.");

        if (string.Equals(effect.EffectType, "projectile_damage", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "frost_projectile_damage", StringComparison.OrdinalIgnoreCase))
        {
            Events.Clear();
            context.State.Spellbook.AssignSlot(0, spellId);
            context.State.Spellbook.SelectSlot(0);
            Assert(
                context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var reason),
                $"Could not cast {spellId} for radius measurement: {reason}");
            await WaitSeconds(0.05f);
            return LastProjectile(spellId).Radius;
        }

        if (string.Equals(effect.EffectType, "hitscan_beam_damage", StringComparison.OrdinalIgnoreCase))
        {
            Events.Clear();
            context.State.Spellbook.AssignSlot(0, spellId);
            context.State.Spellbook.SelectSlot(0);
            Assert(
                context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var reason),
                $"Could not cast {spellId} for radius measurement: {reason}");
            await WaitSeconds(0.05f);
            return Events.LastBeamRadius(spellId);
        }

        if (string.Equals(effect.EffectType, "firewall_hazard", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "blizzard_aoe", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "tornado_vortex", StringComparison.OrdinalIgnoreCase))
        {
            Events.Clear();
            context.State.Spellbook.AssignSlot(0, spellId);
            context.State.Spellbook.SelectSlot(0);
            Assert(
                context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var reason),
                $"Could not cast {spellId} for radius measurement: {reason}");
            await WaitSeconds(0.05f);
            var area = LastArea(spellId);
            return area.IsWall ? area.WallHalfLength : area.Radius;
        }

        if (string.Equals(effect.EffectType, "raise_dead_summon", StringComparison.OrdinalIgnoreCase))
        {
            Assert(
                SpellPlacement.TryBuildPreview(context, spell, _scenarioOrigin, new Vector3(0f, 0f, -1f), out var preview),
                $"Could not build Raise Dead preview for radius measurement.");
            return preview.Radius;
        }

        Assert(false, $"Spell '{spellId}' effect '{effect.EffectType}' has no radius measurement path.");
        return 0f;
    }

    private static bool HasMeasurableRadiusOutput(SpellRuntimeDefinition spell)
    {
        return spell.Effects.Any(effect =>
            string.Equals(effect.EffectType, "projectile_damage", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "frost_projectile_damage", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "hitscan_beam_damage", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "firewall_hazard", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "blizzard_aoe", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "tornado_vortex", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "raise_dead_summon", StringComparison.OrdinalIgnoreCase));
    }

    private static bool HasProjectileRadiusOutput(SpellRuntimeDefinition spell)
    {
        return spell.Effects.Any(effect =>
            string.Equals(effect.EffectType, "projectile_damage", StringComparison.OrdinalIgnoreCase)
            || string.Equals(effect.EffectType, "frost_projectile_damage", StringComparison.OrdinalIgnoreCase));
    }

    private async Task AssertProductionHomingSpells(ContentId itemId)
    {
        var context = StartFreshScenarioRun();
        AddItem(itemId);
        Assert(
            PlayerAttributeResolver.ResolveHomingStrength(context, new[] { TagId.From("spell.delivery.projectile") }, false) > 0f,
            $"Production item '{itemId}' did not resolve projectile homing strength.");
        Assert(
            PlayerAttributeResolver.ResolveHomingStrength(context, new[] { TagId.From("spell.delivery.beam") }, false) > 0f,
            $"Production item '{itemId}' did not resolve beam homing strength.");

        Events.Clear();
        context.State.Spellbook.AssignSlot(0, ContentId.From("fireball"));
        context.State.Spellbook.SelectSlot(0);
        var projectileTarget = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(4f, 0f, -8f)));
        Assert(projectileTarget.IsValid, $"Production item '{itemId}' could not spawn projectile homing target.");
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var projectileReason),
            $"Production item '{itemId}' could not cast Fireball for homing validation: {projectileReason}");
        await WaitSeconds(0.75f);
        Assert(Events.LastDamageToTarget(projectileTarget) > 0f, $"Production item '{itemId}' homing projectile did not bend into an off-axis target.");

        context = StartFreshScenarioRun();
        AddItem(itemId);
        Events.Clear();
        context.State.Spellbook.AssignSlot(0, ContentId.From("arcane_beam"));
        context.State.Spellbook.SelectSlot(0);
        var beamTarget = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(4f, 0f, -10f)));
        Assert(beamTarget.IsValid, $"Production item '{itemId}' could not spawn beam homing target.");
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var beamReason),
            $"Production item '{itemId}' could not cast Arcane Beam for homing validation: {beamReason}");
        await WaitSeconds(0.05f);
        Assert(Events.BeamCount(ContentId.From("arcane_beam")) > 1, $"Production item '{itemId}' homing beam did not emit bent beam segments.");
        Assert(Events.LastDamageToTarget(beamTarget) > 0f, $"Production item '{itemId}' homing beam did not bend into an off-axis target.");
    }

    private async Task AssertProductionOrbitingSpells(ContentId itemId)
    {
        var context = StartFreshScenarioRun();
        AddItem(itemId);
        Assert(
            PlayerAttributeResolver.ResolveSpellOrbitStrength(context, new[] { TagId.From("spell.delivery.projectile") }, false) > 0f,
            $"Production item '{itemId}' did not resolve projectile orbit strength.");
        Assert(
            PlayerAttributeResolver.ResolveSpellOrbitStrength(context, new[] { TagId.From("spell.delivery.area") }, false) > 0f,
            $"Production item '{itemId}' did not resolve area orbit strength.");
        Assert(
            PlayerAttributeResolver.ResolveSpellOrbitStrength(context, new[] { TagId.From("spell.delivery.beam") }, false) > 0f,
            $"Production item '{itemId}' did not resolve beam orbit strength.");

        Events.Clear();
        context.State.Spellbook.AssignSlot(0, ContentId.From("fireball"));
        context.State.Spellbook.SelectSlot(0);
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var projectileReason),
            $"Production item '{itemId}' could not cast Fireball for orbit validation: {projectileReason}");
        await WaitSeconds(0.05f);
        Assert(
            context.GetSystem<ProjectileSystem>().ActiveProjectiles.Any(projectile => projectile.Orbit.Enabled),
            $"Production item '{itemId}' did not mark Fireball projectiles as orbiting.");

        context = StartFreshScenarioRun();
        AddItem(itemId);
        Events.Clear();
        context.State.Spellbook.AssignSlot(0, ContentId.From("blizzard"));
        context.State.Spellbook.SelectSlot(0);
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var areaReason),
            $"Production item '{itemId}' could not cast Blizzard for orbit validation: {areaReason}");
        await WaitSeconds(0.05f);
        Assert(
            context.GetSystem<AoESystem>().ActiveAreas.Any(area => area.Orbit.Enabled),
            $"Production item '{itemId}' did not mark Blizzard areas as orbiting.");

        context = StartFreshScenarioRun();
        AddItem(itemId);
        Events.Clear();
        context.State.Spellbook.AssignSlot(0, ContentId.From("arcane_beam"));
        context.State.Spellbook.SelectSlot(0);
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var beamReason),
            $"Production item '{itemId}' could not cast Arcane Beam for orbit validation: {beamReason}");
        await WaitSeconds(0.05f);
        Assert(Events.BeamCount(ContentId.From("arcane_beam")) > 1, $"Production item '{itemId}' did not emit orbital beam segments.");
    }

    private async Task AssertProductionKeepItIn(ContentId itemId, ItemEffectRuntimeSpec effect)
    {
        var context = RequireContext();
        var spellId = ContentId.From("fireball");
        context.State.Spellbook.AssignSlot(0, spellId);
        context.State.Spellbook.SelectSlot(0);
        Events.Clear();
        var direct = new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f));
        Assert(!context.Commands.TryExecute(direct, context, out var directReason), $"Production item '{itemId}' KeepItIn did not block direct casting.");
        Assert(directReason.Contains("requires charge", StringComparison.OrdinalIgnoreCase), $"Production item '{itemId}' KeepItIn produced unexpected direct-cast reason: {directReason}");
        Assert(
            context.Commands.TryExecute(new BeginCastFlowCommand(context.State.Player.EntityId, 0), context, out var beginReason),
            $"Production item '{itemId}' could not begin KeepItIn charge: {beginReason}");
        await WaitSeconds(context.State.Spellbook.CastFlow.RequiredChargeSeconds + 0.08f);
        Assert(
            context.Commands.TryExecute(new ReleaseCastFlowCommand(_scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var releaseReason),
            $"Production item '{itemId}' could not release KeepItIn charge: {releaseReason}");
        await WaitSeconds(0.05f);
        var expectedProjectiles = 1 + Math.Clamp((int)MathF.Floor(MathF.Max(1f, effect.Value) + 0.001f), 1, 12);
        Assert(Events.ProjectileCount(spellId) >= expectedProjectiles, $"Production item '{itemId}' KeepItIn released too few Fireballs. Expected>={expectedProjectiles}, observed={Events.ProjectileCount(spellId)}.");
    }

    private async Task AssertProductionAbyssalRing(ContentId itemId, ItemEffectRuntimeSpec effect)
    {
        var context = RequireContext();
        var spellId = ContentId.From("fireball");
        context.State.Spellbook.AssignSlot(0, spellId);
        context.State.Spellbook.SelectSlot(0);
        Events.Clear();
        Assert(
            context.Commands.TryExecute(new BeginCastFlowCommand(context.State.Player.EntityId, 0), context, out var beginReason),
            $"Production item '{itemId}' could not begin AbyssalRing charge: {beginReason}");
        Assert(context.State.Spellbook.CastFlow.AllowsConcurrentCasting, $"Production item '{itemId}' AbyssalRing should allow concurrent casting.");
        var target = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(4f, 0f, 0f)));
        Assert(target.IsValid, $"Production item '{itemId}' could not spawn AbyssalRing target.");
        await WaitSeconds(context.State.Spellbook.CastFlow.RequiredChargeSeconds + 0.08f);
        Assert(
            context.Commands.TryExecute(new ReleaseCastFlowCommand(_scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var releaseReason),
            $"Production item '{itemId}' could not release AbyssalRing charge: {releaseReason}");
        await WaitSeconds(0.75f);
        Assert(Events.AoECount(itemId) > 0, $"Production item '{itemId}' AbyssalRing did not spawn an AoE.");
        Assert(Events.DamageBySource(itemId) > 0f, $"Production item '{itemId}' AbyssalRing did not damage through CombatSystem.");
    }

    private async Task AssertProductionFaultyFocus(ContentId itemId, ItemEffectRuntimeSpec effect)
    {
        var context = RequireContext();
        var spellId = ContentId.From("fireball");
        context.State.Spellbook.AssignSlot(0, spellId);
        context.State.Spellbook.SelectSlot(0);
        Events.Clear();
        var target = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(0f, 0f, -4f)));
        Assert(target.IsValid, $"Production item '{itemId}' could not spawn FaultyFocus target.");
        Assert(
            context.Commands.TryExecute(new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f)), context, out var reason),
            $"Production item '{itemId}' could not cast Fireball for FaultyFocus validation: {reason}");
        await WaitSeconds(0.35f);
        Assert(Events.ItemProcCount(itemId, "faulty_focus_split_spell") > 0, $"Production item '{itemId}' FaultyFocus did not trigger from a real spell hit.");
        Assert(Events.ProjectileCount(spellId) >= 4, $"Production item '{itemId}' FaultyFocus did not spawn child spell projectiles.");
        var expectedMultiplier = effect.Value <= 0f ? 0.5f : effect.Value;
        Assert(
            context.GetSystem<ProjectileSystem>().ActiveProjectiles.Any(projectile => projectile.SpellChainGeneration == 1 && Math.Abs(projectile.DamageMultiplier - expectedMultiplier) < 0.001f),
            $"Production item '{itemId}' FaultyFocus child projectiles did not carry expected generation and damage multiplier.");
    }

    private async Task AssertProductionDamagePrevention(ContentId itemId, ItemEffectRuntimeSpec effect)
    {
        await Task.CompletedTask;
        var context = RequireContext();
        var combat = context.GetSystem<CombatSystem>();
        Assert(combat.TryGetHealth(context.State.Player.EntityId, out var health) && health != null, $"Production item '{itemId}' could not read player health.");
        var playerHealth = health ?? throw new InvalidOperationException("Player health missing after assertion.");
        var expectedProtectedHits = Math.Clamp((int)MathF.Floor(MathF.Max(1f, effect.Value) + 0.001f), 1, 99);
        Events.Clear();

        for (var i = 0; i < expectedProtectedHits; i++)
        {
            var before = playerHealth.CurrentHealth;
            var result = combat.ApplyDamage(new DamageRequest(
                EntityId.None,
                context.State.Player.EntityId,
                ContentId.From("regression_incoming_damage"),
                9f,
                DamageType.Physical,
                new HitContext(context.State.Player.Position, Vector3.Up, EntityId.None)));
            Assert(result.AppliedAmount <= 0f, $"Production item '{itemId}' protected hit {i + 1} still applied damage {result.AppliedAmount:0.###}.");
            Assert(Math.Abs(playerHealth.CurrentHealth - before) < 0.001f, $"Production item '{itemId}' protected hit {i + 1} changed player health.");
        }

        Assert(Events.DamagePreventionCount(itemId) >= expectedProtectedHits, $"Production item '{itemId}' did not emit expected damage prevention events.");
        var afterProtectedHits = playerHealth.CurrentHealth;
        var unprotected = combat.ApplyDamage(new DamageRequest(
            EntityId.None,
            context.State.Player.EntityId,
            ContentId.From("regression_incoming_damage"),
            9f,
            DamageType.Physical,
            new HitContext(context.State.Player.Position, Vector3.Up, EntityId.None)));
        Assert(unprotected.AppliedAmount > 0f, $"Production item '{itemId}' did not consume its damage prevention charge.");
        Assert(playerHealth.CurrentHealth < afterProtectedHits, $"Production item '{itemId}' left player health unchanged after its protection was consumed.");
    }

    private async Task AssertProductionPersistentSummons(ContentId itemId)
    {
        var context = RequireContext();
        var summon = await SpawnSummonFromCorpse();
        Assert(
            float.IsPositiveInfinity(summon.LifetimeRemaining),
            $"Production item '{itemId}' did not make raised summons persistent. Lifetime={summon.LifetimeRemaining:0.###}.");
        Assert(context.GetSystem<CombatSystem>().IsAlive(summon.EntityId), $"Production item '{itemId}' persistent summon was not alive after spawning.");
    }

    private void AssertAllDesignerEffectsCompile()
    {
        foreach (var kind in Enum.GetValues<ItemEffectKind>())
        {
            var effect = new ItemEffectSpec
            {
                Kind = kind,
                StatId = ItemEffectCompiler.RequiresFreeStat(kind) ? "damage" : string.Empty,
                Value = kind is ItemEffectKind.RemoveMultiplyStat ? 0.1f : 1.2f,
                SecondaryValue = 0.5f,
                ChancePercent = 35f,
                TargetTag = kind is ItemEffectKind.EnemiesExplodeOnDeath or ItemEffectKind.BurningGroundOnEnemyDeath ? string.Empty : "spell.delivery.projectile"
            };
            var compiled = ItemEffectCompiler.Compile(effect);
            Report($"EFFECT COMPILE {kind}: {ItemEffectCompiler.DescribeCompiled(effect)}");
            Assert(compiled.Count > 0 || kind is ItemEffectKind.KeepItIn or ItemEffectKind.AbyssalRing or ItemEffectKind.FaultyFocus or ItemEffectKind.PreventNextPlayerHealthLoss or ItemEffectKind.PersistentSummons, $"Designer effect '{kind}' did not compile to runtime output.");
            if (kind == ItemEffectKind.RemoveMultiplyStat)
            {
                Assert(
                    Math.Abs(compiled[0].Value - 0.9f) < 0.001f,
                    $"RemoveMultiplyStat should treat 0.1 as a 10% reduction and compile to multiplier 0.9, got {compiled[0].Value:0.###}.");
            }
            else if (kind == ItemEffectKind.AddFlatStat)
            {
                Assert(
                    compiled[0].Operation == DataModifierOp.FlatAdd && Math.Abs(compiled[0].Value - 1.2f) < 0.001f,
                    $"AddFlatStat should compile to a flat add modifier preserving value 1.2, got {compiled[0].Operation} {compiled[0].Value:0.###}.");
            }
            else if (kind == ItemEffectKind.RemoveFlatStat)
            {
                Assert(
                    compiled[0].Operation == DataModifierOp.FlatAdd && Math.Abs(compiled[0].Value + 1.2f) < 0.001f,
                    $"RemoveFlatStat should compile to a negative flat add modifier from authored value 1.2, got {compiled[0].Operation} {compiled[0].Value:0.###}.");
            }
            else if (kind == ItemEffectKind.AddProjectilePierce)
            {
                Assert(
                    Math.Abs(compiled[0].Value - 1f) < 0.001f,
                    $"AddProjectilePierce should compile to an enabled pierce flag, got {compiled[0].Value:0.###}.");
            }
        }
    }

    private async Task AssertAdditionalProjectileRuntime()
    {
        var context = StartFreshScenarioRun();
        AddItem(ContentId.From(RegressionProjectileItem));
        Events.Clear();
        await CastSpell(ContentId.From("fireball"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.05f);
        Assert(Events.ProjectileCount(ContentId.From("fireball")) >= 3, "Designer spell count item did not spawn three Fireball instances.");
        Assert(context.GetSystem<ProjectileSystem>().ActiveProjectiles.Count >= 3, "Repeated Fireball instances were not active in ProjectileSystem.");
        var fireballProjectiles = context.GetSystem<ProjectileSystem>().ActiveProjectiles
            .Where(projectile => projectile.SpellId.Equals(ContentId.From("fireball")))
            .ToArray();
        var baseDirection = new Vector3(0f, 0f, -1f);
        for (var i = 0; i < fireballProjectiles.Length; i++)
        {
            var directionDot = fireballProjectiles[i].Velocity.Normalized().Dot(baseDirection);
            Assert(directionDot > 0.999f, $"Repeated Fireball lane {i} angled away from the cast direction. Dot={directionDot:0.####}.");
        }

        var right = Vector3.Up.Cross(baseDirection).Normalized();
        var lanePositions = fireballProjectiles
            .Select(projectile => projectile.Position.Dot(right))
            .OrderBy(value => value)
            .ToArray();
        for (var i = 1; i < Math.Min(3, lanePositions.Length); i++)
        {
            var spacing = lanePositions[i] - lanePositions[i - 1];
            Assert(spacing > 0.8f, $"Repeated Fireball lanes were not spaced apart enough. Spacing={spacing:0.###}.");
        }

        Events.Clear();
        await CastSpell(ContentId.From("firewall"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.1f);
        Assert(Events.AoECount(ContentId.From("firewall")) >= 3, "Designer spell count item did not spawn three Firewall instances.");

        Events.Clear();
        await CastSpell(ContentId.From("tornado"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.1f);
        Assert(Events.AoECount(ContentId.From("tornado")) >= 3, "Designer spell count item did not spawn three Tornado instances.");
    }

    private async Task AssertFreezeAndImmolateRuntime()
    {
        var context = StartFreshScenarioRun();
        var enemySystem = context.GetSystem<EnemySystem>();
        var statusSystem = context.GetSystem<StatusEffectSystem>();
        var freezeTarget = enemySystem.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(0f, 0f, -4f)));
        var immolateTarget = enemySystem.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(1f, 0f, -4f)));
        Assert(freezeTarget.IsValid && immolateTarget.IsValid, "Could not spawn status targets.");

        statusSystem.Apply(freezeTarget, StatusEffectRules.Freeze, ContentId.From("regression_freeze"), context.State.Player.EntityId);
        statusSystem.Apply(immolateTarget, StatusEffectRules.Immolate, ContentId.From("regression_immolate"), context.State.Player.EntityId);
        await WaitSeconds(0.05f);
        Assert(Events.StatusCount("freeze") > 0, "Freeze status did not publish.");
        Assert(Events.StatusCount("immolate") > 0, "Immolate status did not publish.");
        Assert(statusSystem.GetMoveSpeedMultiplier(freezeTarget) <= 0.1f, "Freeze did not stop enemy movement.");
    }

    private async Task AssertSprint11NumericItems()
    {
        var context = StartFreshScenarioRun();
        var tags = Array.Empty<TagId>();
        var baseDamage = PlayerAttributeResolver.ResolveDamage(context, 10f, tags, false);
        AddItem(ContentId.From(RegressionDamageItem));
        var boostedDamage = PlayerAttributeResolver.ResolveDamage(context, 10f, tags, false);
        Assert(boostedDamage >= baseDamage + 3.9f, $"Designer damage item did not increase damage. Before={baseDamage:0.###}, after={boostedDamage:0.###}.");

        var projectileTags = new[] { TagId.From("spell.delivery.projectile") };
        var baseProjectileSpeed = PlayerAttributeResolver.ResolveProjectileSpeed(context, 18f, projectileTags, false);
        AddItem(ContentId.From(RegressionProjectileItem));
        var boostedProjectileSpeed = PlayerAttributeResolver.ResolveProjectileSpeed(context, 18f, projectileTags, false);
        var additionalSpellCount = PlayerAttributeResolver.ResolveAdditionalSpellCount(context, tags, false);
        var projectilePierce = PlayerAttributeResolver.ResolveProjectilePierce(context, projectileTags, false);
        Assert(boostedProjectileSpeed > baseProjectileSpeed * 1.14f, $"Designer projectile item did not increase projectile speed. Before={baseProjectileSpeed:0.###}, after={boostedProjectileSpeed:0.###}.");
        Assert(additionalSpellCount >= 2, "Designer projectile item did not add two extra spell instances.");
        Assert(projectilePierce, "Designer projectile pierce should make projectile spells pierce indefinitely.");
        await WaitSeconds(0.05f);
    }

    private async Task AssertSprint11OnHitProcItems()
    {
        await AssertHitProc(
            ContentId.From(RegressionStatusItem),
            ContentId.From("frost_bolt"),
            DamageType.Frost,
            "apply_freeze",
            () => Assert(Events.StatusCountFromSource("freeze", ContentId.From(RegressionStatusItem)) > 0, "Designer status item did not apply freeze."));
        await AssertHitProc(
            ContentId.From(RegressionFireStatusItem),
            ContentId.From("fireball"),
            DamageType.Fire,
            "apply_burn",
            () => Assert(Events.StatusCountFromSource("burn", ContentId.From(RegressionFireStatusItem)) > 0, "Designer fire immolate item did not apply burn."));
        await AssertItemSourcedFireCanApplyBurnThroughImmolateItem();
    }

    private async Task AssertItemSourcedFireCanApplyBurnThroughImmolateItem()
    {
        var context = StartFreshScenarioRun();
        AddItem(ContentId.From(RegressionFireStatusItem));
        AddItem(ContentId.From(RegressionBurningGroundItem));
        var combat = context.GetSystem<CombatSystem>();
        var enemies = context.GetSystem<EnemySystem>();
        var primaryPosition = ScenarioPoint(new Vector3(0f, 0f, -3f));
        var primary = enemies.Spawn(ContentId.From("human_grunt"), primaryPosition);
        var secondary = enemies.Spawn(ContentId.From("human_grunt"), primaryPosition + new Vector3(0.6f, 0f, -0.15f));
        Assert(primary.IsValid && secondary.IsValid, "Could not spawn enemies for item-sourced fire immolate regression.");

        combat.ApplyDamage(new DamageRequest(
            context.State.Player.EntityId,
            primary,
            ContentId.From("fireball"),
            999f,
            DamageType.Fire,
            new HitContext(primaryPosition, Vector3.Up, context.State.Player.EntityId)));
        await WaitSeconds(0.8f);

        Assert(Events.AoECount(ContentId.From(RegressionBurningGroundItem)) > 0, "Regression burning ground item did not spawn a fire ground AoE.");
        Assert(Events.DamageBySource(ContentId.From(RegressionBurningGroundItem)) > 0f, "Regression burning ground item did not deal item-sourced fire damage.");
        Assert(Events.StatusCountFromSource("burn", ContentId.From(RegressionFireStatusItem)) > 0, "Item-sourced fire damage did not apply burn through the immolate item.");
    }

    private async Task AssertSprint11OnKillProcItems()
    {
        await AssertKillProcAoE(
            ContentId.From(RegressionKillItem),
            ContentId.From("fireball"),
            DamageType.Fire,
            "kill_explosion",
            "Designer kill item did not spawn a kill explosion.");

        var context = StartFreshScenarioRun();
        AddItem(ContentId.From(RegressionKillItem));
        Events.Clear();
        var enemies = context.GetSystem<EnemySystem>();
        var combat = context.GetSystem<CombatSystem>();
        var first = enemies.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(0f, 0f, -3f)));
        var second = enemies.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(0.6f, 0f, -3.1f)));
        Assert(first.IsValid && second.IsValid, "Could not spawn enemies for proc validation.");
        combat.ApplyDamage(new DamageRequest(
            context.State.Player.EntityId,
            first,
            ContentId.From("fireball"),
            999f,
            DamageType.Fire,
            new HitContext(ScenarioPoint(new Vector3(0f, 0f, -3f)), Vector3.Up, context.State.Player.EntityId)));
        await WaitSeconds(0.25f);
        Assert(Events.ItemProcTriggers > 0, "Designer kill item did not trigger an item proc on kill.");
        Assert(Events.DamageBySource(ContentId.From(RegressionKillItem)) > 0f, "Designer kill item proc did not deal damage through CombatSystem.");
        Assert(context.State.DebugMetrics.LastProcSummary.Contains(RegressionKillItem, StringComparison.Ordinal), "Debug metrics did not record the last proc.");

        var procSystem = context.GetSystem<ProcSystem>();
        procSystem.PerFrameProcBudget = 0;
        var blockedEnemy = enemies.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(1.5f, 0f, -3f)));
        combat.ApplyDamage(new DamageRequest(
            context.State.Player.EntityId,
            blockedEnemy,
            ContentId.From("fireball"),
            999f,
            DamageType.Fire,
            new HitContext(ScenarioPoint(new Vector3(1.5f, 0f, -3f)), Vector3.Up, context.State.Player.EntityId)));
        await WaitSeconds(0.05f);
        Assert(Events.ItemProcBlocks > 0, "Proc budget block was not reported.");
    }

    private async Task AssertHitProc(
        ContentId itemId,
        ContentId sourceId,
        DamageType damageType,
        string expectedEffect,
        Action postAssert)
    {
        var context = StartFreshScenarioRun();
        AddItem(itemId);
        for (var stack = 1; stack < 8; stack++)
        {
            AddItem(itemId);
        }

        var combat = context.GetSystem<CombatSystem>();
        var enemies = context.GetSystem<EnemySystem>();
        for (var attempt = 0; attempt < 80; attempt++)
        {
            Events.Clear();
            var enemy = enemies.Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(attempt * 0.25f, 0f, -8f)));
            Assert(enemy.IsValid, $"Could not spawn enemy for {itemId} hit proc attempt {attempt}.");
            combat.ApplyDamage(new DamageRequest(
                context.State.Player.EntityId,
                enemy,
                sourceId,
                1f,
                damageType,
                new HitContext(ScenarioPoint(new Vector3(attempt * 0.25f, 0f, -8f)), Vector3.Up, context.State.Player.EntityId)));
            await WaitSeconds(0.02f);
            if (Events.ItemProcCount(itemId, expectedEffect) <= 0)
            {
                continue;
            }

            postAssert();
            if (string.Equals(expectedEffect, "apply_freeze", StringComparison.Ordinal)
                || string.Equals(expectedEffect, "apply_burn", StringComparison.Ordinal)
                || string.Equals(expectedEffect, "apply_immolate", StringComparison.Ordinal))
            {
                var statusSystem = context.GetSystem<StatusEffectSystem>();
                var status = statusSystem.ActiveEffects.FirstOrDefault(effect => effect.SourceId.Equals(itemId));
                Assert(status != null, $"{itemId} proc did not create a status sourced from the item.");
                if (status == null)
                {
                    return;
                }

                Assert(status.RemainingSeconds > 1.5f, $"{itemId} status proc did not preserve authored duration. Remaining={status.RemainingSeconds:0.###}.");
            }

            return;
        }

        Assert(false, $"{itemId} did not trigger expected proc '{expectedEffect}' in 80 deterministic hit attempts.");
    }

    private async Task AssertKillProcAoE(
        ContentId itemId,
        ContentId sourceId,
        DamageType damageType,
        string expectedEffect,
        string failureMessage)
    {
        var context = StartFreshScenarioRun();
        AddItem(itemId);
        var combat = context.GetSystem<CombatSystem>();
        var enemies = context.GetSystem<EnemySystem>();
        for (var attempt = 0; attempt < 80; attempt++)
        {
            Events.Clear();
            var primaryPosition = ScenarioPoint(new Vector3(attempt * 0.35f, 0f, -3f));
            var primary = enemies.Spawn(ContentId.From("human_grunt"), primaryPosition);
            var secondary = enemies.Spawn(ContentId.From("human_grunt"), primaryPosition + new Vector3(0.6f, 0f, -0.15f));
            Assert(primary.IsValid && secondary.IsValid, $"Could not spawn enemies for {itemId} kill proc attempt {attempt}.");
            combat.ApplyDamage(new DamageRequest(
                context.State.Player.EntityId,
                primary,
                sourceId,
                999f,
                damageType,
                new HitContext(primaryPosition, Vector3.Up, context.State.Player.EntityId)));
            await WaitSeconds(0.08f);
            if (Events.ItemProcCount(itemId, expectedEffect) <= 0)
            {
                continue;
            }

            Assert(Events.AoECount(itemId) > 0, failureMessage);
            Assert(Events.DamageBySource(itemId) > 0f, $"{itemId} spawned an AoE but did not damage a nearby enemy through CombatSystem.");
            return;
        }

        Assert(false, $"{itemId} did not trigger expected kill proc '{expectedEffect}' in 80 deterministic kill attempts.");
    }

    private async Task AssertKillProcAoEForMatchingSpells(
        ContentId itemId,
        DamageType damageType,
        TagId targetTag,
        string expectedEffect,
        string failureMessage)
    {
        var context = RequireContext();
        var spellIds = EnumerateSpellsMatchingTarget(context, targetTag)
            .Select(spell => spell.Id)
            .ToArray();
        Assert(spellIds.Length > 0, $"Production item '{itemId}' kill proc target '{targetTag}' matched no spells.");

        foreach (var spellId in spellIds)
        {
            Report($"PRODUCTION SPELL KILL PROC item={itemId} spell={spellId} effect={expectedEffect} target={targetTag}");
            await AssertKillProcAoE(
                itemId,
                spellId,
                damageType,
                expectedEffect,
                $"{failureMessage} Spell={spellId}.");
        }
    }

    private async Task<float> CastAtEnemy(ContentId spellId, Vector3 enemyPosition, float waitSeconds)
    {
        var context = RequireContext();
        var worldEnemyPosition = ScenarioPoint(enemyPosition);
        context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), worldEnemyPosition);
        await WaitSeconds(0.05f);
        var before = Events.DamageBySource(spellId);
        await CastSpell(spellId, Vector3.Zero, (worldEnemyPosition - _scenarioOrigin).Normalized(), waitSeconds);
        var damage = Events.DamageBySource(spellId) - before;
        Report($"CastAtEnemy spell={spellId} enemy={Format(worldEnemyPosition)} damageDelta={damage:0.###}");
        return damage;
    }

    private async Task CastSpell(ContentId spellId, Vector3 origin, Vector3 direction, float waitSeconds)
    {
        var context = RequireContext();
        var castOrigin = ScenarioPoint(origin);
        context.State.Spellbook.AssignSlot(0, spellId);
        var command = new CastSpellCommand(context.State.Player.EntityId, 0, castOrigin, direction);
        Report($"CastSpell spell={spellId} origin={Format(castOrigin)} direction={Format(direction)} wait={waitSeconds:0.###}");
        Assert(context.Commands.TryExecute(command, context, out var reason), $"Cast {spellId} failed: {reason}");
        AddMarker(castOrigin + direction.Normalized() * 3f, new Color(0.35f, 0.8f, 1f, 0.85f));
        await WaitSeconds(waitSeconds);
    }

    private float CastAndGetCooldown(ContentId spellId)
    {
        var context = RequireContext();
        context.State.Spellbook.AssignSlot(0, spellId);
        var command = new CastSpellCommand(context.State.Player.EntityId, 0, _scenarioOrigin, new Vector3(0f, 0f, -1f));
        Report($"CastAndGetCooldown spell={spellId} origin={Format(_scenarioOrigin)}");
        Assert(context.Commands.TryExecute(command, context, out var reason), $"Cast {spellId} failed: {reason}");
        Assert(context.State.Spellbook.TryGetSlot(0, out var slot) && slot != null, "Regression spell slot missing.");
        return slot!.CooldownRemainingSeconds;
    }

    private async Task AssertSummonDamageUsesPlayerDamage()
    {
        var context = RequireContext();
        var summon = await SpawnSummonFromCorpse();
        Events.Clear();
        context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), summon.Position + new Vector3(0.5f, 0f, 0f));
        await WaitSeconds(1.1f);
        var expectedDamage = PlayerAttributeResolver.ResolveDamage(
            context,
            summon.Damage,
            new[]
            {
                TagId.From("combat.damage"),
                TagId.From("combat.damage.physical"),
                TagId.From("spell.behavior.summon"),
                TagId.From("spell.delivery.area"),
                TagId.From("combat.source.summon")
            },
            false);
        var observedDamage = Events.DamageBySource(ContentId.From("raise_dead"));
        Assert(
            observedDamage >= expectedDamage - 0.01f,
            $"Summon attack damage did not use resolved player-owned damage. Expected={expectedDamage:0.###}, observed={observedDamage:0.###}.");
    }

    private async Task AssertDefenseDamageUsesPlayerDamage()
    {
        var context = RequireContext();
        Events.Clear();
        context.GetSystem<DefenseSystem>().PlaceDefense(ContentId.From("explosive_seal"), ScenarioPoint(new Vector3(4f, 0f, -2f)));
        await WaitSeconds(0.05f);
        context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(4f, 0f, -2.5f)));
        await WaitSeconds(1.3f);
        Assert(Events.DamageBySource(ContentId.From("explosive_seal")) >= 75f, "Explosive Seal damage did not use global player damage.");
    }

    private async Task AssertEnemyDamageDoesNotUsePlayerDamage()
    {
        var context = RequireContext();
        Events.Clear();
        context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), context.State.Player.Position + new Vector3(0.6f, 0f, 0f));
        await WaitSeconds(1.35f);
        var playerDamage = Events.LastDamageToTarget(context.State.Player.EntityId);
        Assert(playerDamage > 0f, "Enemy did not attack the player during regression.");
        Assert(playerDamage < 9f, $"Enemy damage inherited player damage modifiers: {playerDamage:0.##}.");
    }

    private async Task<SummonState> SpawnSummonFromCorpse()
    {
        var context = RequireContext();
        var enemy = context.GetSystem<EnemySystem>().Spawn(ContentId.From("human_grunt"), ScenarioPoint(new Vector3(2f, 0f, -4f)));
        context.GetSystem<CombatSystem>().ApplyDamage(new DamageRequest(
            context.State.Player.EntityId,
            enemy,
            ContentId.From("regression_setup"),
            999f,
            DamageType.Physical,
            new HitContext(_scenarioOrigin, Vector3.Up, context.State.Player.EntityId)));
        await WaitSeconds(0.1f);
        await CastSpell(ContentId.From("raise_dead"), Vector3.Zero, new Vector3(0f, 0f, -1f), 0.15f);
        var summons = context.GetSystem<SummonSystem>().ActiveSummons;
        Assert(summons.Count > 0, "Raise Dead did not create a summon.");
        return summons[^1];
    }

    private AoEState LastArea(ContentId spellId)
    {
        var areas = RequireContext().GetSystem<AoESystem>().ActiveAreas;
        for (var i = areas.Count - 1; i >= 0; i--)
        {
            if (areas[i].SpellId.Equals(spellId))
            {
                return areas[i];
            }
        }

        throw new InvalidOperationException($"No active AoE for {spellId}.");
    }

    private ProjectileState LastProjectile(ContentId spellId)
    {
        var projectiles = RequireContext().GetSystem<ProjectileSystem>().ActiveProjectiles;
        for (var i = projectiles.Count - 1; i >= 0; i--)
        {
            if (projectiles[i].SpellId.Equals(spellId))
            {
                return projectiles[i];
            }
        }

        throw new InvalidOperationException($"No active projectile for {spellId}.");
    }

    private StatusEffectState LastStatus(string kind)
    {
        var effects = RequireContext().GetSystem<StatusEffectSystem>().ActiveEffects;
        for (var i = effects.Count - 1; i >= 0; i--)
        {
            if (string.Equals(effects[i].Kind, kind, StringComparison.OrdinalIgnoreCase))
            {
                return effects[i];
            }
        }

        throw new InvalidOperationException($"No active status '{kind}'.");
    }

    private void PrepareRunContext(RunContext context)
    {
        context.State.IsBenchmarkRun = true;
        context.State.Player.Position = Vector3.Zero;
        var combat = context.GetSystem<CombatSystem>();
        if (!combat.TryGetHealth(context.State.Player.EntityId, out _))
        {
            combat.RegisterTarget(
                context.State.Player.EntityId,
                CombatTargetKind.Mage,
                context.State.Player.BaseMaxHealth,
                ContentId.From("mage"));
        }

        context.GetSystem<RunPhaseStateMachine>().SetPhase(RunPhase.DayCombat);
    }

    private RunContext StartFreshScenarioRun()
    {
        if (_runController == null)
        {
            throw new InvalidOperationException("RunControllerNode is not available.");
        }

        if (_context != null && _events != null)
        {
            _events.Detach(_context.Events);
        }

        _context = _runController.StartNewRun(
            profileOverride: CreateFreshRegressionProfile(),
            saveProfileOverride: _ => { });
        InstallRegressionItemFixtures(_context.Content);
        Assert(
            _context.Profile.UnlockedItemIds.Count == 0
            && _context.Profile.DiscoveredItemIds.Count == 0
            && _context.Profile.CompletedAchievementIds.Count == 0,
            "Regression run did not start from a fresh profile.");
        Events.Attach(_context.Events);
        PrepareRunContext(_context);
        Report($"FRESH RUN CONTEXT run={_context.State.RunId} profile=fresh-in-memory");
        return _context;
    }

    private RunContext CreateBalanceRunContext(int seed, ProfileSaveDto profile)
    {
        if (_runController == null)
        {
            throw new InvalidOperationException("RunControllerNode is not available.");
        }

        var context = _runController.StartNewRun(
            RunPhase.RunSetup,
            seed,
            profileOverride: profile,
            saveProfileOverride: _ => { });
        PrepareRunContext(context);
        return context;
    }

    private void InstallRegressionItemFixtures(ContentRegistry content)
    {
        var catalog = new ContentCatalog();
        catalog.Items.Add(CreateRegressionItem(
            RegressionDamageItem,
            "Regression Damage Item",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.Damage.Value,
                Value = 1.4f,
                TargetTag = string.Empty
            }));
        catalog.Items.Add(CreateRegressionItem(
            RegressionRangeItem,
            "Regression Range Item",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.Range.Value,
                Value = 1.35f,
                TargetTag = string.Empty
            }));
        catalog.Items.Add(CreateRegressionItem(
            RegressionProjectileItem,
            "Regression Projectile Item",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.ProjectileSpeed.Value,
                Value = 1.15f,
                TargetTag = "spell.delivery.projectile"
            },
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddProjectilePierce
            },
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddSpellCount,
                Value = 2f,
                TargetTag = string.Empty
            }));
        catalog.Items.Add(CreateRegressionItem(
            RegressionStatusItem,
            "Regression Status Item",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.FreezeWithFrostDamage,
                ChancePercent = 100f,
                Value = 2f,
                TargetTag = string.Empty
            }));
        catalog.Items.Add(CreateRegressionItem(
            RegressionFireStatusItem,
            "Regression Fire Status Item",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.ImmolateWithFireDamage,
                ChancePercent = 100f,
                Value = 2f,
                TargetTag = string.Empty
            }));
        catalog.Items.Add(CreateRegressionItem(
            RegressionBurningGroundItem,
            "Regression Burning Ground Item",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.BurningGroundOnEnemyDeath,
                ChancePercent = 100f,
                Value = 5f,
                TargetTag = string.Empty
            }));
        catalog.Items.Add(CreateRegressionItem(
            RegressionKillItem,
            "Regression Kill Item",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.EnemiesExplodeOnDeath,
                ChancePercent = 100f,
                Value = 12f,
                TargetTag = string.Empty
            }));
        var locked = CreateRegressionItem(
            RegressionLockedItem,
            "Regression Locked Item",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.Damage.Value,
                Value = 1.1f,
                TargetTag = string.Empty
            });
        locked.UnlockAchievementId = RegressionUnlockAchievement;
        catalog.Items.Add(locked);

        catalog.Items.Add(CreateRegressionActivatableItem(
            RegressionActivatableLimitedItem,
            "Regression Limited Active Item",
            false,
            2,
            6f,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.Damage.Value,
                Value = 1.2f
            }));
        catalog.Items.Add(CreateRegressionActivatableItem(
            RegressionActivatableUnlimitedItem,
            "Regression Unlimited Active Item",
            true,
            0,
            4f,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.Damage.Value,
                Value = 1.2f
            }));
        catalog.Items.Add(CreateRegressionActivatableItem(
            RegressionActivatableReplacementItem,
            "Regression Replacement Active Item",
            false,
            1,
            3f));
        catalog.Items.Add(CreateRegressionItem(
            RegressionKeepItInItem,
            "Regression Keep It In",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.KeepItIn,
                Value = 3f,
                SecondaryValue = 0.5f
            },
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.FireRate.Value,
                Value = 2f
            }));
        catalog.Items.Add(CreateRegressionItem(
            RegressionAbyssalRingItem,
            "Regression Abyssal Ring",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AbyssalRing,
                Value = 20f,
                SecondaryValue = 1f
            },
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.Damage.Value,
                Value = 2f,
                TargetTag = "combat.damage.arcane"
            },
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.AreaRadius.Value,
                Value = 2f,
                TargetTag = "spell.delivery.area"
            },
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.Duration.Value,
                Value = 2f,
                TargetTag = "spell.delivery.area"
            },
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.AddMultiplyStat,
                StatId = PlayerAttributeResolver.FireRate.Value,
                Value = 2f
            }));
        catalog.Items.Add(CreateRegressionItem(
            RegressionFaultyFocusItem,
            "Regression Faulty Focus",
            string.Empty,
            new ItemEffectSpec
            {
                Kind = ItemEffectKind.FaultyFocus,
                Value = 0.5f,
                SecondaryValue = 4f
            }));

        var achievement = new AchievementDefinition
        {
            Id = RegressionUnlockAchievement,
            DisplayName = "Regression Item Unlock",
            RequirementKey = RegressionUnlockAchievement
        };
        achievement.UnlockItemIds.Add(RegressionLockedItem);
        catalog.Achievements.Add(achievement);

        LocalizationService.Current.AddEnglishContent(catalog);
        var validation = content.AddGeneratedContent(catalog);
        Assert(!validation.HasErrors, "Regression item fixtures failed validation.");
    }

    private static ItemDefinition CreateRegressionItem(
        string id,
        string displayName,
        string tag,
        params ItemEffectSpec[] effects)
    {
        var item = new ItemDefinition
        {
            Id = id,
            DisplayName = displayName,
            HiddenDescription = "Regression-only item fixture.",
            RevealedEffectText = "Regression fixture effect."
        };
        item.PoolWeights.Add(new ItemPoolWeightDefinition { PoolId = ItemPoolIds.NightMarket, Weight = 10f });
        if (!string.IsNullOrWhiteSpace(tag))
        {
            item.Tags.Add(tag);
        }
        foreach (var effect in effects)
        {
            item.Effects.Add(effect);
        }

        return item;
    }

    private static ItemDefinition CreateRegressionActivatableItem(
        string id,
        string displayName,
        bool unlimited,
        int maxActivations,
        float teleportDistance,
        params ItemEffectSpec[] effects)
    {
        var item = CreateRegressionItem(id, displayName, "item.activatable", effects);
        item.Kind = ItemKind.Activatable;
        item.HasUnlimitedActivations = unlimited;
        item.MaxActivations = maxActivations;
        item.ActiveEffects.Add(new EffectSpec
        {
            EffectType = ActiveEffectIds.TeleportPlayer,
            Value = teleportDistance,
            DurationScaling = DurationScalingMode.None
        });
        return item;
    }

    private static bool IsRegressionItem(ContentId itemId)
    {
        return itemId.Equals(ContentId.From(RegressionDamageItem))
            || itemId.Equals(ContentId.From(RegressionRangeItem))
            || itemId.Equals(ContentId.From(RegressionProjectileItem))
            || itemId.Equals(ContentId.From(RegressionStatusItem))
            || itemId.Equals(ContentId.From(RegressionFireStatusItem))
            || itemId.Equals(ContentId.From(RegressionBurningGroundItem))
            || itemId.Equals(ContentId.From(RegressionKillItem))
            || itemId.Equals(ContentId.From(RegressionLockedItem))
            || itemId.Equals(ContentId.From(RegressionActivatableLimitedItem))
            || itemId.Equals(ContentId.From(RegressionActivatableUnlimitedItem))
            || itemId.Equals(ContentId.From(RegressionActivatableReplacementItem))
            || itemId.Equals(ContentId.From(RegressionKeepItInItem))
            || itemId.Equals(ContentId.From(RegressionAbyssalRingItem))
            || itemId.Equals(ContentId.From(RegressionFaultyFocusItem));
    }

    private static ProfileSaveDto CreateFreshRegressionProfile()
    {
        return new ProfileSaveDto();
    }

    private RunContext RequireContext()
    {
        return _context ?? throw new InvalidOperationException("RunContext is not available.");
    }

    private EventLog Events => _events ?? throw new InvalidOperationException("Regression event log is not available.");

    private Vector3 ScenarioPoint(Vector3 localPosition)
    {
        return _scenarioOrigin + localPosition;
    }

    private void AddItem(ContentId itemId)
    {
        var context = RequireContext();
        context.State.Build.AddItem(itemId);
        context.Events.Publish(new ItemAcquiredEvent(itemId));
        Report($"ITEM ADD {itemId}");
        ReportPlayerAttributes();
    }

    private static void KillMage(RunContext context, ContentId sourceId)
    {
        context.GetSystem<CombatSystem>().ApplyDamage(new DamageRequest(
            SourceEntityId: context.State.Player.EntityId,
            TargetEntityId: context.State.Player.EntityId,
            SourceContentId: sourceId,
            Amount: 999_999f,
            DamageType: DamageType.Arcane,
            Hit: new HitContext(context.State.Player.Position, Vector3.Up, context.State.Player.EntityId)));
    }

    private static void KillAllActiveEnemies(RunContext context, ContentId sourceId)
    {
        var combat = context.GetSystem<CombatSystem>();
        var enemies = context.GetSystem<EnemySystem>().ActiveEnemies.ToArray();
        foreach (var enemy in enemies)
        {
            combat.ApplyDamage(new DamageRequest(
                context.State.Player.EntityId,
                enemy.EntityId,
                sourceId,
                999_999f,
                DamageType.Arcane,
                new HitContext(enemy.Position, Vector3.Up, context.State.Player.EntityId),
                ForcePlayerOwned: true));
        }
    }

    private async Task WaitSeconds(float seconds)
    {
        await ToSignal(GetTree().CreateTimer(seconds), SceneTreeTimer.SignalName.Timeout);
    }

    private void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }

    private void Fail(string name, string message)
    {
        _failed++;
        AddLine($"FAIL {name}: {message}");
    }

    private void BuildUi()
    {
        var layer = new CanvasLayer { Name = "RegressionUi", Layer = 20 };
        AddChild(layer);
        var panel = new PanelContainer
        {
            OffsetLeft = 24f,
            OffsetTop = 24f,
            OffsetRight = 980f,
            OffsetBottom = 620f,
            MouseFilter = Control.MouseFilterEnum.Ignore
        };
        layer.AddChild(panel);

        _label = new Label
        {
            AutowrapMode = TextServer.AutowrapMode.WordSmart,
            Text = "Regression runner booting...",
            MouseFilter = Control.MouseFilterEnum.Ignore
        };
        panel.AddChild(_label);
    }

    private void FrameRegressionCamera()
    {
        var camera = GetTree().Root.FindChild("RegressionCamera", true, false) as Camera3D;
        if (camera == null)
        {
            return;
        }

        camera.GlobalPosition = new Vector3(45f, 70f, 55f);
        camera.LookAt(new Vector3(45f, 0f, -10f), Vector3.Up);
    }

    private void SetStatus(string text)
    {
        AddLine(text);
    }

    private void AddLine(string line)
    {
        _lines.Add(line);
        if (_lines.Count > 28)
        {
            _lines.RemoveAt(0);
        }

        if (_label != null)
        {
            _label.Text = string.Join('\n', _lines);
        }

        GD.Print($"Regression: {line}");
    }

    private void InitializeReport()
    {
        if (string.IsNullOrWhiteSpace(_reportPath))
        {
            var reportDirectory = ProjectSettings.GlobalizePath("res://Saved/regression_reports");
            System.IO.Directory.CreateDirectory(reportDirectory);
            _reportPath = System.IO.Path.Combine(reportDirectory, $"tlm_regression_{DateTime.Now:yyyyMMdd_HHmmss}.log");
        }
        else
        {
            var reportDirectory = System.IO.Path.GetDirectoryName(_reportPath);
            if (!string.IsNullOrWhiteSpace(reportDirectory))
            {
                System.IO.Directory.CreateDirectory(reportDirectory);
            }
        }

        AddLine($"Report: {_reportPath}");
        Report($"Report path: {_reportPath}");
        FlushReport();
    }

    private void ReportEnvironmentDiagnostics(string mode)
    {
        Report($"ENV mode={mode}");
        Report($"ENV executable={OS.GetExecutablePath()}");
        Report($"ENV display={DisplayServer.GetName()} audio={AudioServer.GetDriverName()}");
        Report($"ENV project={ProjectSettings.GlobalizePath("res://")} user={ProjectSettings.GlobalizePath("user://")}");
        Report($"ENV runtime_user={RuntimePathResolver.GlobalizeUserPath("user://")}");
        Report($"ENV report={_reportPath} result={_resultPath}");
        Report($"ENV args={string.Join(' ', OS.GetCmdlineArgs())}");
        Report($"ENV user_args={string.Join(' ', OS.GetCmdlineUserArgs())}");
    }

    private void ResetRunState()
    {
        _passed = 0;
        _failed = 0;
        _scenarioIndex = 0;
        _reportLines.Clear();
        _lines.Clear();
        foreach (var marker in _markers)
        {
            marker.QueueFree();
        }

        _markers.Clear();
    }

    private RegressionScenario? FindScenario(string scenarioId)
    {
        foreach (var scenario in _scenarios)
        {
            if (string.Equals(scenario.Id, scenarioId, StringComparison.OrdinalIgnoreCase))
            {
                return scenario;
            }
        }

        return null;
    }

    private void Report(string line)
    {
        var timestamp = Time.GetTicksMsec() / 1000.0;
        _reportLines.Add($"{timestamp:0000.000} [{_currentScenario}] {line}");
        FlushReport();
    }

    private void FlushReport()
    {
        if (string.IsNullOrWhiteSpace(_reportPath))
        {
            return;
        }

        System.IO.File.WriteAllLines(_reportPath, _reportLines);
    }

    private void WriteResult(string summary)
    {
        if (string.IsNullOrWhiteSpace(_resultPath))
        {
            return;
        }

        var resultDirectory = System.IO.Path.GetDirectoryName(_resultPath);
        if (!string.IsNullOrWhiteSpace(resultDirectory))
        {
            System.IO.Directory.CreateDirectory(resultDirectory);
        }

        var status = _failed == 0 ? "PASS" : "FAIL";
        System.IO.File.WriteAllLines(_resultPath, new[]
        {
            $"status={status}",
            $"passed={_passed}",
            $"failed={_failed}",
            $"summary={summary}",
            $"report={_reportPath}"
        });
    }

    private void ReportPlayerAttributes()
    {
        var context = RequireContext();
        var tags = Array.Empty<TagId>();
        Report(
            "ATTR "
            + $"health={PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.Health, context.State.Player.BaseMaxHealth, tags, false):0.###} "
            + $"move_speed={PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.MoveSpeed, context.State.Player.BaseMoveSpeed, tags, false):0.###} "
            + $"damage10={PlayerAttributeResolver.ResolveDamage(context, 10f, tags, false):0.###} "
            + $"fire_rate={PlayerAttributeResolver.ResolveFireRate(context, tags, false):0.###} "
            + $"range10={PlayerAttributeResolver.ResolveRange(context, 10f, tags, false):0.###} "
            + $"luck={PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.Luck, context.State.Player.BaseLuck, tags, false):0.###} "
            + $"duration10={PlayerAttributeResolver.ResolveDuration(context, 10f, tags, false):0.###} "
            + $"items={context.State.Build.Items.Count}");
    }

    private static string Format(Vector3 value)
    {
        return $"({value.X:0.###},{value.Y:0.###},{value.Z:0.###})";
    }

    private static bool ContainsMissingMarker(string text)
    {
        return LocalizationService.Current.ContainsMissingMarker(text);
    }

    private static int CountEditorItemResources()
    {
        const string itemDirectory = "res://data/items";
        var dir = DirAccess.Open(itemDirectory);
        if (dir == null)
        {
            return 0;
        }

        var count = 0;
        dir.ListDirBegin();
        while (true)
        {
            var fileName = dir.GetNext();
            if (string.IsNullOrEmpty(fileName))
            {
                break;
            }

            if (!dir.CurrentIsDir() && fileName.EndsWith(".tres", StringComparison.OrdinalIgnoreCase))
            {
                count++;
            }
        }

        dir.ListDirEnd();
        return count;
    }

    private void AddMarker(Vector3 position, Color color)
    {
        if (_markers.Count >= 48)
        {
            _markers[0].QueueFree();
            _markers.RemoveAt(0);
        }

        var marker = new MeshInstance3D
        {
            Name = "RegressionMarker",
            Position = position,
            Mesh = new SphereMesh { Radius = 0.18f, Height = 0.36f },
            MaterialOverride = new StandardMaterial3D
            {
                AlbedoColor = color,
                Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
            }
        };
        AddChild(marker);
        _markers.Add(marker);
    }

    private sealed class EventLog
    {
        private readonly Action<string> _report;
        private readonly List<DamageAppliedEvent> _damage = new();
        private readonly List<StatusAppliedEvent> _statuses = new();
        private readonly List<ItemProcTriggeredEvent> _itemProcs = new();
        private readonly List<ProjectileSpawnedEvent> _projectiles = new();
        private readonly List<BeamFiredEvent> _beams = new();
        private readonly List<AoESpawnedEvent> _areas = new();
        private readonly List<SummonSpawnedEvent> _summons = new();
        private readonly List<DamagePreventedEvent> _damagePreventions = new();
        private int _enemyAbilityTelegraphs;
        private int _enemyAbilityExecutions;
        private int _itemProcTriggers;
        private int _itemProcBlocks;
        private int _activatableUses;
        private int _activatableClears;
        private int _teleportRequests;
        private readonly List<SubscriptionToken> _tokens = new();

        public int EnemyAbilityTelegraphs => _enemyAbilityTelegraphs;

        public int EnemyAbilityExecutions => _enemyAbilityExecutions;

        public int ItemProcTriggers => _itemProcTriggers;

        public int ItemProcBlocks => _itemProcBlocks;

        public int ActivatableUses => _activatableUses;

        public int ActivatableClears => _activatableClears;

        public int TeleportRequests => _teleportRequests;

        public EventLog(Action<string> report)
        {
            _report = report;
        }

        public void Attach(GameEventBus events)
        {
            _tokens.Add(events.Subscribe<SpellCastEvent>(e => _report($"EVENT SpellCast spell={e.SpellId} caster={e.CasterId}")));
            _tokens.Add(events.Subscribe<ProjectileSpawnedEvent>(e =>
            {
                _projectiles.Add(e);
                _report($"EVENT ProjectileSpawned spell={e.SpellId} projectile={e.ProjectileId}");
            }));
            _tokens.Add(events.Subscribe<DamageAppliedEvent>(e =>
            {
                _damage.Add(e);
                _report($"EVENT Damage source={e.SourceId} target={e.TargetId} amount={e.Amount:0.###}");
            }));
            _tokens.Add(events.Subscribe<DamagePreventionGrantedEvent>(e => _report($"EVENT DamagePreventionGranted source={e.SourceId} target={e.TargetId} remaining={e.RemainingHits} required={e.RequiredIncomingTag}")));
            _tokens.Add(events.Subscribe<DamagePreventedEvent>(e =>
            {
                _damagePreventions.Add(e);
                _report($"EVENT DamagePrevented prevention={e.PreventionSourceId} incoming={e.IncomingSourceId} target={e.TargetId} amount={e.Amount:0.###} remaining={e.RemainingHits}");
            }));
            _tokens.Add(events.Subscribe<StatusAppliedEvent>(e =>
            {
                _statuses.Add(e);
                _report($"EVENT Status kind={e.StatusKind} source={e.SourceId} target={e.TargetId} value={e.Value:0.###} duration={e.DurationSeconds:0.###}");
            }));
            _tokens.Add(events.Subscribe<BeamFiredEvent>(e =>
            {
                _beams.Add(e);
                _report($"EVENT Beam spell={e.SpellId} start={Format(e.Start)} end={Format(e.End)} radius={e.Radius:0.###} distance={e.Start.DistanceTo(e.End):0.###}");
            }));
            _tokens.Add(events.Subscribe<AoESpawnedEvent>(e =>
            {
                _areas.Add(e);
                _report($"EVENT AoE spell={e.SpellId} aoe={e.AoEId} position={Format(e.Position)} radius={e.Radius:0.###}");
            }));
            _tokens.Add(events.Subscribe<SummonSpawnedEvent>(e =>
            {
                _summons.Add(e);
                _report($"EVENT Summon spell={e.SpellId} summon={e.SummonId} position={Format(e.Position)}");
            }));
            _tokens.Add(events.Subscribe<DefensePlacedEvent>(e => _report($"EVENT DefensePlaced defense={e.DefenseId} entity={e.DefenseEntityId} position={Format(e.Position)}")));
            _tokens.Add(events.Subscribe<EnemyDiedEvent>(e => _report($"EVENT EnemyDied enemy={e.EnemyId} archetype={e.EnemyArchetypeId} source={e.SourceId}")));
            _tokens.Add(events.Subscribe<EnemyAbilityTelegraphedEvent>(e =>
            {
                _enemyAbilityTelegraphs++;
                _report($"EVENT EnemyAbilityTelegraph enemy={e.EnemyId} archetype={e.EnemyArchetypeId} ability={e.AbilityType} target={e.TargetId}");
            }));
            _tokens.Add(events.Subscribe<EnemyAbilityExecutedEvent>(e =>
            {
                _enemyAbilityExecutions++;
                _report($"EVENT EnemyAbilityExecuted enemy={e.EnemyId} archetype={e.EnemyArchetypeId} ability={e.AbilityType} target={e.TargetId}");
            }));
            _tokens.Add(events.Subscribe<CorpseAvailableEvent>(e => _report($"EVENT Corpse enemy={e.EnemyId} archetype={e.EnemyArchetypeId} position={Format(e.Position)}")));
            _tokens.Add(events.Subscribe<ItemAcquiredEvent>(e => _report($"EVENT ItemAcquired item={e.ItemId}")));
            _tokens.Add(events.Subscribe<ActivatableItemEquippedEvent>(e => _report($"EVENT ActivatableEquipped item={e.ItemId} replaced={e.ReplacedItemId} remaining={e.RemainingActivations} unlimited={e.HasUnlimitedActivations}")));
            _tokens.Add(events.Subscribe<ActivatableItemUsedEvent>(e =>
            {
                _activatableUses++;
                _report($"EVENT ActivatableUsed item={e.ItemId} remaining={e.RemainingActivations} depleted={e.Depleted}");
            }));
            _tokens.Add(events.Subscribe<ActivatableItemClearedEvent>(e =>
            {
                _activatableClears++;
                _report($"EVENT ActivatableCleared item={e.ItemId}");
            }));
            _tokens.Add(events.Subscribe<PlayerTeleportRequestedEvent>(e =>
            {
                _teleportRequests++;
                _report($"EVENT PlayerTeleport source={e.SourceId} target={Format(e.TargetPosition)}");
            }));
            _tokens.Add(events.Subscribe<ItemProcTriggeredEvent>(e =>
            {
                _itemProcTriggers++;
                _itemProcs.Add(e);
                _report($"EVENT ItemProc item={e.ItemId} trigger={e.Trigger} effect={e.EffectType} target={e.TargetId}");
            }));
            _tokens.Add(events.Subscribe<ItemProcBlockedEvent>(e =>
            {
                _itemProcBlocks++;
                _report($"EVENT ItemProcBlocked item={e.ItemId} trigger={e.Trigger} effect={e.EffectType} reason={e.Reason}");
            }));
            _tokens.Add(events.Subscribe<MarketOffersGeneratedEvent>(e => _report($"EVENT MarketOffers count={e.OfferCount}")));
        }

        public void Detach(GameEventBus events)
        {
            foreach (var token in _tokens)
            {
                events.Unsubscribe(token);
            }

            _tokens.Clear();
        }

        public void Clear()
        {
            _damage.Clear();
            _statuses.Clear();
            _itemProcs.Clear();
            _projectiles.Clear();
            _beams.Clear();
            _areas.Clear();
            _summons.Clear();
            _damagePreventions.Clear();
            _enemyAbilityTelegraphs = 0;
            _enemyAbilityExecutions = 0;
            _itemProcTriggers = 0;
            _itemProcBlocks = 0;
            _activatableUses = 0;
            _activatableClears = 0;
            _teleportRequests = 0;
        }

        public float DamageBySource(ContentId sourceId)
        {
            var total = 0f;
            foreach (var damage in _damage)
            {
                if (damage.SourceId.Equals(sourceId))
                {
                    total += damage.Amount;
                }
            }

            return total;
        }

        public int DamageEventCountBySource(ContentId sourceId)
        {
            var count = 0;
            foreach (var damage in _damage)
            {
                if (damage.SourceId.Equals(sourceId))
                {
                    count++;
                }
            }

            return count;
        }

        public int DamagePreventionCount(ContentId sourceId)
        {
            var count = 0;
            foreach (var prevention in _damagePreventions)
            {
                if (prevention.PreventionSourceId.Equals(sourceId))
                {
                    count++;
                }
            }

            return count;
        }

        public float LastDamageToTarget(EntityId targetId)
        {
            for (var i = _damage.Count - 1; i >= 0; i--)
            {
                if (_damage[i].TargetId.Equals(targetId))
                {
                    return _damage[i].Amount;
                }
            }

            return 0f;
        }

        public int StatusCount(string kind)
        {
            var count = 0;
            foreach (var status in _statuses)
            {
                if (string.Equals(status.StatusKind, kind, StringComparison.OrdinalIgnoreCase))
                {
                    count++;
                }
            }

            return count;
        }

        public int StatusCountFromSource(string kind, ContentId sourceId)
        {
            var count = 0;
            foreach (var status in _statuses)
            {
                if (string.Equals(status.StatusKind, kind, StringComparison.OrdinalIgnoreCase)
                    && status.SourceId.Equals(sourceId))
                {
                    count++;
                }
            }

            return count;
        }

        public int ItemProcCount(ContentId itemId, string effectType)
        {
            var count = 0;
            foreach (var proc in _itemProcs)
            {
                if (proc.ItemId.Equals(itemId)
                    && proc.EffectType.Contains(effectType, StringComparison.OrdinalIgnoreCase))
                {
                    count++;
                }
            }

            return count;
        }

        public float LastStatusDuration(string kind)
        {
            for (var i = _statuses.Count - 1; i >= 0; i--)
            {
                if (string.Equals(_statuses[i].StatusKind, kind, StringComparison.OrdinalIgnoreCase))
                {
                    return _statuses[i].DurationSeconds;
                }
            }

            return 0f;
        }

        public int BeamCount(ContentId spellId)
        {
            var count = 0;
            foreach (var beam in _beams)
            {
                if (beam.SpellId.Equals(spellId))
                {
                    count++;
                }
            }

            return count;
        }

        public int ProjectileCount(ContentId spellId)
        {
            var count = 0;
            foreach (var projectile in _projectiles)
            {
                if (projectile.SpellId.Equals(spellId))
                {
                    count++;
                }
            }

            return count;
        }

        public float LastBeamDistance(ContentId spellId)
        {
            for (var i = _beams.Count - 1; i >= 0; i--)
            {
                if (_beams[i].SpellId.Equals(spellId))
                {
                    return _beams[i].Start.DistanceTo(_beams[i].End);
                }
            }

            throw new InvalidOperationException($"No beam event for {spellId}.");
        }

        public float LastBeamRadius(ContentId spellId)
        {
            for (var i = _beams.Count - 1; i >= 0; i--)
            {
                if (_beams[i].SpellId.Equals(spellId))
                {
                    return _beams[i].Radius;
                }
            }

            throw new InvalidOperationException($"No beam event for {spellId}.");
        }

        public int AoECount(ContentId spellId)
        {
            var count = 0;
            foreach (var area in _areas)
            {
                if (area.SpellId.Equals(spellId))
                {
                    count++;
                }
            }

            return count;
        }

        public int SummonCount(ContentId spellId)
        {
            var count = 0;
            foreach (var summon in _summons)
            {
                if (summon.SpellId.Equals(spellId))
                {
                    count++;
                }
            }

            return count;
        }
    }

    private readonly record struct RegressionScenario(string Id, string DisplayName, Func<Task> Run);

    private readonly record struct ModifierRuntimeCheck(ModifierRuntimeSpec Modifier, float BeforeValue);

    private readonly record struct SpellModifierRuntimeCheck(
        ModifierRuntimeSpec Modifier,
        ContentId SpellId,
        IReadOnlyCollection<TagId> SpellTags,
        float BeforeValue);

    private enum RadiusExpectation
    {
        Increase,
        Decrease
    }

    private readonly record struct RegressionCommandLine(
        string ScenarioId,
        string ReportPath,
        string ResultPath,
        bool QuitWhenDone,
        bool RunAllScenarios,
        bool IncludeEnvironmentDiagnostics)
    {
        public static RegressionCommandLine Parse()
        {
            var scenarioId = string.Empty;
            var reportPath = string.Empty;
            var resultPath = string.Empty;
            var quitWhenDone = false;
            var runAllScenarios = false;
            var includeEnvironmentDiagnostics = false;

            foreach (var argument in OS.GetCmdlineUserArgs())
            {
                if (argument.StartsWith("--tlm-regression-scenario=", StringComparison.OrdinalIgnoreCase))
                {
                    scenarioId = argument["--tlm-regression-scenario=".Length..].Trim();
                }
                else if (argument.StartsWith("--tlm-regression-report=", StringComparison.OrdinalIgnoreCase))
                {
                    reportPath = argument["--tlm-regression-report=".Length..].Trim();
                }
                else if (argument.StartsWith("--tlm-regression-result=", StringComparison.OrdinalIgnoreCase))
                {
                    resultPath = argument["--tlm-regression-result=".Length..].Trim();
                }
                else if (string.Equals(argument, "--tlm-regression-quit", StringComparison.OrdinalIgnoreCase))
                {
                    quitWhenDone = true;
                }
                else if (string.Equals(argument, "--tlm-regression-all", StringComparison.OrdinalIgnoreCase))
                {
                    runAllScenarios = true;
                }
                else if (string.Equals(argument, "--tlm-regression-env", StringComparison.OrdinalIgnoreCase))
                {
                    includeEnvironmentDiagnostics = true;
                }
            }

            return new RegressionCommandLine(
                scenarioId,
                reportPath,
                resultPath,
                quitWhenDone,
                runAllScenarios,
                includeEnvironmentDiagnostics);
        }
    }
}
