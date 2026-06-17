using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Data.Validation;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Validation;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;
using TheLastMage.Save;

namespace TheLastMage.Debugging.Tools;

public static class ToolingGateReport
{
    public const int DefaultSimulationRolls = 10_000;

    public static ValidationResult ValidateKnownGoodCatalog(ContentCatalog catalog)
    {
        return new ContentValidator().Validate(catalog);
    }

    public static ValidationResult ValidateBrokenDebugCatalog()
    {
        return new ContentValidator().Validate(CreateBrokenDebugCatalog());
    }

    public static ContentCatalog CreateBrokenDebugCatalog()
    {
        var duplicateItemA = new ItemDefinition
        {
            Id = "broken_item",
            DisplayName = "Broken Item",
            RevealedEffectText = string.Empty
        };
        duplicateItemA.Effects.Add(new ItemEffectSpec
        {
            Kind = ItemEffectKind.AddMultiplyStat,
            StatId = "Not A Stat",
            Value = 1f,
            TargetTag = "Bad Tag"
        });
        duplicateItemA.Effects.Add(new ItemEffectSpec
        {
            Kind = ItemEffectKind.FreezeWithFrostDamage,
            ChancePercent = 150f,
            Value = 1f
        });

        var duplicateItemB = new ItemDefinition
        {
            Id = "broken_item",
            DisplayName = "Duplicate Broken Item",
            RevealedEffectText = "Duplicate ID sample."
        };

        var spell = new SpellDefinition
        {
            Id = "broken_spell",
            DisplayName = "Broken Spell",
            CooldownSeconds = -1f,
            FeedbackProfileId = "missing_feedback"
        };
        spell.Effects.Add(new EffectSpec
        {
            EffectType = "projectile_damage",
            Radius = -1f,
            DurationSeconds = 3f,
            DurationScaling = DurationScalingMode.PlayerDuration
        });

        var enemy = new EnemyArchetypeDefinition
        {
            Id = "broken_enemy",
            DisplayName = "Broken Enemy",
            MaxHealth = 0f,
            MoveSpeed = -1f,
            AttackRange = 0f
        };

        var wave = new WaveDefinition
        {
            Id = "broken_wave",
            DisplayName = "Broken Wave",
            DurationSeconds = 0f,
            TargetActiveEnemies = -1,
            SpawnIntervalSeconds = 0f
        };
        wave.EnemyIds.Add("missing_enemy");

        var defense = new DefenseDefinition
        {
            Id = "broken_defense",
            DisplayName = "Broken Defense",
            PlacementRule = DefensePlacementRule.None,
            MaxHealth = 0f,
            Cost = -1f
        };

        var achievement = new AchievementDefinition
        {
            Id = "broken_achievement",
            DisplayName = "Broken Achievement",
            RequirementKey = string.Empty
        };
        achievement.UnlockItemIds.Add("missing_item");

        var feedback = new FeedbackProfile
        {
            Id = "broken_feedback",
            DisplayName = "Broken Feedback"
        };

        var catalog = new ContentCatalog();
        catalog.Items.Add(duplicateItemA);
        catalog.Items.Add(duplicateItemB);
        catalog.Spells.Add(spell);
        catalog.Enemies.Add(enemy);
        catalog.Waves.Add(wave);
        catalog.Defenses.Add(defense);
        catalog.Achievements.Add(achievement);
        catalog.FeedbackProfiles.Add(feedback);
        return catalog;
    }

    public static string BuildValidationText(ValidationResult result)
    {
        if (result.Issues.Count == 0)
        {
            return "Validation: no issues.";
        }

        return string.Join('\n', result.Issues.Select(issue => $"{issue.Severity}: {issue.Code} - {issue.Message}"));
    }

