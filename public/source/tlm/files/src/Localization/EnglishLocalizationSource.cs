using TheLastMage.Data.Definitions;
using TheLastMage.Foundation;

namespace TheLastMage.Localization;

public static class EnglishLocalizationSource
{
    public static Dictionary<string, string> Build(ContentCatalog catalog)
    {
        var entries = new Dictionary<string, string>(StringComparer.Ordinal);
        foreach (var entry in BuildSourceEntries(catalog))
        {
            entries[entry.Key] = entry.English;
        }

        return entries;
    }

    public static IReadOnlyList<LocalizationSourceEntry> BuildSourceEntries(ContentCatalog catalog)
    {
        var entries = new List<LocalizationSourceEntry>();
        AddUiEntries(entries);
        AddContentEntries(entries, catalog);
        return entries;
    }

    public static void AddContentEntries(IDictionary<string, string> entries, ContentCatalog catalog)
    {
        foreach (var spell in catalog.Spells)
        {
            AddCommon(entries, "spell", spell);
        }

        foreach (var item in catalog.Items)
        {
            AddCommon(entries, "item", item);
            Add(entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "hidden_description"), item.HiddenDescription);
            Add(entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_stat_text"), item.RevealedStatText);
            Add(entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_behavior_text"), item.RevealedBehaviorText);
            Add(entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_effect_text"), item.GetCombinedRevealedText());
        }

        foreach (var enemy in catalog.Enemies)
        {
            AddCommon(entries, "enemy", enemy);
        }

        foreach (var wave in catalog.Waves)
        {
            AddCommon(entries, "wave", wave);
        }

        foreach (var defense in catalog.Defenses)
        {
            AddCommon(entries, "defense", defense);
        }

        foreach (var mage in catalog.Mages)
        {
            AddCommon(entries, "mage", mage);
        }

        foreach (var achievement in catalog.Achievements)
        {
            AddCommon(entries, "achievement", achievement);
            Add(entries, LocalizationKeys.ContentField("achievement", ContentId.From(achievement.Id), "requirement"), achievement.RequirementKey);
        }
    }

    public static void AddContentEntries(ICollection<LocalizationSourceEntry> entries, ContentCatalog catalog)
    {
        foreach (var spell in catalog.Spells)
        {
            AddCommon(entries, "spell", spell);
        }

        foreach (var item in catalog.Items)
        {
            AddCommon(entries, "item", item);
            Add(entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "hidden_description"), item.HiddenDescription, "Item hidden text", "Shown before the item has been picked once.");
            Add(entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_stat_text"), item.RevealedStatText, "Item revealed stats", "Shown after discovery. Preserve numeric values and symbols.");
            Add(entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_behavior_text"), item.RevealedBehaviorText, "Item revealed behavior", "Shown after discovery.");
            Add(entries, LocalizationKeys.ContentField("item", ContentId.From(item.Id), "revealed_effect_text"), item.GetCombinedRevealedText(), "Item combined reveal text", "Legacy/fallback reveal text.");
        }

        foreach (var enemy in catalog.Enemies)
        {
            AddCommon(entries, "enemy", enemy);
        }

        foreach (var wave in catalog.Waves)
        {
            AddCommon(entries, "wave", wave);
        }

        foreach (var defense in catalog.Defenses)
        {
            AddCommon(entries, "defense", defense);
        }

        foreach (var mage in catalog.Mages)
        {
            AddCommon(entries, "mage", mage);
        }

        foreach (var achievement in catalog.Achievements)
        {
            AddCommon(entries, "achievement", achievement);
            Add(entries, LocalizationKeys.ContentField("achievement", ContentId.From(achievement.Id), "requirement"), achievement.RequirementKey, "Achievement requirement", "Short unlock requirement text.");
        }
    }

    private static void AddCommon(IDictionary<string, string> entries, string kind, ContentDefinition definition)
    {
        var id = ContentId.From(definition.Id);
        Add(entries, LocalizationKeys.ContentName(kind, id), definition.DisplayName);
        Add(entries, LocalizationKeys.ContentDescription(kind, id), definition.Description);
    }

    private static void AddCommon(ICollection<LocalizationSourceEntry> entries, string kind, ContentDefinition definition)
    {
        var id = ContentId.From(definition.Id);
        var context = $"Content {kind}";
        Add(entries, LocalizationKeys.ContentName(kind, id), definition.DisplayName, context, "Display name.");
        Add(entries, LocalizationKeys.ContentDescription(kind, id), definition.Description, context, "Description or designer-facing player text where used.");
    }

    private static void AddUiEntries(ICollection<LocalizationSourceEntry> entries)
    {
        Add(entries, "ui.common.empty", "-", "UI common", "");
        Add(entries, "ui.common.unlimited", "unlimited", "UI common", "");
        Add(entries, "ui.common.sold_out", "Sold out", "UI common", "");
        Add(entries, "ui.common.none", "None", "UI common", "");

        Add(entries, "ui.frontend.title", "The Last Mage", "Front-end", "Main menu title.");
        Add(entries, "ui.frontend.subtitle", "Steam playtest profile", "Front-end", "Main menu subtitle.");
        Add(entries, "ui.frontend.main_subtitle", "Selected save profile", "Front-end", "Main menu subtitle after save selection.");
        Add(entries, "ui.frontend.active_slot", "Save Slot {0}", "Front-end", "Preserve placeholder.");
        Add(entries, "ui.frontend.choose_save", "Choose Save", "Front-end", "Save selection title.");
        Add(entries, "ui.frontend.choose_save_subtitle", "Choose a profile slot to continue.", "Front-end", "Save selection subtitle.");
        Add(entries, "ui.frontend.delete_slot_subtitle", "Choose the save slot to delete.", "Front-end", "Save deletion mode subtitle.");
        Add(entries, "ui.frontend.new_run", "New Run", "Front-end", "Main menu button.");
        Add(entries, "ui.frontend.continue", "Continue", "Front-end", "Main menu button and slot picker title.");
        Add(entries, "ui.frontend.suspended_run_summary", "Suspended run: {0}", "Front-end", "Preserve placeholder.");
        Add(entries, "ui.frontend.stats", "Stats", "Front-end", "Main menu button and stats title.");
        Add(entries, "ui.frontend.options", "Options", "Front-end", "Main menu button and options title.");
        Add(entries, "ui.frontend.credits", "Credits", "Front-end", "Main menu button and credits title.");
        Add(entries, "ui.frontend.feedback", "Feedback/Bug", "Front-end", "Main menu button and feedback title.");
        Add(entries, "ui.frontend.quit", "Quit", "Front-end", "Main menu button.");
        Add(entries, "ui.frontend.back", "Back", "Front-end", "Back button.");
        Add(entries, "ui.frontend.mage_selection", "Mage Selection", "Front-end", "Mage selection title.");
        Add(entries, "ui.frontend.mage_selection_subtitle", "Choose a mage to start the run.", "Front-end", "Mage selection subtitle.");
        Add(entries, "ui.frontend.unlocked", "Unlocked", "Front-end", "Mage lock state.");
        Add(entries, "ui.frontend.locked", "Locked", "Front-end", "Mage lock state.");
        Add(entries, "ui.frontend.unlock_requirement", "Unlock: {0}", "Front-end", "Preserve placeholder.");
        Add(entries, "ui.frontend.mage_card", "{0}  {1}\nStarting spell: {2}\nHealth {3:0.#}  Move {4:0.##}  Fire rate {5:0.##}x  Luck {6:0.##}\nRuns {7}  Wins {8}  Deaths {9}  Best humanity {10}  Best time {11}\n{12}", "Front-end", "Preserve placeholders.");
        Add(entries, "ui.frontend.slot_subtitle", "Select a profile slot.", "Front-end", "Slot picker subtitle.");
        Add(entries, "ui.frontend.slot_in_use", "In use", "Front-end", "Slot state.");
        Add(entries, "ui.frontend.slot_empty", "Empty", "Front-end", "Slot state.");
        Add(entries, "ui.frontend.slot_card", "Slot {0}  {1}\nRuns {2}  Wins {3}  Deaths {4}  Best day {5}\nHumanity {6}  Enemies {7}  Items {8}  Achievements {9}", "Front-end", "Preserve placeholders.");
        Add(entries, "ui.frontend.play_slot", "Play Slot {0}", "Front-end", "Preserve placeholder.");
        Add(entries, "ui.frontend.delete_slot", "Delete", "Front-end", "Delete slot button.");
        Add(entries, "ui.frontend.delete_save", "Delete Save", "Front-end", "Enter save deletion mode button.");
        Add(entries, "ui.frontend.confirm_delete", "Confirm Delete", "Front-end", "Delete confirmation button.");
        Add(entries, "ui.frontend.cancel", "Cancel", "Front-end", "Cancel button.");
        Add(entries, "ui.frontend.stats_subtitle", "Profile and achievement progress.", "Front-end", "Stats subtitle.");
        Add(entries, "ui.frontend.player_stats", "Runs {0}  Wins {1}  Deaths {2}\nTime survived {3}  Enemies {4}  Humanity {5}\nItems discovered {6}  Mages unlocked {7}  Spells cast {8}", "Front-end", "Preserve placeholders.");
        Add(entries, "ui.frontend.no_spell_usage", "No spell usage recorded.", "Front-end", "Stats empty state.");
        Add(entries, "ui.frontend.spell_usage_line", "{0}: {1}", "Front-end", "Preserve placeholders.");
        Add(entries, "ui.frontend.options_subtitle", "Saved player preferences.", "Front-end", "Options subtitle.");
        Add(entries, "ui.frontend.options_summary", "Language {0}\nDisplay {1}  Resolution {2}  VSync {3}  FPS {4}\nRender scale {5:0.##}x  Master {6:0.##}\nMouse {7:0.##}x  Gamepad {8:0.##}x", "Front-end", "Preserve placeholders.");
        Add(entries, "ui.frontend.credits_subtitle", "Production build credits.", "Front-end", "Credits subtitle.");
        Add(entries, "ui.frontend.credits_body", "The Last Mage\nDesign, code, and production by the project owner with Codex implementation support.", "Front-end", "Credits text.");
        Add(entries, "ui.frontend.feedback_subtitle", "Playtest report entry point.", "Front-end", "Feedback subtitle.");
        Add(entries, "ui.frontend.feedback_body", "Feedback reporting is available from this front-end entry point for Steam playtest builds.", "Front-end", "Feedback text.");

        Add(entries, "ui.hud.place_barricade", "Barricade", "HUD", "Defense action button.");
        Add(entries, "ui.hud.place_seal", "Seal", "HUD", "Defense action button.");
        Add(entries, "ui.hud.repair", "Repair", "HUD", "Defense action button.");
        Add(entries, "ui.hud.next_day", "Next Day", "HUD", "Defense action button.");
        Add(entries, "ui.hud.status_active", "Phase {0}  Day {1}\nSouls {2}  Materials {3}  Humanity {4}\nActive {5}  Uses {6}\nWave {7}  Enemies {8}/{9}  Defenses {10}", "HUD", "Preserve placeholders.");
        Add(entries, "ui.hud.status_terminal", "Phase {0}  Day {1}\nSouls {2}  Materials {3}  Humanity {4}\nRun ended  {5}", "HUD", "Preserve placeholders.");
        Add(entries, "ui.hud.health", "Health {0:0}/{1:0}", "HUD", "Preserve placeholders.");
        Add(entries, "ui.hud.prompt_with_readability", "{0}\nReadability: {1}", "HUD", "Preserve placeholders.");
        Add(entries, "ui.hud.cooldown_slot", "{0}{1} {2}", "HUD", "Preserve placeholders.");

        Add(entries, "ui.prompt.day_combat", "Fight the wave. {0} cast, {1}/{2} spells, {3} active item, {4} pause.", "Run prompt", "Preserve placeholders.");
        Add(entries, "ui.prompt.night_market", "Choose one free night relic. {0} confirm, {1} back. Undiscovered relics hide stats and behavior text.", "Run prompt", "Preserve placeholders.");
        Add(entries, "ui.prompt.night_defense", "Prepare defenses, then start the next day. {0}  {1} confirm, {2} pause.", "Run prompt", "Preserve placeholders.");
        Add(entries, "ui.prompt.run_victory", "Humanity has fallen. {0}  {1} continue.", "Run prompt", "Preserve placeholders.");
        Add(entries, "ui.prompt.run_defeat", "Mage death ended the run. {0}  {1} continue.", "Run prompt", "Preserve placeholders.");
        Add(entries, "ui.prompt.run_setup", "Run setup. {0} pause.", "Run prompt", "Preserve placeholder.");
        Add(entries, "ui.readability.summary", "shake {0:0.##}, flash {1:0.##}, text {2:0.##}x", "Readability", "Preserve placeholders.");

        Add(entries, "ui.market.header", "Night Choice  Day {0}", "Night market", "Preserve placeholder.");
        Add(entries, "ui.market.choose", "Choose", "Night market", "Offer button.");
        Add(entries, "ui.market.reroll", "Reroll", "Night market", "Offer reroll button.");
        Add(entries, "ui.market.tags", "Tags: {0}", "Night market", "Preserve placeholder.");
        Add(entries, "ui.market.offer", "{0}\n{1}", "Night market", "Offer card layout. Preserve placeholders.");
        Add(entries, "ui.market.no_reveal_text", "No reveal text authored.", "Night market", "");
        Add(entries, "ui.market.hidden_suffix", "Stats and behavior text hidden until first pickup.", "Night market", "Shown for undiscovered items.");
        Add(entries, "ui.market.hidden_body", "{0}\nStats and behavior text hidden until first pickup.", "Night market", "Preserve placeholder.");
        Add(entries, "ui.market.revealed_stats_section", "Stats\n{0}", "Night market", "Preserve placeholder.");
        Add(entries, "ui.market.revealed_behavior_section", "Behavior\n{0}", "Night market", "Preserve placeholder.");
        Add(entries, "ui.market.modifier_sources", "Modifier Sources", "Night market", "");
        Add(entries, "ui.market.no_acquired_items", "No acquired items.", "Night market", "");
        Add(entries, "ui.market.modifier_line", "{0} x{1}: {2} {3:0.##} {4}{5}", "Night market", "Preserve placeholders.");
        Add(entries, "ui.market.modifier_tag", " tag={0}", "Night market", "Preserve leading space and placeholder.");
        Add(entries, "ui.market.activatable_line", "Activatable: {0} ({1})", "Night market", "Preserve placeholders.");
        Add(entries, "ui.market.activatable_limited", "{0}/{1}", "Night market", "Preserve placeholders.");

        Add(entries, "ui.pause.title", "Pause", "Pause menu", "");
        Add(entries, "ui.pause.subtitle", "Game paused", "Pause menu", "Pause menu subtitle.");
        Add(entries, "ui.pause.resume", "Resume", "Pause menu", "Button.");
        Add(entries, "ui.pause.main_menu", "Main Menu", "Pause menu", "Button.");
        Add(entries, "ui.pause.confirm_main_menu", "Confirm Main Menu", "Pause menu", "Button.");
        Add(entries, "ui.pause.feedback", "Feedback/Bug", "Pause menu", "Button.");
        Add(entries, "ui.pause.quit", "Quit", "Pause menu", "Button.");
        Add(entries, "ui.pause.main_menu_checkpoint_warning", "Returning to the main menu keeps your suspended run at: {0}. Progress after that checkpoint is not saved.", "Pause menu", "Preserve placeholder.");
        Add(entries, "ui.pause.summary", "Display {0}  {1}  VSync {2}  FPS {3}\nRender {4:0.##}x  Master {5:0.##}", "Pause menu", "Preserve placeholders.");
        Add(entries, "ui.pause.video_options", "Video", "Pause menu", "Section title.");
        Add(entries, "ui.pause.display_mode_label", "Display Mode", "Pause menu", "Option label.");
        Add(entries, "ui.pause.resolution_label", "Resolution", "Pause menu", "Option label.");
        Add(entries, "ui.pause.vsync_label", "VSync", "Pause menu", "Option label.");
        Add(entries, "ui.pause.fps_cap_label", "FPS Cap", "Pause menu", "Option label.");
        Add(entries, "ui.pause.render_scale_label", "Render Scale", "Pause menu", "Slider label.");
        Add(entries, "ui.pause.render_scale_description", "Scales the 3D render buffer.", "Pause menu", "Slider description.");
        Add(entries, "ui.pause.audio_options", "Audio", "Pause menu", "Section title.");
        Add(entries, "ui.pause.master_volume_label", "Master Volume", "Pause menu", "Slider label.");
        Add(entries, "ui.pause.master_volume_description", "Controls the master audio bus volume.", "Pause menu", "Slider description.");
        Add(entries, "ui.pause.controls_options", "Controls", "Pause menu", "Section title.");
        Add(entries, "ui.pause.mouse_sensitivity_label", "Mouse Sensitivity", "Pause menu", "Slider label.");
        Add(entries, "ui.pause.mouse_sensitivity_description", "Scales mouse look speed.", "Pause menu", "Slider description.");
        Add(entries, "ui.pause.gamepad_sensitivity_label", "Gamepad Sensitivity", "Pause menu", "Slider label.");
        Add(entries, "ui.pause.gamepad_sensitivity_description", "Scales right-stick look speed.", "Pause menu", "Slider description.");
        Add(entries, "ui.pause.camera_shake_label", "Camera Shake Intensity", "Pause menu", "Slider label.");
        Add(entries, "ui.pause.camera_shake_description", "Scales camera shake requested by spell impacts, boss hits, and other feedback events.", "Pause menu", "Slider description.");
        Add(entries, "ui.pause.flash_label", "Screen Flash Intensity", "Pause menu", "Slider label.");
        Add(entries, "ui.pause.flash_description", "Scales full-screen flash or hit-flash feedback when those effects are authored.", "Pause menu", "Slider description.");
        Add(entries, "ui.pause.text_scale_label", "HUD Text Scale", "Pause menu", "Slider label.");
        Add(entries, "ui.pause.text_scale_description", "Scales HUD and readability text size for accessibility.", "Pause menu", "Slider description.");
        Add(entries, "ui.pause.language_label", "Language", "Pause menu", "Locale selector label.");
        Add(entries, "ui.pause.language_description", "Uses translated text where a localization column exists, with English fallback.", "Pause menu", "Locale selector description.");
        Add(entries, "ui.pause.input_bindings", "Input", "Pause menu", "Section title.");
        Add(entries, "ui.pause.input_description", "Prompts update from the active keyboard, mouse, and controller bindings.", "Pause menu", "");
        Add(entries, "ui.pause.rebind", "Rebind", "Pause menu", "Button.");
        Add(entries, "ui.pause.rebind_waiting", "Press input...", "Pause menu", "Button while waiting for a replacement binding.");
        Add(entries, "ui.pause.binding_summary", "Keyboard {0}  Controller {1}", "Pause menu", "Preserve placeholders.");
        Add(entries, "ui.pause.binding_cast", "Cast", "Pause menu", "Input action label.");
        Add(entries, "ui.pause.binding_active_item", "Active Item", "Pause menu", "Input action label.");
        Add(entries, "ui.pause.binding_spell_previous", "Previous Spell", "Pause menu", "Input action label.");
        Add(entries, "ui.pause.binding_spell_next", "Next Spell", "Pause menu", "Input action label.");
        Add(entries, "ui.pause.binding_confirm", "Confirm", "Pause menu", "Input action label.");
        Add(entries, "ui.pause.binding_back", "Back", "Pause menu", "Input action label.");
        Add(entries, "ui.pause.binding_pause", "Pause", "Pause menu", "Input action label.");
        Add(entries, "ui.pause.debug_shortcuts", "Debug Shortcuts", "Pause menu", "Dev-mode section title.");
        Add(entries, "ui.options.lb_hint", "LB", "Options", "Controller previous tab hint.");
        Add(entries, "ui.options.rb_hint", "RB", "Options", "Controller next tab hint.");
        Add(entries, "ui.options.footer_hint", "Use left/right to change values. LB/RB switches categories.", "Options", "Input hint.");
        Add(entries, "ui.options.tab_display", "Display", "Options", "Tab.");
        Add(entries, "ui.options.tab_graphics", "Graphics", "Options", "Tab.");
        Add(entries, "ui.options.tab_gameplay", "Gameplay", "Options", "Tab.");
        Add(entries, "ui.options.tab_sound", "Sound", "Options", "Tab.");
        Add(entries, "ui.options.tab_language", "Language", "Options", "Tab.");
        Add(entries, "ui.options.tab_controls", "Controls", "Options", "Tab.");
        Add(entries, "ui.options.display_mode", "Window Mode", "Options", "Setting.");
        Add(entries, "ui.options.display_mode_desc", "Choose between windowed, exclusive fullscreen, and borderless fullscreen display.", "Options", "Setting description.");
        Add(entries, "ui.options.resolution", "Resolution", "Options", "Setting.");
        Add(entries, "ui.options.resolution_desc", "Sets the backbuffer size used by the game window.", "Options", "Setting description.");
        Add(entries, "ui.options.vsync", "VSync", "Options", "Setting.");
        Add(entries, "ui.options.vsync_desc", "Synchronizes frame presentation with the monitor to reduce tearing.", "Options", "Setting description.");
        Add(entries, "ui.options.fps_cap", "FPS Cap", "Options", "Setting.");
        Add(entries, "ui.options.fps_cap_desc", "Limits the maximum frame rate. Uncapped lets the engine render as fast as possible.", "Options", "Setting description.");
        Add(entries, "ui.options.render_scale", "Resolution Modifier", "Options", "Setting.");
        Add(entries, "ui.options.render_scale_desc", "Scales the 3D render buffer before final output. Lower values improve performance at the cost of sharpness.", "Options", "Setting description.");
        Add(entries, "ui.options.master_volume", "Master Volume", "Options", "Setting.");
        Add(entries, "ui.options.master_volume_desc", "Controls the global audio bus volume.", "Options", "Setting description.");
        Add(entries, "ui.options.mouse_sensitivity", "Mouse Sensitivity", "Options", "Setting.");
        Add(entries, "ui.options.mouse_sensitivity_desc", "Scales mouse look speed during gameplay.", "Options", "Setting description.");
        Add(entries, "ui.options.gamepad_sensitivity", "Gamepad Sensitivity", "Options", "Setting.");
        Add(entries, "ui.options.gamepad_sensitivity_desc", "Scales right-stick look speed during gameplay.", "Options", "Setting description.");
        Add(entries, "ui.options.language", "Language", "Options", "Setting.");
        Add(entries, "ui.options.language_desc", "Changes the active localization column with English fallback for missing text.", "Options", "Setting description.");
        Add(entries, "ui.options.rebind_desc", "Shows keyboard/mouse and controller bindings. Select Rebind, then press the replacement input.", "Options", "Setting description.");
        Add(entries, "ui.options.rebind_waiting_desc", "Waiting for the next valid keyboard, mouse, or controller input.", "Options", "Setting description.");
        Add(entries, "ui.options.value_windowed", "Windowed", "Options", "Value.");
        Add(entries, "ui.options.value_fullscreen", "Fullscreen", "Options", "Value.");
        Add(entries, "ui.options.value_borderless", "Borderless", "Options", "Value.");
        Add(entries, "ui.options.value_on", "On", "Options", "Value.");
        Add(entries, "ui.options.value_off", "Off", "Options", "Value.");
        Add(entries, "ui.options.value_uncapped", "Uncapped", "Options", "Value.");
        Add(entries, "ui.options.value_0_percent", "0%", "Options", "Value.");
        Add(entries, "ui.options.value_25_percent", "25%", "Options", "Value.");
        Add(entries, "ui.options.value_50_percent", "50%", "Options", "Value.");
        Add(entries, "ui.options.value_65_percent", "65%", "Options", "Value.");
        Add(entries, "ui.options.value_75_percent", "75%", "Options", "Value.");
        Add(entries, "ui.options.value_80_percent", "80%", "Options", "Value.");
        Add(entries, "ui.options.value_100_percent", "100%", "Options", "Value.");
        Add(entries, "ui.options.value_125_percent", "125%", "Options", "Value.");
        Add(entries, "ui.options.value_150_percent", "150%", "Options", "Value.");
        Add(entries, "ui.options.value_200_percent", "200%", "Options", "Value.");
        Add(entries, "ui.options.value_300_percent", "300%", "Options", "Value.");
        Add(entries, "ui.options.value_30", "30", "Options", "Value.");
        Add(entries, "ui.options.value_60", "60", "Options", "Value.");
        Add(entries, "ui.options.value_90", "90", "Options", "Value.");
        Add(entries, "ui.options.value_120", "120", "Options", "Value.");
        Add(entries, "ui.options.value_144", "144", "Options", "Value.");
        Add(entries, "ui.options.value_240", "240", "Options", "Value.");
        Add(entries, "ui.options.value_1280x720", "1280 x 720", "Options", "Value.");
        Add(entries, "ui.options.value_1600x900", "1600 x 900", "Options", "Value.");
        Add(entries, "ui.options.value_1920x1080", "1920 x 1080", "Options", "Value.");
        Add(entries, "ui.options.value_2560x1440", "2560 x 1440", "Options", "Value.");
        Add(entries, "ui.options.value_3840x2160", "3840 x 2160", "Options", "Value.");
        Add(entries, "ui.pause.shortcut_debug_overlay", "Debug overlay", "Pause menu", "Dev-mode shortcut label.");
        Add(entries, "ui.pause.shortcut_debug_overlay_details", "Debug overlay details", "Pause menu", "Dev-mode shortcut label.");
        Add(entries, "ui.pause.shortcut_build_inspector", "Build inspector", "Pause menu", "Dev-mode shortcut label.");
        Add(entries, "ui.pause.shortcut_tooling_panel", "Debug tooling panel", "Pause menu", "Dev-mode shortcut label.");
        Add(entries, "ui.pause.shortcut_meta_panel", "Meta progression debug", "Pause menu", "Dev-mode shortcut label.");
        Add(entries, "ui.pause.shortcut_load_run_scene", "Load run scene", "Pause menu", "Dev-mode shortcut label.");
        Add(entries, "ui.pause.unbound", "unbound", "Pause menu", "Input binding fallback.");

        Add(entries, "ui.view_model.passive_items", "Passive Items", "Build inspector", "");
        Add(entries, "ui.view_model.no_passive_relics", "No passive relics.", "Build inspector", "");
        Add(entries, "ui.view_model.activatable_item", "Activatable Item", "Build inspector", "");
        Add(entries, "ui.view_model.no_activatable_item", "No activatable item equipped.", "Build inspector", "");
        Add(entries, "ui.view_model.item_stack", "{0} x{1}", "Build inspector", "Preserve placeholders.");
        Add(entries, "ui.view_model.activatable_item_uses", "{0} uses={1}", "Build inspector", "Preserve placeholders.");
        Add(entries, "ui.view_model.active_effect_line", "  active {0} value={1:0.##} radius={2:0.##} duration={3:0.##}", "Build inspector", "Preserve leading spaces and placeholders.");
        Add(entries, "ui.view_model.modifier_rule", "  {0} {1:0.##} {2}{3}", "Build inspector", "Preserve leading spaces and placeholders.");
        Add(entries, "ui.view_model.proc_rule", "  proc {0} {1} {2:0.#}%", "Build inspector", "Preserve leading spaces and placeholders.");
        Add(entries, "ui.view_model.activatable_empty", "empty", "Build inspector", "");
        Add(entries, "ui.view_model.tag_list_empty", "-", "Build inspector", "");
        Add(entries, "ui.view_model.attribute_text", "Health {0:0.#}\nMove {1:0.##}\nDamage {2:0.##}/10\nFire rate {3:0.##}x\nRange {4:0.##}/10\nLuck {5:0.##}\nDuration {6:0.##}/10", "Build inspector", "Preserve placeholders.");
        Add(entries, "ui.view_model.proc_safety", "Cast flow: {0}\nProc budget: {1} used, {2} blocked\nLast proc: {3}", "Build inspector", "Preserve placeholders.");
        Add(entries, "ui.view_model.feedback_budgets", "Feedback: {0}\nAudio: {1}", "Build inspector", "Preserve placeholders.");
    }

    private static void Add(IDictionary<string, string> entries, string key, string value)
    {
        entries[key] = value;
    }

    private static void Add(ICollection<LocalizationSourceEntry> entries, string key, string english, string context, string notes)
    {
        entries.Add(new LocalizationSourceEntry(key, context, english, notes));
    }
}
