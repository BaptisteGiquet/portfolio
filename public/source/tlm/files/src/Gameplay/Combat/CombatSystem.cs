using Godot;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Combat;

public sealed class CombatSystem : IGameSystem
{
    private readonly record struct TargetMetadata(CombatTargetKind Kind, ContentId ArchetypeId);

    private readonly Dictionary<EntityId, HealthState> _health = new();
    private readonly Dictionary<EntityId, TargetMetadata> _metadata = new();
    private readonly DeathSystem _deathSystem = new();
    private RunContext? _context;
    private SubscriptionToken _itemAcquiredToken;

    public IReadOnlyDictionary<EntityId, HealthState> HealthStates => _health;

    public void Initialize(RunContext context)
    {
        _context = context;
        _itemAcquiredToken = context.Events.Subscribe<ItemAcquiredEvent>(_ => RefreshPlayerHealthFromStats());
    }

    public void Tick(float delta)
    {
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_itemAcquiredToken);
        }

        _health.Clear();
        _metadata.Clear();
        _context = null;
    }

    public void RegisterTarget(EntityId entityId, CombatTargetKind kind, float maxHealth, ContentId archetypeId)
    {
        _health[entityId] = new HealthState(maxHealth);
        _metadata[entityId] = new TargetMetadata(kind, archetypeId);
    }

    public void UnregisterTarget(EntityId entityId)
    {
        _health.Remove(entityId);
        _metadata.Remove(entityId);
    }

    public bool IsAlive(EntityId entityId)
    {
        return _health.TryGetValue(entityId, out var health) && !health.IsDead;
    }

    public bool TryGetHealth(EntityId entityId, out HealthState? health)
    {
        return _health.TryGetValue(entityId, out health);
    }

    public float Repair(EntityId entityId, float amount)
    {
        return _health.TryGetValue(entityId, out var health) ? health.Repair(amount) : 0f;
    }

    public void RefreshPlayerHealthFromStats()
    {
        if (_context == null || !_health.TryGetValue(_context.State.Player.EntityId, out var health))
        {
            return;
        }

        var effectiveMaxHealth = PlayerAttributeResolver.ResolveValue(
            _context,
            PlayerAttributeResolver.Health,
            _context.State.Player.BaseMaxHealth,
            Array.Empty<TagId>(),
            false);
        health.SetMaxHealth(effectiveMaxHealth, healByIncrease: true);
    }

    public DamageResult ApplyDamage(in DamageRequest request)
    {
        if (_context == null || !_health.TryGetValue(request.TargetEntityId, out var health))
        {
            return new DamageResult(request.TargetEntityId, 0f, 0f, false);
        }

        var sourceTags = BuildDamageTags(request);
        var resolvedAmount = (request.ForcePlayerOwned || IsPlayerOwnedDamage(request))
            ? PlayerAttributeResolver.ResolveDamage(_context, request.Amount, sourceTags)
            : request.Amount;
        var finalAmount = resolvedAmount * MathF.Max(0f, request.DamageMultiplier);
        if (_metadata.TryGetValue(request.TargetEntityId, out var stasisTargetMetadata)
            && stasisTargetMetadata.Kind == CombatTargetKind.Mage
            && _context.State.IsPlayerStasisActive
            && finalAmount > 0f)
        {
            _context.State.DebugMetrics.DamagePreventionsThisFrame++;
            _context.State.DebugMetrics.LastDamagePreventionSummary =
                $"{_context.State.PlayerStasisSourceId} stasis prevented {finalAmount:0.##} from {request.SourceContentId}";
            _context.Events.Publish(new DamagePreventedEvent(
                request.TargetEntityId,
                finalAmount,
                _context.State.PlayerStasisSourceId,
                request.SourceContentId,
                0,
                request.Hit.Position));
            return new DamageResult(request.TargetEntityId, 0f, health.CurrentHealth, false);
        }

        if (_metadata.TryGetValue(request.TargetEntityId, out var preventionTargetMetadata)
            && preventionTargetMetadata.Kind == CombatTargetKind.Mage
            && !health.IsDead
            && _context.TryGetSystem<DamagePreventionSystem>(out var preventionSystem)
            && preventionSystem != null
            && preventionSystem.TryPreventDamage(request, finalAmount, sourceTags, out var prevention))
        {
            finalAmount = MathF.Max(0f, finalAmount - prevention.PreventedAmount);
            if (finalAmount <= 0f)
            {
                _context.State.DebugMetrics.LastDamageSummary =
                    $"{prevention.SourceId} prevented {request.SourceContentId} -> {request.TargetEntityId} amount={prevention.PreventedAmount:0.##}";
                return new DamageResult(request.TargetEntityId, 0f, health.CurrentHealth, false);
            }
        }

        var wasAlive = !health.IsDead;
        var applied = health.ApplyDamage(finalAmount);
        var died = wasAlive && health.IsDead;
        _context.State.DebugMetrics.DamageEventsThisFrame++;
        _context.State.DebugMetrics.LastDamageSummary =
            $"{request.SourceContentId} -> {request.TargetEntityId} amount={applied:0.##} hp={health.CurrentHealth:0.##}/{health.MaxHealth:0.##}";
        if (_metadata.TryGetValue(request.TargetEntityId, out var targetMetadata)
            && targetMetadata.Kind == CombatTargetKind.Mage)
        {
            _context.State.DebugMetrics.LastMageDamageSummary =
                $"{request.SourceContentId} {request.DamageType} amount={applied:0.##} hp={health.CurrentHealth:0.##}/{health.MaxHealth:0.##} at {Format(request.Hit.Position)}";
            if (died)
            {
                _context.State.DebugMetrics.DeathCauseSummary =
                    $"mage killed by {request.SourceContentId} ({request.DamageType}) for {applied:0.##} damage at {Format(request.Hit.Position)}";
            }
        }

        _context.Events.Publish(new DamageAppliedEvent(
            request.TargetEntityId,
            applied,
            request.SourceContentId,
            sourceTags,
            request.SourceEntityId,
            request.Hit.Position,
            MathF.Max(0f, request.DamageMultiplier),
            Math.Max(0, request.SpellChainGeneration)));

        if (died && _metadata.TryGetValue(request.TargetEntityId, out var metadata))
        {
            _deathSystem.PublishDeath(_context, request.TargetEntityId, metadata.Kind, metadata.ArchetypeId, request.SourceContentId);
        }

        return new DamageResult(request.TargetEntityId, applied, health.CurrentHealth, died);
    }

    private bool IsPlayerOwnedDamage(in DamageRequest request)
    {
        if (_context == null)
        {
            return false;
        }

        if (request.SourceEntityId.Equals(_context.State.Player.EntityId))
        {
            return true;
        }

        return _metadata.TryGetValue(request.SourceEntityId, out var sourceMetadata)
            && (sourceMetadata.Kind == CombatTargetKind.Summon || sourceMetadata.Kind == CombatTargetKind.Defense);
    }

    private IReadOnlyList<TagId> BuildDamageTags(in DamageRequest request)
    {
        var tags = new List<TagId>(10)
        {
            TagId.From("combat.damage"),
            TagId.From($"combat.damage.{request.DamageType.ToString().ToLowerInvariant()}")
        };

        if (_context?.Content.TryGetSpell(request.SourceContentId, out var spell) == true && spell != null)
        {
            tags.AddRange(spell.Tags);
        }
        else if (_context?.Content.TryGetDefense(request.SourceContentId, out var defense) == true && defense != null)
        {
            tags.Add(TagId.From("combat.source.defense"));
            tags.AddRange(defense.Tags);
        }

        if (_metadata.TryGetValue(request.SourceEntityId, out var sourceMetadata))
        {
            if (sourceMetadata.Kind == CombatTargetKind.Summon)
            {
                tags.Add(TagId.From("combat.source.summon"));
            }
            else if (sourceMetadata.Kind == CombatTargetKind.Defense)
            {
                tags.Add(TagId.From("combat.source.defense"));
            }
        }

        return GameplayTagSet.From(tags);
    }

    private static string Format(Vector3 value)
    {
        return $"({value.X:0.##},{value.Y:0.##},{value.Z:0.##})";
    }
}
