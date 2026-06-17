using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class EffectSpec : Resource
{
    [Export] public string EffectType { get; set; } = string.Empty;

    [Export] public float Value { get; set; }

    [Export] public float Radius { get; set; }

    [Export] public float DurationSeconds { get; set; }

    [Export] public DurationScalingMode DurationScaling { get; set; } = DurationScalingMode.PlayerDuration;

    [Export] public string TargetStatId { get; set; } = string.Empty;
}
