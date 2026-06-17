using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Foundation.Random;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Waves;

public sealed class WaveDirectorSystem : IGameSystem
{
    private const float EndlessIntensityGrowthPerDay = 0.12f;
    private const float EndlessConcurrentCapGrowthPerDay = 0.15f;

    private sealed class RuntimeSpawnGroup
    {
        public required ContentId EnemyId { get; init; }

        public required int Remaining { get; set; }

        public required float StartTimeSeconds { get; init; }

        public required float SpawnIntervalSeconds { get; init; }

        public float Timer { get; set; }
    }

    private readonly Node3D? _debugRoot;
    private readonly List<RuntimeSpawnGroup> _spawnGroups = new();
    private Node3D? _laneDebugNode;
    private RunContext? _context;
    private WaveRuntimeDefinition? _activeWave;
    private WaveSpawnLaneRuntimeDefinition[] _activeLanes = Array.Empty<WaveSpawnLaneRuntimeDefinition>();
    private float _spawnTimer;
    private float _elapsedSeconds;
    private int _spawnedThisDay;
    private int _spawnBudgetThisDay;
    private SubscriptionToken _dayStartedToken;

    public bool HasActiveWave => _activeWave != null;

    public WaveDirectorSystem(Node3D? debugRoot = null)
    {
        _debugRoot = debugRoot;
    }

    public bool HasWaveForDay(int dayIndex)
    {
        return _context != null && ResolveWaveForDay(dayIndex) != null;
    }

    public bool IsActiveWaveCleared
    {
        get
        {
            if (_context == null || _activeWave == null)
            {
                return false;
            }

            return _spawnedThisDay >= GetSpawnBudget()
                && _context.GetSystem<EnemySystem>().ActiveCount == 0;
        }
    }

    public void Initialize(RunContext context)
    {
        _context = context;
        _dayStartedToken = context.Events.Subscribe<DayStartedEvent>(OnDayStarted);
    }

    public void Tick(float delta)
    {
        if (_context == null || _activeWave == null || !_context.State.PhasePermissions.EnemySpawningActive)
        {
            return;
        }

        _elapsedSeconds += delta;
        _spawnTimer -= delta;
        var enemySystem = _context.GetSystem<EnemySystem>();

        var spawnedFromGroups = false;
        foreach (var group in _spawnGroups)
        {
            if (_elapsedSeconds < group.StartTimeSeconds || group.Remaining <= 0)
            {
                continue;
            }

            group.Timer -= delta;
            if (group.Timer > 0f || !CanSpawn(enemySystem) || !CanSpawnArchetype(enemySystem, group.EnemyId))
            {
                continue;
            }

            spawnedFromGroups = SpawnOne(enemySystem, group.EnemyId);
            if (spawnedFromGroups)
            {
                group.Remaining--;
                group.Timer = group.SpawnIntervalSeconds;
            }
        }

        if (!spawnedFromGroups
            && _spawnGroups.Count > 0
            && enemySystem.ActiveCount == 0
            && CanSpawn(enemySystem)
            && TrySpawnNextPendingGroup(enemySystem))
        {
            spawnedFromGroups = true;
        }

        if (!spawnedFromGroups
            && _spawnGroups.Count == 0
            && _spawnTimer <= 0f
            && CanSpawn(enemySystem))
        {
            SpawnOne(enemySystem, null);
            _spawnTimer = _activeWave.SpawnIntervalSeconds;
        }

        _context.State.DebugMetrics.ActiveWaveId = _activeWave.Id.ToString();
        _context.State.DebugMetrics.WaveElapsedSeconds = _elapsedSeconds;
        _context.State.DebugMetrics.WaveTargetActiveEnemies = GetSpawnBudget();
        _context.State.DebugMetrics.WaveSpawnedThisDay = _spawnedThisDay;
        _context.State.DebugMetrics.SpawnLaneSummary = BuildLaneSummary();
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        if (_context != null)
        {
            _context.Events.Unsubscribe(_dayStartedToken);
        }

        _context = null;
        _activeWave = null;
        _activeLanes = Array.Empty<WaveSpawnLaneRuntimeDefinition>();
        _spawnGroups.Clear();
        _spawnBudgetThisDay = 0;
        _laneDebugNode?.QueueFree();
        _laneDebugNode = null;
    }

