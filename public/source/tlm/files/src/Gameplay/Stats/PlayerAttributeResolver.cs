using TheLastMage.Foundation;
using TheLastMage.Gameplay.Items;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Stats;

public static class PlayerAttributeResolver
{
    public static readonly StatId Health = StatId.From("health");
    public static readonly StatId MoveSpeed = StatId.From("move_speed");
    public static readonly StatId Damage = StatId.From("damage");
    public static readonly StatId FireRate = StatId.From("fire_rate");
    public static readonly StatId Range = StatId.From("range");
    public static readonly StatId Luck = StatId.From("luck");
    public static readonly StatId Duration = StatId.From("duration");
    public static readonly StatId AreaRadius = StatId.From("radius");
    public static readonly StatId ProjectileSpeed = StatId.From("projectile_speed");
    public static readonly StatId StatusPower = StatId.From("status_power");
    public static readonly StatId SummonDamage = StatId.From("summon_damage");
    public static readonly StatId DefenseDamage = StatId.From("defense_damage");
    public static readonly StatId Pierce = StatId.From("pierce");
    public static readonly StatId SpellCount = StatId.From("spell_count");
    public static readonly StatId ProjectileSplit = StatId.From("projectile_split");
    public static readonly StatId HomingStrength = StatId.From("homing_strength");
    public static readonly StatId HomingAngleDegrees = StatId.From("homing_angle_degrees");
    public static readonly StatId HomingAcquireRadius = StatId.From("homing_acquire_radius");
    public static readonly StatId SpellOrbitStrength = StatId.From("spell_orbit_strength");
    public static readonly StatId SpellOrbitRadius = StatId.From("spell_orbit_radius");
    public static readonly StatId SpellOrbitAngularSpeed = StatId.From("spell_orbit_angular_speed");

    private static readonly ModifierSystem ModifierSystem = new();
    private static readonly StatResolver StatResolver = new();

    public static StatBreakdown ResolveBreakdown(
        RunContext context,
        StatId statId,
        float baseValue,
        IReadOnlyCollection<TagId>? targetTags = null,
        bool recordBreakdown = true)
    {
        var modifiers = ModifierSystem.BuildModifiers(context.State.Build, context.Content);
        var breakdown = StatResolver.Resolve(statId, baseValue, modifiers, targetTags);
        if (recordBreakdown)
        {
            context.State.RecordStatBreakdown(breakdown);
        }

        return breakdown;
    }

    public static float ResolveValue(
        RunContext context,
        StatId statId,
        float baseValue,
        IReadOnlyCollection<TagId>? targetTags = null,
        bool recordBreakdown = true)
    {
        return ResolveBreakdown(context, statId, baseValue, targetTags, recordBreakdown).FinalValue;
    }

    public static float ResolveDamage(
        RunContext context,
        float baseDamage,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        sourceTags ??= Array.Empty<TagId>();
        var damage = ResolveValue(context, Damage, baseDamage, sourceTags, recordBreakdown);
        if (GameplayTagPath.MatchesAny(TagId.From("combat.source.summon"), sourceTags))
        {
            damage = ResolveValue(context, SummonDamage, damage, sourceTags, recordBreakdown);
        }

        if (GameplayTagPath.MatchesAny(TagId.From("combat.source.defense"), sourceTags))
        {
            damage = ResolveValue(context, DefenseDamage, damage, sourceTags, recordBreakdown);
        }

        return MathF.Max(0f, damage);
    }

    public static float ResolveFireRate(
        RunContext context,
        IReadOnlyCollection<TagId>? spellTags,
        bool recordBreakdown = true)
    {
        return MathF.Max(0.05f, ResolveValue(context, FireRate, context.State.Player.BaseFireRate, spellTags, recordBreakdown));
    }

    public static float ResolveRange(
        RunContext context,
        float baseDistance,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        return MathF.Max(0.1f, ResolveValue(context, Range, baseDistance, sourceTags, recordBreakdown));
    }

    public static float ResolveDuration(
        RunContext context,
        float baseDuration,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        return MathF.Max(0f, ResolveValue(context, Duration, baseDuration, sourceTags, recordBreakdown));
    }

    public static float ResolveAreaRadius(
        RunContext context,
        float baseRadius,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        return MathF.Max(0.05f, ResolveValue(context, AreaRadius, baseRadius, sourceTags, recordBreakdown));
    }

    public static float ResolveProjectileSpeed(
        RunContext context,
        float baseSpeed,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        return MathF.Max(0.1f, ResolveValue(context, ProjectileSpeed, baseSpeed, sourceTags, recordBreakdown));
    }

    public static bool ResolveProjectilePierce(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        var value = ResolveValue(context, Pierce, 0f, sourceTags, recordBreakdown);
        return value > 0f;
    }

    public static int ResolveAdditionalSpellCount(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        var value = ResolveValue(context, SpellCount, 0f, sourceTags, recordBreakdown);
        return Math.Clamp((int)MathF.Floor(value + 0.001f), 0, 12);
    }

    public static int ResolveProjectileSplitDepth(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        var value = ResolveValue(context, ProjectileSplit, 0f, sourceTags, recordBreakdown);
        return Math.Clamp((int)MathF.Floor(value + 0.001f), 0, 4);
    }

    public static float ResolveHomingStrength(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        return MathF.Max(0f, ResolveValue(context, HomingStrength, 0f, sourceTags, recordBreakdown));
    }

    public static float ResolveHomingAngleRadians(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        float defaultDegrees,
        bool recordBreakdown = true)
    {
        var degrees = ResolveValue(context, HomingAngleDegrees, 0f, sourceTags, recordBreakdown);
        if (degrees <= 0f)
        {
            degrees = defaultDegrees;
        }

        return Math.Clamp(degrees, 1f, 180f) * MathF.PI / 180f;
    }

    public static float ResolveHomingAcquireRadius(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        float defaultRadius,
        bool recordBreakdown = true)
    {
        var radius = ResolveValue(context, HomingAcquireRadius, 0f, sourceTags, recordBreakdown);
        return MathF.Max(0.1f, radius > 0f ? radius : defaultRadius);
    }

    public static float ResolveDamagePresentationScale(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        const float baselineDamage = 10f;
        var effectiveDamage = ResolveDamage(context, baselineDamage, sourceTags, recordBreakdown);
        return Math.Clamp(MathF.Sqrt(effectiveDamage / baselineDamage), 0.75f, 2.5f);
    }

    public static float ResolveSpellOrbitStrength(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        bool recordBreakdown = true)
    {
        return MathF.Max(0f, ResolveValue(context, SpellOrbitStrength, 0f, sourceTags, recordBreakdown));
    }

    public static float ResolveSpellOrbitRadius(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        float defaultRadius,
        bool recordBreakdown = true)
    {
        var value = ResolveValue(context, SpellOrbitRadius, 0f, sourceTags, recordBreakdown);
        return MathF.Max(0.25f, value > 0f ? value : defaultRadius);
    }

    public static float ResolveSpellOrbitAngularSpeed(
        RunContext context,
        IReadOnlyCollection<TagId>? sourceTags,
        float defaultRadiansPerSecond,
        bool recordBreakdown = true)
    {
        var value = ResolveValue(context, SpellOrbitAngularSpeed, 0f, sourceTags, recordBreakdown);
        return value > 0f ? MathF.Max(0.05f, value) : MathF.Max(0.05f, defaultRadiansPerSecond);
    }
}