    public static string BuildItemInspection(ItemRuntimeDefinition item)
    {
        var lines = new List<string>
        {
            $"[b]{EscapeBbcode(item.DisplayName)}[/b] ({EscapeBbcode(item.Id.ToString())})",
            $"[b]Number:[/b] {item.ItemNumber}    [b]Kind:[/b] {EscapeBbcode(item.Kind)}    [b]Pools:[/b] {EscapeBbcode(FormatPools(item.PoolWeights))}",
            $"[b]Tags:[/b] {EscapeBbcode(FormatTags(item.Tags))}",
            $"[b]Hidden:[/b] {EscapeBbcode(string.IsNullOrWhiteSpace(item.HiddenDescription) ? "MISSING" : item.HiddenDescription)}",
            $"[b]Reveal Stats:[/b] {EscapeBbcode(string.IsNullOrWhiteSpace(item.RevealedStatText) ? "-" : item.RevealedStatText)}",
            $"[b]Reveal Behavior:[/b] {EscapeBbcode(string.IsNullOrWhiteSpace(item.RevealedBehaviorText) ? "-" : item.RevealedBehaviorText)}",
            $"[b]Unlock Achievement:[/b] {EscapeBbcode(item.UnlockAchievementId.IsValid ? item.UnlockAchievementId.ToString() : "-")}",
            $"[b]Telemetry:[/b] picked={item.PlayersPicked} win_rate={item.WinRatePercent:0.##}% (read-only external data)",
            $"[b]Activations:[/b] {(item.HasUnlimitedActivations ? "unlimited" : item.MaxActivations.ToString())}",
            string.Empty,
            $"[b]Designer Effects:[/b] {item.Effects.Count}"
        };

        foreach (var effect in item.Effects)
        {
            lines.Add($"  [b]{EscapeBbcode(effect.Kind)}[/b] value={effect.Value:0.###} chance={effect.Chance:0.###} stat={EscapeBbcode(effect.StatId.ToString())} target={EscapeBbcode(FormatTag(effect.TargetTag))}");
        }

        if (item.Effects.Count == 0)
        {
            lines.Add("  none");
        }

        lines.Add(string.Empty);
        lines.AddRange(new[]
        {
            $"[b]Modifiers:[/b] {item.Modifiers.Count}"
        });

        foreach (var modifier in item.Modifiers)
        {
            lines.Add($"  [b]{EscapeBbcode(modifier.StatId.ToString())}[/b] {modifier.Operation} {modifier.Value:0.###} target={EscapeBbcode(FormatTag(modifier.TargetTag))}");
        }

        if (item.Modifiers.Count == 0)
        {
            lines.Add("  none");
        }

        lines.Add(string.Empty);
        lines.Add($"[b]Procs:[/b] {item.Procs.Count}");

        foreach (var proc in item.Procs)
        {
            lines.Add($"  [b]{proc.Trigger}[/b] {EscapeBbcode(proc.EffectType)} chance={proc.Chance:0.###} value={proc.Value:0.###} target={EscapeBbcode(FormatTag(proc.TargetTag))}");
        }

        if (item.Procs.Count == 0)
        {
            lines.Add("  none");
        }

        lines.Add(string.Empty);
        lines.Add($"[b]Active Effects:[/b] {item.ActiveEffects.Count}");
        foreach (var effect in item.ActiveEffects)
        {
            lines.Add($"  [b]{EscapeBbcode(effect.EffectType)}[/b] value={effect.Value:0.###} radius={effect.Radius:0.###} duration={effect.DurationSeconds:0.###}");
        }

        if (item.ActiveEffects.Count == 0)
        {
            lines.Add("  none");
        }

        return string.Join('\n', lines);
    }

