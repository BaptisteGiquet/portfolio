using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Save;

namespace TheLastMage.Debugging.Tools;

public sealed class LootSimulator
{
    private readonly OfferGenerator _offerGenerator = new();

    public LootSimulationSummary Run(
        ContentRegistry content,
        ProfileSaveDto sourceProfile,
        int rolls,
        int seed,
        bool unlockAchievementItems = false)
    {
        var profile = CopyProfile(sourceProfile);
        if (unlockAchievementItems)
        {
            foreach (var achievement in content.Achievements.Values)
            {
                foreach (var itemId in achievement.UnlockItemIds)
                {
                    var value = itemId.ToString();
                    if (!profile.UnlockedItemIds.Contains(value))
                    {
                        profile.UnlockedItemIds.Add(value);
                    }
                }
            }
        }

        var lockedItems = CountLockedItems(content, profile);
        var summary = new LootSimulationSummary
        {
            Rolls = rolls,
            Seed = seed,
            LockedItemsExcluded = lockedItems
        };

        var random = new System.Random(seed);
        var discovered = profile.DiscoveredItemIds.Select(ContentId.From).Where(id => id.IsValid).ToHashSet();
        for (var roll = 0; roll < rolls; roll++)
        {
            var offers = _offerGenerator.Generate(content, profile, new BuildState(), random);
            if (offers.Count == 0)
            {
                summary.EmptyRolls++;
                continue;
            }

            foreach (var offer in offers)
            {
                summary.RecordOffer(offer.ItemId, discovered.Contains(offer.ItemId));
            }
        }

        return summary;
    }

    private static int CountLockedItems(ContentRegistry content, ProfileSaveDto profile)
    {
        var unlocked = profile.UnlockedItemIds.Select(ContentId.From).Where(id => id.IsValid).ToHashSet();
        var locked = new HashSet<ContentId>();
        foreach (var achievement in content.Achievements.Values)
        {
            foreach (var itemId in achievement.UnlockItemIds)
            {
                if (!unlocked.Contains(itemId))
                {
                    locked.Add(itemId);
                }
            }
        }

        return locked.Count;
    }

    private static ProfileSaveDto CopyProfile(ProfileSaveDto profile)
    {
        return new ProfileSaveDto
        {
            UnlockedMageIds = profile.UnlockedMageIds.ToList(),
            UnlockedSpellIds = profile.UnlockedSpellIds.ToList(),
            UnlockedItemIds = profile.UnlockedItemIds.ToList(),
            DiscoveredItemIds = profile.DiscoveredItemIds.ToList(),
            CompletedAchievementIds = profile.CompletedAchievementIds.ToList()
        };
    }
}
