using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public sealed class RepairDefenseCommand : ICommand
{
    private readonly EntityId _defenseEntityId;
    private readonly float _amount;

    public RepairDefenseCommand(EntityId defenseEntityId, float amount)
    {
        _defenseEntityId = defenseEntityId;
        _amount = amount;
    }

    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.State.PhasePermissions.CanRepairDefense)
        {
            reason = $"phase {context.State.CurrentPhase} cannot repair defenses";
            return false;
        }

        var defenseSystem = context.GetSystem<DefenseSystem>();
        var defense = defenseSystem.Defenses.FirstOrDefault(candidate => candidate.EntityId.Equals(_defenseEntityId));
        if (defense == null)
        {
            reason = $"defense {_defenseEntityId} is not active";
            return false;
        }

        var definition = context.Content.GetDefense(defense.DefenseId);
        var cost = (int)MathF.Ceiling(definition.RepairCost);
        if (cost <= 0)
        {
            reason = $"{definition.DisplayName} cannot be repaired";
            return false;
        }

        if (context.State.Materials < cost)
        {
            reason = $"not enough materials to repair {definition.DisplayName}: need {cost}, have {context.State.Materials}";
            return false;
        }

        if (!context.GetSystem<CombatSystem>().TryGetHealth(_defenseEntityId, out var health) || health == null || health.IsDead)
        {
            reason = $"defense {_defenseEntityId} is not repairable";
            return false;
        }

        if (health.CurrentHealth >= health.MaxHealth)
        {
            reason = $"{definition.DisplayName} is already fully repaired";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        var defenseSystem = context.GetSystem<DefenseSystem>();
        var defense = defenseSystem.Defenses.First(candidate => candidate.EntityId.Equals(_defenseEntityId));
        var definition = context.Content.GetDefense(defense.DefenseId);
        context.State.Materials -= (int)MathF.Ceiling(definition.RepairCost);
        defenseSystem.RepairDefense(_defenseEntityId, _amount);
    }
}
