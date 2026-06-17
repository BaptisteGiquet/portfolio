using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class WaveDefinition : ContentDefinition
{
    [Export] public int DayIndex { get; set; } = 1;

    [Export] public Godot.Collections.Array<string> EnemyIds { get; set; } = new();

    [Export] public Godot.Collections.Array<WaveSpawnGroupDefinition> SpawnGroups { get; set; } = new();

    [Export] public Godot.Collections.Array<WaveSpawnLaneDefinition> SpawnLanes { get; set; } = new();

    [Export] public float DurationSeconds { get; set; } = 60f;

    [Export] public int TargetActiveEnemies { get; set; } = 12;

    [Export] public float SpawnIntervalSeconds { get; set; } = 1.0f;

    [Export] public float Intensity { get; set; } = 1f;
}
