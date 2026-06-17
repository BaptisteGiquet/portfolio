using TheLastMage.Data.Runtime;
using TheLastMage.Controls;
using TheLastMage.Gameplay.Combat;
using TheLastMage.Gameplay.Run;
using TheLastMage.Gameplay.Stats;
using TheLastMage.Localization;

namespace TheLastMage.UI;

public sealed record SpellSlotViewModel(
    int SlotNumber,
    string DisplayName,
    bool HasSpell,
    bool IsSelected,
    bool IsReady,
    float CooldownRemainingSeconds,
    float CooldownTotalSeconds);

public sealed record HudViewModel(
    float HealthCurrent,
    float HealthMax,
    RunPhase Phase,
    int Day,
    int Souls,
    int Materials,
    int Humanity,
    int ActiveEnemies,
    int TargetEnemies,
    int ActiveDefenses,
    string ActivatableItemName,
    string ActivatableUsesText,
    string WaveState,
    IReadOnlyList<SpellSlotViewModel> Spells,
    string Prompt,
    string ReadabilitySummary);

public sealed record MarketOfferViewModel(
    int Index,
    string DisplayName,
    string BodyText,
    string TagText,
    bool CanChoose,
    bool Revealed);

public sealed record BuildInspectorViewModel(
    string MageText,
    string AttributeText,
    string ItemText,
    string ProcText,
    string FeedbackText);

public static class RunViewModels
{
    public static HudViewModel BuildHud(RunContext context)
    {
        var healthCurrent = 0f;
        var healthMax = 0f;
        if (context.GetSystem<CombatSystem>().TryGetHealth(context.State.Player.EntityId, out var health) && health != null)
        {
            healthCurrent = health.CurrentHealth;
            healthMax = health.MaxHealth;
        }

        var spells = new List<SpellSlotViewModel>(context.State.Spellbook.Slots.Count);
        for (var index = 0; index < context.State.Spellbook.Slots.Count; index++)
        {
            var slot = context.State.Spellbook.Slots[index];
            var displayName = LocalizationService.Current.Text("ui.common.empty");
            var cooldownTotal = 1f;
            if (slot.HasSpell && context.Content.TryGetSpell(slot.SpellId, out var spell) && spell != null)
            {
                displayName = LocalizationService.Current.ContentName("spell", spell.Id, spell.DisplayName);
                cooldownTotal = MathF.Max(0.1f, spell.CooldownSeconds);
            }

            spells.Add(new SpellSlotViewModel(
                index + 1,
                displayName,
                slot.HasSpell,
                index == context.State.Spellbook.SelectedSlotIndex,
                slot.IsReady,
                slot.CooldownRemainingSeconds,
                cooldownTotal));
        }

        return new HudViewModel(
            healthCurrent,
            healthMax,
            context.State.CurrentPhase,
            context.State.DayIndex,
            context.State.Souls,
            context.State.Materials,
            context.State.HumanityRemaining,
            context.State.DebugMetrics.ActiveEnemies,
            context.State.DebugMetrics.WaveTargetActiveEnemies,
            context.State.DebugMetrics.ActiveDefenses,
            BuildActivatableItemName(context),
            BuildActivatableUsesText(context),
            $"{context.State.DebugMetrics.ActiveWaveId} {context.State.DebugMetrics.WaveElapsedSeconds:0}s",
            spells,
            BuildPromptText(context),
            BuildReadabilitySummary(context));
    }

    public static IReadOnlyList<MarketOfferViewModel> BuildMarketOffers(RunContext context)
    {
        var offers = new List<MarketOfferViewModel>(3);
        for (var index = 0; index < 3; index++)
        {
            if (!context.State.Market.TryGetOffer(index, out var offer) || offer == null)
            {
                offers.Add(new MarketOfferViewModel(
                    index,
                    LocalizationService.Current.Text("ui.common.sold_out"),
                    LocalizationService.Current.Text("ui.common.empty"),
                    LocalizationService.Current.Text("ui.common.empty"),
                    false,
                    false));
                continue;
            }

            var item = context.Content.GetItem(offer.ItemId);
            var revealed = context.ItemKnowledge.CanReveal(item.Id);
            var body = revealed
                ? BuildRevealedItemText(item)
                : LocalizationService.Current.ContentField("item", item.Id, "hidden_description", item.HiddenDescription);
            if (string.IsNullOrWhiteSpace(body))
            {
                body = revealed
                    ? LocalizationService.Current.Text("ui.market.no_reveal_text")
                    : LocalizationService.Current.Text("ui.market.hidden_suffix");
            }
            else if (!revealed)
            {
                body = LocalizationService.Current.Text("ui.market.hidden_body", body);
            }

            offers.Add(new MarketOfferViewModel(
                index,
                LocalizationService.Current.ContentName("item", item.Id, item.DisplayName),
                body,
                BuildTagText(item),
                true,
                revealed));
        }

        return offers;
    }

