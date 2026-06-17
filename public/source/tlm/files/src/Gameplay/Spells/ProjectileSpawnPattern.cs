using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Projectiles;

namespace TheLastMage.Gameplay.Spells;

public static class ProjectileSpawnPattern
{
    public static void SpawnProjectiles(
        ProjectileSystem projectileSystem,
        ContentId spellId,
        EntityId casterId,
        Vector3 origin,
        Vector3 direction,
        int projectileCount,
        float damage,
        DamageType damageType,
        float radius,
        float lifetime,
        float speed,
        string statusKind,
        float presentationScale,
        bool piercesEnemies,
        int splitRemaining,
        float damageMultiplier = 1f,
        int spellChainGeneration = 0,
        float homingStrength = 0f,
        float homingConeRadians = 0f,
        float homingAcquireRadius = 0f,
        SpellOrbitState orbit = default)
    {
        var count = Math.Clamp(projectileCount, 1, 13);
        var normalizedDirection = direction.LengthSquared() <= 0.0001f ? Vector3.Forward : direction.Normalized();
        var right = Vector3.Up.Cross(normalizedDirection);
        if (right.LengthSquared() <= 0.0001f)
        {
            right = Vector3.Right;
        }

        right = right.Normalized();
        var center = (count - 1) * 0.5f;
        var laneSpacing = MathF.Max(0.35f, MathF.Max(0.05f, radius) * 2.15f);
        for (var i = 0; i < count; i++)
        {
            var offsetIndex = i - center;
            var originOffset = right * offsetIndex * laneSpacing;
            var projectileOrbit = orbit;
            if (projectileOrbit.Enabled)
            {
                projectileOrbit.AngleRadians += offsetIndex * 0.38f;
                projectileOrbit.Phase += i * 0.73f;
            }

            projectileSystem.Spawn(
                spellId,
                casterId,
                origin + originOffset,
                normalizedDirection,
                damage,
                damageType,
                radius,
                lifetime,
                speed,
                statusKind,
                presentationScale,
                piercesEnemies,
                splitRemaining,
                damageMultiplier,
                spellChainGeneration,
                homingStrength,
                homingConeRadians,
                homingAcquireRadius,
                projectileOrbit);
        }
    }
}
