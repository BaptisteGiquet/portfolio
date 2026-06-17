using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;

namespace TheLastMage.Gameplay.Enemies;

public sealed class EnemySpatialRegistry
{
    private const float CellSize = 2.5f;

    private readonly Dictionary<EntityId, EnemyState> _enemies = new();
    private readonly Dictionary<Vector2I, List<EnemyState>> _cells = new();
    private CombatSystem? _combat;

    public void BindCombat(CombatSystem combat)
    {
        _combat = combat;
    }

    public void Register(EnemyState enemy)
    {
        _enemies[enemy.EntityId] = enemy;
    }

    public void Unregister(EntityId enemyId)
    {
        _enemies.Remove(enemyId);
    }

    public void Clear()
    {
        _enemies.Clear();
        _cells.Clear();
    }

    public void RebuildHash()
    {
        foreach (var cell in _cells.Values)
        {
            cell.Clear();
        }

        foreach (var enemy in _enemies.Values)
        {
            if (_combat != null && !_combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            var key = ToCell(enemy.Position);
            if (!_cells.TryGetValue(key, out var bucket))
            {
                bucket = new List<EnemyState>(8);
                _cells.Add(key, bucket);
            }

            bucket.Add(enemy);
        }
    }

    public int AccumulateSeparation(
        EnemyState source,
        float radius,
        out Vector3 separation)
    {
        separation = Vector3.Zero;
        var radiusSquared = radius * radius;
        var checks = 0;
        var sourceCell = ToCell(source.Position);

        for (var z = -1; z <= 1; z++)
        {
            for (var x = -1; x <= 1; x++)
            {
                var key = new Vector2I(sourceCell.X + x, sourceCell.Y + z);
                if (!_cells.TryGetValue(key, out var bucket))
                {
                    continue;
                }

                for (var i = 0; i < bucket.Count; i++)
                {
                    var other = bucket[i];
                    if (other.EntityId.Equals(source.EntityId))
                    {
                        continue;
                    }

                    checks++;
                    var offset = source.Position - other.Position;
                    offset.Y = 0f;
                    var distanceSquared = offset.LengthSquared();
                    if (distanceSquared <= 0.0001f || distanceSquared > radiusSquared)
                    {
                        continue;
                    }

                    var distance = MathF.Sqrt(distanceSquared);
                    var strength = (radius - distance) / radius;
                    separation += (offset / distance) * strength;
                }
            }
        }

        return checks;
    }

    public void CopyInRadius(Vector3 position, float radius, List<EnemyState> results)
    {
        results.Clear();
        var radiusSquared = radius * radius;
        var centerCell = ToCell(position);
        var cellRadius = Mathf.CeilToInt(radius / CellSize);

        for (var z = -cellRadius; z <= cellRadius; z++)
        {
            for (var x = -cellRadius; x <= cellRadius; x++)
            {
                var key = new Vector2I(centerCell.X + x, centerCell.Y + z);
                if (!_cells.TryGetValue(key, out var bucket))
                {
                    continue;
                }

                for (var i = 0; i < bucket.Count; i++)
                {
                    var enemy = bucket[i];
                    if (_combat != null && !_combat.IsAlive(enemy.EntityId))
                    {
                        continue;
                    }

                    if (enemy.Position.DistanceSquaredTo(position) <= radiusSquared)
                    {
                        results.Add(enemy);
                    }
                }
            }
        }
    }

    public bool TryGetPosition(EntityId enemyId, out Vector3 position)
    {
        if (_enemies.TryGetValue(enemyId, out var enemy)
            && (_combat == null || _combat.IsAlive(enemy.EntityId)))
        {
            position = enemy.Position;
            return true;
        }

        position = Vector3.Zero;
        return false;
    }

    public bool TryFindNearest(
        Vector3 position,
        float radius,
        IReadOnlyList<EntityId>? ignoredEnemies,
        out EntityId enemyId,
        out Vector3 enemyPosition)
    {
        var radiusSquared = radius * radius;
        var closestDistanceSquared = float.MaxValue;
        enemyId = EntityId.None;
        enemyPosition = position;

        foreach (var enemy in _enemies.Values)
        {
            if (IsIgnored(enemy.EntityId, ignoredEnemies))
            {
                continue;
            }

            if (_combat != null && !_combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            var distanceSquared = enemy.Position.DistanceSquaredTo(position);
            if (distanceSquared > radiusSquared || distanceSquared >= closestDistanceSquared)
            {
                continue;
            }

            closestDistanceSquared = distanceSquared;
            enemyId = enemy.EntityId;
            enemyPosition = enemy.Position;
        }

        return enemyId.IsValid;
    }

    public bool TryFindBestHomingTarget(
        Vector3 origin,
        Vector3 direction,
        float range,
        float acquireRadius,
        float maxAngleRadians,
        out EntityId enemyId,
        out Vector3 enemyPosition)
    {
        var normalizedDirection = direction.LengthSquared() <= 0.0001f ? Vector3.Forward : direction.Normalized();
        var end = origin + normalizedDirection * MathF.Max(0.1f, range);
        var minimumDirectionDot = MathF.Cos(Math.Clamp(maxAngleRadians, 0.01f, MathF.PI));
        var bestScore = float.MaxValue;
        enemyId = EntityId.None;
        enemyPosition = end;

        foreach (var enemy in _enemies.Values)
        {
            if (_combat != null && !_combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            var toEnemy = enemy.Position - origin;
            var alongRay = toEnemy.Dot(normalizedDirection);
            if (alongRay < 0f || alongRay > range)
            {
                continue;
            }

            var distanceToEnemy = toEnemy.Length();
            if (distanceToEnemy <= 0.001f || alongRay / distanceToEnemy < minimumDirectionDot)
            {
                continue;
            }

            var lateralDistance = DistancePointToSegment(enemy.Position, origin, end);
            if (lateralDistance > acquireRadius)
            {
                continue;
            }

            var score = lateralDistance * 4f + alongRay * 0.08f;
            if (score >= bestScore)
            {
                continue;
            }

            bestScore = score;
            enemyId = enemy.EntityId;
            enemyPosition = enemy.Position;
        }

        return enemyId.IsValid;
    }

    public bool TryFindSweepHit(
        Vector3 from,
        Vector3 to,
        float radius,
        out EntityId enemyId,
        out Vector3 hitPosition,
        IReadOnlyList<EntityId>? ignoredEnemies = null)
    {
        var closestDistance = float.MaxValue;
        enemyId = EntityId.None;
        hitPosition = to;
        foreach (var enemy in _enemies.Values)
        {
            if (IsIgnored(enemy.EntityId, ignoredEnemies))
            {
                continue;
            }

            if (_combat != null && !_combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            var distance = DistancePointToSegment(enemy.Position, from, to);
            if (distance <= radius + 0.65f && distance < closestDistance)
            {
                closestDistance = distance;
                enemyId = enemy.EntityId;
                hitPosition = enemy.Position;
            }
        }

        return enemyId.IsValid;
    }

    private static bool IsIgnored(EntityId enemyId, IReadOnlyList<EntityId>? ignoredEnemies)
    {
        if (ignoredEnemies == null)
        {
            return false;
        }

        for (var i = 0; i < ignoredEnemies.Count; i++)
        {
            if (ignoredEnemies[i].Equals(enemyId))
            {
                return true;
            }
        }

        return false;
    }

    public bool TryFindRayHit(Vector3 origin, Vector3 direction, float range, float radius, out EntityId enemyId, out Vector3 hitPosition)
    {
        var normalizedDirection = direction.Normalized();
        var end = origin + normalizedDirection * range;
        var closestAlongRay = float.MaxValue;
        enemyId = EntityId.None;
        hitPosition = end;

        foreach (var enemy in _enemies.Values)
        {
            if (_combat != null && !_combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            var toEnemy = enemy.Position - origin;
            var alongRay = toEnemy.Dot(normalizedDirection);
            if (alongRay < 0f || alongRay > range || alongRay >= closestAlongRay)
            {
                continue;
            }

            var closestPoint = origin + normalizedDirection * alongRay;
            if (enemy.Position.DistanceTo(closestPoint) <= radius + 0.65f)
            {
                closestAlongRay = alongRay;
                enemyId = enemy.EntityId;
                hitPosition = enemy.Position;
            }
        }

        return enemyId.IsValid;
    }

    private static float DistancePointToSegment(Vector3 point, Vector3 segmentStart, Vector3 segmentEnd)
    {
        var segment = segmentEnd - segmentStart;
        var lengthSquared = segment.LengthSquared();
        if (lengthSquared <= 0.0001f)
        {
            return point.DistanceTo(segmentStart);
        }

        var t = Mathf.Clamp((point - segmentStart).Dot(segment) / lengthSquared, 0f, 1f);
        var closest = segmentStart + segment * t;
        return point.DistanceTo(closest);
    }

    private static Vector2I ToCell(Vector3 position)
    {
        return new Vector2I(
            Mathf.FloorToInt(position.X / CellSize),
            Mathf.FloorToInt(position.Z / CellSize));
    }
}
