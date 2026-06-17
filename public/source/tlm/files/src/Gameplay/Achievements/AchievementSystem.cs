using Godot;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Achievements;

public sealed class AchievementSystem : IGameSystem
{
    private const string DiscoverFirstItemRequirement = "discover_first_item";
    private const string SurviveFirstDayRequirement = "survive_first_day";
    private const string DefeatFirstChampionRequirement = "defeat_first_champion";

    private RunContext? _context;
    private SubscriptionToken _itemAcquiredToken;
    private SubscriptionToken _dayStartedToken;
    private SubscriptionToken _enemyDiedToken;

    public void Initialize(RunContext context)
    {
        _context = context;
        _itemAcquiredToken = context.Events.Subscribe<ItemAcquiredEvent>(_ => CompleteByRequirement(DiscoverFirstItemRequirement));
        _dayStartedToken = context.Events.Subscribe<DayStartedEvent>(OnDayStarted);
        _enemyDiedToken = context.Events.Subscribe<EnemyDiedEvent>(OnEnemyDied);
    }

    public void Tick(float delta)
    {
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_itemAcquiredToken);
            _context.Events.Unsubscribe(_dayStartedToken);
            _context.Events.Unsubscribe(_enemyDiedToken);
        }

        _context = null;
    }

    public bool CompleteAchievement(ContentId achievementId)
    {
        if (_context == null || !_context.Content.Achievements.TryGetValue(achievementId, out var achievement))
        {
            return false;
        }

        if (_context.Profile.CompletedAchievementIds.Contains(achievementId.Value, StringComparer.Ordinal))
        {
            return false;
        }

        _context.Profile.CompletedAchievementIds.Add(achievementId.Value);
        UnlockIds(_context.Profile.UnlockedItemIds, achievement.UnlockItemIds);
        UnlockIds(_context.Profile.UnlockedMageIds, achievement.UnlockMageIds);
        _context.SaveProfile(_context.Profile);
        _context.Events.Publish(new AchievementCompletedEvent(achievementId));
        GD.Print($"AchievementCompleted achievement={achievementId}");
        return true;
    }

    private void CompleteByRequirement(string requirementKey)
    {
        if (_context == null)
        {
            return;
        }

        foreach (var achievement in _context.Content.Achievements.Values)
        {
            if (!string.Equals(achievement.RequirementKey, requirementKey, StringComparison.Ordinal))
            {
                continue;
            }

            CompleteAchievement(achievement.Id);
        }
    }

    private void OnDayStarted(DayStartedEvent gameEvent)
    {
        if (gameEvent.DayIndex > 1)
        {
            CompleteByRequirement(SurviveFirstDayRequirement);
        }
    }

    private void OnEnemyDied(EnemyDiedEvent gameEvent)
    {
        if (_context?.Content.TryGetEnemy(gameEvent.EnemyArchetypeId, out var enemy) == true
            && enemy != null
            && GameplayTagPath.MatchesAny(TagId.From("enemy.rank.champion"), enemy.Tags))
        {
            CompleteByRequirement(DefeatFirstChampionRequirement);
        }
    }

    private static void UnlockIds(List<string> destination, IReadOnlyList<ContentId> unlockIds)
    {
        foreach (var unlockId in unlockIds)
        {
            if (unlockId.IsValid && !destination.Contains(unlockId.Value, StringComparer.Ordinal))
            {
                destination.Add(unlockId.Value);
            }
        }
    }
}
