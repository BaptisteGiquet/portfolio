namespace TheLastMage.Gameplay.Defenses;

public readonly record struct DefensePlacementValidationResult(bool IsValid, string Reason)
{
    public static DefensePlacementValidationResult Valid() => new(true, string.Empty);

    public static DefensePlacementValidationResult Invalid(string reason) => new(false, reason);
}
