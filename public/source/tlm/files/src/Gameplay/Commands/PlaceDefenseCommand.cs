using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Defenses;
using TheLastMage.Gameplay.Run;

namespace TheLastMage.Gameplay.Commands;

public sealed class PlaceDefenseCommand : ICommand
{
    private readonly ContentId _defenseId;
    private readonly Vector3 _position;

    public PlaceDefenseCommand(ContentId defenseId, Vector3 position)
    {
        _defenseId = defenseId;
        _position = position;
    }

    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.State.PhasePermissions.CanPlaceDefense)
        {
            reason = $"phase {context.State.CurrentPhase} cannot place defenses";
            return false;
        }

        if (!context.Content.TryGetDefense(_defenseId, out var definition) || definition == null)
        {
            reason = $"defense {_defenseId} is not available";
            return false;
        }

        var cost = (int)MathF.Ceiling(definition.Cost);
        if (context.State.Materials < cost)
        {
            reason = $"not enough materials for {definition.DisplayName}: need {cost}, have {context.State.Materials}";
            return false;
        }

        var validation = context.GetSystem<DefenseSystem>().ValidatePlacement(definition, _position);
        context.State.DebugMetrics.LastDefensePlacementValidation = validation.IsValid
            ? $"{_defenseId} valid at {_position}"
            : $"{_defenseId} invalid at {_position}: {validation.Reason}";
        if (!validation.IsValid)
        {
            reason = validation.Reason;
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        var definition = context.Content.GetDefense(_defenseId);
        context.State.Materials -= (int)MathF.Ceiling(definition.Cost);
        context.GetSystem<DefenseSystem>().PlaceDefense(_defenseId, _position);
    }
}
