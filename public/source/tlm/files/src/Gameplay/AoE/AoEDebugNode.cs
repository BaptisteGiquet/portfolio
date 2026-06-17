using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.AoE;

public partial class AoEDebugNode : Node3D
{
    public EntityId BoundEntityId { get; private set; } = EntityId.None;

    public void Bind(EntityId entityId, float radius, Color color, float presentationScale = 1f)
    {
        BoundEntityId = entityId;
        if (GetChildCount() > 0)
        {
            return;
        }

        var gameplayRadius = MathF.Max(0.1f, radius);
        var visualRadius = MathF.Max(gameplayRadius, radius * MathF.Max(0.1f, presentationScale));

        if (!Mathf.IsEqualApprox(visualRadius, radius))
        {
            AddChild(new MeshInstance3D
            {
                Name = "PresentationAura",
                Position = Vector3.Zero,
                Mesh = new CylinderMesh
                {
                    TopRadius = visualRadius,
                    BottomRadius = visualRadius,
                    Height = 0.02f
                },
                MaterialOverride = CreateMaterial(new Color(color.R, color.G, color.B, 0.16f))
            });
        }

        AddChild(new MeshInstance3D
        {
            Name = "RadiusDisk",
            Position = Vector3.Zero,
            Mesh = new CylinderMesh
            {
                TopRadius = radius,
                BottomRadius = radius,
                Height = 0.035f
            },
            MaterialOverride = new StandardMaterial3D
            {
                AlbedoColor = color,
                Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
                ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
            }
        });
    }

    public void BindExpandingRing(EntityId entityId, float startRadius, float thickness, Color color)
    {
        BoundEntityId = entityId;
        if (GetChildCount() > 0)
        {
            return;
        }

        var visual = new MeshInstance3D
        {
            Name = "ExpandingRingDisk",
            Mesh = new CylinderMesh
            {
                TopRadius = 1f,
                BottomRadius = 1f,
                Height = 0.04f
            },
            MaterialOverride = CreateMaterial(color)
        };
        AddChild(visual);
        UpdateExpandingRing(startRadius, thickness);
    }

    public void UpdateExpandingRing(float radius, float thickness)
    {
        var visual = GetNodeOrNull<MeshInstance3D>("ExpandingRingDisk");
        if (visual == null)
        {
            return;
        }

        var scale = MathF.Max(0.1f, radius);
        visual.Scale = new Vector3(scale, 1f, scale);
        visual.Position = new Vector3(0f, 0.025f + MathF.Max(0f, thickness) * 0.002f, 0f);
    }

    public void BindWall(EntityId entityId, float halfLength, float halfWidth, Vector3 wallRight, Color color, float presentationScale = 1f)
    {
        BoundEntityId = entityId;
        if (GetChildCount() > 0)
        {
            return;
        }

        var yaw = MathF.Atan2(wallRight.X, wallRight.Z);
        var visualRoot = new Node3D
        {
            Name = "WallVisualRoot",
            Rotation = new Vector3(0f, yaw, 0f)
        };
        AddChild(visualRoot);

        visualRoot.AddChild(new MeshInstance3D
        {
            Name = "VerticalWall",
            Position = new Vector3(0f, 1.1f, 0f),
            Mesh = new BoxMesh
            {
                Size = new Vector3(MathF.Max(0.1f, halfLength * 2f), 2.2f, 0.14f)
            },
            MaterialOverride = CreateMaterial(color)
        });

        var visualHalfLength = MathF.Max(0.1f, halfLength * MathF.Max(0.1f, presentationScale));
        if (!Mathf.IsEqualApprox(visualHalfLength, halfLength))
        {
            visualRoot.AddChild(new MeshInstance3D
            {
                Name = "PresentationAura",
                Position = new Vector3(0f, 1.12f, 0f),
                Mesh = new BoxMesh
                {
                    Size = new Vector3(MathF.Max(0.1f, visualHalfLength * 2f), 2.3f, 0.18f)
                },
                MaterialOverride = CreateMaterial(new Color(color.R, color.G, color.B, 0.16f))
            });
        }

        visualRoot.AddChild(new MeshInstance3D
        {
            Name = "GroundFootprint",
            Position = new Vector3(0f, 0.025f, 0f),
            Mesh = new BoxMesh
            {
                Size = new Vector3(MathF.Max(0.1f, halfLength * 2f), 0.04f, MathF.Max(0.1f, halfWidth * 2f))
            },
            MaterialOverride = CreateMaterial(new Color(color.R, color.G, color.B, 0.25f))
        });
    }

    private static StandardMaterial3D CreateMaterial(Color color)
    {
        return new StandardMaterial3D
        {
            AlbedoColor = color,
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
            ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
        };
    }
}