    public void StartBenchmarkWave(int targetActiveEnemies)
    {
        if (_context == null)
        {
            return;
        }

        var enemyIds = _context.Content.Enemies.Keys.ToArray();
        if (enemyIds.Length == 0)
        {
            return;
        }

        _activeWave = new WaveRuntimeDefinition(
            ContentId.From($"benchmark_{targetActiveEnemies}"),
            $"{targetActiveEnemies} Enemy Benchmark",
            _context.State.DayIndex,
            600f,
            targetActiveEnemies,
            0.01f,
            1f,
            enemyIds,
            Array.Empty<WaveSpawnGroupRuntimeDefinition>(),
            BuildBenchmarkLanes());
        ActivateWave(_activeWave);
        _spawnTimer = 0f;
        _spawnedThisDay = 0;
        _spawnBudgetThisDay = targetActiveEnemies;
    }

    public void SpawnUntilTarget()
    {
        if (_context == null || _activeWave == null)
        {
            return;
        }

        var enemySystem = _context.GetSystem<EnemySystem>();
        while (_spawnedThisDay < GetSpawnBudget()
            && enemySystem.ActiveCount < _activeWave.TargetActiveEnemies)
        {
            SpawnOne(enemySystem, null);
        }
    }

    private void OnDayStarted(DayStartedEvent gameEvent)
    {
        if (_context == null)
        {
            return;
        }

        if (_context.State.SuppressAutomaticEnemySpawns)
        {
            _activeWave = null;
            _activeLanes = Array.Empty<WaveSpawnLaneRuntimeDefinition>();
            _spawnGroups.Clear();
            _spawnBudgetThisDay = 0;
            _spawnedThisDay = 0;
            _elapsedSeconds = 0f;
            _context.State.DebugMetrics.ActiveWaveId = "manual_spawn";
            _context.State.DebugMetrics.SpawnLaneSummary = "manual sandbox spawns";
            return;
        }

        _activeWave = ResolveWaveForDay(gameEvent.DayIndex);
        if (_activeWave != null)
        {
            ActivateWave(_activeWave);
        }

        _spawnTimer = 0f;
        _spawnedThisDay = 0;
        _spawnBudgetThisDay = _activeWave == null
            ? 0
            : CapSpawnBudgetToHumanity(CalculateSpawnBudget(_activeWave));
        _elapsedSeconds = 0f;
    }

    private bool CanSpawn(EnemySystem enemySystem)
    {
        return _activeWave != null
            && _spawnedThisDay < GetSpawnBudget()
            && enemySystem.ActiveCount < _activeWave.TargetActiveEnemies;
    }

    private bool SpawnOne(EnemySystem enemySystem, ContentId? requestedEnemyId)
    {
        if (_context == null || _activeWave == null || _activeWave.EnemyIds.Count == 0)
        {
            return false;
        }

        var enemyId = requestedEnemyId ?? PickFallbackEnemy(enemySystem);
        if (!CanSpawnArchetype(enemySystem, enemyId))
        {
            return false;
        }

        var lane = PickLane();
        var spawnPosition = lane?.SpawnPosition ?? GetSpawnPosition(_spawnedThisDay);
        var entityId = enemySystem.Spawn(enemyId, spawnPosition);
        if (entityId.IsValid)
        {
            _spawnedThisDay++;
            _context.State.DebugMetrics.LastSpawnLaneId = lane?.Id ?? "golden_angle";
            _context.Events.Publish(new WaveSpawnedEnemyEvent(_activeWave.Id, enemyId, entityId));
            return true;
        }

        return false;
    }

    private bool TrySpawnNextPendingGroup(EnemySystem enemySystem)
    {
        RuntimeSpawnGroup? nextGroup = null;
        for (var i = 0; i < _spawnGroups.Count; i++)
        {
            var group = _spawnGroups[i];
            if (group.Remaining <= 0)
            {
                continue;
            }

            if (nextGroup == null || group.StartTimeSeconds < nextGroup.StartTimeSeconds)
            {
                nextGroup = group;
            }
        }

        if (nextGroup == null
            || !CanSpawnArchetype(enemySystem, nextGroup.EnemyId)
            || !SpawnOne(enemySystem, nextGroup.EnemyId))
        {
            return false;
        }

        nextGroup.Remaining--;
        nextGroup.Timer = nextGroup.SpawnIntervalSeconds;
        return true;
    }

    private ContentId PickFallbackEnemy(EnemySystem enemySystem)
    {
        if (_context == null || _activeWave == null || _activeWave.EnemyIds.Count == 0)
        {
            return ContentId.From(string.Empty);
        }

        var start = _context.Random.Stream(RandomStreamId.Waves).Next(_activeWave.EnemyIds.Count);
        for (var offset = 0; offset < _activeWave.EnemyIds.Count; offset++)
        {
            var candidate = _activeWave.EnemyIds[(start + offset) % _activeWave.EnemyIds.Count];
            if (CanSpawnArchetype(enemySystem, candidate))
            {
                return candidate;
            }
        }

        return _activeWave.EnemyIds[start];
    }

