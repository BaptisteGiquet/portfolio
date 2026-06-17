using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Debugging.Benchmarks;

public partial class BenchmarkDefensePlacementNode : Node3D
{
    private bool _placed;

    [Export] public string DefenseId { get; set; } = "explosive_seal";

    public override void _Process(double delta)
    {
        if (_placed)
        {
            return;
        }

        var controller = GetTree().Root.FindChild("RunControllerNode", true, false) as RunControllerNode;
        var context = controller?.Context;
        if (context == null)
        {
            return;
        }

        _placed = true;
        context.GetSystem<DefenseSystem>().PlaceDefense(ContentId.From(DefenseId), GlobalPosition);
        GD.Print($"BenchmarkDefensePlacement placed defense={DefenseId} position={GlobalPosition}");
    }
}
