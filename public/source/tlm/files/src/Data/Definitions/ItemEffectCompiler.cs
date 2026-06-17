using System.Globalization;

namespace TheLastMage.Data.Definitions;

public static class ItemEffectCompiler
{
    public static IReadOnlyList<CompiledItemEffect> Compile(ItemEffectSpec effect)
    {
        if (effect == null)
        {
            return Array.Empty<CompiledItemEffect>();
        }

        return effect.Kind switch
        {
            ItemEffectKind.AddMultiplyStat => Modifier(
                NormalizeStat(effect.StatId, "damage"),
                DataModifierOp.Multiplicative,
                NormalizeMoreMultiplier(effect.Value),
                effect.Priority,
                effect.TargetTag),
            ItemEffectKind.AddProjectilePierce => Modifier(
                "pierce",
                DataModifierOp.FlatAdd,
                1f,
                effect.Priority,
                "spell.delivery.projectile"),
            ItemEffectKind.AddFlatStat => Modifier(
                NormalizeStat(effect.StatId, "health"),
                DataModifierOp.FlatAdd,
                effect.Value,
                effect.Priority,
                effect.TargetTag),
            ItemEffectKind.FreezeWithFrostDamage => Proc(
                ProcTrigger.OnHit,
                "apply_freeze",
                effect.ChancePercent,
                MathF.Max(1f, effect.Value),
                "combat.damage.frost"),
            ItemEffectKind.ImmolateWithFireDamage => Proc(
                ProcTrigger.OnHit,
                "apply_burn",
                effect.ChancePercent,
                MathF.Max(1f, effect.Value),
                "combat.damage.fire"),
            ItemEffectKind.EnemiesExplodeOnDeath => Proc(
                ProcTrigger.OnKill,
                "kill_explosion",
                effect.ChancePercent,
                MathF.Max(1f, effect.Value),
                effect.TargetTag),
            ItemEffectKind.BurningGroundOnEnemyDeath => Proc(
                ProcTrigger.OnKill,
                "burning_ground",
                effect.ChancePercent,
                MathF.Max(1f, effect.Value),
                effect.TargetTag),
            ItemEffectKind.RemoveMultiplyStat => Modifier(
                NormalizeStat(effect.StatId, "damage"),
                DataModifierOp.Multiplicative,
                NormalizeLessMultiplier(effect.Value),
                effect.Priority,
                effect.TargetTag),
            ItemEffectKind.RemoveFlatStat => Modifier(
                NormalizeStat(effect.StatId, "health"),
                DataModifierOp.FlatAdd,
                -MathF.Abs(effect.Value),
                effect.Priority,
                effect.TargetTag),
            ItemEffectKind.AddSpellCount => Modifier(
                "spell_count",
                DataModifierOp.FlatAdd,
                MathF.Max(0f, effect.Value),
                effect.Priority,
                string.Empty),
            ItemEffectKind.SplitProjectile => Modifier(
                "projectile_split",
                DataModifierOp.FlatAdd,
                1f,
                effect.Priority,
                "spell.delivery.projectile"),
            ItemEffectKind.HomingSpells => HomingModifiers(effect),
            ItemEffectKind.OrbitingSpells => OrbitingModifiers(effect),
            ItemEffectKind.KeepItIn => Array.Empty<CompiledItemEffect>(),
            ItemEffectKind.AbyssalRing => Array.Empty<CompiledItemEffect>(),
            ItemEffectKind.FaultyFocus => Array.Empty<CompiledItemEffect>(),
            ItemEffectKind.PreventNextPlayerHealthLoss => Array.Empty<CompiledItemEffect>(),
            ItemEffectKind.PersistentSummons => Array.Empty<CompiledItemEffect>(),
            _ => Array.Empty<CompiledItemEffect>()
        };
    }

