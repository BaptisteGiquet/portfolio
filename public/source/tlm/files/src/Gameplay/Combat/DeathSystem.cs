using Godot;
using TheLastMage.Foundation;
using TheLastMage.Foundation.Events;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Combat;

public sealed class DeathSystem
{
    public void PublishDeath(RunContext context, EntityId targetId, CombatTargetKind kind, ContentId archetypeId, ContentId sourceId)
    {
        if (kind == CombatTargetKind.Enemy)
        {
            context.Events.Publish(new EnemyDiedEvent(targetId, archetypeId, sourceId));
            return;
        }

        if (kind == CombatTargetKind.Mage)
        {
            GD.Print($"MageDied mage={targetId} source={sourceId}");
            context.Events.Publish(new MageDiedEvent(targetId, sourceId));
            return;
        }

        if (kind == CombatTargetKind.Defense)
        {
            GD.Print($"DefenseDestroyed defense={targetId} archetype={archetypeId} source={sourceId}");
            context.Events.Publish(new DefenseDestroyedEvent(targetId, archetypeId, sourceId));
            return;
        }

        if (kind == CombatTargetKind.Summon)
        {
            GD.Print($"SummonDestroyed summon={targetId} source={sourceId}");
        }
    }
}