    private bool CanSpawnArchetype(EnemySystem enemySystem, ContentId enemyId)
    {
        if (_context == null || !_context.Content.TryGetEnemy(enemyId, out var definition) || definition == null)
        {
            return false;
        }

        return definition.MaxActive <= 0 || enemySystem.CountActive(enemyId) < definition.MaxActive;
    }

    private WaveSpawnLaneRuntimeDefinition? PickLane()
    {
        if (_context == null || _activeLanes.Length == 0)
        {
            return null;
        }

        var totalWeight = 0f;
        for (var i = 0; i < _activeLanes.Length; i++)
        {
            totalWeight += MathF.Max(0f, _activeLanes[i].Weight);
        }

        if (totalWeight <= 0f)
        {
            return _activeLanes[0];
        }

        var roll = (float)_context.Random.Stream(RandomStreamId.Waves).NextDouble() * totalWeight;
        for (var i = 0; i < _activeLanes.Length; i++)
        {
            roll -= MathF.Max(0f, _activeLanes[i].Weight);
            if (roll <= 0f)
            {
                return _activeLanes[i];
            }
        }

        return _activeLanes[^1];
    }

    private void ActivateWave(WaveRuntimeDefinition wave)
    {
        _activeLanes = wave.SpawnLanes.Count > 0 ? wave.SpawnLanes.ToArray() : BuildBenchmarkLanes();
        _spawnGroups.Clear();
        _spawnBudgetThisDay = 0;
        foreach (var group in wave.SpawnGroups)
        {
            var scaledCount = Math.Max(1, (int)MathF.Ceiling(group.Count * wave.Intensity * group.IntensityMultiplier));
            _spawnBudgetThisDay += scaledCount;
            _spawnGroups.Add(new RuntimeSpawnGroup
            {
                EnemyId = group.EnemyId,
                Remaining = scaledCount,
                StartTimeSeconds = group.StartTimeSeconds,
                SpawnIntervalSeconds = MathF.Max(0.05f, group.SpawnIntervalSeconds / MathF.Max(0.1f, wave.Intensity)),
                Timer = 0f
            });
        }

        if (_spawnGroups.Count == 0)
        {
            _spawnBudgetThisDay = wave.TargetActiveEnemies;
        }

        BuildLaneDebugVisuals();
    }

    private int GetSpawnBudget()
    {
        return _spawnBudgetThisDay > 0 ? _spawnBudgetThisDay : _activeWave?.TargetActiveEnemies ?? 0;
    }

    private int CapSpawnBudgetToHumanity(int spawnBudget)
    {
        if (_context == null || _context.State.IsBenchmarkRun)
        {
            return spawnBudget;
        }

        return Math.Clamp(spawnBudget, 0, Math.Max(0, _context.State.HumanityRemaining));
    }

    private static int CalculateSpawnBudget(WaveRuntimeDefinition wave)
    {
        if (wave.SpawnGroups.Count == 0)
        {
            return wave.TargetActiveEnemies;
        }

        var budget = 0;
        foreach (var group in wave.SpawnGroups)
        {
            budget += Math.Max(1, (int)MathF.Ceiling(group.Count * wave.Intensity * group.IntensityMultiplier));
        }

        return budget;
    }

    private static Vector3 GetSpawnPosition(int index)
    {
        const float radius = 24f;
        var angle = index * 2.3999631f;
        return new Vector3(MathF.Cos(angle) * radius, 0f, MathF.Sin(angle) * radius);
    }

    private WaveRuntimeDefinition? ResolveWaveForDay(int dayIndex)
    {
        if (_context == null)
        {
            return null;
        }

        var exactWave = _context.Content.Waves.Values.FirstOrDefault(wave => wave.DayIndex == dayIndex);
        if (exactWave != null)
        {
            return exactWave;
        }

        var latestWave = _context.Content.Waves.Values
            .Where(wave => wave.DayIndex < dayIndex)
            .OrderByDescending(wave => wave.DayIndex)
            .FirstOrDefault();
        if (latestWave == null)
        {
            return null;
        }

        var extraDays = Math.Max(1, dayIndex - latestWave.DayIndex);
        var intensityMultiplier = 1f + EndlessIntensityGrowthPerDay * extraDays;
        var capMultiplier = 1f + EndlessConcurrentCapGrowthPerDay * extraDays;
        return new WaveRuntimeDefinition(
            ContentId.From($"{latestWave.Id.Value}_repeat_day_{dayIndex:000}"),
            $"{latestWave.DisplayName} Repeat Day {dayIndex}",
            dayIndex,
            latestWave.DurationSeconds,
            Math.Max(1, (int)MathF.Ceiling(latestWave.TargetActiveEnemies * capMultiplier)),
            latestWave.SpawnIntervalSeconds,
            latestWave.Intensity * intensityMultiplier,
            latestWave.EnemyIds,
            latestWave.SpawnGroups,
            latestWave.SpawnLanes);
    }

