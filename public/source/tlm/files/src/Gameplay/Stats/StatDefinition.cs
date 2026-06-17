using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Stats;

public sealed record StatDefinition(
    StatId Id,
    float BaseValue,
    float MinValue = float.NegativeInfinity,
    float MaxValue = float.PositiveInfinity);
