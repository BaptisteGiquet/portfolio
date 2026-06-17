using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Stats;

public sealed record StatBreakdown(
    StatId StatId,
    float BaseValue,
    float FinalValue,
    IReadOnlyList<Modifier> AppliedModifiers);
