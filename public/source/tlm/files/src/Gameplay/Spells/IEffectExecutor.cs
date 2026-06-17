using TheLastMage.Data.Runtime;

namespace TheLastMage.Gameplay.Spells;

public interface IEffectExecutor
{
    string EffectType { get; }

    void Execute(in SpellExecutionContext context, in EffectRuntimeSpec effect);
}
