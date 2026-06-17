using TheLastMage.Foundation;

namespace TheLastMage.Data.Runtime;

public readonly record struct EffectRuntimeSpec(
    string EffectType,
    float Value,
    float Radius,
    float DurationSeconds,
    string DurationScaling,
    ContentId TargetStatId);

public readonly record struct ModifierRuntimeSpec(
    ContentId SourceId,
    StatId StatId,
    string Operation,
    float Value,
    int Priority,
    TagId TargetTag);

public readonly record struct ProcRuntimeSpec(
    ContentId SourceId,
    string Trigger,
    string EffectType,
    float Chance,
    float Value,
    TagId TargetTag);

public readonly record struct ItemEffectRuntimeSpec(
    string Kind,
    StatId StatId,
    float Value,
    float SecondaryValue,
    float TertiaryValue,
    float Chance,
    TagId TargetTag);

public readonly record struct ItemPoolWeightRuntimeSpec(
    ContentId PoolId,
    float Weight);

public readonly record struct EnemyAbilityRuntimeSpec(
    string EffectType,
    float Value,
    float Radius,
    float DurationSeconds,
    string DurationScaling,
    ContentId TargetStatId);

public sealed record SpellRuntimeDefinition(
    ContentId Id,
    string DisplayName,
    string Description,
    IReadOnlyList<TagId> Tags,
    float CooldownSeconds,
    float ManaCost,
    ContentId FeedbackProfileId,
    IReadOnlyList<EffectRuntimeSpec> Effects);

public sealed record ItemRuntimeDefinition(
    ContentId Id,
    int ItemNumber,
    string Kind,
    string DisplayName,
    string Description,
    string HiddenDescription,
    IReadOnlyList<TagId> Tags,
    IReadOnlyList<ItemPoolWeightRuntimeSpec> PoolWeights,
    ContentId UnlockAchievementId,
    bool HasUnlimitedActivations,
    int MaxActivations,
    bool IsFlavorOnly,
    string RevealedStatText,
    string RevealedBehaviorText,
    string RevealedEffectText,
    Godot.Texture2D? Icon,
    Godot.PackedScene? EnemyFormPresentationScene,
    IReadOnlyList<ItemEffectRuntimeSpec> Effects,
    IReadOnlyList<EffectRuntimeSpec> ActiveEffects,
    IReadOnlyList<ModifierRuntimeSpec> Modifiers,
    IReadOnlyList<ProcRuntimeSpec> Procs,
    int PlayersPicked,
    float WinRatePercent);

public sealed record EnemyRuntimeDefinition(
    ContentId Id,
    string DisplayName,
    string Description,
    IReadOnlyList<TagId> Tags,
    string Family,
    string Role,
    float MaxHealth,
    float MoveSpeed,
    float AttackDamage,
    float AttackRange,
    float PreferredRange,
    float AbilityRange,
    float AbilityCooldownSeconds,
    float AbilityTelegraphSeconds,
    float AbilityProjectileSpeed,
    int MaxActive,
    Godot.PackedScene? PresentationScene,
    IReadOnlyList<EnemyAbilityRuntimeSpec> Abilities);

public sealed record WaveRuntimeDefinition(
    ContentId Id,
    string DisplayName,
    int DayIndex,
    float DurationSeconds,
    int TargetActiveEnemies,
    float SpawnIntervalSeconds,
    float Intensity,
    IReadOnlyList<ContentId> EnemyIds,
    IReadOnlyList<WaveSpawnGroupRuntimeDefinition> SpawnGroups,
    IReadOnlyList<WaveSpawnLaneRuntimeDefinition> SpawnLanes);

public sealed record WaveSpawnGroupRuntimeDefinition(
    ContentId EnemyId,
    int Count,
    float StartTimeSeconds,
    float SpawnIntervalSeconds,
    float IntensityMultiplier);

public sealed record WaveSpawnLaneRuntimeDefinition(
    string Id,
    Godot.Vector3 SpawnPosition,
    Godot.Vector3 ApproachPosition,
    float Weight);

public sealed record DefenseRuntimeDefinition(
    ContentId Id,
    string DisplayName,
    IReadOnlyList<TagId> Tags,
    string PlacementRule,
    float MaxHealth,
    float Cost,
    float RepairCost,
    float PlacementRadius,
    float BlockRadius,
    float TriggerRadius,
    float ExplosionRadius,
    float FuseSeconds,
    float RechargeSeconds,
    int MaxCharges,
    float Damage);

public sealed record MageRuntimeDefinition(
    ContentId Id,
    string DisplayName,
    string Description,
    IReadOnlyList<TagId> Tags,
    float MaxHealth,
    float MoveSpeed,
    float FireRate,
    float Luck,
    Godot.PackedScene? PresentationScene,
    IReadOnlyList<ContentId> StartingSpellIds);

public sealed record AchievementRuntimeDefinition(
    ContentId Id,
    string DisplayName,
    string RequirementKey,
    IReadOnlyList<ContentId> UnlockMageIds,
    IReadOnlyList<ContentId> UnlockItemIds);
