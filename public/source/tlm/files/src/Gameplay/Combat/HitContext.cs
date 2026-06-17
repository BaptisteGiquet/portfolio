using Godot;
using TheLastMage.Foundation;

namespace TheLastMage.Gameplay.Combat;

public readonly record struct HitContext(Vector3 Position, Vector3 Normal, EntityId SourceEntityId);
