using Godot;

namespace TheLastMage.Debugging.Benchmarks;

public partial class BenchmarkCameraNode : Camera3D
{
    [Export] public Vector3 LookAtPoint { get; set; } = new(0f, 1.5f, 0f);

    public override void _Ready()
    {
        Current = true;
        LookAt(LookAtPoint, Vector3.Up);
    }
}
