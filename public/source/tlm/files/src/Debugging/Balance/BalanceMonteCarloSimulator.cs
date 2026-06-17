using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Run;
using TheLastMage.Save;

namespace TheLastMage.Debugging.Balance;

public sealed class BalanceMonteCarloSimulator
{
    private readonly OfferGenerator _offerGenerator = new();

    public BalanceMonteCarloResult Run(
        ContentRegistry content,
        BalanceExperimentDefinition experiment,
        ProfileSaveDto sourceProfile,
        IReadOnlyList<IBalanceBotPolicy> policies,
        int seed)
    {
        var result = new BalanceMonteCarloResult
        {
            Trials = experiment.MonteCarloTrials,
            NightsPerTrial = experiment.MonteCarloNightsPerTrial,
            Seed = seed
        };

        foreach (var policy in policies)
        {
            var summary = RunPolicy(content, experiment, sourceProfile, policy, seed);
            result.PolicySummaries.Add(summary);
        }

        return result;
    }

    private BalanceMonteCarloPolicySummary RunPolicy(
        ContentRegistry content,
        BalanceExperimentDefinition experiment,
        ProfileSaveDto sourceProfile,
        IBalanceBotPolicy policy,
        int seed)
    {
        var summary = new BalanceMonteCarloPolicySummary
        {
            PolicyId = policy.Id,
            Trials = experiment.MonteCarloTrials,
            LockedItemsExcluded = CountLockedItems(content, sourceProfile)
        };
        var discovered = sourceProfile.DiscoveredItemIds
            .Select(ContentId.From)
            .Where(id => id.IsValid)
            .ToHashSet();

        for (var trial = 0; trial < experiment.MonteCarloTrials; trial++)
        {
            var random = new System.Random(DeriveTrialSeed(seed, policy.Id, trial));
            var profile = CopyProfile(sourceProfile);
            var build = new BuildState();
            var currentSpellId = content.Spells.Keys.OrderBy(id => id.ToString(), StringComparer.Ordinal).FirstOrDefault();
            for (var night = 0; night < experiment.MonteCarloNightsPerTrial; night++)
            {
                var beforeAcquiredCount = build.AcquiredItemIds.Count;
                var offers = _offerGenerator.Generate(
                    content,
                    profile,
                    build,
                    random,
                    experiment.ItemPoolId);
                if (offers.Count == 0)
                {
                    summary.PoolStarvations++;
                    continue;
                }

                summary.OffersGenerated += offers.Count;
                var uniqueOffers = new HashSet<ContentId>();
                foreach (var offer in offers)
                {
                    if (!uniqueOffers.Add(offer.ItemId) || build.HasAcquiredItem(offer.ItemId))
                    {
                        summary.DuplicateOffersPrevented++;
                    }

                    var key = offer.ItemId.ToString();
                    summary.OfferCounts[key] = summary.OfferCounts.GetValueOrDefault(key) + 1;
                    if (discovered.Contains(offer.ItemId))
                    {
                        summary.DiscoveredOffers++;
                    }
                    else
                    {
                        summary.UndiscoveredOffers++;
                    }
                }

                var fakeContext = BalanceRunContextView.Create(content, profile, build, currentSpellId);
                var chosenIndex = policy.ChooseOffer(fakeContext, offers, random, currentSpellId);
                if (chosenIndex < 0 || chosenIndex >= offers.Count)
                {
                    summary.MissedOffers += offers.Count;
                    continue;
                }

                var chosen = offers[chosenIndex].ItemId;
                build.AddItem(chosen);
                profile.DiscoveredItemIds.Add(chosen.ToString());
                summary.Picks++;
                summary.PickCounts[chosen.ToString()] = summary.PickCounts.GetValueOrDefault(chosen.ToString()) + 1;
                summary.MissedOffers += Math.Max(0, offers.Count - 1);

                if (build.AcquiredItemIds.Count <= beforeAcquiredCount)
                {
                    summary.DuplicateOffersPrevented++;
                }
            }
        }

        return summary;
    }

    private static int CountLockedItems(ContentRegistry content, ProfileSaveDto profile)
    {
        var unlocked = profile.UnlockedItemIds
            .Select(ContentId.From)
            .Where(id => id.IsValid)
            .ToHashSet();
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

    private static int DeriveTrialSeed(int seed, string policyId, int trial)
    {
        unchecked
        {
            var hash = seed;
            foreach (var character in policyId)
            {
                hash = hash * 31 + character;
            }

            return hash * 397 ^ trial;
        }
    }
}

internal static class BalanceRunContextView
{
    public static RunContext Create(
        ContentRegistry content,
        ProfileSaveDto profile,
        BuildState build,
        ContentId currentSpellId)
    {
        var state = new RunState();
        if (content.Mages.Values.FirstOrDefault() is { } mage)
        {
            state.SelectMage(mage);
        }

        state.Spellbook.AssignSlot(0, currentSpellId);
        foreach (var itemId in build.AcquiredItemIds)
        {
            state.Build.AddItem(itemId);
        }

        return new RunContext
        {
            Content = content,
            State = state,
            Events = new Foundation.Events.GameEventBus(),
            Commands = new Gameplay.Commands.CommandDispatcher(),
            Pools = new Foundation.Pooling.PoolHub(),
            Random = new Foundation.Random.RandomService(1),
            Profile = profile,
            SaveProfile = _ => { },
            Settings = new Save.SettingsSaveDto(),
            SaveSettings = _ => { },
            ItemKnowledge = new Gameplay.Items.ItemKnowledgeService(profile, _ => { }),
            Systems = Array.Empty<Gameplay.Run.IGameSystem>()
        };
    }
}
