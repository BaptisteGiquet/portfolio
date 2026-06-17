using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Player;

public sealed class PlayerRuntimeState
{
    public EntityId EntityId { get; init; } = new(0);

    public ContentId MageId { get; private set; } = ContentId.From("mage_recluse");

    public Vector3 Position { get; set; } = Vector3.Zero;

    public Vector3 AimDirection { get; set; } = Vector3.Forward;

    public float BaseMaxHealth { get; private set; } = 100f;

    public float BaseMoveSpeed { get; private set; } = 5.5f;

    public float BaseFireRate { get; private set; } = 1f;

    public float BaseLuck { get; private set; }

    public float MaxHealth => BaseMaxHealth;

    public void ApplyMage(ContentId mageId, float maxHealth, float moveSpeed, float fireRate, float luck)
    {
        MageId = mageId;
        BaseMaxHealth = maxHealth;
        BaseMoveSpeed = moveSpeed;
        BaseFireRate = fireRate;
        BaseLuck = luck;
    }
}
