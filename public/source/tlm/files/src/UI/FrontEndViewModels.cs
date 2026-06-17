using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Save;

namespace TheLastMage.UI;

public sealed record FrontEndMageViewModel(
    ContentId MageId,
    string DisplayName,
    string Description,
    bool IsUnlocked,
    string UnlockRequirement,
    string StartingSpell,
    float MaxHealth,
    float MoveSpeed,
    float FireRate,
    float Luck,
    ProfileMageStatisticsDto Stats);

public sealed record FrontEndAchievementViewModel(
    ContentId AchievementId,
    string DisplayName,
    string Requirement,
    bool IsComplete,
    string Unlocks);

public sealed record FrontEndProfileStatsViewModel(
    int RunsPlayed,
    int Wins,
    int Deaths,
    float TimeSurvivedSeconds,
    int EnemiesKilled,
    int HumanityKilled,
    int ItemsDiscovered,
    int MagesUnlocked,
    int AchievementsCompleted,
    int TotalSpellCasts,
    IReadOnlyList<KeyValuePair<string, int>> SpellUsage);

public static class FrontEndViewModels
{
    public static IReadOnlyList<FrontEndMageViewModel> BuildMageSelection(ContentRegistry content, ProfileSaveDto profile)
    {
        var unlockRequirements = content.Achievements.Values
            .SelectMany(achievement => achievement.UnlockMageIds.Select(mageId => new { mageId, achievement }))
            .GroupBy(pair => pair.mageId)
            .ToDictionary(
                group => group.Key,
                group => string.Join(", ", group.Select(pair => pair.achievement.RequirementKey).Where(value => !string.IsNullOrWhiteSpace(value)).Distinct(StringComparer.Ordinal)),
                ContentIdComparer.Instance);

        return content.Mages.Values
            .OrderBy(mage => mage.DisplayName, StringComparer.Ordinal)
            .Select(mage =>
            {
                var stats = profile.RunStatistics.MageStatistics.GetValueOrDefault(mage.Id.Value) ?? new ProfileMageStatisticsDto();
                var startingSpell = mage.StartingSpellIds.Count == 0
                    ? "-"
                    : string.Join(", ", mage.StartingSpellIds.Select(id => content.Spells.TryGetValue(id, out var spell) ? spell.DisplayName : id.ToString()));
                return new FrontEndMageViewModel(
                    mage.Id,
                    mage.DisplayName,
                    mage.Description,
                    profile.UnlockedMageIds.Contains(mage.Id.Value, StringComparer.Ordinal),
                    unlockRequirements.GetValueOrDefault(mage.Id) ?? "-",
                    startingSpell,
                    mage.MaxHealth,
                    mage.MoveSpeed,
                    mage.FireRate,
                    mage.Luck,
                    stats);
            })
            .ToArray();
    }

    public static FrontEndProfileStatsViewModel BuildProfileStats(ContentRegistry content, ProfileSaveDto profile)
    {
        var stats = profile.RunStatistics;
        return new FrontEndProfileStatsViewModel(
            stats.TotalNormalRuns,
            stats.Victories,
            stats.Defeats,
            stats.TotalPlaySeconds,
            stats.TotalEnemiesKilled,
            stats.TotalHumanityKilled,
            profile.DiscoveredItemIds.Count,
            profile.UnlockedMageIds.Count(id => content.Mages.ContainsKey(ContentId.From(id))),
            profile.CompletedAchievementIds.Count,
            stats.TotalSpellsCast,
            stats.SpellCastsBySpell
                .OrderByDescending(pair => pair.Value)
                .ThenBy(pair => pair.Key, StringComparer.Ordinal)
                .ToArray());
    }

    public static IReadOnlyList<FrontEndAchievementViewModel> BuildAchievements(ContentRegistry content, ProfileSaveDto profile)
    {
        return content.Achievements.Values
            .OrderBy(achievement => achievement.DisplayName, StringComparer.Ordinal)
            .Select(achievement => new FrontEndAchievementViewModel(
                achievement.Id,
                achievement.DisplayName,
                achievement.RequirementKey,
                profile.CompletedAchievementIds.Contains(achievement.Id.Value, StringComparer.Ordinal),
                BuildUnlockText(achievement.UnlockMageIds, achievement.UnlockItemIds)))
            .ToArray();
    }

    private static string BuildUnlockText(IReadOnlyList<ContentId> mageIds, IReadOnlyList<ContentId> itemIds)
    {
        var values = mageIds.Select(id => $"mage:{id}").Concat(itemIds.Select(id => $"item:{id}")).ToArray();
        return values.Length == 0 ? "-" : string.Join(", ", values);
    }

    private sealed class ContentIdComparer : IEqualityComparer<ContentId>
    {
        public static readonly ContentIdComparer Instance = new();

        public bool Equals(ContentId x, ContentId y)
        {
            return x.Equals(y);
        }

        public int GetHashCode(ContentId obj)
        {
            return obj.Value.GetHashCode(StringComparison.Ordinal);
        }
    }
}
