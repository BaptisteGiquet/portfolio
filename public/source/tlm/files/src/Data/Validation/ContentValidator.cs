using TheLastMage.Data.Definitions;
using TheLastMage.Data.Tags;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;
using TheLastMage.Localization;
using System.Text.RegularExpressions;

namespace TheLastMage.Data.Validation;

public sealed class ContentValidator
{
    private static readonly Regex RevealedMultiplierNotationPattern = new(
        @"(?<![A-Za-z0-9_])[-+]?\d+(?:\.\d+)?x\b",
        RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

    private static readonly HashSet<string> ValidModifierStats = new(StringComparer.OrdinalIgnoreCase)
    {
        "health",
        "move_speed",
        "damage",
        "fire_rate",
        "range",
        "luck",
        "duration",
        "radius",
        "projectile_speed",
        "status_power",
        "summon_damage",
        "defense_damage",
        "pierce",
        "chain_count",
        "spell_count",
        "projectile_split",
        "homing_strength",
        "homing_angle_degrees",
        "homing_acquire_radius",
        "spell_orbit_strength",
        "spell_orbit_radius",
        "spell_orbit_angular_speed"
    };

    private static readonly HashSet<string> ValidFreeEffectStats = new(ValidModifierStats, StringComparer.OrdinalIgnoreCase)
    {
    };

    static ContentValidator()
    {
        ValidFreeEffectStats.Remove("pierce");
    }

    public ValidationResult Validate(ContentCatalog catalog)
    {
        var result = ValidationResult.Valid();
        var tagCatalog = GameplayTagCatalogUtility.LoadDefaultCatalog();
        result.Merge(GameplayTagCatalogUtility.ValidateCatalog(tagCatalog));
        result.Merge(LocalizationValidator.Validate(catalog, includeRawStringScan: false));

        ValidateUniqueIds(result, "spell", catalog.Spells);
        ValidateUniqueIds(result, "item", catalog.Items);
        ValidateUniqueIds(result, "enemy", catalog.Enemies);
        ValidateUniqueIds(result, "wave", catalog.Waves);
        ValidateUniqueIds(result, "defense", catalog.Defenses);
        ValidateUniqueIds(result, "mage", catalog.Mages);
        ValidateUniqueIds(result, "achievement", catalog.Achievements);

        foreach (var spell in catalog.Spells)
        {
            ValidateCommonDefinition(result, "spell", spell);

            if (spell.CooldownSeconds < 0f)
            {
                result.AddError("spell.invalid_cooldown", $"Spell '{spell.Id}' has a negative cooldown.");
            }

            if (spell.Effects.Count == 0)
            {
                result.AddError("spell.missing_effects", $"Spell '{spell.Id}' has no effects.");
            }

            foreach (var effect in spell.Effects)
            {
                ValidateEffect(result, "spell", spell.Id, effect);
                ValidateDurationScaling(result, spell, effect);
            }

            foreach (var tag in spell.Tags)
            {
            if (!IsValidTag(tag))
            {
                result.AddError("spell.invalid_tag", $"Spell '{spell.Id}' has invalid tag '{tag}'. Use lowercase tags with optional dot-separated namespaces.");
            }
            }
        }

        foreach (var item in catalog.Items)
        {
            ValidateCommonDefinition(result, "item", item);

            if (!item.IsFlavorOnly && item.Kind == ItemKind.Passive && item.Effects.Count == 0)
            {
                result.AddWarning("item.no_behavior", $"Passive item '{item.Id}' has no designer effects.");
            }

            if (item.Kind == ItemKind.Activatable)
            {
                ValidateActivatableItem(result, item);
            }

            if (item.ItemNumber < 0)
            {
                result.AddError("item.invalid_number", $"Item '{item.Id}' has a negative item number.");
            }

            ValidateItemPoolWeights(result, item);

            if (string.IsNullOrWhiteSpace(item.HiddenDescription))
            {
                result.AddWarning("item.missing_hidden_description", $"Item '{item.Id}' has no hidden description.");
            }

            if (string.IsNullOrWhiteSpace(item.RevealedStatText)
                && string.IsNullOrWhiteSpace(item.RevealedBehaviorText)
                && string.IsNullOrWhiteSpace(item.RevealedEffectText))
            {
                result.AddWarning("item.missing_reveal_text", $"Item '{item.Id}' has no revealed stat or behavior text.");
            }

            ValidateRevealTextUsesPercentages(result, item);

            if (item.Icon != null && (item.Icon.GetWidth() != 256 || item.Icon.GetHeight() != 256))
            {
                result.AddError("item.invalid_icon_size", $"Item '{item.Id}' icon is {item.Icon.GetWidth()}x{item.Icon.GetHeight()}; expected 256x256.");
            }

            foreach (var effect in item.Effects)
            {
                ValidateItemEffect(result, item, effect);
            }

            foreach (var effect in item.ActiveEffects)
            {
                ValidateEffect(result, "item", item.Id, effect);
                if (!IsKnownActiveEffect(effect.EffectType))
                {
                    result.AddError("item.invalid_active_effect", $"Activatable item '{item.Id}' active effect '{effect.EffectType}' is not a known active effect executor.");
                }
            }
        }

        foreach (var enemy in catalog.Enemies)
        {
            ValidateCommonDefinition(result, "enemy", enemy);

            if (enemy.MaxHealth <= 0f || enemy.MoveSpeed < 0f)
            {
                result.AddError("enemy.invalid_stats", $"Enemy '{enemy.Id}' has invalid health or move speed.");
            }

            if (enemy.AttackDamage < 0f || enemy.AttackRange <= 0f)
            {
                result.AddError("enemy.invalid_attack", $"Enemy '{enemy.Id}' has invalid attack damage or range.");
            }

            if (enemy.PreferredRange < 0f
                || enemy.AbilityRange < 0f
                || enemy.AbilityCooldownSeconds < 0f
                || enemy.AbilityTelegraphSeconds < 0f
                || enemy.AbilityProjectileSpeed <= 0f
                || enemy.MaxActive < 0)
            {
                result.AddError("enemy.invalid_ability_tuning", $"Enemy '{enemy.Id}' has invalid ability tuning values.");
            }

            if (enemy.Role is EnemyRole.Ranged or EnemyRole.Specialist or EnemyRole.Boss && enemy.Abilities.Count == 0)
            {
                result.AddError("enemy.missing_role_ability", $"Enemy '{enemy.Id}' role '{enemy.Role}' needs at least one authored ability.");
            }

            if (enemy.Role is EnemyRole.Specialist or EnemyRole.Boss && enemy.MaxActive <= 0)
            {
                result.AddError("enemy.missing_active_limit", $"Enemy '{enemy.Id}' role '{enemy.Role}' needs MaxActive to limit specialist pressure.");
            }

            if (enemy.Abilities.Count > 0 && enemy.AbilityRange <= 0f)
            {
                result.AddError("enemy.missing_ability_range", $"Enemy '{enemy.Id}' has abilities but no ability range.");
            }

            foreach (var ability in enemy.Abilities)
            {
                ValidateEffect(result, "enemy", enemy.Id, ability);
            }
        }

        var enemyIds = catalog.Enemies.Select(enemy => ContentId.From(enemy.Id)).ToHashSet();
        foreach (var wave in catalog.Waves)
        {
            ValidateCommonDefinition(result, "wave", wave);

            if (wave.DurationSeconds <= 0f || wave.TargetActiveEnemies < 0 || wave.SpawnIntervalSeconds <= 0f)
            {
                result.AddError("wave.invalid_budget", $"Wave '{wave.Id}' has invalid duration, target count, or spawn interval.");
            }

            if (wave.Intensity <= 0f)
            {
                result.AddError("wave.invalid_intensity", $"Wave '{wave.Id}' has non-positive intensity.");
            }

            if (wave.EnemyIds.Count == 0)
            {
                result.AddError("wave.no_enemies", $"Wave '{wave.Id}' has no fallback enemy IDs.");
            }

            foreach (var enemyId in wave.EnemyIds.Select(ContentId.From))
            {
                if (!enemyIds.Contains(enemyId))
                {
                    result.AddError("wave.missing_enemy", $"Wave '{wave.Id}' references missing enemy '{enemyId}'.");
                }
            }

            if (wave.SpawnGroups.Count == 0)
            {
                result.AddWarning("wave.no_spawn_groups", $"Wave '{wave.Id}' has no authored spawn groups and will use fallback EnemyIds.");
            }

            var groupBudget = 0;
            foreach (var group in wave.SpawnGroups)
            {
                var groupEnemyId = ContentId.From(group.EnemyId);
                if (!enemyIds.Contains(groupEnemyId))
                {
                    result.AddError("wave.missing_enemy", $"Wave '{wave.Id}' spawn group references missing enemy '{groupEnemyId}'.");
                }

                if (group.Count <= 0 || group.StartTimeSeconds < 0f || group.SpawnIntervalSeconds <= 0f || group.IntensityMultiplier <= 0f)
                {
                    result.AddError("wave.invalid_spawn_group", $"Wave '{wave.Id}' has an invalid spawn group for '{group.EnemyId}'.");
                }

                groupBudget += Math.Max(0, group.Count);
            }

            if (wave.SpawnGroups.Count > 0 && groupBudget < wave.TargetActiveEnemies)
            {
                result.AddWarning("wave.group_budget_below_target", $"Wave '{wave.Id}' spawn groups budget {groupBudget} enemies for target {wave.TargetActiveEnemies}.");
            }

            if (wave.SpawnLanes.Count == 0)
            {
                result.AddError("wave.no_spawn_lanes", $"Wave '{wave.Id}' has no spawn lanes.");
            }

            var laneIds = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var lane in wave.SpawnLanes)
            {
                if (string.IsNullOrWhiteSpace(lane.Id) || !laneIds.Add(lane.Id))
                {
                    result.AddError("wave.invalid_lane_id", $"Wave '{wave.Id}' has a missing or duplicate spawn lane ID '{lane.Id}'.");
                }

                if (lane.Weight <= 0f || lane.SpawnPosition.DistanceSquaredTo(lane.ApproachPosition) < 9f)
                {
                    result.AddError("wave.invalid_lane", $"Wave '{wave.Id}' lane '{lane.Id}' must have positive weight and a clear spawn-to-approach vector.");
                }
            }
        }

        foreach (var defense in catalog.Defenses)
        {
            ValidateCommonDefinition(result, "defense", defense);

            if (defense.PlacementRule == DefensePlacementRule.None)
            {
                result.AddError("defense.no_placement_rule", $"Defense '{defense.Id}' has no placement rule.");
            }

            if (defense.MaxHealth <= 0f || defense.Cost < 0f || defense.RepairCost < 0f)
            {
                result.AddError("defense.invalid_cost_or_health", $"Defense '{defense.Id}' has invalid health or costs.");
            }

            if (defense.PlacementRadius <= 0f || defense.BlockRadius < 0f)
            {
                result.AddError("defense.invalid_placement_numbers", $"Defense '{defense.Id}' has invalid placement or block radius.");
            }

            if (defense.MaxCharges <= 0 || defense.RechargeSeconds < 0f)
            {
                result.AddError("defense.invalid_charges", $"Defense '{defense.Id}' has invalid charges or recharge.");
            }

            if (defense.Id.Contains("seal", StringComparison.OrdinalIgnoreCase)
                && (defense.TriggerRadius <= 0f
                    || defense.ExplosionRadius <= 0f
                    || defense.FuseSeconds < 0f
                    || defense.Damage <= 0f))
            {
                result.AddError("defense.invalid_trap", $"Trap defense '{defense.Id}' needs trigger radius, explosion radius, fuse, and damage.");
            }
        }

        var spellIds = catalog.Spells.Select(spell => ContentId.From(spell.Id)).ToHashSet();
        foreach (var mage in catalog.Mages)
        {
            ValidateCommonDefinition(result, "mage", mage);

            if (mage.MaxHealth <= 0f || mage.MoveSpeed <= 0f || mage.FireRate <= 0f)
            {
                result.AddError("mage.invalid_stats", $"Mage '{mage.Id}' has invalid health, move speed, or fire rate.");
            }

            if (mage.StartingSpellIds.Count == 0)
            {
                result.AddError("mage.missing_starting_spell", $"Mage '{mage.Id}' has no starting spell.");
            }

            foreach (var spellId in mage.StartingSpellIds.Select(ContentId.From))
            {
                if (!spellIds.Contains(spellId))
                {
                    result.AddError("mage.missing_spell", $"Mage '{mage.Id}' references missing starting spell '{spellId}'.");
                }
            }
        }

        var itemIds = catalog.Items.Select(item => ContentId.From(item.Id)).ToHashSet();
        var mageIds = catalog.Mages.Select(mage => ContentId.From(mage.Id)).ToHashSet();
        var achievementIds = catalog.Achievements.Select(achievement => ContentId.From(achievement.Id)).ToHashSet();
        foreach (var item in catalog.Items)
        {
            var unlockAchievementId = ContentId.From(item.UnlockAchievementId);
            if (unlockAchievementId.IsValid && !achievementIds.Contains(unlockAchievementId))
            {
                result.AddError("item.missing_unlock_achievement", $"Item '{item.Id}' references missing unlock achievement '{unlockAchievementId}'.");
            }
        }

        foreach (var achievement in catalog.Achievements)
        {
            ValidateCommonDefinition(result, "achievement", achievement);

            if (string.IsNullOrWhiteSpace(achievement.RequirementKey))
            {
                result.AddError("achievement.missing_requirement", $"Achievement '{achievement.Id}' has no requirement key.");
            }

            foreach (var itemId in achievement.UnlockItemIds.Select(ContentId.From))
            {
                if (!itemIds.Contains(itemId))
                {
                    result.AddError("achievement.missing_item", $"Achievement '{achievement.Id}' unlocks missing item '{itemId}'.");
                }
            }

            foreach (var mageId in achievement.UnlockMageIds.Select(ContentId.From))
            {
                if (!mageIds.Contains(mageId))
                {
                    result.AddError("achievement.missing_mage", $"Achievement '{achievement.Id}' unlocks missing mage '{mageId}'.");
                }
            }
        }

        foreach (var feedback in catalog.FeedbackProfiles)
        {
            ValidateCommonDefinition(result, "feedback", feedback);

            if (feedback.VfxScene == null && feedback.Audio == null)
            {
                result.AddWarning("feedback.empty", $"Feedback profile '{feedback.Id}' has no VFX or audio.");
            }
        }

        var feedbackIds = catalog.FeedbackProfiles.Select(feedback => ContentId.From(feedback.Id)).ToHashSet();
        foreach (var spell in catalog.Spells)
        {
            var feedbackId = ContentId.From(spell.FeedbackProfileId);
            if (!feedbackId.IsValid)
            {
                result.AddError("spell.missing_feedback", $"Spell '{spell.Id}' has no feedback profile ID.");
                continue;
            }

            if (!feedbackIds.Contains(feedbackId))
            {
                result.AddError("spell.missing_feedback", $"Spell '{spell.Id}' references missing feedback profile '{feedbackId}'.");
            }
        }

        ValidateGameplayTagUsage(result, catalog, tagCatalog);

        return result;
    }

