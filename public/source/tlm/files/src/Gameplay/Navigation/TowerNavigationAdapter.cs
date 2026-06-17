using Godot;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Navigation;

public sealed class TowerNavigationAdapter : IGameSystem
{
    private const float EntranceAdvanceRadiusSquared = 36f;
    private const float StairAdvanceRadiusSquared = 20.25f;

    private readonly Node3D? _worldRoot;
    private RunContext? _context;
    private Node3D? _routeDebugNode;
    private MeshInstance3D? _goalMarker;
    private TowerRouteStep _lastSampledRouteStep = TowerRouteStep.Entrance;

    public TowerEntranceState EntranceState { get; private set; } = TowerEntranceState.Intact;

    public TowerRouteState RouteState { get; private set; } = TowerRouteState.MageOutside;

    public Vector3 EntrancePosition { get; private set; } = new(0f, 0f, -5f);

    public Vector3 StairPosition { get; private set; } = new(0f, 2.25f, 0f);

    public Vector3 MageBasePosition { get; private set; } = new(0f, 4.5f, 5f);

    public TowerNavigationAdapter(Node3D? worldRoot)
    {
        _worldRoot = worldRoot;
    }

    public void Initialize(RunContext context)
    {
        _context = context;
        CacheMarkers();
        BuildRouteDebugVisuals();
    }

    public void Tick(float delta)
    {
        if (_context == null)
        {
            return;
        }

        var playerPosition = _context.State.Player.Position;
        RouteState = IsAtTowerTop(playerPosition) ? TowerRouteState.MageAtTop : TowerRouteState.MageOutside;
        _context.State.DebugMetrics.TowerRouteState = RouteState.ToString();
        _context.State.DebugMetrics.TowerEntranceState = EntranceState.ToString();
        _context.State.DebugMetrics.TowerRouteSummary = BuildRouteSummary(playerPosition);
    }

    public void FixedTick(float delta)
    {
    }

    public void Shutdown()
    {
        _context = null;
    }

    public Vector3 GetGoalForEnemy(Vector3 enemyPosition, Vector3 magePosition)
    {
        return GetGoalForEnemy(enemyPosition, magePosition, TowerRouteStep.Entrance, out _, out _);
    }

    public Vector3 GetGoalForEnemy(
        Vector3 enemyPosition,
        Vector3 magePosition,
        TowerRouteStep currentStep,
        out TowerRouteStep nextStep,
        out string reason)
    {
        if (_context != null)
        {
            _context.State.DebugMetrics.PathQueriesThisFrame++;
        }

        if (RouteState == TowerRouteState.MageOutside)
        {
            MoveGoalMarker(magePosition);
            nextStep = TowerRouteStep.MageBase;
            reason = "mage outside: direct chase";
            return magePosition;
        }

        nextStep = CorrectRouteStepForPosition(enemyPosition, currentStep);
        if (nextStep == TowerRouteStep.Entrance && enemyPosition.DistanceSquaredTo(EntrancePosition) <= EntranceAdvanceRadiusSquared)
        {
            nextStep = TowerRouteStep.Stairs;
        }

        if (nextStep == TowerRouteStep.Stairs && enemyPosition.DistanceSquaredTo(StairPosition) <= StairAdvanceRadiusSquared)
        {
            nextStep = TowerRouteStep.MageBase;
        }

        if (nextStep == TowerRouteStep.Entrance)
        {
            MoveGoalMarker(EntrancePosition);
            _lastSampledRouteStep = nextStep;
            reason = $"target entrance dist={enemyPosition.DistanceTo(EntrancePosition):0.##}";
            return EntrancePosition;
        }

        if (nextStep == TowerRouteStep.Stairs)
        {
            MoveGoalMarker(StairPosition);
            _lastSampledRouteStep = nextStep;
            reason = $"target stairs dist={enemyPosition.DistanceTo(StairPosition):0.##}";
            return StairPosition;
        }

        var goal = magePosition.DistanceSquaredTo(MageBasePosition) < 36f ? magePosition : MageBasePosition;
        MoveGoalMarker(goal);
        _lastSampledRouteStep = nextStep;
        reason = $"target mage base dist={enemyPosition.DistanceTo(goal):0.##}";
        return goal;
    }

    private TowerRouteStep CorrectRouteStepForPosition(Vector3 enemyPosition, TowerRouteStep currentStep)
    {
        var beforeEntrance = enemyPosition.Z < EntrancePosition.Z - 1.5f;
        if (currentStep == TowerRouteStep.MageBase
            && beforeEntrance
            && enemyPosition.DistanceSquaredTo(StairPosition) > StairAdvanceRadiusSquared)
        {
            return enemyPosition.DistanceSquaredTo(EntrancePosition) <= EntranceAdvanceRadiusSquared
                ? TowerRouteStep.Stairs
                : TowerRouteStep.Entrance;
        }

        if (currentStep == TowerRouteStep.Stairs
            && beforeEntrance
            && enemyPosition.DistanceSquaredTo(EntrancePosition) > EntranceAdvanceRadiusSquared)
        {
            return TowerRouteStep.Entrance;
        }

        return currentStep;
    }

    public void BreachEntrance()
    {
        EntranceState = TowerEntranceState.Breached;
    }

    private bool IsAtTowerTop(Vector3 position)
    {
        return position.Y >= MageBasePosition.Y - 1.25f && position.DistanceSquaredTo(MageBasePosition) <= 64f;
    }