    public static BuildInspectorViewModel BuildInspector(RunContext context)
    {
        return new BuildInspectorViewModel(
            $"{LocalizationService.Current.ContentName("mage", context.State.SelectedMageId, context.State.SelectedMageName)} ({context.State.SelectedMageId})",
            BuildAttributeText(context),
            BuildItemText(context),
            LocalizationService.Current.Text(
                "ui.view_model.proc_safety",
                context.State.DebugMetrics.LastCastFlowSummary,
                context.State.DebugMetrics.ProcBudgetUsedThisFrame,
                context.State.DebugMetrics.ProcBudgetBlockedThisFrame,
                context.State.DebugMetrics.LastProcSummary),
            LocalizationService.Current.Text(
                "ui.view_model.feedback_budgets",
                context.State.DebugMetrics.ActiveFeedbackBudgetSummary,
                context.State.DebugMetrics.AudioMixSummary));
    }

    private static string BuildPromptText(RunContext context)
    {
        return context.State.CurrentPhase switch
        {
            RunPhase.DayCombat => LocalizationService.Current.Text(
                "ui.prompt.day_combat",
                InputPromptService.ActionLabel(InputActions.CastPrimary),
                InputPromptService.ActionLabel(InputActions.SpellPrevious),
                InputPromptService.ActionLabel(InputActions.SpellNext),
                InputPromptService.ActionLabel(InputActions.UseActivatableItem),
                InputPromptService.ActionLabel(InputActions.Pause)),
            RunPhase.NightMarket => LocalizationService.Current.Text(
                "ui.prompt.night_market",
                InputPromptService.ActionLabel(InputActions.UiAccept),
                InputPromptService.ActionLabel(InputActions.UiCancel)),
            RunPhase.NightDefense => LocalizationService.Current.Text(
                "ui.prompt.night_defense",
                context.State.DebugMetrics.DefensePreparationSummary,
                InputPromptService.ActionLabel(InputActions.UiAccept),
                InputPromptService.ActionLabel(InputActions.Pause)),
            RunPhase.RunVictory => LocalizationService.Current.Text(
                "ui.prompt.run_victory",
                context.State.DebugMetrics.LastRunSummary,
                InputPromptService.ActionLabel(InputActions.UiAccept)),
            RunPhase.RunDefeat => LocalizationService.Current.Text(
                "ui.prompt.run_defeat",
                context.State.DebugMetrics.LastRunSummary,
                InputPromptService.ActionLabel(InputActions.UiAccept)),
            _ => LocalizationService.Current.Text("ui.prompt.run_setup", InputPromptService.ActionLabel(InputActions.Pause))
        };
    }

    private static string BuildReadabilitySummary(RunContext context)
    {
        return LocalizationService.Current.Text(
            "ui.readability.summary",
            context.Settings.ScreenshakeScale,
            context.Settings.FlashScale,
            context.Settings.TextScale);
    }