    private static bool IsValidTag(string tag)
    {
        return GameplayTagPath.IsValid(tag);
    }

    private static void ValidateGameplayTagUsage(
        ValidationResult result,
        ContentCatalog catalog,
        GameplayTagCatalog tagCatalog)
    {
        foreach (var spell in catalog.Spells)
        {
            ValidateContentTags(result, tagCatalog, "spell", spell);
        }

        foreach (var item in catalog.Items)
        {
            ValidateContentTags(result, tagCatalog, "item", item);
            foreach (var effect in item.Effects)
            {
                ValidateTargetTag(result, tagCatalog, "item.effect", item.Id, effect.TargetTag);
                foreach (var compiled in ItemEffectCompiler.Compile(effect))
                {
                    ValidateTargetTag(result, tagCatalog, "item.compiled_effect", item.Id, compiled.TargetTag);
                }
            }
        }

        foreach (var enemy in catalog.Enemies)
        {
            ValidateContentTags(result, tagCatalog, "enemy", enemy);
        }

        foreach (var wave in catalog.Waves)
        {
            ValidateContentTags(result, tagCatalog, "wave", wave);
        }

        foreach (var defense in catalog.Defenses)
        {
            ValidateContentTags(result, tagCatalog, "defense", defense);
        }

        foreach (var mage in catalog.Mages)
        {
            ValidateContentTags(result, tagCatalog, "mage", mage);
        }

        foreach (var achievement in catalog.Achievements)
        {
            ValidateContentTags(result, tagCatalog, "achievement", achievement);
        }
    }

