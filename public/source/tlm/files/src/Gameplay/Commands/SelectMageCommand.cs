using TheLastMage.Foundation;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Spells;
using TheLastMage.Gameplay.Stats;

namespace TheLastMage.Gameplay.Commands;

public sealed class SelectMageCommand : ICommand
{
    private readonly ContentId _mageId;
    private readonly bool _allowLocked;

    public SelectMageCommand(ContentId mageId, bool allowLocked = false)
    {
        _mageId = mageId;
        _allowLocked = allowLocked;
    }

    public bool CanExecute(RunContext context, out string reason)
    {
        if (!context.Content.Mages.ContainsKey(_mageId))
        {
            reason = $"mage '{_mageId}' is not registered";
            return false;
        }

        if (!_allowLocked && !context.Profile.UnlockedMageIds.Contains(_mageId.Value, StringComparer.Ordinal))
        {
            reason = $"mage '{_mageId}' is locked";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    public void Execute(RunContext context)
    {
        if (!context.Content.TryGetMage(_mageId, out var mage) || mage == null)
        {
            return;
        }

        context.State.SelectMage(mage);
        RefreshPlayerCombatHealth(context);
        if (context.TryGetSystem<SpellSystem>(out var spellSystem) && spellSystem != null)
        {
            spellSystem.ApplyMageLoadout();
        }
    }

    private static void RefreshPlayerCombatHealth(RunContext context)
    {
        if (!context.TryGetSystem<CombatSystem>(out var combatSystem) || combatSystem == null)
        {
            return;
        }

        var maxHealth = PlayerAttributeResolver.ResolveValue(
            context,
            PlayerAttributeResolver.Health,
            context.State.Player.BaseMaxHealth,
            recordBreakdown: false);
        combatSystem.RegisterTarget(
            context.State.Player.EntityId,
            CombatTargetKind.Mage,
            maxHealth,
            context.State.Player.MageId);
    }
}
