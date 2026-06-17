using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class WaveSpawnLaneDefinition : Resource
{
    [Export] public string Id { get; set; } = string.Empty;

    [Export] public Vector3 SpawnPosition { get; set; } = new(0f, 0f, -24f);

    [Export] public Vector3 ApproachPosition { get; set; } = new(0f, 0f, -8f);

    [Export] public float Weight { get; set; } = 1f;
}
