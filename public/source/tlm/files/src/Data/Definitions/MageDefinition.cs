using Godot;

namespace TheLastMage.Data.Definitions;

[Tool]
[GlobalClass]
public partial class MageDefinition : ContentDefinition
{
    [Export] public float MaxHealth { get; set; } = 100f;

    [Export] public float MoveSpeed { get; set; } = 5f;

    [Export] public float FireRate { get; set; } = 1f;

    [Export] public float Luck { get; set; }

    [Export] public PackedScene? PresentationScene { get; set; }

    [Export] public Godot.Collections.Array<string> StartingSpellIds { get; set; } = new();
}
