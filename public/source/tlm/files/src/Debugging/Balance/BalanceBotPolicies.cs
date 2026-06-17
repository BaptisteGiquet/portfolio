using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Debugging.Balance;

public static class BalanceBotPolicyIds
{
    public const string RandomBuild = "random_build";
    public const string AggressiveDamage = "aggressive_damage";
    public const string Survival = "survival";
    public const string Synergy = "synergy";
    public const string StationaryOutput = "stationary_output";
}

public interface IBalanceBotPolicy
{
    string Id { get; }

    int ChooseOffer(
        RunContext context,
        IReadOnlyList<MarketOffer> offers,
        System.Random random,
        ContentId currentSpellId);

    ContentId ChooseSpell(RunContext context, System.Random random);

    bool ShouldUseActivatable(RunContext context, float elapsedDaySeconds);
}

public static class BalanceBotPolicyFactory
{
    public static IBalanceBotPolicy Create(string id)
    {
        return id switch
        {
            BalanceBotPolicyIds.AggressiveDamage => new WeightedItemBotPolicy(
                BalanceBotPolicyIds.AggressiveDamage,
                offensiveWeight: 4f,
                survivalWeight: 0.35f,
                synergyWeight: 1.1f,
                randomWeight: 0.15f),
            BalanceBotPolicyIds.Survival => new WeightedItemBotPolicy(
                BalanceBotPolicyIds.Survival,
                offensiveWeight: 0.5f,
                survivalWeight: 4f,
                synergyWeight: 0.75f,
                randomWeight: 0.15f),
            BalanceBotPolicyIds.Synergy => new WeightedItemBotPolicy(
                BalanceBotPolicyIds.Synergy,
                offensiveWeight: 1.25f,
                survivalWeight: 1f,
                synergyWeight: 5f,
                randomWeight: 0.1f),
            BalanceBotPolicyIds.StationaryOutput => new WeightedItemBotPolicy(
                BalanceBotPolicyIds.StationaryOutput,
                offensiveWeight: 3.25f,
                survivalWeight: 0.15f,
                synergyWeight: 1.5f,
                randomWeight: 0f,
                stationarySpellChoice: true),
            _ => new RandomBuildBotPolicy()
        };
    }
}

public sealed class RandomBuildBotPolicy : IBalanceBotPolicy
{
    public string Id => BalanceBotPolicyIds.RandomBuild;

    public int ChooseOffer(
        RunContext context,
        IReadOnlyList<MarketOffer> offers,
        System.Random random,
        ContentId currentSpellId)
    {
        return offers.Count == 0 ? -1 : random.Next(offers.Count);
    }

    public ContentId ChooseSpell(RunContext context, System.Random random)
    {
        var available = context.State.Spellbook.Slots
            .Where(slot => slot.HasSpell)
            .Select(slot => slot.SpellId)
            .ToArray();
        return available.Length == 0 ? ContentId.From(string.Empty) : available[random.Next(available.Length)];
    }

    public bool ShouldUseActivatable(RunContext context, float elapsedDaySeconds)
    {
        return context.State.Build.ActivatableItem?.HasUsesRemaining == true
            && elapsedDaySeconds >= 2f;
    }
}

public sealed class WeightedItemBotPolicy : IBalanceBotPolicy
{
    private readonly float _offensiveWeight;
    private readonly float _survivalWeight;
    private readonly float _synergyWeight;
    private readonly float _randomWeight;
    private readonly bool _stationarySpellChoice;

    public WeightedItemBotPolicy(
        string id,
        float offensiveWeight,
        float survivalWeight,
        float synergyWeight,
        float randomWeight,
        bool stationarySpellChoice = false)
    {
        Id = id;
        _offensiveWeight = offensiveWeight;
        _survivalWeight = survivalWeight;
        _synergyWeight = synergyWeight;
        _randomWeight = randomWeight;
        _stationarySpellChoice = stationarySpellChoice;
    }

    public string Id { get; }