    private static void ValidateContentTags(
        ValidationResult result,
        GameplayTagCatalog tagCatalog,
        string kind,
        ContentDefinition definition)
    {
        foreach (var tag in definition.Tags)
        {
            ValidateCatalogTag(result, tagCatalog, $"{kind}.unknown_tag", $"{kind} '{definition.Id}'", tag);
        }
    }

    private static void ValidateTargetTag(
        ValidationResult result,
        GameplayTagCatalog tagCatalog,
        string kind,
        string ownerId,
        string tag)
    {
        if (string.IsNullOrWhiteSpace(tag))
        {
            return;
        }

        ValidateCatalogTag(result, tagCatalog, $"{kind}.unknown_target_tag", $"{kind} '{ownerId}' target", tag);
    }

    private static void ValidateCatalogTag(
        ValidationResult result,
        GameplayTagCatalog tagCatalog,
        string code,
        string owner,
        string tag)
    {
        var normalized = GameplayTagPath.Normalize(tag);
        if (!GameplayTagPath.IsValid(normalized))
        {
            return;
        }

        if (!tagCatalog.Contains(normalized))
        {
            result.AddError(code, $"{owner} uses gameplay tag '{normalized}', but it is not in {GameplayTagCatalogUtility.DefaultCatalogPath}.");
            return;
        }

        if (GameplayTagCatalogUtility.IsDeprecated(tagCatalog, normalized))
        {
            result.AddWarning("gameplay_tags.deprecated_usage", $"{owner} uses deprecated gameplay tag '{normalized}'.");
        }
    }