    private void CacheMarkers()
    {
        if (_worldRoot == null)
        {
            return;
        }

        EntrancePosition = ReadMarker("Tower/Entrance", EntrancePosition);
        StairPosition = ReadMarker("Tower/StairPath", StairPosition);
        MageBasePosition = ReadMarker("Tower/MageBase", MageBasePosition);
    }

    private Vector3 ReadMarker(string path, Vector3 fallback)
    {
        var marker = _worldRoot?.GetNodeOrNull<Node3D>(path);
        return marker?.GlobalPosition ?? fallback;
    }

    private string BuildRouteSummary(Vector3 playerPosition)
    {
        return RouteState == TowerRouteState.MageOutside
            ? $"outside -> mage {Format(playerPosition)}"
            : $"top route sample={_lastSampledRouteStep} entrance {Format(EntrancePosition)} -> stairs {Format(StairPosition)} -> base {Format(MageBasePosition)}";
    }

    private void BuildRouteDebugVisuals()
    {
        if (_worldRoot == null)
        {
            return;
        }

        _routeDebugNode?.QueueFree();
        _routeDebugNode = new Node3D { Name = "TowerRouteDebug" };
        _worldRoot.AddChild(_routeDebugNode);

        BuildTowerProxy();
        AddRouteMarker("RouteEntrance", EntrancePosition, new Color(0.2f, 0.6f, 1f, 0.8f));
        AddRouteLabel("Tower entrance", EntrancePosition + new Vector3(0f, 0.75f, 0f), new Color(0.65f, 0.85f, 1f, 1f));
        AddRouteMarker("RouteStairs", StairPosition, new Color(0.55f, 0.9f, 1f, 0.8f));
        AddRouteLabel("Stairs route", StairPosition + new Vector3(0f, 0.75f, 0f), new Color(0.65f, 1f, 1f, 1f));
        AddRouteMarker("RouteMageBase", MageBasePosition, new Color(0.95f, 0.95f, 0.2f, 0.8f));
        AddRouteLabel("Mage base / top safe area", MageBasePosition + new Vector3(0f, 0.85f, 0f), new Color(1f, 1f, 0.45f, 1f));
        AddRouteLine("RouteEntranceToStairs", EntrancePosition, StairPosition, new Color(0.2f, 0.7f, 1f, 0.35f));
        AddRouteLine("RouteStairsToBase", StairPosition, MageBasePosition, new Color(0.8f, 0.9f, 0.2f, 0.35f));
        _goalMarker = AddRouteMarker("CurrentEnemyGoal", EntrancePosition, new Color(1f, 0.2f, 0.2f, 0.9f));
        AddRouteLabel("Red sphere = current sampled enemy goal", EntrancePosition + new Vector3(0f, 1.35f, 0f), new Color(1f, 0.5f, 0.45f, 1f));
    }

    private void BuildTowerProxy()
    {
        if (_routeDebugNode == null)
        {
            return;
        }

        AddProxyBox(
            "DebugTowerBody",
            new Vector3(0f, 2.25f, 1.4f),
            new Vector3(5.5f, 4.5f, 8.5f),
            new Color(0.3f, 0.35f, 0.42f, 0.18f));
        AddProxyBox(
            "DebugEntranceGap",
            EntrancePosition + new Vector3(0f, 0.8f, 0f),
            new Vector3(2.8f, 1.6f, 0.35f),
            new Color(0.2f, 0.6f, 1f, 0.28f));
        AddProxyBox(
            "DebugStairRamp",
            (EntrancePosition + StairPosition) * 0.5f + new Vector3(0f, 0.35f, 0f),
            new Vector3(1.15f, 0.28f, EntrancePosition.DistanceTo(StairPosition)),
            new Color(0.55f, 0.9f, 1f, 0.3f));
    }

    private void AddProxyBox(string name, Vector3 position, Vector3 size, Color color)
    {
        var mesh = new MeshInstance3D
        {
            Name = name,
            Position = position,
            Mesh = new BoxMesh { Size = size },
            MaterialOverride = CreateMaterial(color)
        };
        _routeDebugNode?.AddChild(mesh);
    }

    private MeshInstance3D AddRouteMarker(string name, Vector3 position, Color color)
    {
        var marker = new MeshInstance3D
        {
            Name = name,
            Position = position,
            Mesh = new SphereMesh { Radius = 0.28f, Height = 0.56f },
            MaterialOverride = CreateMaterial(color)
        };
        _routeDebugNode?.AddChild(marker);
        return marker;
    }

    private void AddRouteLine(string name, Vector3 start, Vector3 end, Color color)
    {
        var length = start.DistanceTo(end);
        if (_routeDebugNode == null || length <= 0.001f)
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
        _routeDebugNode.AddChild(line);
        line.LookAt(end, Vector3.Up);
    }

    private void AddRouteLabel(string text, Vector3 position, Color color)
    {
        if (_routeDebugNode == null)
        {
            return;
        }

        _routeDebugNode.AddChild(new Label3D
        {
            Name = $"Label_{text.Replace(' ', '_').Replace('/', '_')}",
            Text = text,
            Position = position,
            FontSize = 28,
            Modulate = color,
            OutlineSize = 6,
            Billboard = BaseMaterial3D.BillboardModeEnum.Enabled
        });
    }

    private void MoveGoalMarker(Vector3 position)
    {
        if (_goalMarker != null)
        {
            _goalMarker.Position = position;
        }
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

    private static string Format(Vector3 value)
    {
        return $"({value.X:0.#},{value.Y:0.#},{value.Z:0.#})";
    }
}
