using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Gameplay.Navigation;

namespace TheLastMage.Gameplay.Defenses;

public static class DefensePlacementValidator
{
    private const float ArenaRadius = 22f;
    private const float TowerEntranceMaxDistance = 5f;
    private const float MinimumDefenseSpacing = 1.8f;

    public static DefensePlacementValidationResult Validate(
        DefenseRuntimeDefinition definition,
        Vector3 position,
        IReadOnlyList<DefenseState> existingDefenses,
        TowerNavigationAdapter? tower)
    {
        if (position.Y < -0.25f || position.Y > 1.25f)
        {
            return DefensePlacementValidationResult.Invalid("placement must be near ground level");
        }

        var horizontalDistance = new Vector2(position.X, position.Z).Length();
        if (horizontalDistance > ArenaRadius)
        {
            return DefensePlacementValidationResult.Invalid($"placement outside arena radius {ArenaRadius:0.#}");
        }

        foreach (var defense in existingDefenses)
        {
            var spacing = MathF.Max(MinimumDefenseSpacing, definition.PlacementRadius + defense.PlacementRadius);
            if (position.DistanceSquaredTo(defense.Position) < spacing * spacing)
            {
                return DefensePlacementValidationResult.Invalid($"too close to existing defense {defense.EntityId}");
            }
        }

        if (string.Equals(definition.PlacementRule, "TowerEntrance", StringComparison.OrdinalIgnoreCase))
        {
            var entrance = tower?.EntrancePosition ?? new Vector3(0f, 0f, -5f);
            if (position.DistanceSquaredTo(entrance) > TowerEntranceMaxDistance * TowerEntranceMaxDistance)
            {
                return DefensePlacementValidationResult.Invalid("barricade must protect the tower entrance approach");
            }
        }

        return DefensePlacementValidationResult.Valid();
    }
}
