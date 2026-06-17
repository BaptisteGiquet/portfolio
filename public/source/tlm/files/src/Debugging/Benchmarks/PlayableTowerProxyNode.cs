using Godot;

namespace TheLastMage.Debugging.Benchmarks;

public partial class PlayableTowerProxyNode : Node3D
{
    [Export] public Vector3 EntrancePosition { get; set; } = new(0f, 0f, -5f);

    [Export] public Vector3 MageBasePosition { get; set; } = new(0f, 4.5f, 5f);

    [Export] public Vector3 PlayerRampStartPosition { get; set; } = new(4.2f, 0f, -6.2f);

    [Export] public Vector3 PlayerRampEndPosition { get; set; } = new(3.2f, 4.5f, 4.2f);

    [Export] public bool AddLabels { get; set; } = true;

    public override void _Ready()
    {
        BuildRamp();
        BuildTopPlatform();
        BuildTowerSilhouette();
        if (AddLabels)
        {
            AddLabel("Player side ramp to mage top", (PlayerRampStartPosition + PlayerRampEndPosition) * 0.5f + new Vector3(0f, 1.1f, 0f), new Color(0.7f, 0.95f, 1f, 1f));
            AddLabel("Barricades protect the center entrance", EntrancePosition + new Vector3(0f, 1.1f, 0f), new Color(1f, 0.78f, 0.45f, 1f));
        }
    }

    private void BuildRamp()
    {
        var center = (PlayerRampStartPosition + PlayerRampEndPosition) * 0.5f;
        var direction = PlayerRampEndPosition - PlayerRampStartPosition;
        var length = direction.Length();
        var ramp = new StaticBody3D { Name = "TemporaryTowerRamp" };
        AddChild(ramp);
        ramp.GlobalPosition = center;
        ramp.LookAt(PlayerRampEndPosition, Vector3.Up);

        var shape = new BoxShape3D { Size = new Vector3(2.2f, 0.28f, length) };
        ramp.AddChild(new CollisionShape3D { Name = "Collision", Shape = shape });
        ramp.AddChild(new MeshInstance3D
        {
            Name = "Mesh",
            Mesh = new BoxMesh { Size = shape.Size },
            MaterialOverride = CreateMaterial(new Color(0.28f, 0.5f, 0.58f, 0.72f))
        });
    }

    private void BuildTopPlatform()
    {
        var platform = new StaticBody3D { Name = "TemporaryMageTopPlatform", Position = MageBasePosition + new Vector3(0f, -0.35f, 0f) };
        AddChild(platform);
        var shape = new BoxShape3D { Size = new Vector3(8.5f, 0.3f, 7.5f) };
        platform.AddChild(new CollisionShape3D { Name = "Collision", Shape = shape });
        platform.AddChild(new MeshInstance3D
        {
            Name = "Mesh",
            Mesh = new BoxMesh { Size = shape.Size },
            MaterialOverride = CreateMaterial(new Color(0.2f, 0.25f, 0.32f, 0.6f))
        });
    }

    private void BuildTowerSilhouette()
    {
        AddChild(new MeshInstance3D
        {
            Name = "TemporaryTowerVolume",
            Position = new Vector3(0f, 2.25f, 1.2f),
            Mesh = new BoxMesh { Size = new Vector3(6f, 4.5f, 9f) },
            MaterialOverride = CreateMaterial(new Color(0.18f, 0.2f, 0.26f, 0.16f))
        });
    }

    private void AddLabel(string text, Vector3 position, Color color)
    {
        AddChild(new Label3D
        {
            Name = "TemporaryTowerLabel",
            Text = text,
            Position = position,
            FontSize = 26,
            Modulate = color,
            OutlineSize = 6,
            Billboard = BaseMaterial3D.BillboardModeEnum.Enabled
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
