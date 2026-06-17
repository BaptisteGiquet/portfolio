using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Foundation.Events;

public readonly record struct RunStartedEvent(RunId RunId) : IGameEvent;

public readonly record struct DayStartedEvent(int DayIndex) : IGameEvent;

public readonly record struct NightStartedEvent(int DayIndex) : IGameEvent;

public readonly record struct RunVictoryEvent(int DayIndex, int HumanityRemaining) : IGameEvent;

public readonly record struct RunDefeatEvent(int DayIndex, ContentId SourceId) : IGameEvent;

public readonly record struct RunCompletedEvent(RunSummaryData Summary) : IGameEvent;

public readonly record struct RunRewardsGrantedEvent(ContentId SourceId, int Souls, int Materials, int HumanityReduced) : IGameEvent;

public readonly record struct SpellCastEvent(ContentId SpellId, EntityId CasterId) : IGameEvent;

public readonly record struct SpellChargeStartedEvent(
    ContentId SpellId,
    EntityId CasterId,
    ContentId SourceItemId,
    float RequiredChargeSeconds) : IGameEvent;

public readonly record struct SpellChargeReadyEvent(ContentId SpellId, EntityId CasterId, ContentId SourceItemId) : IGameEvent;

public readonly record struct SpellChargeReleasedEvent(
    ContentId SpellId,
    EntityId CasterId,
    ContentId SourceItemId,
    int SpellInstanceCount) : IGameEvent;

public readonly record struct SpellChargeCanceledEvent(
    ContentId SpellId,
    EntityId CasterId,
    ContentId SourceItemId,
    float ChargeElapsedSeconds,
    string Reason) : IGameEvent;

public readonly record struct ProjectileSpawnedEvent(ContentId SpellId, EntityId ProjectileId) : IGameEvent;

public readonly record struct BeamFiredEvent(ContentId SpellId, EntityId CasterId, Vector3 Start, Vector3 End, float Radius) : IGameEvent;

public readonly record struct AoESpawnedEvent(ContentId SpellId, EntityId AoEId, Vector3 Position, float Radius) : IGameEvent;

public readonly record struct StatusAppliedEvent(EntityId TargetId, string StatusKind, float Value, float DurationSeconds, ContentId SourceId) : IGameEvent;

public readonly record struct CorpseAvailableEvent(EntityId EnemyId, ContentId EnemyArchetypeId, Vector3 Position) : IGameEvent;

public readonly record struct SummonSpawnedEvent(ContentId SpellId, EntityId SummonId, Vector3 Position) : IGameEvent;

public readonly record struct DamageAppliedEvent(
    EntityId TargetId,
    float Amount,
    ContentId SourceId,
    IReadOnlyList<TagId>? SourceTags = null,
    EntityId SourceEntityId = default,
    Vector3 HitPosition = default,
    float DamageMultiplier = 1f,
    int SpellChainGeneration = 0) : IGameEvent;

public readonly record struct DamagePreventionGrantedEvent(
    ContentId SourceId,
    EntityId TargetId,
    int RemainingHits,
    TagId RequiredIncomingTag) : IGameEvent;

public readonly record struct DamagePreventedEvent(
    EntityId TargetId,
    float Amount,
    ContentId PreventionSourceId,
    ContentId IncomingSourceId,
    int RemainingHits,
    Vector3 HitPosition) : IGameEvent;

public readonly record struct MageDiedEvent(EntityId MageId, ContentId SourceId) : IGameEvent;

public readonly record struct EnemyDiedEvent(EntityId EnemyId, ContentId EnemyArchetypeId, ContentId SourceId) : IGameEvent;

public readonly record struct DefensePlacedEvent(ContentId DefenseId, EntityId DefenseEntityId, Vector3 Position) : IGameEvent;

public readonly record struct DefenseDestroyedEvent(EntityId DefenseEntityId, ContentId DefenseId, ContentId SourceId) : IGameEvent;

public readonly record struct WaveSpawnedEnemyEvent(ContentId WaveId, ContentId EnemyArchetypeId, EntityId EnemyId) : IGameEvent;

public readonly record struct EnemyAbilityTelegraphedEvent(EntityId EnemyId, ContentId EnemyArchetypeId, string AbilityType, EntityId TargetId) : IGameEvent;

public readonly record struct EnemyAbilityExecutedEvent(EntityId EnemyId, ContentId EnemyArchetypeId, string AbilityType, EntityId TargetId) : IGameEvent;

public readonly record struct FeedbackRequestedEvent(string Kind, ContentId SourceId, Vector3 Position) : IGameEvent;

public readonly record struct ItemAcquiredEvent(ContentId ItemId) : IGameEvent;

public readonly record struct ActivatableItemEquippedEvent(ContentId ItemId, ContentId ReplacedItemId, int RemainingActivations, bool HasUnlimitedActivations) : IGameEvent;

public readonly record struct ActivatableItemUsedEvent(ContentId ItemId, int RemainingActivations, bool Depleted) : IGameEvent;

public readonly record struct ActivatableItemClearedEvent(ContentId ItemId) : IGameEvent;

public readonly record struct PlayerTeleportRequestedEvent(ContentId SourceId, Vector3 TargetPosition) : IGameEvent;

public readonly record struct ItemProcTriggeredEvent(ContentId ItemId, string Trigger, string EffectType, EntityId TargetId) : IGameEvent;

public readonly record struct ItemProcBlockedEvent(ContentId ItemId, string Trigger, string EffectType, string Reason) : IGameEvent;

public readonly record struct MarketOffersGeneratedEvent(int OfferCount) : IGameEvent;

public readonly record struct AchievementCompletedEvent(ContentId AchievementId) : IGameEvent;