    public int ChooseOffer(
        RunContext context,
        IReadOnlyList<MarketOffer> offers,
        System.Random random,
        ContentId currentSpellId)
    {
        var bestIndex = -1;
        var bestScore = float.NegativeInfinity;
        var currentSpellTags = context.Content.TryGetSpell(currentSpellId, out var spell) && spell != null
            ? spell.Tags
            : Array.Empty<TagId>();
        for (var i = 0; i < offers.Count; i++)
        {
            if (!context.Content.Items.TryGetValue(offers[i].ItemId, out var item))
            {
                continue;
            }

            var score = ScoreItem(item, currentSpellTags);
            score += (float)random.NextDouble() * _randomWeight;
            if (score > bestScore)
            {
                bestScore = score;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    public ContentId ChooseSpell(RunContext context, System.Random random)
    {
        var spells = context.State.Spellbook.Slots
            .Where(slot => slot.HasSpell)
            .Select(slot => slot.SpellId)
            .ToArray();
        if (spells.Length == 0)
        {
            return ContentId.From(string.Empty);
        }

        if (_stationarySpellChoice)
        {
            return spells[0];
        }

        var best = spells[0];
        var bestScore = float.NegativeInfinity;
        foreach (var spellId in spells)
        {
            var score = context.Content.TryGetSpell(spellId, out var spell) && spell != null
                ? spell.Tags.Count(tag => tag.Value.Contains("damage", StringComparison.OrdinalIgnoreCase)
                    || tag.Value.Contains("projectile", StringComparison.OrdinalIgnoreCase)
                    || tag.Value.Contains("area", StringComparison.OrdinalIgnoreCase))
                : 0;
            var finalScore = score + (float)random.NextDouble() * 0.2f;
            if (finalScore > bestScore)
            {
                bestScore = finalScore;
                best = spellId;
            }
        }

        return best;
    }

    public bool ShouldUseActivatable(RunContext context, float elapsedDaySeconds)
    {
        return context.State.Build.ActivatableItem?.HasUsesRemaining == true
            && elapsedDaySeconds >= 1.5f
            && !string.Equals(Id, BalanceBotPolicyIds.StationaryOutput, StringComparison.Ordinal);
    }

    private float ScoreItem(ItemRuntimeDefinition item, IReadOnlyList<TagId> currentSpellTags)
    {
        var offensive = 0f;
        var survival = 0f;
        var synergy = 0f;

        foreach (var modifier in item.Modifiers)
        {
            var stat = modifier.StatId.Value;
            if (stat is "damage" or "fire_rate" or "spell_count" or "projectile_speed" or "projectile_pierce" or "projectile_split" or "area_radius")
            {
                offensive += 1f + MathF.Abs(modifier.Value);
            }
            else if (stat is "health" or "move_speed" or "duration" or "range" or "luck")
            {
                survival += 1f + MathF.Abs(modifier.Value);
            }

            if (modifier.TargetTag.IsValid && GameplayTagPath.MatchesAny(modifier.TargetTag, currentSpellTags))
            {
                synergy += 1.5f;
            }
        }

        foreach (var proc in item.Procs)
        {
            offensive += 1.25f;
            if (proc.EffectType.Contains("freeze", StringComparison.OrdinalIgnoreCase)
                || proc.EffectType.Contains("slow", StringComparison.OrdinalIgnoreCase)
                || proc.EffectType.Contains("sleep", StringComparison.OrdinalIgnoreCase))
            {
                survival += 1.5f;
            }

            if (proc.TargetTag.IsValid && GameplayTagPath.MatchesAny(proc.TargetTag, currentSpellTags))
            {
                synergy += 1.5f;
            }
        }

        foreach (var effect in item.Effects)
        {
            if (effect.TargetTag.IsValid && GameplayTagPath.MatchesAny(effect.TargetTag, currentSpellTags))
            {
                synergy += 1.25f;
            }

            if (effect.Kind.Contains("Health", StringComparison.OrdinalIgnoreCase)
                || effect.Kind.Contains("Freeze", StringComparison.OrdinalIgnoreCase)
                || effect.Kind.Contains("Stasis", StringComparison.OrdinalIgnoreCase))
            {
                survival += 1f;
            }

            if (effect.Kind.Contains("Damage", StringComparison.OrdinalIgnoreCase)
                || effect.Kind.Contains("Projectile", StringComparison.OrdinalIgnoreCase)
                || effect.Kind.Contains("Spell", StringComparison.OrdinalIgnoreCase)
                || effect.Kind.Contains("Explode", StringComparison.OrdinalIgnoreCase))
            {
                offensive += 1f;
            }
        }

        if (item.ActiveEffects.Count > 0)
        {
            survival += 0.5f;
            offensive += 0.5f;
        }

        return offensive * _offensiveWeight
            + survival * _survivalWeight
            + synergy * _synergyWeight
            + MathF.Max(0f, item.PoolWeights.Sum(pool => pool.Weight)) * 0.01f;
    }
}
