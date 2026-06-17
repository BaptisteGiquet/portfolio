using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class WaveSpawnGroupDefinition : Resource
{
    [Export] public string EnemyId { get; set; } = string.Empty;

    [Export] public int Count { get; set; } = 1;

    [Export] public float StartTimeSeconds { get; set; }

    [Export] public float SpawnIntervalSeconds { get; set; } = 1f;

    [Export] public float IntensityMultiplier { get; set; } = 1f;
}