    private static void ValidateCommonDefinition(
        ValidationResult result,
        string kind,
        ContentDefinition definition)
    {
        if (string.IsNullOrWhiteSpace(definition.DisplayName))
        {
            result.AddWarning($"{kind}.missing_display_name", $"{kind} '{definition.Id}' has no display name.");
        }

        foreach (var tag in definition.Tags)
        {
            if (!IsValidTag(tag))
            {
                result.AddError($"{kind}.invalid_tag", $"{kind} '{definition.Id}' has invalid tag '{tag}'. Use lowercase tags with optional dot-separated namespaces.");
            }
        }
    }

    private static void ValidateModifier(ValidationResult result, ItemDefinition item, ModifierSpec modifier)
    {
        if (string.IsNullOrWhiteSpace(modifier.StatId) || !ValidModifierStats.Contains(modifier.StatId))
        {
            result.AddError("item.invalid_modifier_stat", $"Item '{item.Id}' modifier targets unknown stat '{modifier.StatId}'.");
        }

        if (!string.IsNullOrWhiteSpace(modifier.TargetTag) && !IsValidTag(modifier.TargetTag))
        {
            result.AddError("item.invalid_modifier_target", $"Item '{item.Id}' modifier has invalid target tag '{modifier.TargetTag}'.");
        }

        if (modifier.Operation is DataModifierOp.Multiplicative or DataModifierOp.AdditivePercent && modifier.Value < 0f)
        {
            result.AddError("item.invalid_modifier_value", $"Item '{item.Id}' modifier '{modifier.StatId}' has a negative percent/multiplier value.");
        }
    }

