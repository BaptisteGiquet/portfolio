using Godot;
using TheLastMage.Foundation;
using TheLastMage.Gameplay.Loot;
using TheLastMage.Save;

namespace TheLastMage.Gameplay.Run;

public static class RunCheckpointService
{
    public static bool CanUseCheckpoint(RunCheckpointSaveDto? checkpoint)
    {
        return checkpoint != null
            && !string.IsNullOrWhiteSpace(checkpoint.Phase)
            && !string.IsNullOrWhiteSpace(checkpoint.MageId);
    }

    public static void SaveStableCheckpoint(RunContext context, string checkpointLabel)
    {
        if (!context.State.CountsTowardPlayerStats
            || context.State.CurrentPhase is not (RunPhase.DayCombat or RunPhase.NightMarket or RunPhase.NightDefense))
        {
            return;
        }

        context.Profile.SuspendedRun = Capture(context, checkpointLabel);
        context.SaveProfile(context.Profile);
        context.State.DebugMetrics.LastRunModeSummary =
            $"checkpoint saved: {context.Profile.SuspendedRun.CheckpointLabel} day {context.Profile.SuspendedRun.DayIndex}";
    }

    public static void ClearSuspendedRun(RunContext context)
    {
        if (context.Profile.SuspendedRun == null)
        {
            return;
        }

        context.Profile.SuspendedRun = null;
        context.SaveProfile(context.Profile);
    }

    public static RunCheckpointSaveDto Capture(RunContext context, string checkpointLabel)
    {
        var state = context.State;
        return new RunCheckpointSaveDto
        {
            RunId = state.RunId.ToString(),
            Seed = context.Random.Seed,
            Phase = state.CurrentPhase.ToString(),
            CheckpointLabel = checkpointLabel,
            DayIndex = state.DayIndex,
            HumanityRemaining = state.HumanityRemaining,
            Souls = state.Souls,
            Materials = state.Materials,
            MageId = state.SelectedMageId.ToString(),
            TotalElapsedSeconds = state.Clock.TotalElapsedSeconds,
            PhaseElapsedSeconds = state.Clock.PhaseElapsedSeconds,
            IntensitySeconds = state.Clock.IntensitySeconds,
            SpellSlots = state.Spellbook.Slots
                .Select((slot, index) => new RunSpellSlotSaveDto
                {
                    SlotIndex = index,
                    SpellId = slot.SpellId.ToString(),
                    CooldownRemainingSeconds = slot.CooldownRemainingSeconds
                })
                .ToList(),
            SelectedSpellSlotIndex = state.Spellbook.SelectedSlotIndex,
            Items = state.Build.Items
                .Select(item => new RunItemStackSaveDto { ItemId = item.ItemId.ToString(), Count = item.Count })
                .ToList(),
            AcquiredItemIds = state.Build.AcquiredItemIds.Select(itemId => itemId.ToString()).ToList(),
            ActivatableItem = state.Build.ActivatableItem == null
                ? null
                : new RunActivatableItemSaveDto
                {
                    ItemId = state.Build.ActivatableItem.ItemId.ToString(),
                    HasUnlimitedActivations = state.Build.ActivatableItem.HasUnlimitedActivations,
                    RemainingActivations = state.Build.ActivatableItem.RemainingActivations,
                    MaxActivations = state.Build.ActivatableItem.MaxActivations
                },
            MarketOfferItemIds = state.Market.Offers.Select(offer => offer.ItemId.ToString()).ToList(),
            MarketRerollCount = state.Market.RerollCount,
            MarketChoiceCompletedThisNight = state.Market.ChoiceCompletedThisNight,
            EnemiesKilled = state.Telemetry.EnemiesKilled,
            HumanityKilled = state.Telemetry.HumanityKilled,
            TotalSpellsCast = state.Telemetry.TotalSpellsCast,
            SpellsUsed = new Dictionary<string, int>(state.Telemetry.SpellCasts, StringComparer.Ordinal),
            ItemsDiscoveredThisRun = state.Telemetry.ItemsDiscovered.ToList()
        };
    }

    public static void Restore(RunContext context, RunCheckpointSaveDto checkpoint)
    {
        var state = context.State;
        if (context.Content.TryGetMage(ContentId.From(checkpoint.MageId), out var mage) && mage != null)
        {
            state.SelectMage(mage);
        }

        state.DayIndex = Math.Max(1, checkpoint.DayIndex);
        state.HumanityRemaining = Math.Max(0, checkpoint.HumanityRemaining);
        state.Souls = Math.Max(0, checkpoint.Souls);
        state.Materials = Math.Max(0, checkpoint.Materials);

        state.Spellbook.ClearSlots();
        foreach (var slot in checkpoint.SpellSlots)
        {
            state.Spellbook.RestoreSlot(
                slot.SlotIndex,
                ContentId.From(slot.SpellId),
                slot.CooldownRemainingSeconds);
        }

        state.Spellbook.SelectSlot(checkpoint.SelectedSpellSlotIndex);

        state.Build.ClearForRestore();
        foreach (var item in checkpoint.Items)
        {
            state.Build.RestoreItemStack(ContentId.From(item.ItemId), item.Count);
        }

        foreach (var itemId in checkpoint.AcquiredItemIds)
        {
            state.Build.RestoreAcquiredItem(ContentId.From(itemId));
        }

        if (checkpoint.ActivatableItem != null)
        {
            state.Build.RestoreActivatableItem(
                ContentId.From(checkpoint.ActivatableItem.ItemId),
                checkpoint.ActivatableItem.HasUnlimitedActivations,
                checkpoint.ActivatableItem.RemainingActivations,
                checkpoint.ActivatableItem.MaxActivations);
        }

        state.Telemetry.Restore(
            checkpoint.EnemiesKilled,
            checkpoint.HumanityKilled,
            checkpoint.TotalSpellsCast,
            checkpoint.SpellsUsed,
            checkpoint.ItemsDiscoveredThisRun);

        if (!Enum.TryParse<RunPhase>(checkpoint.Phase, out var phase)
            || phase is not (RunPhase.DayCombat or RunPhase.NightMarket or RunPhase.NightDefense))
        {
            phase = RunPhase.DayCombat;
        }

        context.GetSystem<RunPhaseStateMachine>().SetPhase(phase, saveCheckpoint: false);
        state.Market.Restore(
            checkpoint.MarketOfferItemIds.Select(ContentId.From),
            checkpoint.MarketRerollCount,
            checkpoint.MarketChoiceCompletedThisNight);
        state.Clock.Restore(
            checkpoint.TotalElapsedSeconds,
            checkpoint.PhaseElapsedSeconds,
            checkpoint.IntensitySeconds);
        state.DebugMetrics.LastRunModeSummary =
            $"resumed checkpoint: {checkpoint.CheckpointLabel} day {checkpoint.DayIndex}";
        GD.Print($"RunCheckpointRestored phase={phase} day={state.DayIndex} checkpoint={checkpoint.CheckpointLabel}");
    }

    public static string Describe(RunCheckpointSaveDto checkpoint)
    {
        return $"{checkpoint.CheckpointLabel}  Day {checkpoint.DayIndex}  Humanity {checkpoint.HumanityRemaining}";
    }
}
