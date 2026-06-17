using Godot;

namespace TheLastMage.Gameplay.Spells;

public readonly record struct SpellPlacementPreview(
    SpellPlacementShape Shape,
    Vector3 Position,
    float Radius,
    float WallHalfLength,
    float WallHalfWidth,
    Vector3 WallRight,
    Color Color);
