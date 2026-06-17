using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Run;
using System.Globalization;

namespace TheLastMage.Presentation.Feedback;

public sealed class FeedbackSystem : IGameSystem
{
    private readonly Queue<FeedbackRequest> _recentRequests = new();
    private readonly List<ActiveFeedbackEntry> _activeEntries = new();
    private readonly FeedbackBudget _budget = new();
    private readonly FeedbackAudioRules _audioRules = new();
    private RunContext? _context;
    private SubscriptionToken _spellCastToken;
    private SubscriptionToken _projectileToken;
    private SubscriptionToken _aoeToken;
    private SubscriptionToken _statusToken;
    private SubscriptionToken _summonToken;
    private SubscriptionToken _damageToken;
    private SubscriptionToken _enemyDiedToken;
    private SubscriptionToken _enemyTelegraphToken;
    private SubscriptionToken _enemyAbilityToken;

    public IReadOnlyCollection<FeedbackRequest> RecentRequests => _recentRequests;

    public void Initialize(RunContext context)
    {
        _context = context;
        context.State.DebugMetrics.ActiveFeedbackBudgetSummary = _budget.BuildSummary();
        context.State.DebugMetrics.AudioMixSummary = _audioRules.BuildSummary();
        context.State.DebugMetrics.ReadabilitySummary = BuildReadabilitySummary(context);
        _spellCastToken = context.Events.Subscribe<SpellCastEvent>(gameEvent => Add("cast", gameEvent.SpellId));
        _projectileToken = context.Events.Subscribe<ProjectileSpawnedEvent>(gameEvent => Add("projectile", gameEvent.SpellId));
        _aoeToken = context.Events.Subscribe<AoESpawnedEvent>(gameEvent => Add("aoe", gameEvent.SpellId));
        _statusToken = context.Events.Subscribe<StatusAppliedEvent>(gameEvent => Add($"status:{gameEvent.StatusKind}", gameEvent.SourceId));
        _summonToken = context.Events.Subscribe<SummonSpawnedEvent>(gameEvent => Add("summon", gameEvent.SpellId));
        _damageToken = context.Events.Subscribe<DamageAppliedEvent>(gameEvent => Add("damage", gameEvent.SourceId));
        _enemyDiedToken = context.Events.Subscribe<EnemyDiedEvent>(gameEvent => Add("death", gameEvent.SourceId));
        _enemyTelegraphToken = context.Events.Subscribe<EnemyAbilityTelegraphedEvent>(gameEvent => Add($"enemy_telegraph:{gameEvent.AbilityType}", gameEvent.EnemyArchetypeId));
        _enemyAbilityToken = context.Events.Subscribe<EnemyAbilityExecutedEvent>(gameEvent => Add($"enemy_ability:{gameEvent.AbilityType}", gameEvent.EnemyArchetypeId));
    }