    private static void ValidateItemEffect(ValidationResult result, ItemDefinition item, ItemEffectSpec effect)
    {
        if (!Enum.IsDefined(effect.Kind))
        {
            result.AddError("item.invalid_effect_kind", $"Item '{item.Id}' effect has obsolete or unknown kind value '{(int)effect.Kind}'. Recreate it as AddMultiplyStat, AddFlatStat, RemoveMultiplyStat, or RemoveFlatStat.");
            return;
        }

        if (effect.ChancePercent < 0f || effect.ChancePercent > 100f)
        {
            result.AddError("item.invalid_effect_chance", $"Item '{item.Id}' effect '{effect.Kind}' has chance {effect.ChancePercent:0.###}; expected 0..100%.");
        }

        if (!string.IsNullOrWhiteSpace(effect.TargetTag) && !IsValidTag(effect.TargetTag))
        {
            result.AddError("item.invalid_effect_target", $"Item '{item.Id}' effect '{effect.Kind}' has invalid target tag '{effect.TargetTag}'.");
        }

        if (ItemEffectCompiler.RequiresFreeStat(effect.Kind)
            && (string.IsNullOrWhiteSpace(effect.StatId) || !ValidFreeEffectStats.Contains(effect.StatId)))
        {
            result.AddError("item.invalid_effect_stat", $"Item '{item.Id}' effect '{effect.Kind}' targets unknown or reserved stat '{effect.StatId}'. Use the dedicated projectile pierce effect for pierce.");
        }

        if (effect.Kind == ItemEffectKind.KeepItIn)
        {
            if (effect.Value < 1f)
            {
                result.AddError("item.invalid_keep_it_in_count", $"Item '{item.Id}' KeepItIn effect must launch at least one extra spell instance.");
            }

            if (effect.SecondaryValue <= 0f)
            {
                result.AddError("item.invalid_keep_it_in_charge", $"Item '{item.Id}' KeepItIn effect must have a positive base charge time.");
            }
        }
        else if (effect.Kind == ItemEffectKind.AbyssalRing)
        {
            if (effect.Value <= 0f)
            {
                result.AddError("item.invalid_abyssal_ring_damage", $"Item '{item.Id}' AbyssalRing effect must have a positive player damage multiplier.");
            }

            if (effect.SecondaryValue <= 0f)
            {
                result.AddError("item.invalid_abyssal_ring_charge", $"Item '{item.Id}' AbyssalRing effect must have a positive base charge time.");
            }
        }
        else if (effect.Kind == ItemEffectKind.FaultyFocus)
        {
            if (effect.Value <= 0f || effect.Value > 1f)
            {
                result.AddError("item.invalid_faulty_focus_damage", $"Item '{item.Id}' FaultyFocus effect must use a damage multiplier in the 0..1 range.");
            }

            if (effect.SecondaryValue < 1f || effect.SecondaryValue > 8f)
            {
                result.AddError("item.invalid_faulty_focus_generations", $"Item '{item.Id}' FaultyFocus effect must use 1..8 max generations.");
            }
        }
        else if (effect.Kind == ItemEffectKind.PreventNextPlayerHealthLoss)
        {
            if (effect.Value < 1f || effect.Value > 99f)
            {
                result.AddError("item.invalid_damage_prevention_hits", $"Item '{item.Id}' PreventNextPlayerHealthLoss effect must protect 1..99 hits.");
            }
        }
        else if (effect.Kind == ItemEffectKind.HomingSpells)
        {
            if (effect.Value <= 0f)
            {
                result.AddError("item.invalid_homing_strength", $"Item '{item.Id}' HomingSpells effect must have a positive homing strength.");
            }

            if (effect.SecondaryValue < 0f || effect.SecondaryValue > 180f)
            {
                result.AddError("item.invalid_homing_angle", $"Item '{item.Id}' HomingSpells detection cone angle must be 0..180 degrees. Use 0 for the delivery default.");
            }

            if (effect.TertiaryValue < 0f)
            {
                result.AddError("item.invalid_homing_width", $"Item '{item.Id}' HomingSpells detection width must be zero or positive. Use 0 for the delivery default.");
            }
        }
        else if (effect.Kind == ItemEffectKind.OrbitingSpells)
        {
            if (effect.Value <= 0f)
            {
                result.AddError("item.invalid_spell_orbit_strength", $"Item '{item.Id}' OrbitingSpells effect must have a positive orbit strength.");
            }

            if (effect.SecondaryValue < 0f)
            {
                result.AddError("item.invalid_spell_orbit_radius", $"Item '{item.Id}' OrbitingSpells radius must be zero or positive. Use 0 for the runtime default.");
            }

            if (effect.TertiaryValue < 0f)
            {
                result.AddError("item.invalid_spell_orbit_speed", $"Item '{item.Id}' OrbitingSpells angular speed must be zero or positive. Use 0 for the runtime default.");
            }
        }

        foreach (var compiled in ItemEffectCompiler.Compile(effect))
        {
            if (compiled.IsModifier)
            {
                ValidateModifier(result, item, new ModifierSpec
                {
                    StatId = compiled.StatId,
                    Operation = compiled.Operation,
                    Value = compiled.Value,
                    Priority = compiled.Priority,
                    TargetTag = compiled.TargetTag
                });
            }
            else
            {
                ValidateProc(result, item, new ProcSpec
                {
                    Trigger = compiled.Trigger,
                    EffectType = compiled.EffectType,
                    Chance = compiled.Chance,
                    Value = compiled.Value,
                    TargetTag = compiled.TargetTag
                });
            }
        }
    }