    public static Godot.Collections.Array<ModifierSpec> BuildModifierSpecs(IEnumerable<ItemEffectSpec> effects)
    {
        var modifiers = new Godot.Collections.Array<ModifierSpec>();
        foreach (var effect in effects)
        {
            foreach (var compiled in Compile(effect))
            {
                if (!compiled.IsModifier)
                {
                    continue;
                }

                modifiers.Add(new ModifierSpec
                {
                    StatId = compiled.StatId,
                    Operation = compiled.Operation,
                    Value = compiled.Value,
                    Priority = compiled.Priority,
                    TargetTag = compiled.TargetTag
                });
            }
        }

        return modifiers;
    }

    public static Godot.Collections.Array<ProcSpec> BuildProcSpecs(IEnumerable<ItemEffectSpec> effects)
    {
        var procs = new Godot.Collections.Array<ProcSpec>();
        foreach (var effect in effects)
        {
            foreach (var compiled in Compile(effect))
            {
                if (compiled.IsModifier)
                {
                    continue;
                }

                procs.Add(new ProcSpec
                {
                    Trigger = compiled.Trigger,
                    EffectType = compiled.EffectType,
                    Chance = compiled.Chance,
                    Value = compiled.Value,
                    TargetTag = compiled.TargetTag
                });
            }
        }

        return procs;
    }

    public static string Describe(ItemEffectSpec effect)
    {
        if (effect == null)
        {
            return "No effect";
        }

        var stat = string.IsNullOrWhiteSpace(effect.StatId) ? "damage" : effect.StatId.Trim();
        var target = EmptyAsAll(effect.TargetTag);
        return effect.Kind switch
        {
            ItemEffectKind.AddMultiplyStat => $"Increase {stat} by {FormatPercent(NormalizeIncreaseFraction(effect.Value))} target={target}",
            ItemEffectKind.AddFlatStat => $"Add {FormatSigned(effect.Value)} {stat} target={target}",
            ItemEffectKind.RemoveMultiplyStat => $"Reduce {stat} by {FormatPercent(NormalizeReductionFraction(effect.Value))} target={target}",
            ItemEffectKind.RemoveFlatStat => $"Remove {FormatNumber(MathF.Abs(effect.Value))} {stat} target={target}",
            ItemEffectKind.AddProjectilePierce => "Enable spell projectile pierce",
            ItemEffectKind.FreezeWithFrostDamage => $"Frost hits freeze for {FormatSeconds(MathF.Max(1f, effect.Value))} chance={FormatPercent(effect.ChancePercent / 100f)}",
            ItemEffectKind.ImmolateWithFireDamage => $"Fire hits immolate for {FormatSeconds(MathF.Max(1f, effect.Value))} chance={FormatPercent(effect.ChancePercent / 100f)}",
            ItemEffectKind.EnemiesExplodeOnDeath => $"Killed enemies explode power={FormatNumber(MathF.Max(1f, effect.Value))} chance={FormatPercent(effect.ChancePercent / 100f)} target={target}",
            ItemEffectKind.BurningGroundOnEnemyDeath => $"Killed enemies leave burning ground power={FormatNumber(MathF.Max(1f, effect.Value))} chance={FormatPercent(effect.ChancePercent / 100f)} target={target}",
            ItemEffectKind.AddSpellCount => $"Add {FormatNumber(MathF.Max(0f, effect.Value))} extra spell cast(s)",
            ItemEffectKind.SplitProjectile => "Enable projectile split target=projectile",
            ItemEffectKind.HomingSpells => $"Enable homing spells strength={FormatNumber(MathF.Max(0.1f, effect.Value))} angle={FormatHomingAngle(effect.SecondaryValue)} width={FormatHomingWidth(effect.TertiaryValue)} target={target}",
            ItemEffectKind.OrbitingSpells => $"Force spells into unstable player orbit strength={FormatNumber(MathF.Max(0.1f, effect.Value))} radius={FormatOrbitRadius(effect.SecondaryValue)} speed={FormatOrbitSpeed(effect.TertiaryValue)} target={target}",
            ItemEffectKind.KeepItIn => $"Hold fire to charge for {FormatSeconds(MathF.Max(0.05f, effect.SecondaryValue))} before releasing {FormatNumber(1f + MathF.Max(1f, effect.Value))} spell instance(s)",
            ItemEffectKind.AbyssalRing => $"Hold fire for {FormatSeconds(MathF.Max(0.05f, effect.SecondaryValue))} while casting normally; release a spreading ring for {FormatNumber(MathF.Max(0.1f, effect.Value))}x player damage",
            ItemEffectKind.FaultyFocus => $"Spell hits split into 3 child spells at {FormatPercent(MathF.Max(0.01f, effect.Value))} damage for up to {FormatNumber(MathF.Max(1f, effect.SecondaryValue))} generation(s)",
            ItemEffectKind.PreventNextPlayerHealthLoss => $"Prevent the next {FormatNumber(MathF.Max(1f, effect.Value))} player health-removing hit(s)",
            ItemEffectKind.PersistentSummons => "Summons stay until killed instead of expiring by duration",
            _ => $"{effect.Kind}: no compiled gameplay output."
        };
    }

