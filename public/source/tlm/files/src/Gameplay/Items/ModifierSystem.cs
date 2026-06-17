using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Items;

public sealed class ModifierSystem
{
    public IReadOnlyList<Modifier> BuildModifiers(BuildState build, ContentRegistry registry)
    {
        var modifiers = new List<Modifier>();
        foreach (var item in build.Items)
        {
            var definition = registry.GetItem(item.ItemId);
            foreach (var spec in definition.Modifiers)
            {
                if (!Enum.TryParse<ModifierOp>(spec.Operation, out var operation))
                {
                    continue;
                }

                for (var i = 0; i < item.Count; i++)
                {
                    modifiers.Add(new Modifier(
                        spec.SourceId,
                        spec.StatId,
                        operation,
                        spec.Value,
                        spec.Priority,
                        new TargetFilter(spec.TargetTag)));
                }
            }
        }

        if (build.ActivatableItem != null)
        {
            var definition = registry.GetItem(build.ActivatableItem.ItemId);
            foreach (var spec in definition.Modifiers)
            {
                if (!Enum.TryParse<ModifierOp>(spec.Operation, out var operation))
                {
                    continue;
                }

                modifiers.Add(new Modifier(
                    spec.SourceId,
                    spec.StatId,
                    operation,
                    spec.Value,
                    spec.Priority,
                    new TargetFilter(spec.TargetTag)));
            }
        }

        foreach (var temporary in build.TemporaryModifiers)
        {
            modifiers.Add(new Modifier(
                temporary.SourceId,
                temporary.StatId,
                temporary.Operation,
                temporary.Value,
                temporary.Priority,
                new TargetFilter(temporary.TargetTag)));
        }

        return modifiers;
    }
}
