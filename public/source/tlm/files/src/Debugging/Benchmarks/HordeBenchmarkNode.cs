using Godot;
using System.Globalization;
using System.IO;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Enemies;
using TheLastMage.Gameplay.Navigation;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Waves;

namespace TheLastMage.Debugging.Benchmarks;

public partial class HordeBenchmarkNode : Node
{
    private bool _started;
    private RunContext? _context;
    private string _reportPath = string.Empty;
    private double _elapsedSeconds;
    private double _logTimer;

    [Export] public int TargetEnemyCount { get; set; } = 50;

    [Export] public bool PlaceTowerPressureDefenses { get; set; } = true;

    [Export] public float RouteLogIntervalSeconds { get; set; } = 5f;

    public override void _Process(double delta)
    {
        if (_started)
        {
            TickRouteReport(delta);
            return;
        }

        var controller = GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        _context = controller?.Context;
        if (_context == null)
        {
            return;
        }

        _started = true;
        _context.State.IsBenchmarkRun = true;
        _context.State.Player.Position = new Vector3(0f, 4.5f, 5f);
        InitializeRouteReport();
        if (_context.TryGetSystem<TowerNavigationAdapter>(out var tower) && tower != null)
        {
            _context.State.DebugMetrics.TowerRouteState = tower.RouteState.ToString();
        }

        if (PlaceTowerPressureDefenses && _context.TryGetSystem<DefenseSystem>(out var defenses) && defenses != null)
        {
            defenses.PlaceDefense(ContentId.From("barricade"), new Vector3(0f, 0f, -5f));
            defenses.PlaceDefense(ContentId.From("explosive_seal"), new Vector3(0f, 0f, -8f));
        }

        _context.GetSystem<WaveDirectorSystem>().StartBenchmarkWave(TargetEnemyCount);
        _context.GetSystem<WaveDirectorSystem>().SpawnUntilTarget();
        AppendRouteReport($"START target={TargetEnemyCount} defenses={PlaceTowerPressureDefenses} player={Format(_context.State.Player.Position)}");
        AppendRouteSnapshot(forceSamples: true);
        GD.Print($"HordeBenchmark started target={TargetEnemyCount} report={_reportPath}");
    }

    private void TickRouteReport(double delta)
    {
        if (_context == null)
        {
            return;
        }

        _elapsedSeconds += delta;
        _logTimer -= delta;
        if (_logTimer > 0.0)
        {
            return;
        }

        _logTimer = Math.Max(1f, RouteLogIntervalSeconds);
        AppendRouteSnapshot(forceSamples: false);
    }

    private void InitializeRouteReport()
    {
        var directory = RuntimePathResolver.GlobalizeUserPath("user://benchmark_reports");
        Directory.CreateDirectory(directory);
        _reportPath = Path.Combine(directory, $"tower_route_{DateTime.Now:yyyyMMdd_HHmmss}.log");
        _context!.State.DebugMetrics.BenchmarkReportPath = _reportPath;
        File.WriteAllText(_reportPath, "The Last Mage tower route benchmark report\n");
    }

    private void AppendRouteSnapshot(bool forceSamples)
    {
        if (_context == null)
        {
            return;
        }

        var enemies = _context.GetSystem<EnemySystem>().ActiveEnemies;
        var tower = _context.TryGetSystem<TowerNavigationAdapter>(out var towerSystem) ? towerSystem : null;
        var defenses = _context.TryGetSystem<DefenseSystem>(out var defenseSystem) ? defenseSystem : null;
        var combat = _context.GetSystem<CombatSystem>();

        var entrance = 0;
        var stairs = 0;
        var baseStep = 0;
        var alive = 0;
        var defenseTargets = 0;
        var mageTargets = 0;
        var distanceEntrance = 0f;
        var distanceStairs = 0f;
        var distanceBase = 0f;
        var sampleLines = new List<string>(8);

        foreach (var enemy in enemies)
        {
            if (!combat.IsAlive(enemy.EntityId))
            {
                continue;
            }

            alive++;
            switch (enemy.TowerRouteStep)
            {
                case TowerRouteStep.Entrance:
                    entrance++;
                    break;
                case TowerRouteStep.Stairs:
                    stairs++;
                    break;
                case TowerRouteStep.MageBase:
                    baseStep++;
                    break;
            }

            if (enemy.CurrentGoal.DistanceSquaredTo(_context.State.Player.Position) < 0.25f)
            {
                mageTargets++;
            }
            else if (defenses?.Defenses.Any(defense => defense.Position.DistanceSquaredTo(enemy.CurrentGoal) < 0.25f) == true)
            {
                defenseTargets++;
            }

            if (tower != null)
            {
                distanceEntrance += enemy.Position.DistanceTo(tower.EntrancePosition);
                distanceStairs += enemy.Position.DistanceTo(tower.StairPosition);
                distanceBase += enemy.Position.DistanceTo(tower.MageBasePosition);
            }

            if (sampleLines.Count < 6 && (forceSamples || enemy.EntityId.Value % 8 == 0))
            {
                sampleLines.Add(
                    $"  enemy={enemy.EntityId} role={enemy.Role} pos={Format(enemy.Position)} step={enemy.TowerRouteStep} " +
                    $"goal={Format(enemy.CurrentGoal)} slot={Format(enemy.CurrentSlotGoal)} decision=\"{enemy.RouteDecision}\"");
            }
        }

        var denominator = Math.Max(1, alive);
        var stageSummary = $"entrance={entrance} stairs={stairs} base={baseStep} defenseTargets={defenseTargets} mageTargets={mageTargets}";
        _context.State.DebugMetrics.TowerRouteStageSummary = stageSummary;

        AppendRouteReport(
            $"t={_elapsedSeconds:0.0}s alive={alive}/{enemies.Count} {stageSummary} " +
            $"avgDist entrance={distanceEntrance / denominator:0.##} stairs={distanceStairs / denominator:0.##} base={distanceBase / denominator:0.##} " +
            $"tower={_context.State.DebugMetrics.TowerRouteState}/{_context.State.DebugMetrics.TowerEntranceState} " +
            $"activeDefenses={defenses?.ActiveCount ?? 0}");
        foreach (var line in sampleLines)
        {
            AppendRouteReport(line);
        }
    }

    private void AppendRouteReport(string line)
    {
        if (!string.IsNullOrWhiteSpace(_reportPath))
        {
            File.AppendAllLines(_reportPath, new[] { line });
        }
    }

    private static string Format(Vector3 value)
    {
        var culture = CultureInfo.InvariantCulture;
        return string.Create(culture, $"({value.X:0.##},{value.Y:0.##},{value.Z:0.##})");
    }
}
