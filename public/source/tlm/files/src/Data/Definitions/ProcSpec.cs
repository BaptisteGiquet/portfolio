using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class ProcSpec : Resource
{
    [Export] public ProcTrigger Trigger { get; set; }

    [Export] public string EffectType { get; set; } = string.Empty;

    [Export] public float Chance { get; set; } = 1f;

    [Export] public float Value { get; set; }

    [Export] public string TargetTag { get; set; } = string.Empty;
}
