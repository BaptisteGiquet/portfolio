using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Spells;

public sealed class CastFlowState
{
    public bool IsActive { get; private set; }

    public bool IsReady => IsActive && ChargeElapsedSeconds >= RequiredChargeSeconds;

    public EntityId CasterId { get; private set; } = EntityId.None;

    public int SlotIndex { get; private set; } = -1;

    public ContentId SpellId { get; private set; }

    public ContentId SourceItemId { get; private set; }

    public float RequiredChargeSeconds { get; private set; }

    public float ChargeElapsedSeconds { get; private set; }

    public int BonusSpellInstances { get; private set; }

    public CastFlowPlan Plan { get; private set; }

    public bool AllowsConcurrentCasting => IsActive && Plan.AllowsConcurrentCasting;

    public bool SuppressesNormalCasting => IsActive && Plan.SuppressesNormalCasting;

    public bool ReadyEventPublished { get; set; }

    public void Begin(EntityId casterId, int slotIndex, ContentId spellId, CastFlowPlan plan)
    {
        IsActive = true;
        ReadyEventPublished = false;
        CasterId = casterId;
        SlotIndex = slotIndex;
        SpellId = spellId;
        SourceItemId = plan.SourceItemId;
        RequiredChargeSeconds = MathF.Max(0.05f, plan.RequiredChargeSeconds);
        ChargeElapsedSeconds = 0f;
        BonusSpellInstances = Math.Clamp(plan.BonusSpellInstances, 0, 12);
        Plan = plan;
    }

    public void Tick(float delta)
    {
        if (!IsActive)
        {
            return;
        }

        ChargeElapsedSeconds = MathF.Min(RequiredChargeSeconds, ChargeElapsedSeconds + MathF.Max(0f, delta));
    }

    public void Clear()
    {
        IsActive = false;
        ReadyEventPublished = false;
        CasterId = EntityId.None;
        SlotIndex = -1;
        SpellId = default;
        SourceItemId = default;
        RequiredChargeSeconds = 0f;
        ChargeElapsedSeconds = 0f;
        BonusSpellInstances = 0;
        Plan = default;
    }
}
