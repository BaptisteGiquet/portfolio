using Godot;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.StatusEffects;
using TheLastMage.Gameplay.Summons;

namespace TheLastMage.Gameplay.Projectiles;

public sealed class ProjectileSystem : IGameSystem
{
    private readonly List<ProjectileState> _projectiles = new();
    private readonly Dictionary<EntityId, ProjectileViewNode> _views = new();
    private RunContext? _context;
    private Node3D? _viewRoot;

    public int ActiveCount => _projectiles.Count;

    public IReadOnlyList<ProjectileState> ActiveProjectiles => _projectiles;

    public ProjectileSystem(Node3D? viewRoot)
    {
        _viewRoot = viewRoot;
    }

    public void Initialize(RunContext context)
    {
        _context = context;
    }

    public void Tick(float delta)
    {
        if (_context != null)
        {
            _context.State.DebugMetrics.ActiveProjectiles = _projectiles.Count;
        }
    }

    public void FixedTick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        var combat = _context.GetSystem<CombatSystem>();
        var enemyRegistry = _context.GetSystem<EnemySystem>().SpatialRegistry;
        for (var i = _projectiles.Count - 1; i >= 0; i--)
        {
            var projectile = _projectiles[i];
            projectile.PreviousPosition = projectile.Position;
            if (projectile.Orbit.Enabled)
            {
                var orbit = projectile.Orbit;
                projectile.Position = orbit.Advance(_context.State.Player.Position, delta, out var orbitVelocity);
                projectile.Velocity = orbitVelocity;
                projectile.Orbit = orbit;
            }
            else
            {
                UpdateHoming(ref projectile, enemyRegistry, combat, delta);
                projectile.Position += projectile.Velocity * delta;
            }

            projectile.LifetimeRemaining -= delta;

            if (_views.TryGetValue(projectile.EntityId, out var view))
            {
                view.GlobalPosition = projectile.Position;
            }

            if (projectile.IsDirectTargetProjectile)
            {
                if (!combat.IsAlive(projectile.DirectTargetId))
                {
                    RemoveAt(i);
                    continue;
                }

                var hitPosition = ResolveCurrentDirectTargetPosition(projectile);
                if (SegmentTouchesPoint(projectile.PreviousPosition, projectile.Position, hitPosition, projectile.DirectTargetRadius))
                {
                    combat.ApplyDamage(new DamageRequest(
                        projectile.OwnerId,
                        projectile.DirectTargetId,
                        projectile.SpellId,
                        projectile.Damage,
                        projectile.DamageType,
                        new HitContext(hitPosition, Vector3.Up, projectile.OwnerId),
                        DamageMultiplier: projectile.DamageMultiplier,
                        SpellChainGeneration: projectile.SpellChainGeneration));
                    RemoveAt(i);
                    continue;
                }
            }
            else if (enemyRegistry.TryFindSweepHit(
                         projectile.PreviousPosition,
                         projectile.Position,
                         projectile.Radius,
                         out var enemyId,
                         out var hitPosition,
                         projectile.HitTargets))
            {
                var request = new DamageRequest(
                    projectile.OwnerId,
                    enemyId,
                    projectile.SpellId,
                    projectile.Damage,
                    projectile.DamageType,
                    new HitContext(hitPosition, Vector3.Up, projectile.OwnerId),
                    DamageMultiplier: projectile.DamageMultiplier,
                    SpellChainGeneration: projectile.SpellChainGeneration);
                combat.ApplyDamage(request);
                if (!string.IsNullOrWhiteSpace(projectile.StatusKind)
                    && _context.TryGetSystem<StatusEffectSystem>(out var statusSystem)
                    && statusSystem != null)
                {
                    statusSystem.Apply(
                        enemyId,
                        projectile.StatusKind,
                        projectile.SpellId,
                        projectile.OwnerId,
                        damageMultiplier: projectile.DamageMultiplier,
                        spellChainGeneration: projectile.SpellChainGeneration);
                }

                if (projectile.SplitRemaining > 0)
                {
                    SpawnSplitProjectiles(projectile, hitPosition);
                }

                if (projectile.PiercesEnemies)
                {
                    projectile.HitTargets ??= new List<EntityId>(8);
                    projectile.HitTargets.Add(enemyId);
                    _projectiles[i] = projectile;
                    continue;
                }

                RemoveAt(i);
                continue;
            }

            if (projectile.LifetimeRemaining <= 0f)
            {
                RemoveAt(i);
                continue;
            }

            _projectiles[i] = projectile;
        }

