using Godot;
using System.Diagnostics;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Navigation;
using TheLastMage.Gameplay.Projectiles;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.StatusEffects;
using TheLastMage.Gameplay.Summons;
using TheLastMage.Gameplay.Waves;

namespace TheLastMage.Gameplay.Enemies;

public sealed class EnemySystem : IGameSystem
{
    private const float SeparationRadius = 1.15f;
    private const float SeparationStrength = 1.15f;
    private const float SiegeSlotSpacing = 0.85f;
    private const int SiegeSlotColumnCount = 18;
    private const float SlotArrivalDistance = 0.45f;
    private const float SteeringAcceleration = 8f;
    private const float SteeringDamping = 10f;
    private const string EnemyFormAmongUs = "among_us";

    private readonly List<EnemyState> _enemies = new();
    private readonly Dictionary<EntityId, EnemyActorNode> _actors = new();
    private readonly Node3D? _actorRoot;
    private RunContext? _context;
    private SubscriptionToken _enemyDiedToken;
    private SubscriptionToken _dayStartedToken;
    private SubscriptionToken _damageAppliedToken;
    private SubscriptionToken _nightStartedToken;

    public EnemySpatialRegistry SpatialRegistry { get; } = new();

    public int ActiveCount => _enemies.Count;

    public IReadOnlyList<EnemyState> ActiveEnemies => _enemies;

    public EnemySystem(Node3D? actorRoot)
    {
        _actorRoot = actorRoot;
    }

    public void Initialize(RunContext context)
    {
        _context = context;
        SpatialRegistry.BindCombat(context.GetSystem<CombatSystem>());
        _enemyDiedToken = context.Events.Subscribe<EnemyDiedEvent>(OnEnemyDied);
        _dayStartedToken = context.Events.Subscribe<DayStartedEvent>(_ => SpawnOpeningEnemy());
        _damageAppliedToken = context.Events.Subscribe<DamageAppliedEvent>(OnDamageApplied);
        _nightStartedToken = context.Events.Subscribe<NightStartedEvent>(_ => ClearForNight());
    }

    public void Tick(float delta)
    {
    }

    public void FixedTick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        if (_context.State.CurrentPhase is RunPhase.RunVictory or RunPhase.RunDefeat)
        {
            return;
        }

