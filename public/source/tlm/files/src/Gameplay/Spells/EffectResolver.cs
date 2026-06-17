using TheLastMage.Data.Runtime;

namespace TheLastMage.Gameplay.Spells;

public sealed class EffectResolver
{
    private readonly Dictionary<string, IEffectExecutor> _executors = new(StringComparer.OrdinalIgnoreCase);

    public EffectResolver(IEnumerable<IEffectExecutor> executors)
    {
        foreach (var executor in executors)
        {
            _executors[executor.EffectType] = executor;
        }
    }

    public void Execute(in SpellExecutionContext context, IReadOnlyList<EffectRuntimeSpec> effects)
    {
        for (var i = 0; i < effects.Count; i++)
        {
            var effect = effects[i];
            if (_executors.TryGetValue(effect.EffectType, out var executor))
            {
                executor.Execute(context, effect);
            }
        }
    }
}
