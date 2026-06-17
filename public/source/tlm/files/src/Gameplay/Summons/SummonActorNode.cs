using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Summons;

public partial class SummonActorNode : Node3D
{
    private static readonly Color AliveColor = new(0.35f, 0.95f, 0.68f, 1f);
    private static readonly Color DamagedColor = new(1f, 0.95f, 0.2f, 1f);
    private static readonly Color DeadColor = new(0.05f, 0.25f, 0.16f, 1f);

    private StandardMaterial3D? _material;
    private float _damageFlashRemaining;
    private float _deathTimer;
    private bool _isDead;

    public EntityId BoundEntityId { get; private set; } = EntityId.None;

    public void Bind(EntityId entityId, float presentationScale = 1f)
    {
        BoundEntityId = entityId;
        Scale = Vector3.One * MathF.Max(0.25f, presentationScale);
        if (GetChildCount() > 0)
        {
            return;
        }

        var mesh = new MeshInstance3D
        {
            Mesh = new CapsuleMesh
            {
                Radius = 0.32f,
                Height = 1.65f
            },
            MaterialOverride = CreateMaterial(AliveColor)
        };
        AddChild(mesh);
        _material = mesh.MaterialOverride as StandardMaterial3D;
    }

    public override void _Process(double delta)
    {
        if (_damageFlashRemaining > 0f)
        {
            _damageFlashRemaining -= (float)delta;
            if (_damageFlashRemaining <= 0f && !_isDead && _material != null)
            {
                _material.AlbedoColor = AliveColor;
            }
        }

        if (!_isDead)
        {
            return;
        }

        _deathTimer -= (float)delta;
        if (_deathTimer <= 0f)
        {
            QueueFree();
        }
    }

    public void FlashDamage()
    {
        if (_isDead)
        {
            return;
        }

        _damageFlashRemaining = 0.16f;
        if (_material != null)
        {
            _material.AlbedoColor = DamagedColor;
        }
    }

    public void MarkDead()
    {
        _isDead = true;
        _deathTimer = 0.25f;
        if (_material != null)
        {
            _material.AlbedoColor = DeadColor;
        }
    }

    private static StandardMaterial3D CreateMaterial(Color color)
    {
        return new StandardMaterial3D
        {
            AlbedoColor = color,
            ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
        };
    }
}