    public static string DescribeCompiled(ItemEffectSpec effect)
    {
        var compiled = Compile(effect);
        if (compiled.Count == 0)
        {
            return effect.Kind switch
            {
                ItemEffectKind.KeepItIn => "custom cast-flow behavior keep_it_in",
                ItemEffectKind.AbyssalRing => "custom cast-flow behavior abyssal_ring",
                ItemEffectKind.FaultyFocus => "custom on-hit spell split behavior faulty_focus",
                ItemEffectKind.PreventNextPlayerHealthLoss => "custom incoming damage prevention behavior",
                ItemEffectKind.PersistentSummons => "custom summon lifetime behavior",
                _ => $"{effect.Kind}: no compiled gameplay output."
            };
        }

        return string.Join("; ", compiled.Select(value => value.IsModifier
            ? $"{value.Operation} {value.StatId} {FormatNumber(value.Value)} target={EmptyAsAll(value.TargetTag)}"
            : $"{value.Trigger} {value.EffectType} chance={FormatNumber(value.Chance)} value={FormatNumber(value.Value)} target={EmptyAsAll(value.TargetTag)}"));
    }

    public static bool RequiresFreeStat(ItemEffectKind kind)
    {
        return kind is ItemEffectKind.AddMultiplyStat or ItemEffectKind.AddFlatStat or ItemEffectKind.RemoveMultiplyStat or ItemEffectKind.RemoveFlatStat;
    }

    private static IReadOnlyList<CompiledItemEffect> Modifier(
        string statId,
        DataModifierOp operation,
        float value,
        int priority,
        string targetTag)
    {
        return new[]
        {
            CompiledItemEffect.Modifier(statId, operation, value, priority, targetTag)
        };
    }

    private static IReadOnlyList<CompiledItemEffect> HomingModifiers(ItemEffectSpec effect)
    {
        var modifiers = new List<CompiledItemEffect>
        {
            CompiledItemEffect.Modifier(
                "homing_strength",
                DataModifierOp.FlatAdd,
                MathF.Max(0.1f, effect.Value),
                effect.Priority,
                effect.TargetTag)
        };

        if (effect.SecondaryValue > 1f)
        {
            modifiers.Add(CompiledItemEffect.Modifier(
                "homing_angle_degrees",
                DataModifierOp.FlatAdd,
                Math.Clamp(effect.SecondaryValue, 1f, 180f),
                effect.Priority,
                effect.TargetTag));
        }

        if (effect.TertiaryValue > 0f)
        {
            modifiers.Add(CompiledItemEffect.Modifier(
                "homing_acquire_radius",
                DataModifierOp.FlatAdd,
                effect.TertiaryValue,
                effect.Priority,
                effect.TargetTag));
        }

        return modifiers;
    }