    public static string BuildEnemyInspection(EnemyRuntimeDefinition enemy, ValidationResult validation)
    {
        var lines = new List<string>
        {
            $"[b]{EscapeBbcode(enemy.DisplayName)}[/b] ({EscapeBbcode(enemy.Id.ToString())})",
            $"[b]Family:[/b] {EscapeBbcode(enemy.Family)}    [b]Role:[/b] {EscapeBbcode(enemy.Role)}",
            $"[b]Tags:[/b] {EscapeBbcode(FormatTags(enemy.Tags))}",
            $"[b]Stats:[/b] health={enemy.MaxHealth:0.##} move={enemy.MoveSpeed:0.##} damage={enemy.AttackDamage:0.##} attack_range={enemy.AttackRange:0.##}",
            $"[b]Ability Tuning:[/b] preferred={enemy.PreferredRange:0.##} range={enemy.AbilityRange:0.##} cooldown={enemy.AbilityCooldownSeconds:0.##} telegraph={enemy.AbilityTelegraphSeconds:0.##} projectile_speed={enemy.AbilityProjectileSpeed:0.##} max_active={(enemy.MaxActive <= 0 ? "unlimited" : enemy.MaxActive.ToString())}",
            string.Empty,
            $"[b]Abilities:[/b] {enemy.Abilities.Count}"
        };

        foreach (var ability in enemy.Abilities)
        {
            lines.Add($"  [b]{EscapeBbcode(ability.EffectType)}[/b] value={ability.Value:0.###} radius={ability.Radius:0.###} duration={ability.DurationSeconds:0.###}");
        }

        if (enemy.Abilities.Count == 0)
        {
            lines.Add("  none");
        }

        lines.Add(string.Empty);
        lines.Add("[b]Validation Issues:[/b]");
        var issueCount = 0;
        foreach (var issue in validation.Issues.Where(issue => issue.Message.Contains($"'{enemy.Id}'", StringComparison.OrdinalIgnoreCase)))
        {
            lines.Add($"  {issue.Severity}: {EscapeBbcode(issue.Code)} - {EscapeBbcode(issue.Message)}");
            issueCount++;
        }

        if (issueCount == 0)
        {
            lines.Add("  none");
        }

        return string.Join('\n', lines);
    }

    public static ValidationResult ValidateCatalogResource()
    {
        var catalog = ResourceLoader.Load<ContentCatalog>("res://data/catalogs/content_catalog.tres");
        return catalog == null ? ValidationResult.Valid() : ValidateKnownGoodCatalog(catalog);
    }

