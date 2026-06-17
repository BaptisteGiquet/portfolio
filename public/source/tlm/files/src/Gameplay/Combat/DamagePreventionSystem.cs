using Godot;
using TheLastMage.Data.Definitions;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Combat;

public sealed class DamagePreventionSystem : IGameSystem
{
    private readonly List<DamagePreventionState> _protections = new();
    private RunContext? _context;
    private SubscriptionToken _itemAcquiredToken;

    public IReadOnlyList<DamagePreventionState> ActiveProtections => _protections;

    public void Initialize(RunContext context)
    {
        _context = context;
        _itemAcquiredToken = context.Events.Subscribe<ItemAcquiredEvent>(OnItemAcquired);
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

        _protections.Clear();
        _context = null;
    }

    public void GrantFullHitProtection(EntityId protectedEntityId, ContentId sourceId, int hitCount, TagId requiredIncomingTag = default)
    {
        if (_context == null || !protectedEntityId.IsValid || hitCount <= 0)
        {
            return;
        }

        var existing = _protections.FirstOrDefault(protection =>
            protection.ProtectedEntityId.Equals(protectedEntityId)
            && protection.SourceId.Equals(sourceId)
            && protection.Mode == DamagePreventionMode.PreventFullHit
            && protection.RequiredIncomingTag.Equals(requiredIncomingTag));
        if (existing != null)
        {
            existing.AddHits(hitCount);
            PublishGranted(existing);
            return;
        }

        var state = new DamagePreventionState(
            sourceId,
            protectedEntityId,
            DamagePreventionMode.PreventFullHit,
            hitCount,
            float.PositiveInfinity,
            requiredIncomingTag);
        _protections.Add(state);
        PublishGranted(state);
    }

    public bool TryPreventDamage(
        in DamageRequest request,
        float incomingAmount,
        IReadOnlyList<TagId> incomingTags,
        out DamagePreventionResult result)
    {
        result = default;
        if (_context == null || incomingAmount <= 0f)
        {
            return false;
        }

        for (var i = 0; i < _protections.Count; i++)
        {
            var protection = _protections[i];
            if (!ProtectionMatches(protection, request.TargetEntityId, incomingTags))
            {
                continue;
            }

            var prevented = protection.Consume(incomingAmount);
            if (protection.IsDepleted)
            {
                _protections.RemoveAt(i);
            }

            if (prevented <= 0f)
            {
                return false;
            }

            result = new DamagePreventionResult(
                protection.SourceId,
                protection.ProtectedEntityId,
                prevented,
                protection.RemainingHits);
            _context.State.DebugMetrics.DamagePreventionsThisFrame++;
            _context.State.DebugMetrics.LastDamagePreventionSummary =
                $"{protection.SourceId} prevented {prevented:0.##} from {request.SourceContentId}; remaining={protection.RemainingHits}";
            _context.Events.Publish(new DamagePreventedEvent(
                request.TargetEntityId,
                prevented,
                protection.SourceId,
                request.SourceContentId,
                protection.RemainingHits,
                request.Hit.Position));
            return true;
        }

        return false;
    }

    private void OnItemAcquired(ItemAcquiredEvent gameEvent)
    {
        if (_context == null || !_context.Content.Items.TryGetValue(gameEvent.ItemId, out var item))
        {
            return;
        }

        GrantFromItem(item, _context.State.Player.EntityId);
    }

    private void GrantFromItem(ItemRuntimeDefinition item, EntityId protectedEntityId)
    {
        foreach (var effect in item.Effects)
        {
            if (!string.Equals(effect.Kind, nameof(ItemEffectKind.PreventNextPlayerHealthLoss), StringComparison.Ordinal))
            {
                continue;
            }

            var hitCount = Math.Clamp((int)MathF.Floor(MathF.Max(1f, effect.Value) + 0.001f), 1, 99);
            GrantFullHitProtection(protectedEntityId, item.Id, hitCount, effect.TargetTag);
        }
    }

    private void PublishGranted(DamagePreventionState state)
    {
        if (_context == null)
        {
            return;
        }

        _context.State.DebugMetrics.LastDamagePreventionSummary =
            $"{state.SourceId} granted {state.RemainingHits} protected hit(s)";
        _context.Events.Publish(new DamagePreventionGrantedEvent(
            state.SourceId,
            state.ProtectedEntityId,
            state.RemainingHits,
            state.RequiredIncomingTag));
    }

    private static bool ProtectionMatches(
        DamagePreventionState protection,
        EntityId targetEntityId,
        IReadOnlyList<TagId> incomingTags)
    {
        return protection.ProtectedEntityId.Equals(targetEntityId)
            && !protection.IsDepleted
            && GameplayTagPath.MatchesAny(protection.RequiredIncomingTag, incomingTags);
    }
}