    public void Tick(float delta)
    {
        DecayActiveEntries(delta);
        if (_context != null)
        {
            _context.State.DebugMetrics.ActiveFeedbackBudgetSummary = BuildActiveBudgetSummary();
            _context.State.DebugMetrics.ReadabilitySummary = BuildReadabilitySummary(_context);
        }
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_spellCastToken);
            _context.Events.Unsubscribe(_projectileToken);
            _context.Events.Unsubscribe(_aoeToken);
            _context.Events.Unsubscribe(_statusToken);
            _context.Events.Unsubscribe(_summonToken);
            _context.Events.Unsubscribe(_damageToken);
            _context.Events.Unsubscribe(_enemyDiedToken);
            _context.Events.Unsubscribe(_enemyTelegraphToken);
            _context.Events.Unsubscribe(_enemyAbilityToken);
        }

        _recentRequests.Clear();
        _activeEntries.Clear();
        _context = null;
    }

    private void Add(string kind, Foundation.ContentId sourceId)
    {
        if (_context == null)
        {
            return;
        }

        var category = MapKindToBudgetCategory(kind);
        if (!_budget.CanRequest(category, CountActive(category)))
        {
            _context.State.DebugMetrics.FeedbackBudgetDropsThisFrame++;
            return;
        }

        if (_recentRequests.Count >= 64)
        {
            _recentRequests.Dequeue();
        }

        var request = new FeedbackRequest(kind, sourceId, Godot.Vector3.Zero, 1);
        _recentRequests.Enqueue(request);
        _activeEntries.Add(new ActiveFeedbackEntry(category, ActiveLifetimeFor(category)));
        _context.State.DebugMetrics.FeedbackRequestsThisFrame++;
        _context.State.DebugMetrics.ActiveFeedbackBudgetSummary = BuildActiveBudgetSummary();
        _context.State.DebugMetrics.AudioMixSummary = $"{_audioRules.ResolveGroup(kind)}; {_audioRules.BuildSummary()}";
        _context.State.DebugMetrics.ReadabilitySummary = BuildReadabilitySummary(_context);
        _context?.Events.Publish(new FeedbackRequestedEvent(kind, sourceId, request.Position));
    }

    private void DecayActiveEntries(float delta)
    {
        for (var index = _activeEntries.Count - 1; index >= 0; index--)
        {
            var entry = _activeEntries[index];
            entry.RemainingSeconds -= delta;
            if (entry.RemainingSeconds <= 0f)
            {
                _activeEntries.RemoveAt(index);
                continue;
            }

            _activeEntries[index] = entry;
        }
    }

    private int CountActive(string category)
    {
        var count = 0;
        foreach (var entry in _activeEntries)
        {
            if (string.Equals(entry.Category, category, StringComparison.OrdinalIgnoreCase))
            {
                count++;
            }
        }

        return count;
    }

    private string BuildActiveBudgetSummary()
    {
        var parts = new List<string>();
        foreach (var budget in _budget.MaxSimultaneous)
        {
            var active = CountActive(budget.Key);
            if (active > 0)
            {
                parts.Add($"{budget.Key}:{active}/{budget.Value}");
            }
        }

        if (parts.Count == 0)
        {
            parts.Add("active none");
        }

        parts.Add($"limits {_budget.BuildSummary()}");
        return string.Join("; ", parts);
    }

    private static float ActiveLifetimeFor(string category)
    {
        return category switch
        {
            "cast" => 0.35f,
            "projectile" => 1.25f,
            "impact" => 0.75f,
            "aoe" => 2.0f,
            "status" => 1.5f,
            "summon" => 1.25f,
            "boss" => 1.5f,
            "ui" => 0.5f,
            "audio_horde" => 1.0f,
            _ => 0.75f
        };
    }

    private static string MapKindToBudgetCategory(string kind)
    {
        if (kind.StartsWith("enemy_ability", StringComparison.OrdinalIgnoreCase)
            || kind.StartsWith("enemy_telegraph", StringComparison.OrdinalIgnoreCase))
        {
            return "boss";
        }

        if (kind.StartsWith("status", StringComparison.OrdinalIgnoreCase))
        {
            return "status";
        }

        return kind switch
        {
            "damage" or "death" => "impact",
            "aoe" => "aoe",
            "summon" => "summon",
            _ => kind
        };
    }

    private static string BuildReadabilitySummary(RunContext context)
    {
        return string.Create(
            CultureInfo.InvariantCulture,
            $"shake {context.Settings.ScreenshakeScale:0.##}, flash {context.Settings.FlashScale:0.##}, text {context.Settings.TextScale:0.##}x");
    }

    private struct ActiveFeedbackEntry
    {
        public ActiveFeedbackEntry(string category, float remainingSeconds)
        {
            Category = category;
            RemainingSeconds = remainingSeconds;
        }

        public string Category { get; }

        public float RemainingSeconds { get; set; }
    }
}