        _context.State.DebugMetrics.ActiveProjectiles = _projectiles.Count;
    }

    public void Shutdown()
    {
        foreach (var view in _views.Values)
        {
            view.QueueFree();
        }

        _views.Clear();
        _projectiles.Clear();
        if (_context != null)
        {
            _context.State.DebugMetrics.ActiveProjectiles = 0;
        }

        _context = null;
        _viewRoot = null;
    }

    public EntityId Spawn(
        ContentId spellId,
        EntityId ownerId,
        Vector3 origin,
        Vector3 direction,
        float damage,
        DamageType damageType,
        float radius,
        float lifetimeSeconds,
        float speed,
        string statusKind = "",
        float presentationScale = 1f,
        bool piercesEnemies = false,
        int splitRemaining = 0,
        float damageMultiplier = 1f,
        int spellChainGeneration = 0,
        float homingStrength = 0f,
        float homingConeRadians = 0f,
        float homingAcquireRadius = 0f,
        SpellOrbitState orbit = default)
    {
        if (_context == null)
        {
            return EntityId.None;
        }

        var id = _context.State.AllocateEntityId();
        var initialPosition = origin;
        var initialVelocity = direction.Normalized() * MathF.Max(0.1f, speed);
        if (orbit.Enabled)
        {
            initialPosition = orbit.PositionAt(_context.State.Player.Position);
            var advanced = orbit;
            _ = advanced.Advance(_context.State.Player.Position, 0.016f, out initialVelocity);
        }

        var projectile = new ProjectileState
        {
            EntityId = id,
            SpellId = spellId,
            OwnerId = ownerId,
            Position = initialPosition,
            PreviousPosition = initialPosition,
            Velocity = initialVelocity.LengthSquared() <= 0.0001f
                ? direction.Normalized() * MathF.Max(0.1f, speed)
                : initialVelocity,
            Radius = radius,
            Damage = damage,
            DamageType = damageType,
            LifetimeRemaining = lifetimeSeconds,
            PiercesEnemies = piercesEnemies,
            SplitRemaining = Math.Max(0, splitRemaining),
            DamageMultiplier = MathF.Max(0f, damageMultiplier),
            SpellChainGeneration = Math.Max(0, spellChainGeneration),
            HitTargets = piercesEnemies ? new List<EntityId>(8) : null,
            HomingStrength = MathF.Max(0f, homingStrength),
            HomingTargetId = EntityId.None,
            HomingAcquireRange = 12f + MathF.Max(0f, homingStrength) * 6f,
            HomingAcquireRadius = homingAcquireRadius > 0f ? homingAcquireRadius : 4.5f + MathF.Max(0f, homingStrength) * 2.5f,
            HomingConeRadians = homingConeRadians > 0f ? homingConeRadians : 0.65f,
            HomingTurnRateRadians = 7f,
            HomingRetargetRemaining = 0f,
            Orbit = orbit,
            StatusKind = statusKind,
            Active = true
        };

        _projectiles.Add(projectile);
        if (_viewRoot != null)
        {
            var view = new ProjectileViewNode { Name = $"Projectile_{id.Value}" };
            _viewRoot.AddChild(view);
            view.GlobalPosition = initialPosition;
            view.Scale = Vector3.One * MathF.Max(0.1f, presentationScale * radius / ProjectileViewNode.MeshRadius);
            _views[id] = view;
        }

        _context.Events.Publish(new ProjectileSpawnedEvent(spellId, id));
        return id;
    }

    public EntityId SpawnTargeted(
        ContentId sourceId,
        EntityId ownerId,
        EntityId targetId,
        Vector3 origin,
        Vector3 targetPosition,
        float damage,
        DamageType damageType,
        float radius,
        float lifetimeSeconds,
        float speed,
        float presentationScale = 1f)
    {
        if (_context == null || !targetId.IsValid)
        {
            return EntityId.None;
        }

        var direction = targetPosition - origin;
        if (direction.LengthSquared() <= 0.0001f)
        {
            direction = Vector3.Forward;
        }

        var id = Spawn(
            sourceId,
            ownerId,
            origin,
            direction,
            damage,
            damageType,
            radius,
            lifetimeSeconds,
            speed,
            presentationScale: presentationScale);
        if (!id.IsValid)
        {
            return EntityId.None;
        }

        for (var i = 0; i < _projectiles.Count; i++)
        {
            if (!_projectiles[i].EntityId.Equals(id))
            {
                continue;
            }

            var projectile = _projectiles[i];
            projectile.IsDirectTargetProjectile = true;
            projectile.DirectTargetId = targetId;
            projectile.DirectTargetPosition = targetPosition;
            projectile.DirectTargetRadius = MathF.Max(radius, 0.45f);
            _projectiles[i] = projectile;
            break;
        }

        return id;
    }

    private void SpawnSplitProjectiles(ProjectileState parent, Vector3 hitPosition)
    {
        if (_context == null)
        {
            return;
        }

        var direction = parent.Velocity.LengthSquared() <= 0.0001f
            ? Vector3.Forward
            : parent.Velocity.Normalized();
        const float splitAngleRadians = 0.42f;
        var lifetime = MathF.Max(0.12f, parent.LifetimeRemaining * 0.55f);
        var damage = MathF.Max(0.1f, parent.Damage * 0.55f);
        var speed = MathF.Max(0.1f, parent.Velocity.Length());
        var radius = MathF.Max(0.05f, parent.Radius * 0.85f);
        var splitRemaining = Math.Max(0, parent.SplitRemaining - 1);

        Spawn(
            parent.SpellId,
            parent.OwnerId,
            hitPosition,
            RotateYaw(direction, -splitAngleRadians),
            damage,
            parent.DamageType,
            radius,
            lifetime,
            speed,
            parent.StatusKind,
            0.85f,
            parent.PiercesEnemies,
            splitRemaining,
            parent.DamageMultiplier,
            parent.SpellChainGeneration,
            parent.HomingStrength,
            parent.HomingConeRadians,
            parent.HomingAcquireRadius,
            parent.Orbit);
        Spawn(
            parent.SpellId,
            parent.OwnerId,
            hitPosition,
            RotateYaw(direction, splitAngleRadians),
            damage,
            parent.DamageType,
            radius,
            lifetime,
            speed,
            parent.StatusKind,
            0.85f,
            parent.PiercesEnemies,
            splitRemaining,
            parent.DamageMultiplier,
            parent.SpellChainGeneration,
            parent.HomingStrength,
            parent.HomingConeRadians,
            parent.HomingAcquireRadius,
            parent.Orbit);
    }

    private static void UpdateHoming(
        ref ProjectileState projectile,
        EnemySpatialRegistry enemyRegistry,
        CombatSystem combat,
        float delta)
    {
        if (projectile.HomingStrength <= 0f || projectile.IsDirectTargetProjectile)
        {
            return;
        }

        projectile.HomingRetargetRemaining -= delta;
        var targetPosition = Vector3.Zero;
        var hasTarget = projectile.HomingTargetId.IsValid
            && combat.IsAlive(projectile.HomingTargetId)
            && enemyRegistry.TryGetPosition(projectile.HomingTargetId, out targetPosition);

        if (!hasTarget || projectile.HomingRetargetRemaining <= 0f)
        {
            hasTarget = enemyRegistry.TryFindBestHomingTarget(
                projectile.Position,
                projectile.Velocity,
                projectile.HomingAcquireRange,
                projectile.HomingAcquireRadius,
                projectile.HomingConeRadians,
                out var targetId,
                out targetPosition);
            if (hasTarget && projectile.HitTargets != null)
            {
                for (var i = 0; i < projectile.HitTargets.Count; i++)
                {
                    if (!projectile.HitTargets[i].Equals(targetId))
                    {
                        continue;
                    }

                    hasTarget = false;
                    break;
                }
            }

            projectile.HomingTargetId = hasTarget ? targetId : EntityId.None;
            projectile.HomingRetargetRemaining = 0.16f;
        }

        if (!hasTarget)
        {
            return;
        }

        targetPosition += new Vector3(0f, 0.65f, 0f);
        var desired = targetPosition - projectile.Position;
        if (desired.LengthSquared() <= 0.0001f || projectile.Velocity.LengthSquared() <= 0.0001f)
        {
            return;
        }

        var speed = projectile.Velocity.Length();
        var currentDirection = projectile.Velocity / speed;
        var desiredDirection = desired.Normalized();
        var turn = Math.Clamp(projectile.HomingTurnRateRadians * projectile.HomingStrength * delta, 0f, 1f);
        var newDirection = currentDirection.Lerp(desiredDirection, turn);
        if (newDirection.LengthSquared() <= 0.0001f)
        {
            return;
        }

        projectile.Velocity = newDirection.Normalized() * speed;
    }

    private static Vector3 RotateYaw(Vector3 direction, float radians)
    {
        var cos = MathF.Cos(radians);
        var sin = MathF.Sin(radians);
        return new Vector3(
            direction.X * cos - direction.Z * sin,
            direction.Y,
            direction.X * sin + direction.Z * cos).Normalized();
    }

    private void RemoveAt(int index)
    {
        var projectile = _projectiles[index];
        if (_views.Remove(projectile.EntityId, out var view))
        {
            view.QueueFree();
        }

        _projectiles.RemoveAt(index);
    }

    private Vector3 ResolveCurrentDirectTargetPosition(ProjectileState projectile)
    {
        if (_context == null)
        {
            return projectile.DirectTargetPosition;
        }

        if (projectile.DirectTargetId.Equals(_context.State.Player.EntityId))
        {
            return _context.State.Player.Position + new Vector3(0f, 0.8f, 0f);
        }

        if (_context.TryGetSystem<DefenseSystem>(out var defenseSystem)
            && defenseSystem?.TryGetDefensePosition(projectile.DirectTargetId, out var defensePosition) == true)
        {
            return defensePosition + new Vector3(0f, 0.8f, 0f);
        }

        if (_context.TryGetSystem<SummonSystem>(out var summonSystem)
            && summonSystem?.TryGetSummonPosition(projectile.DirectTargetId, out var summonPosition) == true)
        {
            return summonPosition + new Vector3(0f, 0.8f, 0f);
        }

        return projectile.DirectTargetPosition;
    }

    private static bool SegmentTouchesPoint(Vector3 start, Vector3 end, Vector3 point, float radius)
    {
        var segment = end - start;
        var lengthSquared = segment.LengthSquared();
        if (lengthSquared <= 0.0001f)
        {
            return start.DistanceSquaredTo(point) <= radius * radius;
        }

        var t = Mathf.Clamp((point - start).Dot(segment) / lengthSquared, 0f, 1f);
        var closest = start + segment * t;
        return closest.DistanceSquaredTo(point) <= radius * radius;
    }
}
