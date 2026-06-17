using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Random;
using TheLastMage.Gameplay.Items;
using TheLastMage.Save;

namespace TheLastMage.Gameplay.Loot;

public sealed class OfferGenerator
{
    public const int DefaultOfferCount = 3;

    public IReadOnlyList<MarketOffer> Generate(
        ContentRegistry content,
        ProfileSaveDto profile,
        BuildState build,
        System.Random random,
        ContentId poolId,
        int offerCount = DefaultOfferCount)
    {
        var candidates = BuildCandidates(content, profile, build, poolId);

        var result = new List<MarketOffer>(offerCount);
        while (candidates.Count > 0 && result.Count < offerCount)
        {
            var itemIndex = PickWeightedIndex(candidates, random, poolId);
            var item = candidates[itemIndex];
            result.Add(new MarketOffer(item.Id));
            candidates.RemoveAt(itemIndex);
        }

        return result;
    }

    public IReadOnlyList<MarketOffer> Generate(
        ContentRegistry content,
        ProfileSaveDto profile,
        BuildState build,
        System.Random random,
        int offerCount = DefaultOfferCount)
    {
        return Generate(content, profile, build, random, ContentId.From(ItemPoolIds.NightMarket), offerCount);
    }

    private static List<ItemRuntimeDefinition> BuildCandidates(ContentRegistry content, ProfileSaveDto profile, BuildState build, ContentId poolId)
    {
        var lockedByAchievement = new HashSet<ContentId>();
        foreach (var achievement in content.Achievements.Values)
        {
            foreach (var itemId in achievement.UnlockItemIds)
            {
                lockedByAchievement.Add(itemId);
            }
        }

        var unlocked = profile.UnlockedItemIds
            .Select(ContentId.From)
            .Where(id => id.IsValid)
            .ToHashSet();

        var candidates = new List<ItemRuntimeDefinition>();
        foreach (var item in content.Items.Values)
        {
            if (build.HasAcquiredItem(item.Id))
            {
                continue;
            }

            if (lockedByAchievement.Contains(item.Id) && !unlocked.Contains(item.Id))
            {
                continue;
            }

            if (GetPoolWeight(item, poolId) > 0f)
            {
                candidates.Add(item);
            }
        }

        return candidates;
    }

    private static int PickWeightedIndex(IReadOnlyList<ItemRuntimeDefinition> candidates, System.Random random, ContentId poolId)
    {
        var total = 0f;
        for (var i = 0; i < candidates.Count; i++)
        {
            total += MathF.Max(0f, GetPoolWeight(candidates[i], poolId));
        }

        if (total <= 0f)
        {
            return random.Next(candidates.Count);
        }

        var roll = (float)(random.NextDouble() * total);
        var cumulative = 0f;
        for (var i = 0; i < candidates.Count; i++)
        {
            cumulative += MathF.Max(0f, GetPoolWeight(candidates[i], poolId));
            if (roll <= cumulative)
            {
                return i;
            }
        }

        return candidates.Count - 1;
    }

    private static float GetPoolWeight(ItemRuntimeDefinition item, ContentId poolId)
    {
        for (var i = 0; i < item.PoolWeights.Count; i++)
        {
            var poolWeight = item.PoolWeights[i];
            if (poolWeight.PoolId.Equals(poolId))
            {
                return poolWeight.Weight;
            }
        }

        return 0f;
    }
}
