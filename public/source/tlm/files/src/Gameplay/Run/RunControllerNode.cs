using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Data.Validation;
using TheLastMage.Debugging.Reports;
using TheLastMage.Foundation.Events;
using TheLastMage.Foundation.Pooling;
using TheLastMage.Foundation.Random;
using TheLastMage.Gameplay.AoE;
using TheLastMage.Gameplay.Achievements;
using TheLastMage.Gameplay.Commands;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Navigation;
using TheLastMage.Gameplay.Projectiles;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.StatusEffects;
using TheLastMage.Gameplay.Summons;
using TheLastMage.Gameplay.Waves;
using TheLastMage.Localization;
using TheLastMage.Presentation.Feedback;
using TheLastMage.Save;

namespace TheLastMage.Gameplay.Run;

public partial class RunControllerNode : Node
{
    private const string CatalogPath = "res://data/catalogs/content_catalog.tres";

    private RunContext? _context;

    [Export] public RunPhase InitialPhase { get; set; } = RunPhase.RunSetup;

    [Export] public int DefaultSeed { get; set; } = 12345;

    [Export] public bool SuppressAutomaticEnemySpawns { get; set; }

    public RunContext? Context => _context;

    public override void _Ready()
    {
        var saveService = GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        if (InitialPhase == RunPhase.DayCombat
            && RunCheckpointService.CanUseCheckpoint(saveService?.Profile.SuspendedRun)
            && saveService?.Profile.SuspendedRun != null)
        {
            StartFromCheckpoint(saveService.Profile.SuspendedRun);
            return;
        }

        StartNewRun(InitialPhase);
    }

    public override void _Process(double delta)
    {
        if (_context == null)
        {
            return;
        }

        _context.State.DebugMetrics.FrameMilliseconds = (float)(delta * 1000.0);
        foreach (var system in _context.Systems)
        {
            system.Tick((float)delta);
        }
    }

    public override void _PhysicsProcess(double delta)
    {
        if (_context == null)
        {
            return;
        }

        _context.State.DebugMetrics.ResetFrameCounters();
        foreach (var system in _context.Systems)
        {
            system.FixedTick((float)delta);
        }
    }

    public override void _ExitTree()
    {
        ShutdownCurrentRun();
    }

    public RunContext StartNewRun(
        RunPhase initialPhase = RunPhase.RunSetup,
        int? seed = null,
        ProfileSaveDto? profileOverride = null,
        Action<ProfileSaveDto>? saveProfileOverride = null)
    {
        ShutdownCurrentRun();
        _context = CreateContext(seed ?? DefaultSeed, profileOverride, saveProfileOverride);
        _context.State.SuppressAutomaticEnemySpawns = SuppressAutomaticEnemySpawns;
        foreach (var system in _context.Systems)
        {
            system.Initialize(_context);
        }

        if (initialPhase != RunPhase.RunSetup)
        {
            _context.GetSystem<RunPhaseStateMachine>().SetPhase(initialPhase);
        }

        GD.Print($"RunContext created. RunId={_context.State.RunId} Seed={_context.Random.Seed}");
        return _context;
    }

    public RunContext StartFromCheckpoint(
        RunCheckpointSaveDto checkpoint,
        ProfileSaveDto? profileOverride = null,
        Action<ProfileSaveDto>? saveProfileOverride = null)
    {
        ShutdownCurrentRun();
        _context = CreateContext(
            checkpoint.Seed == 0 ? DefaultSeed : checkpoint.Seed,
            profileOverride,
            saveProfileOverride,
            checkpoint);
        _context.State.SuppressAutomaticEnemySpawns = SuppressAutomaticEnemySpawns;
        foreach (var system in _context.Systems)
        {
            system.Initialize(_context);
        }

        RunCheckpointService.Restore(_context, checkpoint);
        GD.Print($"RunContext resumed. RunId={_context.State.RunId} Seed={_context.Random.Seed}");
        return _context;
    }

    private void ShutdownCurrentRun()
    {
        if (_context == null)
        {
            return;
        }

        foreach (var system in _context.Systems)
        {
            system.Shutdown();
        }

        _context = null;
    }

