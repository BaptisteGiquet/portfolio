using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Items;

public sealed class AbyssalRingBehavior : ICastFlowModifier
{
    private const float DefaultDamageMultiplier = 20f;
    private const float DefaultBaseChargeSeconds = 1.25f;
    private const float BaseEndRadius = 8f;
    private const float BaseDurationSeconds = 1.2f;
    private const float StartRadius = 0.75f;
    private const float RingThickness = 1.05f;
    private const float TickIntervalSeconds = 0.04f;

    private readonly ContentId _sourceItemId;
    private readonly ItemEffectRuntimeSpec _effect;

    public AbyssalRingBehavior(ContentId sourceItemId, ItemEffectRuntimeSpec effect)
    {
        _sourceItemId = sourceItemId;
        _effect = effect;
    }

    public bool TryCreatePlan(RunContext context, SpellRuntimeDefinition spell, out CastFlowPlan plan)
    {
        if (!new TargetFilter(_effect.TargetTag).Matches(spell.Tags))
        {
            plan = default;
            return false;
        }

        var fireRate = PlayerAttributeResolver.ResolveFireRate(context, spell.Tags, recordBreakdown: false);
        var baseChargeSeconds = _effect.SecondaryValue > 0f ? _effect.SecondaryValue : DefaultBaseChargeSeconds;
        var requiredChargeSeconds = MathF.Max(0.05f, baseChargeSeconds / fireRate);
        var damageMultiplier = _effect.Value > 0f ? _effect.Value : DefaultDamageMultiplier;

        plan = new CastFlowPlan(
            CastFlowMode.SustainedCastThenRelease,
            _sourceItemId,
            requiredChargeSeconds,
            BonusSpellInstances: 0,
            CastFlowReleaseKind.ExpandingRing,
            damageMultiplier,
            BaseEndRadius,
            BaseDurationSeconds,
            StartRadius,
            RingThickness,
            TickIntervalSeconds);
        return true;
    }
}
