using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Gameplay.Items;

public static class ItemBehaviorResolver
{
    public static bool TryCreateCastFlowPlan(RunContext context, SpellRuntimeDefinition spell, out CastFlowPlan plan)
    {
        foreach (var stack in context.State.Build.Items)
        {
            if (TryCreateCastFlowPlanFromItem(context, stack.ItemId, spell, out plan))
            {
                return true;
            }
        }

        if (context.State.Build.ActivatableItem != null
            && TryCreateCastFlowPlanFromItem(context, context.State.Build.ActivatableItem.ItemId, spell, out plan))
        {
            return true;
        }

        plan = default;
        return false;
    }

    private static bool TryCreateCastFlowPlanFromItem(
        RunContext context,
        ContentId itemId,
        SpellRuntimeDefinition spell,
        out CastFlowPlan plan)
    {
        if (!context.Content.Items.TryGetValue(itemId, out var item) || item == null)
        {
            plan = default;
            return false;
        }

        foreach (var effect in item.Effects)
        {
            ICastFlowModifier? behavior = effect.Kind switch
            {
                "KeepItIn" => new KeepItInBehavior(item.Id, effect),
                "AbyssalRing" => new AbyssalRingBehavior(item.Id, effect),
                _ => null
            };

            if (behavior == null)
            {
                continue;
            }

            if (behavior.TryCreatePlan(context, spell, out plan))
            {
                return true;
            }
        }

        plan = default;
        return false;
    }
}
