using TheLastMage.Foundation;

namespace TheLastMage.Save;

public static class ProfileDefaultsService
{
    public const string DefaultMageId = "mage_recluse";

    public static bool EnsureCurrent(ProfileSaveDto profile)
    {
        var changed = false;
        if (string.IsNullOrWhiteSpace(profile.PreferredMageId))
        {
            profile.PreferredMageId = DefaultMageId;
            changed = true;
        }

        profile.UnlockedMageIds ??= new List<string>();
        profile.UnlockedSpellIds ??= new List<string>();
        profile.UnlockedItemIds ??= new List<string>();
        profile.DiscoveredItemIds ??= new List<string>();
        profile.CompletedAchievementIds ??= new List<string>();
        profile.RunStatistics ??= new ProfileRunStatisticsDto();
        profile.RunStatistics.RunsByMage ??= new Dictionary<string, int>(StringComparer.Ordinal);
        profile.RunStatistics.SpellCastsBySpell ??= new Dictionary<string, int>(StringComparer.Ordinal);
        profile.RunStatistics.MageStatistics ??= new Dictionary<string, ProfileMageStatisticsDto>(StringComparer.Ordinal);
        if (profile.LastCompletedRun != null)
        {
            profile.LastCompletedRun.SpellsUsed ??= new Dictionary<string, int>(StringComparer.Ordinal);
            profile.LastCompletedRun.ItemsDiscovered ??= new List<string>();
        }

        changed |= NormalizeList(profile.UnlockedMageIds);
        changed |= NormalizeList(profile.UnlockedSpellIds);
        changed |= NormalizeList(profile.UnlockedItemIds);
        changed |= NormalizeList(profile.DiscoveredItemIds);
        changed |= NormalizeList(profile.CompletedAchievementIds);
        changed |= NormalizeDictionary(profile.RunStatistics.RunsByMage);
        changed |= NormalizeDictionary(profile.RunStatistics.SpellCastsBySpell);
        changed |= NormalizeMageStatistics(profile.RunStatistics.MageStatistics);

        if (!profile.UnlockedMageIds.Contains(DefaultMageId, StringComparer.Ordinal))
        {
            profile.UnlockedMageIds.Add(DefaultMageId);
            changed = true;
        }

        if (!ContentId.From(profile.PreferredMageId).IsValid
            || !profile.UnlockedMageIds.Contains(profile.PreferredMageId, StringComparer.Ordinal))
        {
            profile.PreferredMageId = profile.UnlockedMageIds.FirstOrDefault(id => ContentId.From(id).IsValid) ?? DefaultMageId;
            changed = true;
        }

        return changed;
    }

    private static bool NormalizeList(List<string> values)
    {
        var changed = false;
        var seen = new HashSet<string>(StringComparer.Ordinal);
        for (var i = 0; i < values.Count; i++)
        {
            var value = values[i];
            if (string.IsNullOrWhiteSpace(value) || !seen.Add(value))
            {
                values.RemoveAt(i);
                i--;
                changed = true;
            }
        }

        return changed;
    }

    private static bool NormalizeDictionary(Dictionary<string, int> values)
    {
        var changed = false;
        foreach (var key in values.Keys.ToArray())
        {
            if (!string.IsNullOrWhiteSpace(key) && values[key] > 0)
            {
                continue;
            }

            values.Remove(key);
            changed = true;
        }

        return changed;
    }

    private static bool NormalizeMageStatistics(Dictionary<string, ProfileMageStatisticsDto> values)
    {
        var changed = false;
        foreach (var (key, stats) in values.ToArray())
        {
            if (string.IsNullOrWhiteSpace(key) || stats == null)
            {
                values.Remove(key);
                changed = true;
                continue;
            }

            stats.SpellCastsBySpell ??= new Dictionary<string, int>(StringComparer.Ordinal);
            changed |= NormalizeDictionary(stats.SpellCastsBySpell);
        }

        return changed;
    }
}
