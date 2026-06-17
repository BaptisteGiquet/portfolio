using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Items;

public sealed class KeepItInBehavior : ICastFlowModifier
{
    private readonly ContentId _sourceItemId;
    private readonly ItemEffectRuntimeSpec _effect;

    public KeepItInBehavior(ContentId sourceItemId, ItemEffectRuntimeSpec effect)
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
        var baseChargeSeconds = MathF.Max(0.05f, _effect.SecondaryValue);
        var requiredChargeSeconds = MathF.Max(0.05f, baseChargeSeconds / fireRate);
        var bonusSpellInstances = Math.Clamp((int)MathF.Floor(MathF.Max(1f, _effect.Value) + 0.001f), 1, 12);
        plan = new CastFlowPlan(CastFlowMode.ChargeThenRelease, _sourceItemId, requiredChargeSeconds, bonusSpellInstances);
        return true;
    }
}
