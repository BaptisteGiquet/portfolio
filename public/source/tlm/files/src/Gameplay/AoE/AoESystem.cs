using Godot;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.StatusEffects;

namespace TheLastMage.Gameplay.AoE;

public sealed class AoESystem : IGameSystem
{
    private readonly List<AoEState> _areas = new();
    private readonly List<EnemyState> _candidates = new(64);
    private readonly Dictionary<EntityId, AoEDebugNode> _debugNodes = new();
    private Node3D? _debugRoot;
    private RunContext? _context;

    public int ActiveCount => _areas.Count;

    public IReadOnlyList<AoEState> ActiveAreas => _areas;

    public AoESystem(Node3D? debugRoot)
    {
        _debugRoot = debugRoot;
    }

    public void Initialize(RunContext context)
    {
        _context = context;
    }

    public void Tick(float delta)
    {
        if (_context != null)
        {
            _context.State.DebugMetrics.ActiveAoEs = _areas.Count;
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
        var status = _context.TryGetSystem<StatusEffectSystem>(out var statusSystem) ? statusSystem : null;

        for (var i = _areas.Count - 1; i >= 0; i--)
        {
            var area = _areas[i];
            if (area.IsExpandingRing)
            {
                area.Radius = CurrentExpandingRingRadius(area);
            }

            if (area.Orbit.Enabled)
            {
                var orbit = area.Orbit;
                area.Position = orbit.Advance(_context.State.Player.Position, delta, out var orbitVelocity);
                area.Orbit = orbit;
                var planarVelocity = new Vector3(orbitVelocity.X, 0f, orbitVelocity.Z);
                if (planarVelocity.LengthSquared() > 0.0001f)
                {
                    area.Direction = planarVelocity.Normalized();
                    if (area.IsWall)
                    {
                        area.WallRight = area.Direction.Cross(Vector3.Up).Normalized();
                    }
                }
            }
            else if (area.MoveSpeed > 0f)
            {
                UpdateHoming(area, enemySystem.SpatialRegistry, delta);
                area.Position += area.Direction * area.MoveSpeed * delta;
            }

            area.LifetimeRemaining -= delta;
            area.TickRemainingSeconds -= delta;
            if (_debugNodes.TryGetValue(area.EntityId, out var debugNode))
            {
                debugNode.GlobalPosition = area.Position;
                if (area.IsExpandingRing)
                {
                    debugNode.UpdateExpandingRing(area.Radius, area.RingThickness);
                }
            }

            if (area.PullStrength > 0f)
            {
                enemySystem.SpatialRegistry.CopyInRadius(area.Position, GetQueryRadius(area), _candidates);
                for (var candidateIndex = 0; candidateIndex < _candidates.Count; candidateIndex++)
                {
                    var enemy = _candidates[candidateIndex];
                    if (!combat.IsAlive(enemy.EntityId) || !Contains(area, enemy.Position))
                    {
                        continue;
                    }

                    var toCenter = area.Position - enemy.Position;
                    toCenter.Y = 0f;
                    var distance = toCenter.Length();
                    if (distance > 0.05f)
                    {
                        enemy.Position += toCenter / distance * area.PullStrength * delta;
                    }
                }
            }

            if (area.TickRemainingSeconds <= 0f)
            {
                area.TickRemainingSeconds = area.TickIntervalSeconds;
                enemySystem.SpatialRegistry.CopyInRadius(area.Position, GetQueryRadius(area), _candidates);
                for (var candidateIndex = 0; candidateIndex < _candidates.Count; candidateIndex++)
                {
                    var enemy = _candidates[candidateIndex];
                    if (!combat.IsAlive(enemy.EntityId) || !Contains(area, enemy.Position))
                    {
                        continue;
                    }

                    if (area.Damage > 0f)
                    {
                        if (area.IsExpandingRing && !area.HitTargets.Add(enemy.EntityId))
                        {
                            continue;
                        }

                        combat.ApplyDamage(new DamageRequest(
                            area.OwnerId,
                            enemy.EntityId,
                            area.SpellId,
                            area.Damage,
                            area.DamageType,
                            new HitContext(enemy.Position, Vector3.Up, area.OwnerId),
                            DamageMultiplier: area.DamageMultiplier,
                            SpellChainGeneration: area.SpellChainGeneration));
                    }

                    if (!string.IsNullOrWhiteSpace(area.StatusKind))
                    {
                        status?.Apply(
                            enemy.EntityId,
                            area.StatusKind,
                            area.SpellId,
                            area.OwnerId,
                            damageMultiplier: area.DamageMultiplier,
                            spellChainGeneration: area.SpellChainGeneration);
                    }
                }
            }

            if (area.LifetimeRemaining <= 0f)
            {
                RemoveAt(i);
            }
        }
    }

    public void Shutdown()
    {
        foreach (var node in _debugNodes.Values)
        {
            node.QueueFree();
        }

        _debugNodes.Clear();
        _areas.Clear();
        _context = null;
        _debugRoot = null;
    }

    public EntityId Spawn(
        ContentId spellId,
        EntityId ownerId,
        Vector3 position,
        Vector3 direction,
        float radius,
        float durationSeconds,
        float tickIntervalSeconds,
        float damage,
        DamageType damageType,
        string statusKind = "",
        float pullStrength = 0f,
        float moveSpeed = 0f,
        float presentationScale = 1f,
        float damageMultiplier = 1f,
        int spellChainGeneration = 0,
        float homingStrength = 0f,
        float homingAcquireRadius = 0f,
        SpellOrbitState orbit = default)
    {
        if (_context == null)
        {
            return EntityId.None;
        }

        var id = _context.State.AllocateEntityId();
        var initialPosition = orbit.Enabled ? orbit.PositionAt(_context.State.Player.Position) : position;
        var area = new AoEState
        {
            EntityId = id,
            SpellId = spellId,
            OwnerId = ownerId,
            Position = initialPosition,
            Direction = direction.Normalized(),
            Radius = MathF.Max(0.1f, radius),
            LifetimeRemaining = MathF.Max(0.05f, durationSeconds),
            TickIntervalSeconds = MathF.Max(0.05f, tickIntervalSeconds),
            TickRemainingSeconds = 0f,
            Damage = MathF.Max(0f, damage),
            DamageMultiplier = MathF.Max(0f, damageMultiplier),
            SpellChainGeneration = Math.Max(0, spellChainGeneration),
            DamageType = damageType,
            StatusKind = statusKind,
            PullStrength = MathF.Max(0f, pullStrength),
            MoveSpeed = MathF.Max(0f, moveSpeed),
            HomingStrength = MathF.Max(0f, homingStrength),
            HomingAcquireRadius = homingAcquireRadius > 0f ? homingAcquireRadius : 14f + MathF.Max(0f, homingStrength) * 6f,
            HomingTurnRateRadians = 2.6f,
            Orbit = orbit
        };
        _areas.Add(area);

        if (_debugRoot != null)
        {
            var node = new AoEDebugNode { Name = $"AoE_{id.Value}_{spellId}" };
            _debugRoot.AddChild(node);
            node.GlobalPosition = initialPosition;
            node.Bind(id, area.Radius, ColorFor(damageType, pullStrength), presentationScale);
            _debugNodes[id] = node;
        }

        _context.Events.Publish(new AoESpawnedEvent(spellId, id, initialPosition, area.Radius));
        return id;
    }

    public EntityId SpawnExpandingRing(
        ContentId spellId,
        EntityId ownerId,
        Vector3 position,
        float startRadius,
        float endRadius,
        float durationSeconds,
        float tickIntervalSeconds,
        float ringThickness,
        float damage,
        DamageType damageType,
        float damageMultiplier = 1f,
        int spellChainGeneration = 0)
    {
        if (_context == null)
        {
            return EntityId.None;
        }

        var clampedStartRadius = MathF.Max(0.05f, startRadius);
        var clampedEndRadius = MathF.Max(clampedStartRadius, endRadius);
        var clampedDuration = MathF.Max(0.05f, durationSeconds);
        var id = _context.State.AllocateEntityId();
        var area = new AoEState
        {
            EntityId = id,
            SpellId = spellId,
            OwnerId = ownerId,
            Position = position,
            Direction = Vector3.Zero,
            Radius = clampedStartRadius,
            IsExpandingRing = true,
            StartRadius = clampedStartRadius,
            EndRadius = clampedEndRadius,
            RingThickness = MathF.Max(0.1f, ringThickness),
            LifetimeSeconds = clampedDuration,
            LifetimeRemaining = clampedDuration,
            TickIntervalSeconds = MathF.Max(0.02f, tickIntervalSeconds),
            TickRemainingSeconds = 0f,
            Damage = MathF.Max(0f, damage),
            DamageMultiplier = MathF.Max(0f, damageMultiplier),
            SpellChainGeneration = Math.Max(0, spellChainGeneration),
            DamageType = damageType
        };
        _areas.Add(area);

        if (_debugRoot != null)
        {
            var node = new AoEDebugNode { Name = $"AbyssalRing_{id.Value}_{spellId}" };
            _debugRoot.AddChild(node);
            node.GlobalPosition = position;
            node.BindExpandingRing(id, area.Radius, area.RingThickness, new Color(0.45f, 0.18f, 0.75f, 0.28f));
            _debugNodes[id] = node;
        }

        _context.Events.Publish(new AoESpawnedEvent(spellId, id, position, area.Radius));
        return id;
    }

    public EntityId SpawnWall(
        ContentId spellId,
        EntityId ownerId,
        Vector3 position,
        Vector3 wallRight,
        float halfLength,
        float halfWidth,
        float durationSeconds,
        float tickIntervalSeconds,
        float damage,
        DamageType damageType,
        string statusKind = "",
        float presentationScale = 1f,
        float damageMultiplier = 1f,
        int spellChainGeneration = 0,
        SpellOrbitState orbit = default)
    {
        if (_context == null)
        {
            return EntityId.None;
        }

        var id = _context.State.AllocateEntityId();
        var initialPosition = orbit.Enabled ? orbit.PositionAt(_context.State.Player.Position) : position;
        var normalizedRight = wallRight.Normalized();
        var area = new AoEState
        {
            EntityId = id,
            SpellId = spellId,
            OwnerId = ownerId,
            Position = initialPosition,
            Direction = Vector3.Zero,
            Radius = MathF.Max(0.1f, halfLength),
            IsWall = true,
            WallHalfLength = MathF.Max(0.1f, halfLength),
            WallHalfWidth = MathF.Max(0.1f, halfWidth),
            WallRight = normalizedRight,
            LifetimeRemaining = MathF.Max(0.05f, durationSeconds),
            TickIntervalSeconds = MathF.Max(0.05f, tickIntervalSeconds),
            TickRemainingSeconds = 0f,
            Damage = MathF.Max(0f, damage),
            DamageMultiplier = MathF.Max(0f, damageMultiplier),
            SpellChainGeneration = Math.Max(0, spellChainGeneration),
            DamageType = damageType,
            StatusKind = statusKind,
            Orbit = orbit
        };
        _areas.Add(area);

        if (_debugRoot != null)
        {
            var node = new AoEDebugNode { Name = $"AoEWall_{id.Value}_{spellId}" };
            _debugRoot.AddChild(node);
            node.GlobalPosition = initialPosition;
            node.BindWall(id, area.WallHalfLength, area.WallHalfWidth, normalizedRight, ColorFor(damageType, 0f), presentationScale);
            _debugNodes[id] = node;
        }

        _context.Events.Publish(new AoESpawnedEvent(spellId, id, initialPosition, area.WallHalfLength));
        return id;
    }

    private void RemoveAt(int index)
    {
        var area = _areas[index];
        if (_debugNodes.Remove(area.EntityId, out var node))
        {
            node.QueueFree();
        }

        _areas.RemoveAt(index);
    }

    private static Color ColorFor(DamageType damageType, float pullStrength)
    {
        if (pullStrength > 0f)
        {
            return new Color(0.65f, 0.85f, 1f, 0.22f);
        }

        return damageType switch
        {
            DamageType.Fire => new Color(1f, 0.22f, 0.05f, 0.2f),
            DamageType.Frost => new Color(0.45f, 0.8f, 1f, 0.2f),
            DamageType.Arcane => new Color(0.75f, 0.45f, 1f, 0.2f),
            _ => new Color(0.8f, 0.8f, 0.8f, 0.16f)
        };
    }

    private static bool Contains(AoEState area, Vector3 position)
    {
        if (!area.IsWall)
        {
            if (area.IsExpandingRing)
            {
                var ringOffset = position - area.Position;
                ringOffset.Y = 0f;
                var distance = ringOffset.Length();
                var halfThickness = MathF.Max(0.05f, area.RingThickness * 0.5f);
                return distance >= area.Radius - halfThickness && distance <= area.Radius + halfThickness;
            }

            return area.Position.DistanceSquaredTo(position) <= area.Radius * area.Radius;
        }

        var offset = position - area.Position;
        offset.Y = 0f;
        var along = MathF.Abs(offset.Dot(area.WallRight));
        var forward = area.WallRight.Cross(Vector3.Up).Normalized();
        var across = MathF.Abs(offset.Dot(forward));
        return along <= area.WallHalfLength && across <= area.WallHalfWidth;
    }

    private static float GetQueryRadius(AoEState area)
    {
        if (!area.IsWall)
        {
            return area.IsExpandingRing
                ? area.Radius + MathF.Max(0.05f, area.RingThickness * 0.5f)
                : area.Radius;
        }

        return MathF.Sqrt(area.WallHalfLength * area.WallHalfLength + area.WallHalfWidth * area.WallHalfWidth);
    }

    private static float CurrentExpandingRingRadius(AoEState area)
    {
        if (area.LifetimeSeconds <= 0f)
        {
            return area.EndRadius;
        }

        var elapsed = Math.Clamp(area.LifetimeSeconds - area.LifetimeRemaining, 0f, area.LifetimeSeconds);
        var t = elapsed / area.LifetimeSeconds;
        return Mathf.Lerp(area.StartRadius, area.EndRadius, t);
    }

    private static void UpdateHoming(AoEState area, EnemySpatialRegistry registry, float delta)
    {
        if (area.HomingStrength <= 0f)
        {
            return;
        }

        if (!registry.TryFindNearest(area.Position, area.HomingAcquireRadius, null, out _, out var targetPosition))
        {
            return;
        }

        var desired = targetPosition - area.Position;
        desired.Y = 0f;
        if (desired.LengthSquared() <= 0.0001f)
        {
            return;
        }

        var current = area.Direction;
        current.Y = 0f;
        if (current.LengthSquared() <= 0.0001f)
        {
            current = desired.Normalized();
        }

        var turn = Math.Clamp(area.HomingTurnRateRadians * area.HomingStrength * delta, 0f, 1f);
        var direction = current.Normalized().Lerp(desired.Normalized(), turn);
        if (direction.LengthSquared() > 0.0001f)
        {
            area.Direction = direction.Normalized();
        }
    }
}
