using TheLastMage.Data.Definitions;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;

namespace TheLastMage.Data.Runtime;

public sealed class ContentRegistry
{
    private readonly Dictionary<ContentId, SpellRuntimeDefinition> _spells = new();
    private readonly Dictionary<ContentId, ItemRuntimeDefinition> _items = new();
    private readonly Dictionary<ContentId, EnemyRuntimeDefinition> _enemies = new();
    private readonly Dictionary<ContentId, WaveRuntimeDefinition> _waves = new();
    private readonly Dictionary<ContentId, DefenseRuntimeDefinition> _defenses = new();
    private readonly Dictionary<ContentId, MageRuntimeDefinition> _mages = new();
    private readonly Dictionary<ContentId, AchievementRuntimeDefinition> _achievements = new();

    public IReadOnlyDictionary<ContentId, SpellRuntimeDefinition> Spells => _spells;
    public IReadOnlyDictionary<ContentId, ItemRuntimeDefinition> Items => _items;
    public IReadOnlyDictionary<ContentId, EnemyRuntimeDefinition> Enemies => _enemies;
    public IReadOnlyDictionary<ContentId, WaveRuntimeDefinition> Waves => _waves;
    public IReadOnlyDictionary<ContentId, DefenseRuntimeDefinition> Defenses => _defenses;
    public IReadOnlyDictionary<ContentId, MageRuntimeDefinition> Mages => _mages;
    public IReadOnlyDictionary<ContentId, AchievementRuntimeDefinition> Achievements => _achievements;

    public ValidationResult Load(ContentCatalog catalog)
    {
        Clear();
        var result = ValidationResult.Valid();
        Add(catalog, result);
        ResolveReferences(result);
        return result;
    }

    public ValidationResult AddGeneratedContent(ContentCatalog catalog)
    {
        var result = ValidationResult.Valid();
        Add(catalog, result);
        ResolveReferences(result);
        return result;
    }

    private void Add(ContentCatalog catalog, ValidationResult result)
    {

        foreach (var spell in catalog.Spells)
        {
            AddUnique(_spells, ConvertSpell(spell), result, "spell");
        }

        foreach (var item in catalog.Items)
        {
            AddUnique(_items, ConvertItem(item), result, "item");
        }

        foreach (var enemy in catalog.Enemies)
        {
            AddUnique(_enemies, ConvertEnemy(enemy), result, "enemy");
        }

        foreach (var wave in catalog.Waves)
        {
            AddUnique(_waves, ConvertWave(wave), result, "wave");
        }

        foreach (var defense in catalog.Defenses)
        {
            AddUnique(_defenses, ConvertDefense(defense), result, "defense");
        }

        foreach (var mage in catalog.Mages)
        {
            AddUnique(_mages, ConvertMage(mage), result, "mage");
        }

        foreach (var achievement in catalog.Achievements)
        {
            AddUnique(_achievements, ConvertAchievement(achievement), result, "achievement");
        }
    }

    public ItemRuntimeDefinition GetItem(ContentId id) => _items[id];

    public EnemyRuntimeDefinition GetEnemy(ContentId id) => _enemies[id];

    public DefenseRuntimeDefinition GetDefense(ContentId id) => _defenses[id];

    public bool TryGetSpell(ContentId id, out SpellRuntimeDefinition? definition)
    {
        return _spells.TryGetValue(id, out definition);
    }

    public bool TryGetEnemy(ContentId id, out EnemyRuntimeDefinition? definition)
    {
        return _enemies.TryGetValue(id, out definition);
    }

    public bool TryGetDefense(ContentId id, out DefenseRuntimeDefinition? definition)
    {
        return _defenses.TryGetValue(id, out definition);
    }

    public bool TryGetMage(ContentId id, out MageRuntimeDefinition? definition)
    {
        return _mages.TryGetValue(id, out definition);
    }

    private void Clear()
    {
        _spells.Clear();
        _items.Clear();
        _enemies.Clear();
        _waves.Clear();
        _defenses.Clear();
        _mages.Clear();
        _achievements.Clear();
    }

    private static void AddUnique<TDefinition>(
        Dictionary<ContentId, TDefinition> dictionary,
        TDefinition definition,
        ValidationResult result,
        string contentKind)
        where TDefinition : notnull
    {
        var id = definition switch
        {
            SpellRuntimeDefinition value => value.Id,
            ItemRuntimeDefinition value => value.Id,
            EnemyRuntimeDefinition value => value.Id,
            WaveRuntimeDefinition value => value.Id,
            DefenseRuntimeDefinition value => value.Id,
            MageRuntimeDefinition value => value.Id,
            AchievementRuntimeDefinition value => value.Id,
            _ => ContentId.From(string.Empty)
        };

        if (!id.IsValid)
        {
            result.AddError("content.empty_id", $"{contentKind} has an empty ID.");
            return;
        }

        if (dictionary.ContainsKey(id))
        {
            result.AddError("content.duplicate_id", $"Duplicate {contentKind} ID '{id}'.");
            return;
        }

        dictionary.Add(id, definition);
    }