    private static void ValidateItemPoolWeights(ValidationResult result, ItemDefinition item)
    {
        if (item.PoolWeights.Count == 0)
        {
            result.AddError("item.missing_pool", $"Item '{item.Id}' is not assigned to any item pool.");
            return;
        }

        var poolIds = new HashSet<ContentId>();
        foreach (var poolWeight in item.PoolWeights)
        {
            if (poolWeight == null)
            {
                result.AddError("item.invalid_pool", $"Item '{item.Id}' has an empty pool entry.");
                continue;
            }

            var poolId = ContentId.From(poolWeight.PoolId);
            if (!poolId.IsValid)
            {
                result.AddError("item.invalid_pool", $"Item '{item.Id}' has an empty item pool ID.");
                continue;
            }

            if (!IsValidTag(poolWeight.PoolId))
            {
                result.AddError("item.invalid_pool", $"Item '{item.Id}' has invalid item pool ID '{poolWeight.PoolId}'. Use lowercase IDs with optional dot-separated namespaces.");
            }

            if (!poolIds.Add(poolId))
            {
                result.AddError("item.duplicate_pool", $"Item '{item.Id}' has duplicate item pool '{poolId}'.");
            }

            if (poolWeight.Weight <= 0f)
            {
                result.AddError("item.invalid_pool_weight", $"Item '{item.Id}' has non-positive weight {poolWeight.Weight:0.###} in pool '{poolId}'.");
            }
        }
    }

