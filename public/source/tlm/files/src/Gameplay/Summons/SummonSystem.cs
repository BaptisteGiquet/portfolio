using Godot;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Summons;

public sealed class SummonSystem : IGameSystem
{
    private readonly record struct CorpseBudget(ContentId ArchetypeId, Vector3 Position);

    private const int MaxCorpseBudget = 32;

    private readonly List<CorpseBudget> _corpses = new(MaxCorpseBudget);
    private readonly List<SummonState> _summons = new();
    private readonly List<EnemyState> _candidates = new(32);
    private readonly Dictionary<EntityId, SummonActorNode> _actors = new();
    private Node3D? _actorRoot;
    private RunContext? _context;
    private SubscriptionToken _corpseToken;
    private SubscriptionToken _damageToken;

    public int ActiveCount => _summons.Count;

    public IReadOnlyList<SummonState> ActiveSummons => _summons;

    public SummonSystem(Node3D? actorRoot)
    {
        _actorRoot = actorRoot;
    }

    public void Initialize(RunContext context)
    {
        _context = context;
        _corpseToken = context.Events.Subscribe<CorpseAvailableEvent>(OnCorpseAvailable);
        _damageToken = context.Events.Subscribe<DamageAppliedEvent>(OnDamageApplied);
    }

    public void Tick(float delta)
    {
        if (_context != null)
        {
            _context.State.DebugMetrics.ActiveSummons = _summons.Count;
        }
    }