    public static string BuildStatInspection(RunContext context)
    {
        var tags = Array.Empty<TagId>();
        var lines = new List<string>
        {
            "[b]Canonical Player Attributes[/b]",
            FormatStatLine("health", PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.Health, context.State.Player.BaseMaxHealth, tags, false)),
            FormatStatLine("move_speed", PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.MoveSpeed, context.State.Player.BaseMoveSpeed, tags, false)),
            FormatStatLine("damage", PlayerAttributeResolver.ResolveDamage(context, 10f, tags, false), "from base 10"),
            FormatStatLine("fire_rate", PlayerAttributeResolver.ResolveFireRate(context, tags, false), "x"),
            FormatStatLine("range", PlayerAttributeResolver.ResolveRange(context, 10f, tags, false), "from base 10"),
            FormatStatLine("luck", PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.Luck, context.State.Player.BaseLuck, tags, false)),
            string.Empty,
            "[b]Broader Modifier Stats[/b]",
            FormatStatLine("duration", PlayerAttributeResolver.ResolveDuration(context, 10f, tags, false), "from base 10"),
            FormatStatLine("status_power", PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.StatusPower, 1f, tags, false)),
            FormatStatLine("summon_damage", PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.SummonDamage, 10f, new[] { TagId.From("combat.source.summon") }, false), "from base 10"),
            FormatStatLine("defense_damage", PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.DefenseDamage, 10f, new[] { TagId.From("combat.source.defense") }, false), "from base 10"),
            FormatStatLine("radius", PlayerAttributeResolver.ResolveAreaRadius(context, 1f, tags, false)),
            FormatStatLine("projectile_speed", PlayerAttributeResolver.ResolveProjectileSpeed(context, 1f, tags, false)),
            FormatFlagLine("pierce", PlayerAttributeResolver.ResolveProjectilePierce(context, new[] { TagId.From("spell.delivery.projectile") }, false)),
            FormatStatLine("spell_count", PlayerAttributeResolver.ResolveAdditionalSpellCount(context, tags, false)),
            FormatStatLine("projectile_split", PlayerAttributeResolver.ResolveProjectileSplitDepth(context, new[] { TagId.From("spell.delivery.projectile") }, false)),
            string.Empty,
            "[b]Recent Stat Sources[/b]"
        };

        foreach (var breakdown in context.State.RecentStatBreakdowns.TakeLast(8))
        {
            var sources = breakdown.AppliedModifiers.Count == 0
                ? "base"
                : string.Join(", ", breakdown.AppliedModifiers.Select(modifier => modifier.SourceId.ToString()));
            lines.Add($"[b]{EscapeBbcode(breakdown.StatId.ToString())}:[/b] {breakdown.BaseValue:0.###} -> {breakdown.FinalValue:0.###}  [i]{EscapeBbcode(sources)}[/i]");
        }

        return string.Join('\n', lines);
    }

    public static string BuildRangeInspection(RunContext context)
    {
        var lines = new List<string>
        {
            "[b]Spells[/b]"
        };
        foreach (var spell in context.Content.Spells.Values.OrderBy(spell => spell.Id.ToString()))
        {
            lines.Add(string.Empty);
            lines.Add($"[b]{EscapeBbcode(spell.DisplayName)}[/b] ({EscapeBbcode(spell.Id.ToString())})");
            foreach (var effect in spell.Effects)
            {
                AppendEffectRangeLine(lines, context, spell, effect);
            }
        }

        lines.Add(string.Empty);
        lines.Add("[b]Defenses[/b]");
        foreach (var defense in context.Content.Defenses.Values.OrderBy(defense => defense.Id.ToString()))
        {
            lines.Add($"[b]{EscapeBbcode(defense.Id.ToString())}:[/b] trigger={defense.TriggerRadius:0.##}  explosion={defense.ExplosionRadius:0.##}  placement={defense.PlacementRule}");
        }

        lines.Add(string.Empty);
        lines.Add("[b]Spawn Volumes[/b]");
        lines.Add("[b]combat sandbox:[/b] radial spawn around player");
        lines.Add("[b]wave spawner:[/b] radial siege approach");
        return string.Join('\n', lines);
    }

    public static string BuildLootSimulationText(ContentRegistry content, ProfileSaveDto profile, int rolls, int seed)
    {
        var summary = new LootSimulator().Run(content, profile, rolls, seed);
        var lines = new List<string>
        {
            $"[b]Rolls:[/b] {summary.Rolls}    [b]Seed:[/b] {summary.Seed}",
            $"[b]Offers generated:[/b] {summary.OffersGenerated}",
            $"[b]Empty rolls:[/b] {summary.EmptyRolls}    [b]Locked excluded:[/b] {summary.LockedItemsExcluded}",
            $"[b]Discovered offers:[/b] {summary.DiscoveredOffers}    [b]Undiscovered offers:[/b] {summary.UndiscoveredOffers}",
            string.Empty,
            "[b]Top Offers[/b]"
        };

        var fixtureOfferCount = summary.OfferCounts
            .Where(pair => IsRegressionFixtureItem(pair.Key))
            .Sum(pair => pair.Value);
        var topProductionOffers = summary.OfferCounts
            .Where(pair => !IsRegressionFixtureItem(pair.Key))
            .OrderByDescending(pair => pair.Value)
            .ThenBy(pair => pair.Key.ToString())
            .Take(12)
            .ToArray();

        foreach (var pair in topProductionOffers)
        {
            var percentage = summary.OffersGenerated == 0 ? 0f : pair.Value * 100f / summary.OffersGenerated;
            lines.Add($"[b]{EscapeBbcode(pair.Key.ToString())}:[/b] {pair.Value} ({percentage:0.##}%)");
        }

        if (topProductionOffers.Length == 0)
        {
            lines.Add("none");
        }

        if (fixtureOfferCount > 0)
        {
            var percentage = summary.OffersGenerated == 0 ? 0f : fixtureOfferCount * 100f / summary.OffersGenerated;
            lines.Add($"[i]Regression fixture offers:[/i] {fixtureOfferCount} ({percentage:0.##}%)");
        }

        lines.Add(string.Empty);
        lines.Add(BuildItemBatchReport(content, ValidateCatalogResource()));
        return string.Join('\n', lines);
    }

    public static string BuildItemBatchReport(ContentRegistry content, ValidationResult validation)
    {
        var productionItems = content.Items.Values
            .Where(item => !IsRegressionFixtureItem(item.Id))
            .ToArray();
        var fixtureCount = content.Items.Count - productionItems.Length;
        var numericItems = 0;
        var behaviorItems = 0;
        var activatableItems = 0;
        var activeEffectCount = 0;
        var designerEffectItems = 0;
        var nightMarketItems = 0;
        var procCount = 0;
        var targetedRules = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        var branchTargets = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        foreach (var item in productionItems)
        {
            if (string.Equals(item.Kind, nameof(ItemKind.Activatable), StringComparison.OrdinalIgnoreCase))
            {
                activatableItems++;
                activeEffectCount += item.ActiveEffects.Count;
            }

            if (item.Effects.Count > 0)
            {
                designerEffectItems++;
            }

            if (item.PoolWeights.Any(poolWeight => poolWeight.PoolId.Equals(ContentId.From(ItemPoolIds.NightMarket)) && poolWeight.Weight > 0f))
            {
                nightMarketItems++;
            }

            if (item.Modifiers.Count > 0)
            {
                numericItems++;
            }

            if (item.Procs.Count > 0
                || item.Effects.Any(effect => effect.Kind == nameof(ItemEffectKind.KeepItIn)
                    || effect.Kind == nameof(ItemEffectKind.AbyssalRing)
                    || effect.Kind == nameof(ItemEffectKind.FaultyFocus)
                    || effect.Kind == nameof(ItemEffectKind.PreventNextPlayerHealthLoss)
                    || effect.Kind == nameof(ItemEffectKind.HomingSpells)
                    || effect.Kind == nameof(ItemEffectKind.OrbitingSpells)))
            {
                behaviorItems++;
                procCount += item.Procs.Count;
            }

            foreach (var modifier in item.Modifiers)
            {
                var key = modifier.TargetTag.IsValid ? modifier.TargetTag.ToString() : "all";
                targetedRules[key] = targetedRules.GetValueOrDefault(key) + 1;
                AddBranchTarget(branchTargets, key);
            }

            foreach (var proc in item.Procs)
            {
                var key = proc.TargetTag.IsValid ? proc.TargetTag.ToString() : "all";
                targetedRules[key] = targetedRules.GetValueOrDefault(key) + 1;
                AddBranchTarget(branchTargets, key);
            }
        }

        var itemIssues = validation.Issues
            .Where(issue => issue.Code.StartsWith("item.", StringComparison.OrdinalIgnoreCase))
            .ToArray();

        var lines = new List<string>
        {
            "[b]Sprint 11 Item Batch[/b]",
            $"[b]Production items:[/b] {productionItems.Length}    [b]Night Market:[/b] {nightMarketItems}    [b]Activatable:[/b] {activatableItems}    [b]Active effects:[/b] {activeEffectCount}    [b]Regression fixtures:[/b] {fixtureCount}    [b]Designer effects:[/b] {designerEffectItems}    [b]Numeric:[/b] {numericItems}    [b]Behavior/proc:[/b] {behaviorItems}    [b]Procs:[/b] {procCount}",
            $"[b]Proc safety:[/b] per-frame budget and recursion guard active in ProcSystem; blocked procs are logged to debug metrics.",
            $"[b]Item validation issues:[/b] {itemIssues.Length}",
            "[b]Synergy targets:[/b] " + EscapeBbcode(FormatTargetedRules(targetedRules)),
            "[b]Synergy branches:[/b] " + EscapeBbcode(FormatTargetedRules(branchTargets))
        };

        foreach (var issue in itemIssues.Take(8))
        {
            lines.Add($"  {issue.Severity}: {EscapeBbcode(issue.Code)} - {EscapeBbcode(issue.Message)}");
        }

        return string.Join('\n', lines);
    }

    private static bool IsRegressionFixtureItem(ContentId itemId)
    {
        return itemId.ToString().StartsWith("regression_item_", StringComparison.Ordinal);
    }

    private static string FormatTargetedRules(Dictionary<string, int> targetedRules)
    {
        return targetedRules.Count == 0
            ? "-"
            : string.Join(", ", targetedRules.OrderBy(pair => pair.Key).Select(pair => $"{pair.Key}:{pair.Value}"));
    }

    private static void AddBranchTarget(Dictionary<string, int> branchTargets, string tag)
    {
        if (string.IsNullOrWhiteSpace(tag) || string.Equals(tag, "all", StringComparison.OrdinalIgnoreCase))
        {
            branchTargets["all"] = branchTargets.GetValueOrDefault("all") + 1;
            return;
        }

        var branch = GameplayTagPath.GetParent(tag);
        if (string.IsNullOrWhiteSpace(branch))
        {
            branch = tag;
        }

        branchTargets[branch] = branchTargets.GetValueOrDefault(branch) + 1;
    }

    private static string FormatPools(IReadOnlyList<ItemPoolWeightRuntimeSpec> poolWeights)
    {
        return poolWeights.Count == 0
            ? "-"
            : string.Join(", ", poolWeights.Select(poolWeight => $"{poolWeight.PoolId}:{poolWeight.Weight:0.##}"));
    }

    private static void AppendEffectRangeLine(
        List<string> lines,
        RunContext context,
        SpellRuntimeDefinition spell,
        EffectRuntimeSpec effect)
    {
        var type = effect.EffectType;
        if (type.Contains("projectile", StringComparison.OrdinalIgnoreCase))
        {
            var baseSpeed = type.Contains("frost", StringComparison.OrdinalIgnoreCase) ? 24f : 18f;
            var speed = PlayerAttributeResolver.ResolveProjectileSpeed(context, baseSpeed, spell.Tags, false);
            var travel = PlayerAttributeResolver.ResolveRange(context, effect.DurationSeconds * speed, spell.Tags, false);
            var radius = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, false);
            lines.Add($"  [b]projectile:[/b] travel={travel:0.##}  radius={radius:0.##}  speed={speed:0.##}  duration_scaling={effect.DurationScaling}");
            return;
        }

        if (type.Contains("beam", StringComparison.OrdinalIgnoreCase))
        {
            var length = PlayerAttributeResolver.ResolveRange(context, effect.DurationSeconds, spell.Tags, false);
            var radius = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, false);
            lines.Add($"  [b]beam:[/b] length={length:0.##}  radius={radius:0.##}  duration_scaling={effect.DurationScaling}");
            return;
        }

        if (type.Contains("summon", StringComparison.OrdinalIgnoreCase))
        {
            var baseSearchRadius = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, false);
            var search = PlayerAttributeResolver.ResolveRange(context, baseSearchRadius, spell.Tags, false);
            var aggro = PlayerAttributeResolver.ResolveRange(context, 26f, spell.Tags, false);
            var lifetime = PlayerAttributeResolver.ResolveDuration(context, effect.DurationSeconds, spell.Tags, false);
            lines.Add($"  [b]summon:[/b] corpse_search={search:0.##}  aggro={aggro:0.##}  lifetime={lifetime:0.##}  duration_scaling={effect.DurationScaling}");
            return;
        }

        if (type.Contains("aoe", StringComparison.OrdinalIgnoreCase)
            || type.Contains("hazard", StringComparison.OrdinalIgnoreCase)
            || type.Contains("vortex", StringComparison.OrdinalIgnoreCase))
        {
            var placement = type.Contains("firewall", StringComparison.OrdinalIgnoreCase)
                ? 7f
                : type.Contains("tornado", StringComparison.OrdinalIgnoreCase) ? 3f : 10f;
            var placementRange = PlayerAttributeResolver.ResolveRange(context, placement, spell.Tags, false);
            var radius = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, false);
            var lifetime = effect.DurationScaling == nameof(DurationScalingMode.PlayerDuration)
                ? PlayerAttributeResolver.ResolveDuration(context, effect.DurationSeconds, spell.Tags, false)
                : effect.DurationSeconds;
            lines.Add($"  [b]area:[/b] placement={placementRange:0.##}  radius={radius:0.##}  lifetime={lifetime:0.##}  duration_scaling={effect.DurationScaling}");
        }
    }

    private static string FormatStatLine(string name, float value, string suffix = "")
    {
        var line = $"[b]{EscapeBbcode(name)}:[/b] {value:0.###}";
        return string.IsNullOrWhiteSpace(suffix) ? line : $"{line} {suffix}";
    }

    private static string FormatFlagLine(string name, bool enabled)
    {
        return $"[b]{EscapeBbcode(name)}:[/b] {(enabled ? "enabled" : "disabled")}";
    }

    private static string FormatTags(IReadOnlyList<TagId> tags)
    {
        return tags.Count == 0 ? "-" : string.Join(", ", tags.Select(tag => tag.ToString()));
    }

    private static string FormatTag(TagId tag)
    {
        return tag.IsValid ? tag.ToString() : "-";
    }

    private static string EscapeBbcode(string value)
    {
        return value.Replace("[", "(").Replace("]", ")");
    }
}
