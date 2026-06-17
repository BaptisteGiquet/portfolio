namespace TheLastMage.Data.Definitions;

public static class ActiveEffectIds
{
    public const string MagicBall = "magic_ball";
    public const string TeleportPlayer = "teleport_player";
    public const string CooldownTick = "cooldown_tick";
    public const string GrantSouls = "grant_souls";
    public const string GrantMaterials = "grant_materials";
    public const string HealPlayer = "heal_player";
    public const string GrantHitProtection = "grant_hit_protection";
    public const string SleepNearbyEnemies = "sleep_nearby_enemies";
    public const string TransformEnemiesToForm = "transform_enemies_to_form";
    public const string PlayerStasis = "player_stasis";
    public const string TemporaryStatModifier = "temporary_stat_modifier";
    public const string AssaultRifleMode = "assault_rifle_mode";
    public const string ThrowTeleportProjectile = "throw_teleport_projectile";

    public static readonly IReadOnlyList<string> All = new[]
    {
        MagicBall,
        TeleportPlayer,
        CooldownTick,
        GrantSouls,
        GrantMaterials,
        HealPlayer,
        GrantHitProtection,
        SleepNearbyEnemies,
        TransformEnemiesToForm,
        PlayerStasis,
        TemporaryStatModifier,
        AssaultRifleMode,
        ThrowTeleportProjectile
    };

    public static bool IsKnown(string effectType)
    {
        return All.Any(value => string.Equals(value, effectType, StringComparison.OrdinalIgnoreCase));
    }
}