    private static void ValidateActivatableItem(ValidationResult result, ItemDefinition item)
    {
        if (item.MaxActivations < 0)
        {
            result.AddError("item.invalid_activation_count", $"Activatable item '{item.Id}' has negative max activations.");
        }

        if (!item.HasUnlimitedActivations && item.MaxActivations <= 0)
        {
            result.AddError("item.invalid_activation_count", $"Limited activatable item '{item.Id}' needs a positive max activation count.");
        }

        if (item.HasUnlimitedActivations && item.MaxActivations > 0)
        {
            result.AddError("item.contradictory_activation_settings", $"Activatable item '{item.Id}' is unlimited but also has max activations {item.MaxActivations}.");
        }
    }

    private static void ValidateRevealTextUsesPercentages(ValidationResult result, ItemDefinition item)
    {
        ValidateRevealTextField(result, item, "revealed stat text", item.RevealedStatText);
        ValidateRevealTextField(result, item, "revealed effect text", item.RevealedEffectText);
    }

    private static void ValidateRevealTextField(ValidationResult result, ItemDefinition item, string fieldName, string value)
    {
        if (string.IsNullOrWhiteSpace(value) || !RevealedMultiplierNotationPattern.IsMatch(value))
        {
            return;
        }

        result.AddWarning(
            "item.reveal_uses_multiplier_notation",
            $"Item '{item.Id}' {fieldName} uses 'x' multiplier notation; use percent text such as +50% or -25%.");
    }

    private static void ValidateProc(ValidationResult result, ItemDefinition item, ProcSpec proc)
    {
        if (proc.Trigger == ProcTrigger.None)
        {
            result.AddError("item.invalid_proc_trigger", $"Item '{item.Id}' has a proc without a trigger.");
        }

        if (string.IsNullOrWhiteSpace(proc.EffectType))
        {
            result.AddError("item.invalid_proc_effect", $"Item '{item.Id}' has a proc without an effect type.");
        }

        if (proc.Chance < 0f || proc.Chance > 1f)
        {
            result.AddError("item.invalid_proc_chance", $"Item '{item.Id}' proc '{proc.EffectType}' has chance {proc.Chance:0.###}; expected 0..1.");
        }

        if (!string.IsNullOrWhiteSpace(proc.TargetTag) && !IsValidTag(proc.TargetTag))
        {
            result.AddError("item.invalid_proc_target", $"Item '{item.Id}' proc '{proc.EffectType}' has invalid target tag '{proc.TargetTag}'.");
        }

        if (!IsKnownItemProcEffect(proc.EffectType))
        {
            result.AddError("item.invalid_proc_effect", $"Item '{item.Id}' proc '{proc.EffectType}' is not a generated item proc effect.");
        }

        if (proc.EffectType.Contains("proc", StringComparison.OrdinalIgnoreCase)
            || proc.EffectType.Contains("trigger", StringComparison.OrdinalIgnoreCase))
        {
            result.AddError("item.recursive_proc_risk", $"Item '{item.Id}' proc '{proc.EffectType}' may recursively trigger other procs.");
        }

        if ((proc.EffectType.Contains("damage", StringComparison.OrdinalIgnoreCase)
                || proc.EffectType.Contains("explosion", StringComparison.OrdinalIgnoreCase)
                || proc.EffectType.Contains("ground", StringComparison.OrdinalIgnoreCase)
                || proc.EffectType.Contains("cooldown", StringComparison.OrdinalIgnoreCase))
            && proc.Value <= 0f)
        {
            result.AddError("item.invalid_proc_value", $"Item '{item.Id}' proc '{proc.EffectType}' needs a positive value.");
        }
    }

