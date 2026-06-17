using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Stats;

public readonly record struct Modifier(
    ContentId SourceId,
    StatId StatId,
    ModifierOp Operation,
    float Value,
    int Priority,
    TargetFilter TargetFilter);
