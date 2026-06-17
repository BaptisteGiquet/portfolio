using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Defenses;

public partial class DefenseActorNode : Node3D
{
    public EntityId BoundEntityId { get; private set; } = EntityId.None;

    public void Bind(EntityId entityId, bool isTrap, float triggerRadius, float explosionRadius)
    {
        BoundEntityId = entityId;
        if (GetChildCount() > 0)
        {
            return;
        }

        var body = new MeshInstance3D
        {
            Mesh = isTrap
                ? new CylinderMesh { TopRadius = 0.8f, BottomRadius = 0.8f, Height = 0.08f }
                : new BoxMesh { Size = new Vector3(2.6f, 1.5f, 0.45f) }
        };
        body.MaterialOverride = CreateMaterial(isTrap ? new Color(1f, 0.28f, 0.08f, 1f) : new Color(0.45f, 0.27f, 0.12f, 1f), false);
        AddChild(body);

        if (isTrap)
        {
            AddDebugDisk("TriggerRadius", triggerRadius, new Color(1f, 0.9f, 0.1f, 0.22f), 0.015f);
            AddDebugDisk("ExplosionRadius", explosionRadius, new Color(1f, 0.15f, 0.05f, 0.14f), 0.01f);
            AddDebugLabel("Explosive Seal\nYellow = trigger\nRed = explosion", new Vector3(0f, 0.55f, 0f), new Color(1f, 0.75f, 0.35f, 1f));
        }
        else
        {
            AddDebugDisk("BlockerInfluence", 4f, new Color(0.25f, 0.55f, 1f, 0.1f), 0.008f);
            AddDebugLabel("Barricade blocker\nBrutes attack this when it blocks route", new Vector3(0f, 1.4f, 0f), new Color(0.75f, 0.9f, 1f, 1f));
        }
    }

    private void AddDebugDisk(string nodeName, float radius, Color color, float yOffset)
    {
        if (radius <= 0f)
        {
            return;
        }

        var disk = new MeshInstance3D
        {
            Name = nodeName,
            Mesh = new CylinderMesh
            {
                TopRadius = radius,
                BottomRadius = radius,
                Height = 0.025f
            },
            MaterialOverride = CreateMaterial(color, true),
            Position = new Vector3(0f, yOffset, 0f)
        };
        AddChild(disk);
    }

    private void AddDebugLabel(string text, Vector3 position, Color color)
    {
        AddChild(new Label3D
        {
            Name = "DefenseDebugLabel",
            Text = text,
            Position = position,
            FontSize = 22,
            Modulate = color,
            OutlineSize = 6,
            Billboard = BaseMaterial3D.BillboardModeEnum.Enabled
        });
    }

    private static StandardMaterial3D CreateMaterial(Color color, bool transparent)
    {
        return new StandardMaterial3D
        {
            AlbedoColor = color,
            Transparency = transparent ? BaseMaterial3D.TransparencyEnum.Alpha : BaseMaterial3D.TransparencyEnum.Disabled,
            ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
        };
    }
}
