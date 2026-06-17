using TheLastMage.Gameplay.Combat;

namespace TheLastMage.Gameplay.StatusEffects;

public readonly record struct StatusEffectRule(
    string Kind,
    string DisplayText,
    int DisplayPriority,
    float BasePower,
    float BaseDurationSeconds,
    DamageType? PeriodicDamageType,
    float PeriodicTickSeconds);
