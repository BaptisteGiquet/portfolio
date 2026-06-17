using TheLastMage.Gameplay.Combat;

namespace TheLastMage.Gameplay.StatusEffects;

public static class StatusEffectRules
{
    public const string Slow = "slow";
    public const string Burn = "burn";
    public const string Freeze = "freeze";
    public const string Immolate = "immolate";
    public const string Sleep = "sleep";

    private static readonly StatusEffectRule SlowRule = new(
        Slow,
        "SLOW",
        10,
        0.65f,
        1.25f,
        null,
        0f);

    private static readonly StatusEffectRule BurnRule = new(
        Burn,
        "BURN",
        30,
        5f,
        3f,
        DamageType.Fire,
        0.5f);

    private static readonly StatusEffectRule FreezeRule = new(
        Freeze,
        "FREEZE",
        40,
        0.05f,
        0.9f,
        null,
        0f);

    private static readonly StatusEffectRule ImmolateRule = new(
        Immolate,
        "IMMOLATE",
        35,
        7f,
        3.5f,
        DamageType.Fire,
        0.5f);

    private static readonly StatusEffectRule SleepRule = new(
        Sleep,
        "SLEEP",
        45,
        0f,
        5f,
        null,
        0f);

    public static bool TryGet(string kind, out StatusEffectRule rule)
    {
        if (string.Equals(kind, Slow, StringComparison.OrdinalIgnoreCase))
        {
            rule = SlowRule;
            return true;
        }

        if (string.Equals(kind, Burn, StringComparison.OrdinalIgnoreCase))
        {
            rule = BurnRule;
            return true;
        }

        if (string.Equals(kind, Freeze, StringComparison.OrdinalIgnoreCase))
        {
            rule = FreezeRule;
            return true;
        }

        if (string.Equals(kind, Immolate, StringComparison.OrdinalIgnoreCase))
        {
            rule = ImmolateRule;
            return true;
        }

        if (string.Equals(kind, Sleep, StringComparison.OrdinalIgnoreCase))
        {
            rule = SleepRule;
            return true;
        }

        rule = default;
        return false;
    }

    public static StatusEffectRule GetOrDefault(string kind)
    {
        return TryGet(kind, out var rule)
            ? rule
            : new StatusEffectRule(kind, kind.ToUpperInvariant(), 1, 1f, 1f, null, 0f);
    }

    public static string NormalizeKind(string kind)
    {
        if (string.Equals(kind, Slow, StringComparison.OrdinalIgnoreCase))
        {
            return Slow;
        }

        if (string.Equals(kind, Burn, StringComparison.OrdinalIgnoreCase))
        {
            return Burn;
        }

        if (string.Equals(kind, Freeze, StringComparison.OrdinalIgnoreCase))
        {
            return Freeze;
        }

        if (string.Equals(kind, Immolate, StringComparison.OrdinalIgnoreCase))
        {
            return Immolate;
        }

        if (string.Equals(kind, Sleep, StringComparison.OrdinalIgnoreCase))
        {
            return Sleep;
        }

        return kind.Trim().ToLowerInvariant();
    }
}