    private static string BuildAttributeText(RunContext context)
    {
        var tags = Array.Empty<Foundation.TagId>();
        var health = PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.Health, context.State.Player.BaseMaxHealth, tags, false);
        var move = PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.MoveSpeed, context.State.Player.BaseMoveSpeed, tags, false);
        var damage = PlayerAttributeResolver.ResolveDamage(context, 10f, tags, false);
        var fireRate = PlayerAttributeResolver.ResolveFireRate(context, tags, false);
        var range = PlayerAttributeResolver.ResolveRange(context, 10f, tags, false);
        var duration = PlayerAttributeResolver.ResolveDuration(context, 10f, tags, false);
        var luck = PlayerAttributeResolver.ResolveValue(context, PlayerAttributeResolver.Luck, context.State.Player.BaseLuck, tags, false);

        return LocalizationService.Current.Text(
            "ui.view_model.attribute_text",
            health,
            move,
            damage,
            fireRate,
            range,
            luck,
            duration);
    }

    private static string BuildItemText(RunContext context)
    {
        var lines = new List<string>();
        lines.Add(LocalizationService.Current.Text("ui.view_model.passive_items"));
        if (context.State.Build.Items.Count == 0)
        {
            lines.Add(LocalizationService.Current.Text("ui.view_model.no_passive_relics"));
        }
        else
        {
            foreach (var stack in context.State.Build.Items)
            {
                var item = context.Content.GetItem(stack.ItemId);
                lines.Add(LocalizationService.Current.Text(
                    "ui.view_model.item_stack",
                    LocalizationService.Current.ContentName("item", item.Id, item.DisplayName),
                    stack.Count));
                AppendItemRules(lines, item);
            }
        }

        lines.Add(string.Empty);
        lines.Add(LocalizationService.Current.Text("ui.view_model.activatable_item"));
        if (context.State.Build.ActivatableItem == null)
        {
            lines.Add(LocalizationService.Current.Text("ui.view_model.no_activatable_item"));
        }
        else
        {
            var active = context.State.Build.ActivatableItem;
            var item = context.Content.GetItem(active.ItemId);
            lines.Add(LocalizationService.Current.Text(
                "ui.view_model.activatable_item_uses",
                LocalizationService.Current.ContentName("item", item.Id, item.DisplayName),
                BuildActivatableUsesText(context)));
            AppendItemRules(lines, item);
            foreach (var effect in item.ActiveEffects)
            {
                lines.Add(LocalizationService.Current.Text(
                    "ui.view_model.active_effect_line",
                    effect.EffectType,
                    effect.Value,
                    effect.Radius,
                    effect.DurationSeconds));
            }
        }

        return string.Join('\n', lines);
    }

    private static void AppendItemRules(List<string> lines, ItemRuntimeDefinition item)
    {
        foreach (var modifier in item.Modifiers)
        {
            var tagText = modifier.TargetTag.IsValid
                ? LocalizationService.Current.Text("ui.market.modifier_tag", modifier.TargetTag)
                : string.Empty;
            lines.Add(LocalizationService.Current.Text(
                "ui.view_model.modifier_rule",
                modifier.Operation,
                modifier.Value,
                modifier.StatId,
                tagText));
        }

        foreach (var proc in item.Procs)
        {
            lines.Add(LocalizationService.Current.Text(
                "ui.view_model.proc_rule",
                proc.Trigger,
                proc.EffectType,
                proc.Chance));
        }
    }

    private static string BuildActivatableItemName(RunContext context)
    {
        var active = context.State.Build.ActivatableItem;
        if (active == null)
        {
            return LocalizationService.Current.Text("ui.common.empty");
        }

        var item = context.Content.GetItem(active.ItemId);
        return LocalizationService.Current.ContentName("item", item.Id, item.DisplayName);
    }

    private static string BuildActivatableUsesText(RunContext context)
    {
        var active = context.State.Build.ActivatableItem;
        if (active == null)
        {
            return LocalizationService.Current.Text("ui.view_model.activatable_empty");
        }

        return active.HasUnlimitedActivations
            ? LocalizationService.Current.Text("ui.common.unlimited")
            : LocalizationService.Current.Text("ui.market.activatable_limited", active.RemainingActivations, active.MaxActivations);
    }

    private static string BuildTagText(ItemRuntimeDefinition item)
    {
        return item.Tags.Count == 0
            ? LocalizationService.Current.Text("ui.view_model.tag_list_empty")
            : string.Join(", ", item.Tags.Select(tag => tag.ToString()));
    }

    private static string BuildRevealedItemText(ItemRuntimeDefinition item)
    {
        var statText = LocalizationService.Current.ContentField("item", item.Id, "revealed_stat_text", item.RevealedStatText);
        var behaviorText = LocalizationService.Current.ContentField("item", item.Id, "revealed_behavior_text", item.RevealedBehaviorText);
        var effectText = LocalizationService.Current.ContentField("item", item.Id, "revealed_effect_text", item.RevealedEffectText);
        var hasStats = !string.IsNullOrWhiteSpace(statText);
        var hasBehavior = !string.IsNullOrWhiteSpace(behaviorText);
        if (!hasStats && !hasBehavior)
        {
            return effectText;
        }

        var sections = new List<string>(2);
        if (hasStats)
        {
            sections.Add(LocalizationService.Current.Text("ui.market.revealed_stats_section", statText.Trim()));
        }

        if (hasBehavior)
        {
            sections.Add(LocalizationService.Current.Text("ui.market.revealed_behavior_section", behaviorText.Trim()));
        }

        return string.Join("\n\n", sections);
    }
}
