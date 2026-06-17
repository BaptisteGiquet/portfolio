using Godot;
using System.Globalization;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Navigation;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Defenses;

public sealed class DefenseSystem : IGameSystem
{
    private readonly List<DefenseState> _defenses = new();
    private readonly Dictionary<EntityId, DefenseActorNode> _actors = new();
    private readonly Node3D? _actorRoot;
    private RunContext? _context;
    private SubscriptionToken _destroyedToken;

    public int ActiveCount => _defenses.Count;

    public IReadOnlyList<DefenseState> Defenses => _defenses;

    public DefenseSystem(Node3D? actorRoot)
    {
        _actorRoot = actorRoot;
    }

    public void Initialize(RunContext context)
    {
        _context = context;
        _destroyedToken = context.Events.Subscribe<DefenseDestroyedEvent>(OnDefenseDestroyed);
    }

    public void Tick(float delta)
    {
        if (_context != null)
        {
            _context.State.DebugMetrics.ActiveDefenses = _defenses.Count;
            _context.State.DebugMetrics.DefensePreparationSummary = BuildPreparationSummary();
        }
    }

    public void FixedTick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        var enemySystem = _context.GetSystem<EnemySystem>();
        var combat = _context.GetSystem<CombatSystem>();
        for (var i = _defenses.Count - 1; i >= 0; i--)
        {
            var defense = _defenses[i];
            if (!defense.IsExplosiveSeal || defense.HasTriggered || !combat.IsAlive(defense.EntityId))
            {
                continue;
            }

            if (defense.RechargeRemaining > 0f)
            {
                defense.RechargeRemaining = MathF.Max(0f, defense.RechargeRemaining - delta);
                continue;
            }

            if (defense.IsArmed)
            {
                defense.FuseRemaining -= delta;
                if (defense.FuseRemaining <= 0f)
                {
                    TriggerExplosiveSeal(defense, enemySystem);
                }

                continue;
            }

            for (var enemyIndex = enemySystem.ActiveEnemies.Count - 1; enemyIndex >= 0; enemyIndex--)
            {
                var enemy = enemySystem.ActiveEnemies[enemyIndex];
                if (!combat.IsAlive(enemy.EntityId))
                {
                    continue;
                }

                if (enemy.Position.DistanceSquaredTo(defense.Position) > defense.TriggerRadius * defense.TriggerRadius)
                {
                    continue;
                }

                defense.IsArmed = true;
                defense.FuseRemaining = defense.FuseSeconds;
                defense.ArmedByEnemyId = enemy.EntityId;
                defense.ArmedByEnemyPosition = enemy.Position;
                var distance = enemy.Position.DistanceTo(defense.Position);
                GD.Print(
                    $"ExplosiveSealArmed defense={defense.EntityId} sealPos={FormatVector(defense.Position)} " +
                    $"enemy={enemy.EntityId} enemyPos={FormatVector(enemy.Position)} distance={distance:0.##} " +
                    $"trigger={defense.TriggerRadius:0.##} fuse={defense.FuseSeconds:0.##} explosion={defense.ExplosionRadius:0.##}");
                break;
            }
        }
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_destroyedToken);
        }

        foreach (var actor in _actors.Values)
        {
            actor.QueueFree();
        }

        _actors.Clear();
        _defenses.Clear();
        _context = null;
    }

    public EntityId PlaceDefense(ContentId defenseId, Vector3 position)
    {
        if (_context == null || !_context.Content.TryGetDefense(defenseId, out var definition) || definition == null)
        {
            return EntityId.None;
        }

        var validation = ValidatePlacement(definition, position);
        _context.State.DebugMetrics.LastDefensePlacementValidation = validation.IsValid
            ? $"{defenseId} valid at {FormatVector(position)}"
            : $"{defenseId} invalid at {FormatVector(position)}: {validation.Reason}";
        if (!validation.IsValid)
        {
            GD.Print($"DefensePlacementRejected defense={defenseId} pos={FormatVector(position)} reason={validation.Reason}");
            return EntityId.None;
        }

        var id = _context.State.AllocateEntityId();
        var state = Convert(definition, id, position);
        _defenses.Add(state);
        _context.GetSystem<CombatSystem>().RegisterTarget(id, CombatTargetKind.Defense, definition.MaxHealth, defenseId);
        GD.Print(
            $"DefensePlaced id={id} defense={defenseId} pos={FormatVector(position)} " +
            $"trigger={state.TriggerRadius:0.##} explosion={state.ExplosionRadius:0.##} fuse={state.FuseSeconds:0.##} damage={state.Damage:0.##}");

        if (_actorRoot != null)
        {
            var actor = new DefenseActorNode { Name = $"Defense_{id.Value}_{defenseId}" };
            _actorRoot.AddChild(actor);
            actor.GlobalPosition = position;
            actor.Bind(id, state.IsExplosiveSeal, state.TriggerRadius, state.ExplosionRadius);
            _actors[id] = actor;
        }

        _context.Events.Publish(new DefensePlacedEvent(defenseId, id, position));
        return id;
    }

    public bool RepairDefense(EntityId entityId, float amount)
    {
        if (_context == null)
        {
            return false;
        }

        return _context.GetSystem<CombatSystem>().Repair(entityId, amount) > 0f;
    }

    public bool TryGetDefensePosition(EntityId entityId, out Vector3 position)
    {
        for (var i = 0; i < _defenses.Count; i++)
        {
            var defense = _defenses[i];
            if (defense.EntityId.Equals(entityId))
            {
                position = defense.Position;
                return true;
            }
        }

        position = Vector3.Zero;
        return false;
    }

    public DefensePlacementValidationResult ValidatePlacement(ContentId defenseId, Vector3 position)
    {
        if (_context == null || !_context.Content.TryGetDefense(defenseId, out var definition) || definition == null)
        {
            return DefensePlacementValidationResult.Invalid($"defense {defenseId} is not available");
        }

        return ValidatePlacement(definition, position);
    }

    public DefensePlacementValidationResult ValidatePlacement(DefenseRuntimeDefinition definition, Vector3 position)
    {
        var tower = _context?.TryGetSystem<TowerNavigationAdapter>(out var towerSystem) == true ? towerSystem : null;
        return DefensePlacementValidator.Validate(definition, position, _defenses, tower);
    }

    public bool TryGetObstructingDefense(Vector3 enemyPosition, out DefenseState? defense)
    {
        defense = null;
        if (_context == null || !_context.TryGetSystem<TowerNavigationAdapter>(out var tower) || tower == null)
        {
            return false;
        }

        if (tower.RouteState != TowerRouteState.MageAtTop)
        {
            return false;
        }

        var combat = _context.GetSystem<CombatSystem>();
        var bestDistance = float.MaxValue;
        foreach (var candidate in _defenses)
        {
            if (!candidate.IsBarricade || !combat.IsAlive(candidate.EntityId))
            {
                continue;
            }

            var distanceToEntrance = candidate.Position.DistanceSquaredTo(tower.EntrancePosition);
            if (distanceToEntrance > 16f)
            {
                continue;
            }

            var distance = enemyPosition.DistanceSquaredTo(candidate.Position);
            var influenceRadius = MathF.Max(18f, candidate.BlockRadius);
            if (distance < bestDistance && distance <= influenceRadius * influenceRadius)
            {
                bestDistance = distance;
                defense = candidate;
            }
        }

        return defense != null;
    }

    public bool TryGetPriorityBlocker(Vector3 enemyPosition, out DefenseState? defense)
    {
        defense = null;
        var combat = _context?.GetSystem<CombatSystem>();
        if (combat == null)
        {
            return false;
        }

        var bestDistance = float.MaxValue;
        foreach (var candidate in _defenses)
        {
            if (!candidate.IsBarricade || !combat.IsAlive(candidate.EntityId))
            {
                continue;
            }

            var priorityRadius = MathF.Max(candidate.BlockRadius * 2.5f, 8f);
            var distance = enemyPosition.DistanceSquaredTo(candidate.Position);
            if (distance < bestDistance && distance <= priorityRadius * priorityRadius)
            {
                bestDistance = distance;
                defense = candidate;
            }
        }

        return defense != null;
    }

    private void TriggerExplosiveSeal(DefenseState seal, EnemySystem enemySystem)
    {
        if (_context == null)
        {
            return;
        }

        var combat = _context.GetSystem<CombatSystem>();
        var explosionRadiusSquared = seal.ExplosionRadius * seal.ExplosionRadius;
        var hits = 0;
        var closestEnemyId = EntityId.None;
        var closestDistance = float.MaxValue;
        for (var enemyIndex = enemySystem.ActiveEnemies.Count - 1; enemyIndex >= 0; enemyIndex--)
        {
            var enemy = enemySystem.ActiveEnemies[enemyIndex];
            if (!combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            var distance = enemy.Position.DistanceTo(seal.Position);
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestEnemyId = enemy.EntityId;
            }

            if (distance * distance > explosionRadiusSquared)
            {
                continue;
            }

            hits++;
            combat.ApplyDamage(new DamageRequest(
                seal.EntityId,
                enemy.EntityId,
                seal.DefenseId,
                seal.Damage,
                DamageType.Fire,
                new HitContext(enemy.Position, Vector3.Up, seal.EntityId)));
        }

        GD.Print(
            $"ExplosiveSealDetonated defense={seal.EntityId} sealPos={FormatVector(seal.Position)} " +
            $"armedBy={seal.ArmedByEnemyId} armedEnemyPos={FormatVector(seal.ArmedByEnemyPosition)} " +
            $"radius={seal.ExplosionRadius:0.##} hits={hits} closestEnemy={closestEnemyId} closestDistance={closestDistance:0.##}");
        seal.ChargesRemaining = Math.Max(0, seal.ChargesRemaining - 1);
        seal.IsArmed = false;
        seal.FuseRemaining = 0f;
        seal.RechargeRemaining = seal.ChargesRemaining > 0 ? seal.RechargeSeconds : 0f;
        if (seal.ChargesRemaining <= 0)
        {
            seal.HasTriggered = true;
            combat.ApplyDamage(new DamageRequest(
                seal.EntityId,
                seal.EntityId,
                seal.DefenseId,
                999999f,
                DamageType.Fire,
                new HitContext(seal.Position, Vector3.Up, seal.EntityId)));
        }
    }

    private void OnDefenseDestroyed(DefenseDestroyedEvent gameEvent)
    {
        for (var i = _defenses.Count - 1; i >= 0; i--)
        {
            if (_defenses[i].EntityId.Equals(gameEvent.DefenseEntityId))
            {
                _defenses.RemoveAt(i);
                break;
            }
        }

        if (_actors.Remove(gameEvent.DefenseEntityId, out var actor))
        {
            actor.QueueFree();
        }

        if (gameEvent.DefenseId.Value.Contains("barricade", StringComparison.OrdinalIgnoreCase)
            && _context?.TryGetSystem<TowerNavigationAdapter>(out var tower) == true
            && tower != null)
        {
            tower.BreachEntrance();
        }
    }

    private static DefenseState Convert(DefenseRuntimeDefinition definition, EntityId id, Vector3 position)
    {
        return new DefenseState
        {
            EntityId = id,
            DefenseId = definition.Id,
            Position = position,
            PlacementRule = definition.PlacementRule,
            PlacementRadius = definition.PlacementRadius,
            BlockRadius = definition.BlockRadius,
            TriggerRadius = definition.TriggerRadius,
            ExplosionRadius = definition.ExplosionRadius,
            FuseSeconds = definition.FuseSeconds,
            RechargeSeconds = definition.RechargeSeconds,
            MaxCharges = definition.MaxCharges,
            ChargesRemaining = definition.MaxCharges,
            Damage = definition.Damage
        };
    }

    private string BuildPreparationSummary()
    {
        if (_context == null)
        {
            return "-";
        }

        if (!_context.State.PhasePermissions.CanPlaceDefense && !_context.State.PhasePermissions.CanRepairDefense)
        {
            return "defense prep closed";
        }

        var affordable = _context.Content.Defenses.Values
            .Where(defense => _context.State.Materials >= (int)MathF.Ceiling(defense.Cost))
            .Select(defense => $"{defense.Id}({MathF.Ceiling(defense.Cost):0})")
            .ToArray();
        return affordable.Length == 0
            ? $"materials {_context.State.Materials}: no affordable defenses"
            : $"materials {_context.State.Materials}: {string.Join(", ", affordable)}";
    }

    private static string FormatVector(Vector3 value)
    {
        var culture = CultureInfo.InvariantCulture;
        return string.Create(
            culture,
            $"({value.X:0.##},{value.Y:0.##},{value.Z:0.##})");
    }
}
