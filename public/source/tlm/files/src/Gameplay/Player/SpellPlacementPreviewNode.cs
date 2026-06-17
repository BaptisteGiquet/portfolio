using Godot;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Gameplay.Player;

public partial class SpellPlacementPreviewNode : Node3D
{
    private SpellPlacementShape _currentShape = SpellPlacementShape.None;
    private float _currentRadius;
    private float _currentHalfLength;
    private float _currentHalfWidth;

    public void ShowPreview(in SpellPlacementPreview preview)
    {
        Visible = true;
        GlobalPosition = preview.Position;

        if (NeedsRebuild(preview))
        {
            Rebuild(preview);
        }

        if (preview.Shape == SpellPlacementShape.Wall)
        {
            GlobalRotation = new Vector3(0f, MathF.Atan2(preview.WallRight.X, preview.WallRight.Z), 0f);
        }
        else
        {
            GlobalRotation = Vector3.Zero;
        }
    }

    public void HidePreview()
    {
        Visible = false;
    }

    private bool NeedsRebuild(in SpellPlacementPreview preview)
    {
        return preview.Shape != _currentShape
            || MathF.Abs(preview.Radius - _currentRadius) > 0.01f
            || MathF.Abs(preview.WallHalfLength - _currentHalfLength) > 0.01f
            || MathF.Abs(preview.WallHalfWidth - _currentHalfWidth) > 0.01f;
    }

    private void Rebuild(in SpellPlacementPreview preview)
    {
        foreach (var child in GetChildren())
        {
            if (child is Node node)
            {
                node.QueueFree();
            }
        }

        _currentShape = preview.Shape;
        _currentRadius = preview.Radius;
        _currentHalfLength = preview.WallHalfLength;
        _currentHalfWidth = preview.WallHalfWidth;

        if (preview.Shape == SpellPlacementShape.Disk)
        {
            AddChild(new MeshInstance3D
            {
                Name = "AoeDiskPreview",
                Position = new Vector3(0f, 0.035f, 0f),
                Mesh = new CylinderMesh
                {
                    TopRadius = preview.Radius,
                    BottomRadius = preview.Radius,
                    Height = 0.045f
                },
                MaterialOverride = CreateMaterial(preview.Color)
            });
            return;
        }

        if (preview.Shape == SpellPlacementShape.Wall)
        {
            AddChild(new MeshInstance3D
            {
                Name = "WallPreview",
                Position = new Vector3(0f, 1.1f, 0f),
                Mesh = new BoxMesh
                {
                    Size = new Vector3(preview.WallHalfLength * 2f, 2.2f, 0.12f)
                },
                MaterialOverride = CreateMaterial(preview.Color)
            });

            AddChild(new MeshInstance3D
            {
                Name = "WallFootprintPreview",
                Position = new Vector3(0f, 0.04f, 0f),
                Mesh = new BoxMesh
                {
                    Size = new Vector3(preview.WallHalfLength * 2f, 0.05f, preview.WallHalfWidth * 2f)
                },
                MaterialOverride = CreateMaterial(new Color(preview.Color.R, preview.Color.G, preview.Color.B, 0.22f))
            });
        }
    }

    private static StandardMaterial3D CreateMaterial(Color color)
    {
        return new StandardMaterial3D
        {
            AlbedoColor = color,
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
            ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded,
            NoDepthTest = true
        };
    }
}