    private void ResolveReferences(ValidationResult result)
    {
        foreach (var wave in _waves.Values)
        {
            foreach (var enemyId in wave.EnemyIds)
            {
                if (!_enemies.ContainsKey(enemyId))
                {
                    result.AddError("content.missing_enemy", $"Wave '{wave.Id}' references missing enemy '{enemyId}'.");
                }
            }

            foreach (var group in wave.SpawnGroups)
            {
                if (!_enemies.ContainsKey(group.EnemyId))
                {
                    result.AddError("content.missing_enemy", $"Wave '{wave.Id}' spawn group references missing enemy '{group.EnemyId}'.");
                }
            }
        }

        foreach (var mage in _mages.Values)
        {
            foreach (var spellId in mage.StartingSpellIds)
            {
                if (!_spells.ContainsKey(spellId))
                {
                    result.AddError("content.missing_spell", $"Mage '{mage.Id}' references missing spell '{spellId}'.");
                }
            }
        }

        foreach (var achievement in _achievements.Values)
        {
            foreach (var itemId in achievement.UnlockItemIds)
            {
                if (!_items.ContainsKey(itemId))
                {
                    result.AddError("content.missing_item", $"Achievement '{achievement.Id}' references missing item '{itemId}'.");
                }
            }

            foreach (var mageId in achievement.UnlockMageIds)
            {
                if (!_mages.ContainsKey(mageId))
                {
                    result.AddError("content.missing_mage", $"Achievement '{achievement.Id}' references missing mage '{mageId}'.");
                }
            }
        }
    }

    private static SpellRuntimeDefinition ConvertSpell(SpellDefinition definition)
    {
        return new SpellRuntimeDefinition(
            ContentId.From(definition.Id),
            definition.DisplayName,
            definition.Description,
            ConvertTags(definition.Tags),
            definition.CooldownSeconds,
            definition.ManaCost,
            ContentId.From(definition.FeedbackProfileId),
            definition.Effects.Select(ConvertEffect).ToArray());
    }

    private static ItemRuntimeDefinition ConvertItem(ItemDefinition definition)
    {
        var id = ContentId.From(definition.Id);
        var compiledModifiers = ItemEffectCompiler.BuildModifierSpecs(definition.Effects);
        var compiledProcs = ItemEffectCompiler.BuildProcSpecs(definition.Effects);

        return new ItemRuntimeDefinition(
            id,
            definition.ItemNumber,
            definition.Kind.ToString(),
            definition.DisplayName,
            definition.Description,
            definition.HiddenDescription,
            ConvertTags(definition.Tags),
            definition.PoolWeights.Select(ConvertItemPoolWeight).ToArray(),
            ContentId.From(definition.UnlockAchievementId),
            definition.HasUnlimitedActivations,
            definition.MaxActivations,
            definition.IsFlavorOnly,
            definition.GetResolvedRevealedStatText(),
            definition.RevealedBehaviorText,
            definition.GetCombinedRevealedText(),
            definition.Icon,
            definition.EnemyFormPresentationScene,
            definition.Effects.Select(ConvertItemEffect).ToArray(),
            definition.ActiveEffects.Select(ConvertEffect).ToArray(),
            compiledModifiers.Select(modifier => ConvertModifier(id, modifier)).ToArray(),
            compiledProcs.Select(proc => ConvertProc(id, proc)).ToArray(),
            definition.PlayersPicked,
            definition.WinRatePercent);
    }

    private static EnemyRuntimeDefinition ConvertEnemy(EnemyArchetypeDefinition definition)
    {
        return new EnemyRuntimeDefinition(
            ContentId.From(definition.Id),
            definition.DisplayName,
            definition.Description,
            ConvertTags(definition.Tags),
            definition.Family.ToString(),
            definition.Role.ToString(),
            definition.MaxHealth,
            definition.MoveSpeed,
            definition.AttackDamage,
            definition.AttackRange,
            definition.PreferredRange,
            definition.AbilityRange,
            definition.AbilityCooldownSeconds,
            definition.AbilityTelegraphSeconds,
            definition.AbilityProjectileSpeed,
            definition.MaxActive,
            definition.PresentationScene,
            definition.Abilities.Select(ConvertEnemyAbility).ToArray());
    }