    private static IReadOnlyList<CompiledItemEffect> OrbitingModifiers(ItemEffectSpec effect)
    {
        var modifiers = new List<CompiledItemEffect>
        {
            CompiledItemEffect.Modifier(
                "spell_orbit_strength",
                DataModifierOp.FlatAdd,
                MathF.Max(0.1f, effect.Value),
                effect.Priority,
                effect.TargetTag)
        };

        if (effect.SecondaryValue > 0f)
        {
            modifiers.Add(CompiledItemEffect.Modifier(
                "spell_orbit_radius",
                DataModifierOp.FlatAdd,
                effect.SecondaryValue,
                effect.Priority,
                effect.TargetTag));
        }

        if (effect.TertiaryValue > 0f)
        {
            modifiers.Add(CompiledItemEffect.Modifier(
                "spell_orbit_angular_speed",
                DataModifierOp.FlatAdd,
                effect.TertiaryValue,
                effect.Priority,
                effect.TargetTag));
        }

        return modifiers;
    }

    private static IReadOnlyList<CompiledItemEffect> Proc(
        ProcTrigger trigger,
        string effectType,
        float chancePercent,
        float value,
        string targetTag)
    {
        return new[]
        {
            CompiledItemEffect.Proc(trigger, effectType, Math.Clamp(chancePercent / 100f, 0f, 1f), value, targetTag)
        };
    }

    private static string NormalizeStat(string statId, string fallback)
    {
        return string.IsNullOrWhiteSpace(statId) ? fallback : statId.Trim();
    }

    private static float NormalizeMoreMultiplier(float value)
    {
        return value <= 0f ? 1f : value < 1f ? 1f + value : value;
    }

    private static float NormalizeLessMultiplier(float value)
    {
        return Math.Clamp(1f - MathF.Abs(value), 0.05f, 1f);
    }

    private static float NormalizeIncreaseFraction(float value)
    {
        return value <= 0f ? 0f : value < 1f ? value : value - 1f;
    }

    private static float NormalizeReductionFraction(float value)
    {
        return 1f - NormalizeLessMultiplier(value);
    }

    private static string FormatSigned(float value)
    {
        return value == 0f ? "0" : $"+{FormatNumber(value)}";
    }

    private static string FormatSeconds(float value)
    {
        return $"{FormatNumber(value)}s";
    }

    private static string FormatPercent(float value)
    {
        return $"{FormatNumber(value * 100f)}%";
    }

    private static string FormatNumber(float value)
    {
        return value.ToString("0.###", CultureInfo.InvariantCulture);
    }

    private static string FormatHomingAngle(float value)
    {
        return value > 1f ? $"{FormatNumber(value)}deg" : "delivery default";
    }

    private static string FormatHomingWidth(float value)
    {
        return value > 0f ? $"{FormatNumber(value)}m" : "delivery default";
    }

    private static string FormatOrbitRadius(float value)
    {
        return value > 0f ? $"{FormatNumber(value)}m" : "delivery default";
    }

    private static string FormatOrbitSpeed(float value)
    {
        return value > 0f ? $"{FormatNumber(value)}rad/s" : "delivery default";
    }

    private static string EmptyAsAll(string value)
    {
        return string.IsNullOrWhiteSpace(value) ? "all" : value;
    }
}

public readonly record struct CompiledItemEffect(
    bool IsModifier,
    string StatId,
    DataModifierOp Operation,
    ProcTrigger Trigger,
    string EffectType,
    float Chance,
    float Value,
    int Priority,
    string TargetTag)
{
    public static CompiledItemEffect Modifier(
        string statId,
        DataModifierOp operation,
        float value,
        int priority,
        string targetTag)
    {
        return new CompiledItemEffect(true, statId, operation, ProcTrigger.None, string.Empty, 0f, value, priority, targetTag);
    }

    public static CompiledItemEffect Proc(
        ProcTrigger trigger,
        string effectType,
        float chance,
        float value,
        string targetTag)
    {
        return new CompiledItemEffect(false, string.Empty, DataModifierOp.FlatAdd, trigger, effectType, chance, value, 0, targetTag);
    }
}
