using Godot;
using TheLastMage.Data.Runtime;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Spells;

public readonly record struct SpellExecutionContext(
    RunContext RunContext,
    SpellRuntimeDefinition Spell,
    EntityId CasterId,
    Vector3 Origin,
    Vector3 Direction,
    float DamageMultiplier = 1f,
    int SpellChainGeneration = 0,
    Vector3? PlacementOverride = null);
