using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;

namespace TheLastMage.Gameplay.Items;

public interface ICastFlowModifier
{
    bool TryCreatePlan(RunContext context, SpellRuntimeDefinition spell, out CastFlowPlan plan);
}