    private static WaveRuntimeDefinition ConvertWave(WaveDefinition definition)
    {
        return new WaveRuntimeDefinition(
            ContentId.From(definition.Id),
            definition.DisplayName,
            definition.DayIndex,
            definition.DurationSeconds,
            definition.TargetActiveEnemies,
            definition.SpawnIntervalSeconds,
            definition.Intensity,
            definition.EnemyIds.Select(ContentId.From).ToArray(),
            definition.SpawnGroups.Select(ConvertSpawnGroup).ToArray(),
            definition.SpawnLanes.Select(ConvertSpawnLane).ToArray());
    }

    private static DefenseRuntimeDefinition ConvertDefense(DefenseDefinition definition)
    {
        return new DefenseRuntimeDefinition(
            ContentId.From(definition.Id),
            definition.DisplayName,
            ConvertTags(definition.Tags),
            definition.PlacementRule.ToString(),
            definition.MaxHealth,
            definition.Cost,
            definition.RepairCost,
            definition.PlacementRadius,
            definition.BlockRadius,
            definition.TriggerRadius,
            definition.ExplosionRadius,
            definition.FuseSeconds,
            definition.RechargeSeconds,
            definition.MaxCharges,
            definition.Damage);
    }

    private static WaveSpawnGroupRuntimeDefinition ConvertSpawnGroup(WaveSpawnGroupDefinition definition)
    {
        return new WaveSpawnGroupRuntimeDefinition(
            ContentId.From(definition.EnemyId),
            definition.Count,
            definition.StartTimeSeconds,
            definition.SpawnIntervalSeconds,
            definition.IntensityMultiplier);
    }

    private static WaveSpawnLaneRuntimeDefinition ConvertSpawnLane(WaveSpawnLaneDefinition definition)
    {
        return new WaveSpawnLaneRuntimeDefinition(
            definition.Id,
            definition.SpawnPosition,
            definition.ApproachPosition,
            definition.Weight);
    }

    private static MageRuntimeDefinition ConvertMage(MageDefinition definition)
    {
        return new MageRuntimeDefinition(
            ContentId.From(definition.Id),
            definition.DisplayName,
            definition.Description,
            ConvertTags(definition.Tags),
            definition.MaxHealth,
            definition.MoveSpeed,
            definition.FireRate,
            definition.Luck,
            definition.PresentationScene,
            definition.StartingSpellIds.Select(ContentId.From).ToArray());
    }

    private static AchievementRuntimeDefinition ConvertAchievement(AchievementDefinition definition)
    {
        return new AchievementRuntimeDefinition(
            ContentId.From(definition.Id),
            definition.DisplayName,
            definition.RequirementKey,
            definition.UnlockMageIds.Select(ContentId.From).ToArray(),
            definition.UnlockItemIds.Select(ContentId.From).ToArray());
    }

    private static EffectRuntimeSpec ConvertEffect(EffectSpec spec)
    {
        return new EffectRuntimeSpec(
            spec.EffectType,
            spec.Value,
            spec.Radius,
            spec.DurationSeconds,
            spec.DurationScaling.ToString(),
            ContentId.From(spec.TargetStatId));
    }

    private static EnemyAbilityRuntimeSpec ConvertEnemyAbility(EffectSpec spec)
    {
        return new EnemyAbilityRuntimeSpec(
            spec.EffectType,
            spec.Value,
            spec.Radius,
            spec.DurationSeconds,
            spec.DurationScaling.ToString(),
            ContentId.From(spec.TargetStatId));
    }

    private static ModifierRuntimeSpec ConvertModifier(ContentId sourceId, ModifierSpec spec)
    {
        return new ModifierRuntimeSpec(
            sourceId,
            StatId.From(spec.StatId),
            spec.Operation.ToString(),
            spec.Value,
            spec.Priority,
            TagId.From(spec.TargetTag));
    }

    private static ProcRuntimeSpec ConvertProc(ContentId sourceId, ProcSpec spec)
    {
        return new ProcRuntimeSpec(
            sourceId,
            spec.Trigger.ToString(),
            spec.EffectType,
            spec.Chance,
            spec.Value,
            TagId.From(spec.TargetTag));
    }

    private static ItemEffectRuntimeSpec ConvertItemEffect(ItemEffectSpec spec)
    {
        return new ItemEffectRuntimeSpec(
            spec.Kind.ToString(),
            StatId.From(spec.StatId),
            spec.Value,
            spec.SecondaryValue,
            spec.TertiaryValue,
            Math.Clamp(spec.ChancePercent / 100f, 0f, 1f),
            TagId.From(spec.TargetTag));
    }

    private static ItemPoolWeightRuntimeSpec ConvertItemPoolWeight(ItemPoolWeightDefinition spec)
    {
        return new ItemPoolWeightRuntimeSpec(
            ContentId.From(spec.PoolId),
            spec.Weight);
    }

    private static IReadOnlyList<TagId> ConvertTags(IEnumerable<string> tags)
    {
        return GameplayTagSet.From(tags.Select(TagId.From));
    }
}