        var beforeAllocations = GC.GetAllocatedBytesForCurrentThread();
        var stopwatch = Stopwatch.StartNew();
        var combat = _context.GetSystem<CombatSystem>();
        var playerPosition = _context.State.Player.Position;
        var tower = _context.TryGetSystem<TowerNavigationAdapter>(out var towerSystem) ? towerSystem : null;
        var defenses = _context.TryGetSystem<DefenseSystem>(out var defenseSystem) ? defenseSystem : null;
        var statuses = _context.TryGetSystem<StatusEffectSystem>(out var statusSystem) ? statusSystem : null;
        var summons = _context.TryGetSystem<SummonSystem>(out var summonSystem) ? summonSystem : null;
        SpatialRegistry.RebuildHash();
        foreach (var enemy in _enemies)
        {
            if (!combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            UpdateTemporaryForm(enemy, delta, combat);
            enemy.AttackCooldownRemaining = MathF.Max(0f, enemy.AttackCooldownRemaining - delta);
            enemy.AbilityCooldownRemaining = MathF.Max(0f, enemy.AbilityCooldownRemaining - delta);
            EntityId attackTarget = _context.State.Player.EntityId;
            var goal = playerPosition;
            if (tower != null)
            {
                goal = tower.GetGoalForEnemy(
                    enemy.Position,
                    playerPosition,
                    enemy.TowerRouteStep,
                    out var nextRouteStep,
                    out var routeReason);
                enemy.TowerRouteStep = nextRouteStep;
                enemy.RouteDecision = routeReason;
            }

            if (summons?.TryGetEnemyPriorityTarget(enemy.Position, playerPosition, combat, out var summonTargetId, out var summonTargetPosition) == true)
            {
                goal = summonTargetPosition;
                attackTarget = summonTargetId;
            }

            DefenseState? defense = null;
            var pressuresDefenses = HasTag(enemy, "enemy.trait.defense_pressure");
            var hasDefenseTarget = pressuresDefenses
                ? defenses?.TryGetPriorityBlocker(enemy.Position, out defense) == true
                : defenses?.TryGetObstructingDefense(enemy.Position, out defense) == true;
            if (hasDefenseTarget && defense != null)
            {
                goal = defense.Position;
                attackTarget = defense.EntityId;
            }

            enemy.CurrentGoal = goal;
            var slotGoal = GetSiegeSlotGoal(enemy, goal);
            enemy.CurrentSlotGoal = slotGoal;
            _context.State.DebugMetrics.SlotAssignmentsThisFrame++;

            var actionsBlocked = enemy.TemporaryFormDisablesAttacks || statuses?.BlocksActions(enemy.EntityId) == true;
            if (actionsBlocked)
            {
                enemy.PendingAbilityIndex = -1;
                enemy.PendingAbilityTargetId = EntityId.None;
                enemy.AbilityTelegraphRemaining = 0f;
            }

            if (!actionsBlocked && UpdatePendingAbility(enemy, delta, combat))
            {
                UpdateActor(enemy, statuses);
                continue;
            }

            var toGoal = slotGoal - enemy.Position;
            var distance = toGoal.Length();
            var distanceToAttackTarget = enemy.Position.DistanceTo(goal);
            var desiredRange = GetDesiredRange(enemy);
            if (distance > SlotArrivalDistance && distanceToAttackTarget > desiredRange)
            {
                var direction = distance > 0.001f ? toGoal / distance : Vector3.Zero;
                var separationChecks = SpatialRegistry.AccumulateSeparation(enemy, SeparationRadius, out var separation);
                _context.State.DebugMetrics.SeparationChecksThisFrame += separationChecks;
                if (separation.LengthSquared() > 0.0001f)
                {
                    var slotDistanceFactor = Mathf.Clamp(distance / 3f, 0.15f, 1f);
                    direction = (direction + separation.Normalized() * SeparationStrength * slotDistanceFactor).Normalized();
                }

                var moveSpeedMultiplier = statuses?.GetMoveSpeedMultiplier(enemy.EntityId) ?? 1f;
                var desiredVelocity = direction * enemy.MoveSpeed * moveSpeedMultiplier;
                enemy.SteeringVelocity = enemy.SteeringVelocity.Lerp(desiredVelocity, Mathf.Clamp(SteeringAcceleration * delta, 0f, 1f));
                enemy.Position += enemy.SteeringVelocity * delta;
            }
            else
            {
                enemy.SteeringVelocity = enemy.SteeringVelocity.Lerp(Vector3.Zero, Mathf.Clamp(SteeringDamping * delta, 0f, 1f));
            }

            if (!actionsBlocked && TryBeginAbility(enemy, attackTarget, goal))
            {
                UpdateActor(enemy, statuses);
                continue;
            }

            if (!actionsBlocked && distanceToAttackTarget <= enemy.AttackRange + SiegeSlotSpacing && enemy.AttackCooldownRemaining <= 0f)
            {
                _context.State.DebugMetrics.AttackChecksThisFrame++;
                var request = new DamageRequest(
                    enemy.EntityId,
                    attackTarget,
                    enemy.ArchetypeId,
                    enemy.AttackDamage,
                    DamageType.Physical,
                    new HitContext(goal, Vector3.Up, enemy.EntityId));
                combat.ApplyDamage(request);
                enemy.AttackCooldownRemaining = 1.25f;
            }

            UpdateActor(enemy, statuses);
        }

        stopwatch.Stop();
        _context.State.DebugMetrics.ActiveEnemies = _enemies.Count;
        _context.State.DebugMetrics.EnemyAiMilliseconds = (float)stopwatch.Elapsed.TotalMilliseconds;
        _context.State.DebugMetrics.AllocatedBytesThisFrame += GC.GetAllocatedBytesForCurrentThread() - beforeAllocations;
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_enemyDiedToken);
            _context.Events.Unsubscribe(_dayStartedToken);
            _context.Events.Unsubscribe(_damageAppliedToken);
            _context.Events.Unsubscribe(_nightStartedToken);
        }

        foreach (var actor in _actors.Values)
        {
            actor.QueueFree();
        }

        _actors.Clear();
        _enemies.Clear();
        _context = null;
    }

    public EntityId Spawn(ContentId archetypeId, Vector3 position)
    {
        if (_context == null || !_context.Content.TryGetEnemy(archetypeId, out var definition) || definition == null)
        {
            return EntityId.None;
        }

        if (definition.MaxActive > 0 && CountActive(archetypeId) >= definition.MaxActive)
        {
            return EntityId.None;
        }

        var id = _context.State.AllocateEntityId();
        var enemy = new EnemyState
        {
            EntityId = id,
            ArchetypeId = archetypeId,
            Role = definition.Role,
            Tags = definition.Tags,
            Position = position,
            MoveSpeed = definition.MoveSpeed,
            AttackDamage = definition.AttackDamage,
            AttackRange = definition.AttackRange,
            PreferredRange = definition.PreferredRange,
            AbilityRange = definition.AbilityRange,
            AbilityCooldownSeconds = definition.AbilityCooldownSeconds,
            AbilityTelegraphSeconds = definition.AbilityTelegraphSeconds,
            AbilityProjectileSpeed = definition.AbilityProjectileSpeed,
            Abilities = definition.Abilities
        };
        _enemies.Add(enemy);
        SpatialRegistry.Register(enemy);
        _context.GetSystem<CombatSystem>().RegisterTarget(id, CombatTargetKind.Enemy, definition.MaxHealth, archetypeId);

        if (_actorRoot != null)
        {
            var actor = CreateActor(definition);
            actor.Name = $"Enemy_{id.Value}";
            _actorRoot.AddChild(actor);
            actor.Bind(id, archetypeId, definition.Role, definition.Tags);
            actor.GlobalPosition = position;
            _actors[id] = actor;
        }

        return id;
    }

    private static EnemyActorNode CreateActor(EnemyRuntimeDefinition definition)
    {
        if (definition.PresentationScene != null)
        {
            var instance = definition.PresentationScene.Instantiate();
            if (instance is EnemyActorNode actor)
            {
                return actor;
            }

            instance.QueueFree();
            GD.PushWarning($"Enemy presentation scene for '{definition.Id}' must have EnemyActorNode as its root. Falling back to placeholder actor.");
        }

        return new EnemyActorNode();
    }

    public int CountActive(ContentId archetypeId)
    {
        var count = 0;
        for (var i = 0; i < _enemies.Count; i++)
        {
            if (_enemies[i].ArchetypeId.Equals(archetypeId))
            {
                count++;
            }
        }

        return count;
    }

    public int ApplySleepInRadius(ContentId sourceItemId, Vector3 center, float radius, float durationSeconds)
    {
        if (_context == null || !_context.TryGetSystem<StatusEffectSystem>(out var statuses) || statuses == null)
        {
            return 0;
        }

        var radiusSquared = MathF.Max(0f, radius) * MathF.Max(0f, radius);
        var duration = MathF.Max(0.05f, durationSeconds);
        var count = 0;
        for (var i = 0; i < _enemies.Count; i++)
        {
            var enemy = _enemies[i];
            if (enemy.Position.DistanceSquaredTo(center) > radiusSquared)
            {
                continue;
            }

            statuses.Apply(enemy.EntityId, StatusEffectRules.Sleep, sourceItemId, _context.State.Player.EntityId, duration, 0f);
            count++;
        }

        return count;
    }

    public int ApplyTemporaryFormToAll(ContentId sourceItemId, float durationSeconds, float maxHealth, bool disablesAttacks, PackedScene? presentationScene)
    {
        if (_context == null)
        {
            return 0;
        }

        var combat = _context.GetSystem<CombatSystem>();
        var count = 0;
        for (var i = 0; i < _enemies.Count; i++)
        {
            var enemy = _enemies[i];
            if (!combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            if (combat.TryGetHealth(enemy.EntityId, out var health) && health != null)
            {
                if (!enemy.HasTemporaryFormHealthSnapshot)
                {
                    enemy.TemporaryFormPreviousMaxHealth = health.MaxHealth;
                    enemy.TemporaryFormPreviousCurrentHealth = health.CurrentHealth;
                    enemy.HasTemporaryFormHealthSnapshot = true;
                }

                health.SetMaxHealth(MathF.Max(1f, maxHealth), healByIncrease: false);
            }

            enemy.TemporaryFormId = EnemyFormAmongUs;
            enemy.TemporaryFormRemainingSeconds = MathF.Max(0.05f, durationSeconds);
            enemy.TemporaryFormDisablesAttacks = disablesAttacks;
            if (_actors.TryGetValue(enemy.EntityId, out var actor))
            {
                actor.SetTemporaryForm(enemy.TemporaryFormId, presentationScene);
            }

            count++;
        }

        return count;
    }

    private bool TryBeginAbility(EnemyState enemy, EntityId targetId, Vector3 targetPosition)
    {
        if (_context == null
            || enemy.Abilities.Count == 0
            || enemy.AbilityCooldownRemaining > 0f
            || enemy.PendingAbilityIndex >= 0
            || enemy.Position.DistanceTo(targetPosition) > GetAbilityRange(enemy))
        {
            return false;
        }

        enemy.PendingAbilityIndex = 0;
        enemy.PendingAbilityTargetId = targetId;
        enemy.PendingAbilityTargetPosition = targetPosition;
        enemy.AbilityTelegraphRemaining = MathF.Max(0.05f, enemy.AbilityTelegraphSeconds);
        enemy.SteeringVelocity = Vector3.Zero;

        var ability = enemy.Abilities[enemy.PendingAbilityIndex];
        _context.Events.Publish(new EnemyAbilityTelegraphedEvent(enemy.EntityId, enemy.ArchetypeId, ability.EffectType, targetId));
        if (_actors.TryGetValue(enemy.EntityId, out var actor))
        {
            actor.SetAbilityTelegraph(GetAbilityTelegraphText(enemy, ability));
        }

        return true;
    }

    private void UpdateTemporaryForm(EnemyState enemy, float delta, CombatSystem combat)
    {
        if (string.IsNullOrWhiteSpace(enemy.TemporaryFormId))
        {
            return;
        }

        enemy.TemporaryFormRemainingSeconds -= delta;
        if (enemy.TemporaryFormRemainingSeconds > 0f)
        {
            return;
        }

        enemy.TemporaryFormId = string.Empty;
        enemy.TemporaryFormRemainingSeconds = 0f;
        enemy.TemporaryFormDisablesAttacks = false;
        if (enemy.HasTemporaryFormHealthSnapshot
            && combat.TryGetHealth(enemy.EntityId, out var health)
            && health != null
            && !health.IsDead)
        {
            health.SetHealth(enemy.TemporaryFormPreviousCurrentHealth, enemy.TemporaryFormPreviousMaxHealth);
        }

        enemy.HasTemporaryFormHealthSnapshot = false;
        if (_actors.TryGetValue(enemy.EntityId, out var actor))
        {
            actor.ClearTemporaryForm();
        }
    }

    private bool UpdatePendingAbility(EnemyState enemy, float delta, CombatSystem combat)
    {
        if (enemy.PendingAbilityIndex < 0)
        {
            return false;
        }

        enemy.AbilityTelegraphRemaining -= delta;
        if (_actors.TryGetValue(enemy.EntityId, out var actor))
        {
            actor.SetAbilityTelegraph(enemy.AbilityTelegraphRemaining > 0f
                ? GetAbilityTelegraphText(enemy, enemy.Abilities[enemy.PendingAbilityIndex])
                : string.Empty);
        }

        if (enemy.AbilityTelegraphRemaining > 0f)
        {
            return true;
        }

        ExecuteAbility(enemy, combat);
        enemy.PendingAbilityIndex = -1;
        enemy.PendingAbilityTargetId = EntityId.None;
        enemy.AbilityCooldownRemaining = MathF.Max(0.1f, enemy.AbilityCooldownSeconds);
        return true;
    }

    private void ExecuteAbility(EnemyState enemy, CombatSystem combat)
    {
        if (_context == null || enemy.PendingAbilityIndex < 0 || enemy.PendingAbilityIndex >= enemy.Abilities.Count)
        {
            return;
        }

        var ability = enemy.Abilities[enemy.PendingAbilityIndex];
        var targetId = enemy.PendingAbilityTargetId;
        if (!combat.IsAlive(targetId))
        {
            return;
        }

        var damageType = ResolveDamageType(enemy, ability);
        if ((HasTag(enemy, "enemy.attack.projectile")
                || ability.EffectType.Contains("projectile", StringComparison.OrdinalIgnoreCase))
            && _context.TryGetSystem<ProjectileSystem>(out var projectileSystem)
            && projectileSystem != null)
        {
            var origin = enemy.Position + new Vector3(0f, 1.2f, 0f);
            projectileSystem.SpawnTargeted(
                enemy.ArchetypeId,
                enemy.EntityId,
                targetId,
                origin,
                enemy.PendingAbilityTargetPosition + new Vector3(0f, 0.8f, 0f),
                GetAbilityDamage(enemy, ability),
                damageType,
                MathF.Max(0.35f, ability.Radius),
                MathF.Max(0.35f, ability.DurationSeconds),
                enemy.AbilityProjectileSpeed,
                0.75f);
        }
        else
        {
            combat.ApplyDamage(new DamageRequest(
                enemy.EntityId,
                targetId,
                enemy.ArchetypeId,
                GetAbilityDamage(enemy, ability),
                damageType,
                new HitContext(enemy.PendingAbilityTargetPosition, Vector3.Up, enemy.EntityId)));
        }

        _context.Events.Publish(new EnemyAbilityExecutedEvent(enemy.EntityId, enemy.ArchetypeId, ability.EffectType, targetId));
    }

    private void UpdateActor(EnemyState enemy, StatusEffectSystem? statuses)
    {
        if (!_actors.TryGetValue(enemy.EntityId, out var actor))
        {
            return;
        }

        actor.GlobalPosition = enemy.Position;
        actor.SetMoving(enemy.SteeringVelocity.LengthSquared() > 0.01f);
        if (statuses?.TryGetPrimaryStatusKind(enemy.EntityId, out var statusKind) == true)
        {
            actor.SetStatusOverlay(statusKind);
        }
        else
        {
            actor.SetStatusOverlay(string.Empty);
        }
    }

    private static float GetDesiredRange(EnemyState enemy)
    {
        if (enemy.PreferredRange > 0f && enemy.Abilities.Count > 0)
        {
            return enemy.PreferredRange;
        }

        return enemy.AttackRange;
    }

    private static float GetAbilityRange(EnemyState enemy)
    {
        return enemy.AbilityRange > 0f ? enemy.AbilityRange : enemy.AttackRange;
    }

    private static float GetAbilityDamage(EnemyState enemy, EnemyAbilityRuntimeSpec ability)
    {
        return ability.Value > 0f ? ability.Value : enemy.AttackDamage;
    }

    private static DamageType ResolveDamageType(EnemyState enemy, EnemyAbilityRuntimeSpec ability)
    {
        if (HasTag(enemy, "enemy.trait.anti_magic"))
        {
            return DamageType.Arcane;
        }

        if (ability.EffectType.Contains("fire", StringComparison.OrdinalIgnoreCase))
        {
            return DamageType.Fire;
        }

        if (ability.EffectType.Contains("frost", StringComparison.OrdinalIgnoreCase))
        {
            return DamageType.Frost;
        }

        if (ability.EffectType.Contains("arcane", StringComparison.OrdinalIgnoreCase)
            || ability.EffectType.Contains("anti_magic", StringComparison.OrdinalIgnoreCase))
        {
            return DamageType.Arcane;
        }

        return DamageType.Physical;
    }

    private static string GetAbilityTelegraphText(EnemyState enemy, EnemyAbilityRuntimeSpec ability)
    {
        if (HasTag(enemy, "enemy.trait.defense_pressure")
            || ability.EffectType.Contains("brute", StringComparison.OrdinalIgnoreCase)
            || ability.EffectType.Contains("slam", StringComparison.OrdinalIgnoreCase))
        {
            return "SLAM";
        }

        if (HasTag(enemy, "enemy.trait.anti_magic")
            || ability.EffectType.Contains("anti_magic", StringComparison.OrdinalIgnoreCase))
        {
            return "ANTI";
        }

        if (HasTag(enemy, "enemy.rank.boss")
            || HasTag(enemy, "enemy.rank.champion")
            || ability.EffectType.Contains("boss", StringComparison.OrdinalIgnoreCase)
            || ability.EffectType.Contains("champion", StringComparison.OrdinalIgnoreCase))
        {
            return "CHAMP";
        }

        if (HasTag(enemy, "enemy.attack.projectile")
            || ability.EffectType.Contains("projectile", StringComparison.OrdinalIgnoreCase))
        {
            return "SHOT";
        }

        return "CAST";
    }

    private static bool HasTag(EnemyState enemy, string tag)
    {
        return GameplayTagPath.MatchesAny(TagId.From(tag), enemy.Tags);
    }

    private void OnEnemyDied(EnemyDiedEvent gameEvent)
    {
        for (var i = _enemies.Count - 1; i >= 0; i--)
        {
            if (_enemies[i].EntityId.Equals(gameEvent.EnemyId))
            {
                _context?.Events.Publish(new CorpseAvailableEvent(gameEvent.EnemyId, gameEvent.EnemyArchetypeId, _enemies[i].Position));
                _enemies.RemoveAt(i);
                break;
            }
        }

        SpatialRegistry.Unregister(gameEvent.EnemyId);
        if (_actors.Remove(gameEvent.EnemyId, out var actor))
        {
            actor.MarkDead();
        }
    }

    private void OnDamageApplied(DamageAppliedEvent gameEvent)
    {
        if (_actors.TryGetValue(gameEvent.TargetId, out var actor))
        {
            actor.FlashDamage();
        }
    }

    private void ClearForNight()
    {
        if (_context == null || _context.State.IsBenchmarkRun || _enemies.Count == 0)
        {
            return;
        }

        var combat = _context.GetSystem<CombatSystem>();
        for (var i = 0; i < _enemies.Count; i++)
        {
            combat.UnregisterTarget(_enemies[i].EntityId);
        }

        foreach (var actor in _actors.Values)
        {
            actor.QueueFree();
        }

        GD.Print($"EnemiesClearedForNight count={_enemies.Count}");
        _actors.Clear();
        _enemies.Clear();
        SpatialRegistry.Clear();
        _context.State.DebugMetrics.ActiveEnemies = 0;
    }

    private void SpawnOpeningEnemy()
    {
        if (_context == null
            || _context.State.SuppressAutomaticEnemySpawns
            || _enemies.Count > 0
            || !_context.Content.Enemies.ContainsKey(ContentId.From("human_grunt")))
        {
            return;
        }

        if (_context.TryGetSystem<WaveDirectorSystem>(out var waveDirector)
            && waveDirector?.HasWaveForDay(_context.State.DayIndex) == true)
        {
            return;
        }

        Spawn(ContentId.From("human_grunt"), new Vector3(0f, 0f, -12f));
    }

    private static Vector3 GetSiegeSlotGoal(EnemyState enemy, Vector3 target)
    {
        var slot = Math.Max(0, enemy.EntityId.Value);
        var column = slot % SiegeSlotColumnCount;
        var row = slot / SiegeSlotColumnCount;
        var centeredColumn = column - (SiegeSlotColumnCount - 1) * 0.5f;
        var angle = centeredColumn * 0.18f;
        var radius = 1.15f + row * SiegeSlotSpacing;
        var offset = new Vector3(MathF.Sin(angle) * radius, 0f, MathF.Cos(angle) * radius);
        return target + offset;
    }
}
