using Godot;

namespace TheLastMage.Presentation.Feedback;

public partial class BeamDebugNode : Node3D
{
    private float _lifetimeRemaining;

    public void Bind(Vector3 start, Vector3 end, float radius)
    {
        _lifetimeRemaining = 0.09f;
        var midpoint = (start + end) * 0.5f;
        var direction = end - start;
        var length = direction.Length();
        GlobalPosition = midpoint;

        if (length > 0.001f)
        {
            LookAt(end, Vector3.Up);
        }

        AddChild(new MeshInstance3D
        {
            Mesh = new CylinderMesh
            {
                TopRadius = MathF.Max(0.035f, radius * 0.12f),
                BottomRadius = MathF.Max(0.035f, radius * 0.12f),
                Height = MathF.Max(0.1f, length)
            },
            Rotation = new Vector3(Mathf.Pi * 0.5f, 0f, 0f),
            MaterialOverride = new StandardMaterial3D
            {
                AlbedoColor = new Color(0.65f, 0.25f, 1f, 0.82f),
                EmissionEnabled = true,
                Emission = new Color(0.65f, 0.25f, 1f),
                EmissionEnergyMultiplier = 1.6f,
                Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
            }
        });
    }

    public override void _Process(double delta)
    {
        _lifetimeRemaining -= (float)delta;
        if (_lifetimeRemaining <= 0f)
        {
            QueueFree();
        }
    }
}
