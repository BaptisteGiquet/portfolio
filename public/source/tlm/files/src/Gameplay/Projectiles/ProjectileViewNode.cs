using Godot;

namespace TheLastMage.Gameplay.Projectiles;

public partial class ProjectileViewNode : Node3D
{
    public const float MeshRadius = 0.16f;

    public override void _Ready()
    {
        if (GetChildCount() > 0)
        {
            return;
        }

        var mesh = new MeshInstance3D
        {
            Mesh = new SphereMesh
            {
                Radius = MeshRadius,
                Height = MeshRadius * 2f
            }
        };
        AddChild(mesh);
    }
}
