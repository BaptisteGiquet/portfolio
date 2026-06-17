using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Spells;

public static class SpellPlacement
{
    private static readonly Color FirePreview = new(1f, 0.18f, 0.04f, 0.34f);
    private static readonly Color FrostPreview = new(0.35f, 0.75f, 1f, 0.28f);
    private static readonly Color NecromancyPreview = new(0.25f, 1f, 0.55f, 0.28f);
    private static readonly Color VortexPreview = new(0.68f, 0.9f, 1f, 0.28f);
    private static readonly Color GenericPreview = new(0.8f, 0.8f, 0.8f, 0.22f);

    public static bool TryBuildPreview(
        RunContext context,
        SpellRuntimeDefinition spell,
        Vector3 origin,
        Vector3 direction,
        out SpellPlacementPreview preview)
    {
        for (var i = 0; i < spell.Effects.Count; i++)
        {
            var effect = spell.Effects[i];
            if (TryBuildPreview(context, spell, effect, origin, direction, out preview))
            {
                return true;
            }
        }

        preview = default;
        return false;
    }

    public static bool TryBuildPreview(
        SpellRuntimeDefinition spell,
        Vector3 origin,
        Vector3 direction,
        out SpellPlacementPreview preview)
    {
        for (var i = 0; i < spell.Effects.Count; i++)
        {
            var effect = spell.Effects[i];
            if (TryBuildPreview(effect, origin, direction, out preview))
            {
                return true;
            }
        }

        preview = default;
        return false;
    }

    public static bool TryBuildPreview(
        RunContext context,
        SpellRuntimeDefinition spell,
        EffectRuntimeSpec effect,
        Vector3 origin,
        Vector3 direction,
        out SpellPlacementPreview preview)
    {
        if (string.Equals(effect.EffectType, "firewall_hazard", StringComparison.OrdinalIgnoreCase))
        {
            var halfLength = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, recordBreakdown: false);
            var range = PlayerAttributeResolver.ResolveRange(context, 7f, spell.Tags, recordBreakdown: false);
            preview = BuildWall(origin, direction, halfLength, 0.55f, range, FirePreview);
            return true;
        }

        if (string.Equals(effect.EffectType, "blizzard_aoe", StringComparison.OrdinalIgnoreCase))
        {
            var radius = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, recordBreakdown: false);
            var range = PlayerAttributeResolver.ResolveRange(context, 10f, spell.Tags, recordBreakdown: false);
            preview = BuildDisk(origin, direction, radius, range, FrostPreview);
            return true;
        }

        if (string.Equals(effect.EffectType, "tornado_vortex", StringComparison.OrdinalIgnoreCase))
        {
            var radius = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, recordBreakdown: false);
            var range = PlayerAttributeResolver.ResolveRange(context, 3f, spell.Tags, recordBreakdown: false);
            preview = BuildDisk(origin, direction, radius, range, VortexPreview);
            return true;
        }

        if (string.Equals(effect.EffectType, "raise_dead_summon", StringComparison.OrdinalIgnoreCase))
        {
            var radius = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, recordBreakdown: false);
            var searchRadius = PlayerAttributeResolver.ResolveRange(context, radius, spell.Tags, recordBreakdown: false);
            preview = BuildDisk(origin, direction, searchRadius, MathF.Max(2f, searchRadius * 0.5f), NecromancyPreview);
            return true;
        }

        if (effect.EffectType.Contains("aoe", StringComparison.OrdinalIgnoreCase)
            || effect.EffectType.Contains("hazard", StringComparison.OrdinalIgnoreCase)
            || effect.EffectType.Contains("zone", StringComparison.OrdinalIgnoreCase))
        {
            var radius = PlayerAttributeResolver.ResolveAreaRadius(context, effect.Radius, spell.Tags, recordBreakdown: false);
            var range = PlayerAttributeResolver.ResolveRange(context, 8f, spell.Tags, recordBreakdown: false);
            preview = BuildDisk(origin, direction, radius, range, GenericPreview);
            return true;
        }

        preview = default;
        return false;
    }

    public static bool TryBuildPreview(
        EffectRuntimeSpec effect,
        Vector3 origin,
        Vector3 direction,
        out SpellPlacementPreview preview)
    {
        if (string.Equals(effect.EffectType, "firewall_hazard", StringComparison.OrdinalIgnoreCase))
        {
            preview = BuildWall(origin, direction, effect.Radius, 0.55f, 7f, FirePreview);
            return true;
        }

        if (string.Equals(effect.EffectType, "blizzard_aoe", StringComparison.OrdinalIgnoreCase))
        {
            preview = BuildDisk(origin, direction, effect.Radius, 10f, FrostPreview);
            return true;
        }

        if (string.Equals(effect.EffectType, "tornado_vortex", StringComparison.OrdinalIgnoreCase))
        {
            preview = BuildDisk(origin, direction, effect.Radius, 3f, VortexPreview);
            return true;
        }

        if (string.Equals(effect.EffectType, "raise_dead_summon", StringComparison.OrdinalIgnoreCase))
        {
            preview = BuildDisk(origin, direction, effect.Radius, MathF.Max(2f, effect.Radius * 0.5f), NecromancyPreview);
            return true;
        }

        if (effect.EffectType.Contains("aoe", StringComparison.OrdinalIgnoreCase)
            || effect.EffectType.Contains("hazard", StringComparison.OrdinalIgnoreCase)
            || effect.EffectType.Contains("zone", StringComparison.OrdinalIgnoreCase))
        {
            preview = BuildDisk(origin, direction, effect.Radius, 8f, GenericPreview);
            return true;
        }

        preview = default;
        return false;
    }

    public static SpellPlacementPreview BuildDisk(
        Vector3 origin,
        Vector3 direction,
        float radius,
        float fallbackRange,
        Color color)
    {
        return new SpellPlacementPreview(
            SpellPlacementShape.Disk,
            ProjectAimToGround(origin, direction, fallbackRange),
            MathF.Max(0.1f, radius),
            0f,
            0f,
            Vector3.Right,
            color);
    }

    public static SpellPlacementPreview BuildWall(
        Vector3 origin,
        Vector3 direction,
        float halfLength,
        float halfWidth,
        float fallbackRange,
        Color color)
    {
        var forward = GetHorizontalForward(direction);
        var wallRight = new Vector3(-forward.Z, 0f, forward.X).Normalized();
        return new SpellPlacementPreview(
            SpellPlacementShape.Wall,
            ProjectAimToGround(origin, direction, fallbackRange),
            MathF.Max(0.1f, halfLength),
            MathF.Max(0.1f, halfLength),
            MathF.Max(0.1f, halfWidth),
            wallRight,
            color);
    }

    public static Vector3 ProjectAimToGround(Vector3 origin, Vector3 direction, float fallbackRange)
    {
        var normalized = direction.Normalized();
        if (normalized.Y < -0.05f)
        {
            var distance = -origin.Y / normalized.Y;
            if (distance > 0f && distance <= 40f)
            {
                var hit = origin + normalized * distance;
                hit.Y = 0f;
                return hit;
            }
        }

        var fallback = origin + GetHorizontalForward(normalized) * fallbackRange;
        fallback.Y = 0f;
        return fallback;
    }

    private static Vector3 GetHorizontalForward(Vector3 direction)
    {
        var horizontal = new Vector3(direction.X, 0f, direction.Z);
        if (horizontal.LengthSquared() <= 0.0001f)
        {
            return Vector3.Forward;
        }

        return horizontal.Normalized();
    }
}