    public void FixedTick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        var combat = _context.GetSystem<CombatSystem>();
        var enemySystem = _context.GetSystem<EnemySystem>();
        for (var i = _summons.Count - 1; i >= 0; i--)
        {
            var summon = _summons[i];
            if (!float.IsPositiveInfinity(summon.LifetimeRemaining))
            {
                summon.LifetimeRemaining -= delta;
            }

            summon.AttackCooldownRemaining = MathF.Max(0f, summon.AttackCooldownRemaining - delta);
            if (summon.LifetimeRemaining <= 0f || !combat.IsAlive(summon.EntityId))
            {
                RemoveAt(i);
                continue;
            }

            enemySystem.SpatialRegistry.CopyInRadius(summon.Position, summon.AggroRange, _candidates);
            var target = FindClosestLivingEnemy(summon.Position, combat);
            if (target == null)
            {
                UpdateActorPosition(summon);
                continue;
            }

            summon.CurrentTargetPosition = target.Position;
            var toTarget = target.Position - summon.Position;
            toTarget.Y = 0f;
            var distance = toTarget.Length();
            if (distance > summon.AttackRange)
            {
                summon.Position += toTarget / MathF.Max(0.001f, distance) * summon.MoveSpeed * delta;
                UpdateActorPosition(summon);
                continue;
            }

            UpdateActorPosition(summon);
            if (summon.AttackCooldownRemaining > 0f)
            {
                continue;
            }

            combat.ApplyDamage(new DamageRequest(
                summon.EntityId,
                target.EntityId,
                summon.SpellId,
                summon.Damage,
                DamageType.Physical,
                new HitContext(target.Position, Vector3.Up, summon.EntityId),
                ForcePlayerOwned: true,
                DamageMultiplier: summon.DamageMultiplier,
                SpellChainGeneration: summon.SpellChainGeneration));
            summon.AttackCooldownRemaining = 0.8f;
        }
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_corpseToken);
            _context.Events.Unsubscribe(_damageToken);
        }

        foreach (var actor in _actors.Values)
        {
            actor.QueueFree();
        }

        _actors.Clear();
        _summons.Clear();
        _corpses.Clear();
        _context = null;
        _actorRoot = null;
    }

    public EntityId TryRaiseFromCorpse(
        ContentId spellId,
        EntityId ownerId,
        Vector3 desiredPosition,
        float searchRadius,
        float lifetimeSeconds,
        float damage,
        float aggroRange,
        float presentationScale = 1f,
        float damageMultiplier = 1f,
        int spellChainGeneration = 0)
    {
        if (_context == null || _corpses.Count == 0)
        {
            return EntityId.None;
        }

        var bestIndex = -1;
        var bestDistanceSquared = MathF.Max(1f, searchRadius) * MathF.Max(1f, searchRadius);
        for (var i = 0; i < _corpses.Count; i++)
        {
            var distanceSquared = _corpses[i].Position.DistanceSquaredTo(desiredPosition);
            if (distanceSquared <= bestDistanceSquared)
            {
                bestIndex = i;
                bestDistanceSquared = distanceSquared;
            }
        }

        if (bestIndex < 0)
        {
            return EntityId.None;
        }

        var corpse = _corpses[bestIndex];
        _corpses.RemoveAt(bestIndex);
        return Spawn(spellId, ownerId, corpse.Position, lifetimeSeconds, damage, aggroRange, presentationScale, damageMultiplier, spellChainGeneration);
    }

    private EntityId Spawn(
        ContentId spellId,
        EntityId ownerId,
        Vector3 position,
        float lifetimeSeconds,
        float damage,
        float aggroRange,
        float presentationScale,
        float damageMultiplier,
        int spellChainGeneration)
    {
        if (_context == null)
        {
            return EntityId.None;
        }

        var id = _context.State.AllocateEntityId();
        var summon = new SummonState
        {
            EntityId = id,
            SpellId = spellId,
            OwnerId = ownerId,
            Position = position,
            LifetimeRemaining = MathF.Max(1f, lifetimeSeconds),
            Damage = MathF.Max(1f, damage),
            DamageMultiplier = MathF.Max(0f, damageMultiplier),
            SpellChainGeneration = Math.Max(0, spellChainGeneration),
            AttackRange = 2.4f,
            MoveSpeed = 3.4f,
            AggroRange = MathF.Max(1f, aggroRange),
            CurrentTargetPosition = position
        };
        _summons.Add(summon);
        _context.GetSystem<CombatSystem>().RegisterTarget(id, CombatTargetKind.Summon, 35f, spellId);

        if (_actorRoot != null)
        {
            var actor = new SummonActorNode { Name = $"Summon_{id.Value}_{spellId}" };
            _actorRoot.AddChild(actor);
            actor.GlobalPosition = position;
            actor.Bind(id, presentationScale);
            _actors[id] = actor;
        }

        _context.Events.Publish(new SummonSpawnedEvent(spellId, id, position));
        return id;
    }

    private EnemyState? FindClosestLivingEnemy(Vector3 summonPosition, CombatSystem combat)
    {
        EnemyState? closest = null;
        var closestDistanceSquared = float.MaxValue;
        for (var i = 0; i < _candidates.Count; i++)
        {
            var candidate = _candidates[i];
            if (!combat.IsAlive(candidate.EntityId))
            {
                continue;
            }

            var distanceSquared = candidate.Position.DistanceSquaredTo(summonPosition);
            if (distanceSquared < closestDistanceSquared)
            {
                closestDistanceSquared = distanceSquared;
                closest = candidate;
            }
        }

        return closest;
    }

    private void UpdateActorPosition(SummonState summon)
    {
        if (_actors.TryGetValue(summon.EntityId, out var actor))
        {
            actor.GlobalPosition = summon.Position;
        }
    }

    private void RemoveAt(int index)
    {
        var summon = _summons[index];
        _context?.GetSystem<CombatSystem>().UnregisterTarget(summon.EntityId);
        if (_actors.Remove(summon.EntityId, out var actor))
        {
            actor.MarkDead();
        }

        _summons.RemoveAt(index);
    }

    private void OnCorpseAvailable(CorpseAvailableEvent gameEvent)
    {
        if (_corpses.Count >= MaxCorpseBudget)
        {
            _corpses.RemoveAt(0);
        }

        _corpses.Add(new CorpseBudget(gameEvent.EnemyArchetypeId, gameEvent.Position));
    }

    private void OnDamageApplied(DamageAppliedEvent gameEvent)
    {
        if (_actors.TryGetValue(gameEvent.TargetId, out var actor))
        {
            actor.FlashDamage();
        }
    }

    public bool TryGetEnemyPriorityTarget(
        Vector3 enemyPosition,
        Vector3 playerPosition,
        CombatSystem combat,
        out EntityId targetId,
        out Vector3 targetPosition)
    {
        targetId = EntityId.None;
        targetPosition = playerPosition;

        var playerDistance = enemyPosition.DistanceTo(playerPosition);
        var bestDistance = float.MaxValue;
        SummonState? bestSummon = null;
        for (var i = 0; i < _summons.Count; i++)
        {
            var summon = _summons[i];
            if (!combat.IsAlive(summon.EntityId))
            {
                continue;
            }

            var distance = enemyPosition.DistanceTo(summon.Position);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestSummon = summon;
            }
        }

        if (bestSummon == null)
        {
            return false;
        }

        if (bestDistance > 7.5f && bestDistance > playerDistance * 1.25f)
        {
            return false;
        }

        targetId = bestSummon.EntityId;
        targetPosition = bestSummon.Position;
        return true;
    }

    public bool TryGetSummonPosition(EntityId entityId, out Vector3 position)
    {
        for (var i = 0; i < _summons.Count; i++)
        {
            var summon = _summons[i];
            if (summon.EntityId.Equals(entityId))
            {
                position = summon.Position;
                return true;
            }
        }

        position = Vector3.Zero;
        return false;
    }
}
