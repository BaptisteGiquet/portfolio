using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class ModifierSpec : Resource
{
    [Export] public string StatId { get; set; } = string.Empty;

    [Export] public DataModifierOp Operation { get; set; }

    [Export] public float Value { get; set; }

    [Export] public int Priority { get; set; }

    [Export] public string TargetTag { get; set; } = string.Empty;
}