    private static bool IsKnownItemProcEffect(string effectType)
    {
        return effectType is "apply_burn"
            or "apply_immolate"
            or "apply_slow"
            or "apply_freeze"
            or "bonus_damage"
            or "shatter_damage"
            or "chain_damage"
            or "kill_explosion"
            or "burning_ground"
            or "explosive_seal"
            or "cooldown_tick";
    }

    private static bool IsKnownActiveEffect(string effectType)
    {
        return ActiveEffectIds.IsKnown(effectType);
    }

    private static void ValidateEffect(ValidationResult result, string ownerKind, string ownerId, EffectSpec effect)
    {
        if (string.IsNullOrWhiteSpace(effect.EffectType))
        {
            result.AddError($"{ownerKind}.invalid_effect", $"{ownerKind} '{ownerId}' has an effect without an effect type.");
        }

        if (effect.Radius < 0f || effect.DurationSeconds < 0f)
        {
            result.AddError($"{ownerKind}.invalid_effect_numbers", $"{ownerKind} '{ownerId}' effect '{effect.EffectType}' has negative radius or duration.");
        }
    }

    private static void ValidateDurationScaling(ValidationResult result, SpellDefinition spell, EffectSpec effect)
    {
        if (IsProjectileOrBeam(effect.EffectType) && effect.DurationScaling != DurationScalingMode.None)
        {
            result.AddError(
                "spell.invalid_duration_scaling",
                $"Spell '{spell.Id}' effect '{effect.EffectType}' uses projectile/beam travel data and must set DurationScaling=None.");
            return;
        }

        if (IsTimedEffect(effect.EffectType) && effect.DurationSeconds > 0f && effect.DurationScaling == DurationScalingMode.None)
        {
            result.AddWarning(
                "spell.duration_not_scaled",
                $"Spell '{spell.Id}' effect '{effect.EffectType}' has a timed lifetime but does not scale with player duration.");
        }
    }

    private static bool IsProjectileOrBeam(string effectType)
    {
        return effectType.Contains("projectile", StringComparison.OrdinalIgnoreCase)
            || effectType.Contains("beam", StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsTimedEffect(string effectType)
    {
        return effectType.Contains("aoe", StringComparison.OrdinalIgnoreCase)
            || effectType.Contains("hazard", StringComparison.OrdinalIgnoreCase)
            || effectType.Contains("vortex", StringComparison.OrdinalIgnoreCase)
            || effectType.Contains("summon", StringComparison.OrdinalIgnoreCase)
            || effectType.Contains("status", StringComparison.OrdinalIgnoreCase)
            || effectType.Contains("totem", StringComparison.OrdinalIgnoreCase);
    }

    private static void ValidateUniqueIds<TDefinition>(
        ValidationResult result,
        string kind,
        IEnumerable<TDefinition> definitions)
        where TDefinition : ContentDefinition
    {
        var ids = new HashSet<ContentId>();
        foreach (var definition in definitions)
        {
            var id = ContentId.From(definition.Id);
            if (!id.IsValid)
            {
                result.AddError("content.empty_id", $"{kind} has an empty ID.");
                continue;
            }

            if (!ids.Add(id))
            {
                result.AddError("content.duplicate_id", $"Duplicate {kind} ID '{id}'.");
            }
        }
    }
}