    private RunContext CreateContext(
        int seed,
        ProfileSaveDto? profileOverride = null,
        Action<ProfileSaveDto>? saveProfileOverride = null,
        RunCheckpointSaveDto? checkpoint = null)
    {
        var catalog = ResourceLoader.Load<ContentCatalog>(CatalogPath) ?? new ContentCatalog();

        var validator = new ContentValidator();
        var validation = validator.Validate(catalog);
        PrintValidation(validation.Issues);

        var localizationValidation = LocalizationValidator.Validate(catalog);
        PrintValidation(localizationValidation.Issues);

        var registry = new ContentRegistry();
        var registryValidation = registry.Load(catalog);
        PrintValidation(registryValidation.Issues);

        var events = new GameEventBus();
        var eventCounter = new GameEventCounter();
        eventCounter.Attach(events);
        var saveService = GetNodeOrNull<SaveServiceNode>("/root/SaveService");
        var profile = profileOverride ?? saveService?.Profile ?? new ProfileSaveDto();
        var settings = saveService?.Settings ?? new SettingsSaveDto();
        LocalizationService.Current.LoadEnglish(catalog, settings.Locale);
        Action<ProfileSaveDto> saveProfile = saveProfileOverride ?? (_ => { });
        if (saveProfileOverride == null && saveService != null)
        {
            saveProfile = saveService.SaveProfile;
        }

        Action<SettingsSaveDto> saveSettings = saveService != null ? saveService.SaveSettings : _ => { };

        if (ProfileDefaultsService.EnsureCurrent(profile))
        {
            saveProfile(profile);
        }

        var itemKnowledge = new ItemKnowledgeService(profile, saveProfile);
        var state = checkpoint == null
            ? new RunState()
            : new RunState { RunId = TheLastMage.Foundation.RunId.FromString(checkpoint.RunId) };
        ApplyDefaultMage(state, registry, profile);

        var runRoot = GetOwnerOrNull<Node>() ?? GetTree().CurrentScene ?? this;
        var worldRoot = runRoot.GetNodeOrNull<Node3D>("WorldRoot");
        var enemyRoot = runRoot.GetNodeOrNull<Node3D>("PresentationRoot/EnemyPresentationPool");
        var projectileRoot = runRoot.GetNodeOrNull<Node3D>("PresentationRoot/ProjectilePresentationPool");
        var itemPresentationRoot = projectileRoot
            ?? runRoot.GetNodeOrNull<Node3D>("PresentationRoot/VfxPool")
            ?? runRoot.GetNodeOrNull<Node3D>("PresentationRoot");
        var defenseRoot = runRoot.GetNodeOrNull<Node3D>("PresentationRoot/DefensePresentationPool")
            ?? runRoot.GetNodeOrNull<Node3D>("PresentationRoot");
        var aoeRoot = runRoot.GetNodeOrNull<Node3D>("PresentationRoot/AoEPresentationPool")
            ?? runRoot.GetNodeOrNull<Node3D>("PresentationRoot");
        var summonRoot = runRoot.GetNodeOrNull<Node3D>("PresentationRoot/SummonPresentationPool")
            ?? runRoot.GetNodeOrNull<Node3D>("PresentationRoot");

        var phaseMachine = new RunPhaseStateMachine();
        var systems = new IGameSystem[]
        {
            phaseMachine,
            new RunRewardSystem(),
            new TowerNavigationAdapter(worldRoot),
            new DamagePreventionSystem(),
            new CombatSystem(),
            new ActivatableItemSystem(itemPresentationRoot),
            new ProcSystem(),
            new DefenseSystem(defenseRoot),
            new EnemySystem(enemyRoot),
            new ProjectileSystem(projectileRoot),
            new AoESystem(aoeRoot),
            new StatusEffectSystem(),
            new SummonSystem(summonRoot),
            new SpellSystem(),
            new WaveDirectorSystem(worldRoot),
            new MarketSystem(),
            new AchievementSystem(),
            new BeamDebugSystem(runRoot.GetNodeOrNull<Node3D>("PresentationRoot/VfxPool")
                ?? runRoot.GetNodeOrNull<Node3D>("PresentationRoot")),
            new FeedbackSystem(),
            new GameplayProfilingSystem(),
            new RunReportSystem()
        };

        return new RunContext
        {
            Content = registry,
            State = state,
            Events = events,
            Commands = new CommandDispatcher(),
            Pools = new PoolHub(),
            Random = new RandomService(seed),
            Profile = profile,
            SaveProfile = saveProfile,
            Settings = settings,
            SaveSettings = saveSettings,
            ItemKnowledge = itemKnowledge,
            Systems = systems
        };
    }

    private static void ApplyDefaultMage(RunState state, ContentRegistry registry, ProfileSaveDto profile)
    {
        var preferredMageId = Foundation.ContentId.From(profile.PreferredMageId);
        var selectedMageId = preferredMageId.IsValid
            && registry.Mages.ContainsKey(preferredMageId)
            && profile.UnlockedMageIds.Contains(preferredMageId.Value, StringComparer.Ordinal)
                ? preferredMageId
                : profile.UnlockedMageIds
                    .Select(Foundation.ContentId.From)
                    .FirstOrDefault(id => id.IsValid && registry.Mages.ContainsKey(id));

        if (!selectedMageId.IsValid || !registry.TryGetMage(selectedMageId, out var mage) || mage == null)
        {
            selectedMageId = Foundation.ContentId.From(ProfileDefaultsService.DefaultMageId);
            registry.TryGetMage(selectedMageId, out mage);
        }

        if (mage != null)
        {
            state.SelectMage(mage);
        }
    }

    private static void PrintValidation(IEnumerable<Foundation.Validation.ValidationIssue> issues)
    {
        var compactWarnings = ShouldUseCompactValidationWarnings();
        foreach (var issue in issues)
        {
            var message = $"[{issue.Severity}] {issue.Code}: {issue.Message}";
            if (issue.Severity == Foundation.Validation.ValidationSeverity.Error)
            {
                GD.PushError(message);
            }
            else if (issue.Severity == Foundation.Validation.ValidationSeverity.Warning)
            {
                if (compactWarnings)
                {
                    GD.Print(message);
                }
                else
                {
                    GD.PushWarning(message);
                }
            }
            else
            {
                GD.Print(message);
            }
        }
    }

    private static bool ShouldUseCompactValidationWarnings()
    {
        foreach (var argument in OS.GetCmdlineUserArgs())
        {
            if (string.Equals(argument, "--tlm-regression-compact-validation-log", StringComparison.OrdinalIgnoreCase))
            {
                return true;
            }
        }

        return false;
    }
}