    private static WaveSpawnLaneRuntimeDefinition[] BuildBenchmarkLanes()
    {
        return new[]
        {
            new WaveSpawnLaneRuntimeDefinition("north_road", new Vector3(0f, 0f, -24f), new Vector3(0f, 0f, -8f), 1f),
            new WaveSpawnLaneRuntimeDefinition("west_ridge", new Vector3(-22f, 0f, -14f), new Vector3(-6f, 0f, -7f), 0.8f),
            new WaveSpawnLaneRuntimeDefinition("east_ridge", new Vector3(22f, 0f, -14f), new Vector3(6f, 0f, -7f), 0.8f)
        };
    }

    private string BuildLaneSummary()
    {
        if (_activeLanes.Length == 0)
        {
            return "-";
        }

        return string.Join(", ", _activeLanes.Select(lane => $"{lane.Id}->{FormatVector(lane.ApproachPosition)}"));
    }

    private void BuildLaneDebugVisuals()
    {
        if (_debugRoot == null)
        {
            return;
        }

        _laneDebugNode?.QueueFree();
        _laneDebugNode = new Node3D { Name = "WaveLaneDebug" };
        _debugRoot.AddChild(_laneDebugNode);

        foreach (var lane in _activeLanes)
        {
            AddDebugSphere($"LaneSpawn_{lane.Id}", lane.SpawnPosition, 0.35f, new Color(0.9f, 0.2f, 0.1f, 0.8f));
            AddDebugLabel($"Spawn lane: {lane.Id}", lane.SpawnPosition + new Vector3(0f, 0.75f, 0f), new Color(1f, 0.55f, 0.45f, 1f));
            AddDebugSphere($"LaneApproach_{lane.Id}", lane.ApproachPosition, 0.25f, new Color(0.15f, 0.85f, 0.25f, 0.8f));
            AddDebugLabel($"Approach: {lane.Id}", lane.ApproachPosition + new Vector3(0f, 0.55f, 0f), new Color(0.45f, 1f, 0.55f, 1f));
            AddDebugLine($"LaneVector_{lane.Id}", lane.SpawnPosition, lane.ApproachPosition, new Color(1f, 0.78f, 0.15f, 0.45f));
        }
    }

    private void AddDebugSphere(string name, Vector3 position, float radius, Color color)
    {
        if (_laneDebugNode == null)
        {
            return;
        }

        _laneDebugNode.AddChild(new MeshInstance3D
        {
            Name = name,
            Position = position,
            Mesh = new SphereMesh { Radius = radius, Height = radius * 2f },
            MaterialOverride = CreateMaterial(color)
        });
    }

    private void AddDebugLine(string name, Vector3 start, Vector3 end, Color color)
    {
        if (_laneDebugNode == null)
        {
            return;
        }

        var length = start.DistanceTo(end);
        if (length <= 0.001f)
        {
            return;
        }

        var line = new MeshInstance3D
        {
            Name = name,
            Position = (start + end) * 0.5f,
            Mesh = new BoxMesh { Size = new Vector3(0.08f, 0.08f, length) },
            MaterialOverride = CreateMaterial(color)
        };
        _laneDebugNode.AddChild(line);
        line.LookAt(end, Vector3.Up);
    }

    private void AddDebugLabel(string text, Vector3 position, Color color)
    {
        if (_laneDebugNode == null)
        {
            return;
        }

        _laneDebugNode.AddChild(new Label3D
        {
            Name = $"Label_{text.Replace(' ', '_').Replace(':', '_')}",
            Text = text,
            Position = position,
            FontSize = 24,
            Modulate = color,
            OutlineSize = 6,
            Billboard = BaseMaterial3D.BillboardModeEnum.Enabled
        });
    }

    private static StandardMaterial3D CreateMaterial(Color color)
    {
        return new StandardMaterial3D
        {
            AlbedoColor = color,
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
            ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
        };
    }

    private static string FormatVector(Vector3 value)
    {
        return $"({value.X:0.#},{value.Y:0.#},{value.Z:0.#})";
    }
}
