import type {
  CodeReference,
  ContentSection,
  MediaSlot,
  ProjectPageContent,
} from "./types";

const mediaSlot = (
  id: string,
  type: MediaSlot["type"],
  caption: string,
): MediaSlot => ({
  id,
  type,
  aspectRatio: "16 / 9",
  status: "placeholder",
  caption,
});

const codeReference = (
  label: string,
  file: string,
  line?: number,
): CodeReference => ({
  kind: "code-reference",
  label,
  file,
  ...(line ? { line } : {}),
});

export const tlmMediaSlots: MediaSlot[] = [
  mediaSlot("gameplay-loop", "video", "Gameplay loop capture pending: mage selection, run start, day combat, night market, and run summary"),
  mediaSlot("spell-item-synergy", "video", "Spell, item, proc, and build-synergy capture pending"),
  mediaSlot("horde-tower-defense", "video", "Horde routing, tower pressure, barricade, and explosive seal capture pending"),
  mediaSlot("day-night-market", "video", "Day/night phase, market offer, reroll, and progression capture pending"),
  mediaSlot("authoring-tools", "video", "Item authoring, validation, tags, and localization tooling capture pending"),
  mediaSlot("production-shell", "video", "Save slots, options, controller prompts, localization, and front-end flow capture pending"),
  mediaSlot("source-tooling-proof", "video", "Regression, Balance Lab, reports, and source-browser proof capture pending"),
];

export const tlmSections: ContentSection[] = [
  {
    title: "Production gameplay architecture",
    mediaSlotId: "gameplay-loop",
    body:
      "TLM is built as a Godot 4.7 C# production spine rather than a throwaway prototype. The run composition root loads static Resource catalogs, validates them, creates a run-scoped context, wires commands, events, save/profile data, localization, pools, and gameplay systems, then lets those systems own the simulation instead of scattering rules through scenes or UI nodes.",
    points: [
      [
        codeReference("RunControllerNode", "src/Gameplay/Run/RunControllerNode.cs", 1),
        " is the inspectable composition root for content loading, validation, run state, event bus, command dispatcher, profile access, localization setup, and ordered runtime systems.",
      ],
      "Godot scenes, Resources, docs, and reports are part of the evidence base, but the source-browser links stay on committed C# files so every link is stable in the static snapshot.",
      "The page presents architecture and playtest-readiness work without offering a public build, claiming a shipped Steam release, completed Steam integration, final balance, or final media polish.",
    ],
    sourceReferences: [
      codeReference("RunControllerNode", "src/Gameplay/Run/RunControllerNode.cs", 1),
      codeReference("RunContext", "src/Gameplay/Run/RunContext.cs", 1),
      codeReference("RunPhaseStateMachine", "src/Gameplay/Run/RunPhaseStateMachine.cs", 1),
      codeReference("CommandDispatcher", "src/Gameplay/Commands/CommandDispatcher.cs", 1),
      codeReference("GameEventBus", "src/Foundation/Events/GameEventBus.cs", 1),
      codeReference("ContentRegistry", "src/Data/Runtime/ContentRegistry.cs", 1),
    ],
  },
  {
    title: "Combat, spells, items, and build synergies",
    mediaSlotId: "spell-item-synergy",
    body:
      "Combat is routed through shared systems for spell slots, cast flow, effect execution, projectiles, AoE, status effects, damage, item modifiers, and proc chains. The first seven production spell directions and item behaviors sit on the same C# contracts, so build-changing items can affect damage, range, duration, spell count, homing, splitting, burning ground, activatable effects, and cast-flow behavior without creating one-off spell scripts.",
    points: [
      [
        codeReference("SpellSystem", "src/Gameplay/Spells/SpellSystem.cs", 1),
        " dispatches shared effect executors for projectiles, frost, beams, firewall, Blizzard, Tornado, and summons instead of treating each spell as a custom scene script.",
      ],
      [
        codeReference("ProcSystem", "src/Gameplay/Items/ProcSystem.cs", 1),
        " keeps triggered item chains bounded with recursion and per-frame budgets, which is critical for roguelite build synergies.",
      ],
      "Production item Resources and Godot-authored content are described as evidence, not linked directly, because the committed source browser intentionally exposes C# implementation files only.",
    ],
    sourceReferences: [
      codeReference("SpellSystem", "src/Gameplay/Spells/SpellSystem.cs", 1),
      codeReference("EffectResolver", "src/Gameplay/Spells/EffectResolver.cs", 1),
      codeReference("ProjectileSystem", "src/Gameplay/Projectiles/ProjectileSystem.cs", 1),
      codeReference("AoESystem", "src/Gameplay/AoE/AoESystem.cs", 1),
      codeReference("CombatSystem", "src/Gameplay/Combat/CombatSystem.cs", 1),
      codeReference("ProcSystem", "src/Gameplay/Items/ProcSystem.cs", 1),
      codeReference("ItemEffectCompiler", "src/Data/Definitions/ItemEffectCompiler.cs", 1),
    ],
  },
  {
    title: "Horde enemies, tower routing, and defenses",
    mediaSlotId: "horde-tower-defense",
    body:
      "The horde work is structured around real active enemies with health, attacks, routing, spatial queries, presentation binding, and debug metrics. Tower routing is treated as gameplay pressure: enemies chase the mage outside, route through entrance and stairs when the mage retreats to the top, and attack defenses when blockers or threat priorities justify it. The tower breach is pressure, not a fail state.",
    points: [
      [
        codeReference("EnemySystem", "src/Gameplay/Enemies/EnemySystem.cs", 1),
        " owns enemy simulation, attack checks, defense targeting, separation, route decisions, actor binding, and per-frame debug metrics.",
      ],
      [
        codeReference("TowerNavigationAdapter", "src/Gameplay/Navigation/TowerNavigationAdapter.cs", 1),
        " keeps the entrance, stair, and mage-base route explicit and exposes route summaries for validation.",
      ],
      "Benchmark scenes and tower/debug visuals are non-code evidence for this work; source references remain on the C# systems that make those scenes meaningful.",
    ],
    sourceReferences: [
      codeReference("EnemySystem", "src/Gameplay/Enemies/EnemySystem.cs", 1),
      codeReference("EnemySpatialRegistry", "src/Gameplay/Enemies/EnemySpatialRegistry.cs", 1),
      codeReference("TowerNavigationAdapter", "src/Gameplay/Navigation/TowerNavigationAdapter.cs", 1),
      codeReference("DefenseSystem", "src/Gameplay/Defenses/DefenseSystem.cs", 1),
      codeReference("DefensePlacementValidator", "src/Gameplay/Defenses/DefensePlacementValidator.cs", 1),
      codeReference("WaveDirectorSystem", "src/Gameplay/Waves/WaveDirectorSystem.cs", 1),
    ],
  },
  {
    title: "Day/night run loop and market progression",
    mediaSlotId: "day-night-market",
    body:
      "The run loop separates combat days, peak pressure, cleanup, night market decisions, night defense, victory, and defeat through explicit phase permissions. Market offers, item discovery, achievements, run rewards, and profile statistics are run-facing systems rather than UI-only state, which makes the loop inspectable and testable before public playtest packaging exists.",
    points: [
      "Night item offers are free choose-one rewards with deterministic generation, locked-item filtering, in-run exclusion, reroll support, and persistent item knowledge.",
      "Achievements and profile data support meta-progression evidence without implying final progression tuning or a completed public release path.",
      "Steam playtest work is positioned as roadmap and readiness infrastructure, not as completed Steam distribution.",
    ],
    sourceReferences: [
      codeReference("RunPhaseStateMachine", "src/Gameplay/Run/RunPhaseStateMachine.cs", 1),
      codeReference("PhasePermissions", "src/Gameplay/Run/PhasePermissions.cs", 1),
      codeReference("OfferGenerator", "src/Gameplay/Loot/OfferGenerator.cs", 1),
      codeReference("MarketSystem", "src/Gameplay/Loot/MarketSystem.cs", 1),
      codeReference("ItemKnowledgeService", "src/Gameplay/Items/ItemKnowledgeService.cs", 1),
      codeReference("AchievementSystem", "src/Gameplay/Achievements/AchievementSystem.cs", 1),
      codeReference("RunRewardSystem", "src/Gameplay/Run/RunRewardSystem.cs", 1),
    ],
  },
  {
    title: "Content authoring and validation tools",
    mediaSlotId: "authoring-tools",
    body:
      "The strongest production-tooling evidence is the content pipeline around Godot Resources, validation, item authoring, gameplay tags, localization data, and repeatable reports. The item editor and related docks are project-owned editor tooling, while production item content itself remains product-owner authored rather than Codex-generated.",
    points: [
      [
        codeReference("ItemAuthoringDock", "addons/the_last_mage_tools/docks/ItemAuthoringDock.cs", 1),
        " provides an editor-facing spreadsheet/create-edit workflow for item IDs, names, pool weights, tags, unlocks, hidden/revealed text, icons, passive effects, activatable effects, previews, and validation output.",
      ],
      [
        codeReference("ContentValidator", "src/Data/Validation/ContentValidator.cs", 1),
        " checks content contracts such as IDs, references, effects, unlocks, catalogs, and authored runtime assumptions.",
      ],
      "Resource files, scenes, CSV spreadsheets, saved reports, and roadmap docs are cited as supporting evidence in copy only; they are not source-browser targets.",
    ],
    sourceReferences: [
      codeReference("ContentValidator", "src/Data/Validation/ContentValidator.cs", 1),
      codeReference("ToolingGatePanelNode", "src/Debugging/Tools/ToolingGatePanelNode.cs", 1),
      codeReference("ItemAuthoringDock", "addons/the_last_mage_tools/docks/ItemAuthoringDock.cs", 1),
      codeReference("GameplayTagEditorDock", "addons/the_last_mage_tools/docks/GameplayTagEditorDock.cs", 1),
      codeReference("LocalizationDock", "addons/the_last_mage_tools/docks/LocalizationDock.cs", 1),
      codeReference("TheLastMageToolsPlugin", "addons/the_last_mage_tools/TheLastMageToolsPlugin.cs", 1),
    ],
  },
  {
    title: "Debugging, profiling, regression, and Balance Lab",
    mediaSlotId: "source-tooling-proof",
    body:
      "TLM has regression and analysis infrastructure for the project spine, combat contracts, stats, range, duration, night market, tower and defense passes, enemies, meta-progression, item behavior, localization, controller input, save slots, options, and Balance Lab simulations. Balance Lab is presented as engineering evidence and smoke/regression infrastructure, not as proof of final numerical balance.",
    points: [
      [
        codeReference("RegressionRunnerNode", "src/Debugging/Regression/RegressionRunnerNode.cs", 1),
        " centralizes visible/headless scenario coverage across the production systems and records pass/fail reports.",
      ],
      [
        codeReference("BalanceSimulationRunner", "src/Debugging/Balance/BalanceSimulationRunner.cs", 1),
        " supports deterministic simulation reports for engineering signals, while playtest feedback remains the authority for final tuning.",
      ],
      "Project-local saved reports and headless/editor regression outputs are evidence text rather than browser links because they are generated artifacts outside the C# source snapshot.",
    ],
    sourceReferences: [
      codeReference("RegressionRunnerNode", "src/Debugging/Regression/RegressionRunnerNode.cs", 1),
      codeReference("RunReportSystem", "src/Debugging/Reports/RunReportSystem.cs", 1),
      codeReference("GameplayProfilingSystem", "src/Debugging/Reports/GameplayProfilingSystem.cs", 1),
      codeReference("BalanceSimulationRunner", "src/Debugging/Balance/BalanceSimulationRunner.cs", 1),
      codeReference("BalanceReportWriter", "src/Debugging/Balance/BalanceReportWriter.cs", 1),
      codeReference("BalanceLabDock", "addons/the_last_mage_tools/docks/BalanceLabDock.cs", 1),
    ],
  },
  {
    title: "Production shell, save slots, options, controller, and localization",
    mediaSlotId: "production-shell",
    body:
      "The current shell work moves TLM beyond a debug-only start flow: the project has save-slot selection, mage selection, profile stats, suspended-run checkpoints, settings, pause/options surfaces, controller prompts/rebinding, and localization services. Sprint 19 options/pause is still framed as a checkpoint with product-owner UI smoke pending, so this page does not present those surfaces as final polish.",
    points: [
      [
        codeReference("FrontEndPanel", "src/UI/FrontEndPanel.cs", 1),
        " owns the save-slot-first front end, main menu, mage selection, stats, options, credits, feedback entry, and continue/new-run flow.",
      ],
      [
        codeReference("SaveServiceNode", "src/Save/SaveServiceNode.cs", 1),
        " and ",
        codeReference("RunCheckpointService", "src/Gameplay/Run/RunCheckpointService.cs", 1),
        " keep profile/settings persistence and safe suspended-run checkpoints separate from live scene objects.",
      ],
      "Controller support, localization, options, and save/profile flows are production-shell evidence; public packaging, final accessibility/readability polish, and Steam integration remain future validation work.",
    ],
    sourceReferences: [
      codeReference("FrontEndPanel", "src/UI/FrontEndPanel.cs", 1),
      codeReference("SaveServiceNode", "src/Save/SaveServiceNode.cs", 1),
      codeReference("RunCheckpointService", "src/Gameplay/Run/RunCheckpointService.cs", 1),
      codeReference("OptionsMenuPanel", "src/UI/OptionsMenuPanel.cs", 1),
      codeReference("InputBindingService", "src/Input/InputBindingService.cs", 1),
      codeReference("InputPromptService", "src/Input/InputPromptService.cs", 1),
      codeReference("LocalizationService", "src/Localization/LocalizationService.cs", 1),
    ],
  },
];

export const tlmPage: ProjectPageContent = {
  template: "original",
  overview:
    "The Last Mage is my self-directed Godot 4.7 C# FPS horde-survivor roguelite project. I use it to show production-system work around run architecture, spell/item synergies, horde and tower pressure, data-driven content, editor tooling, regression validation, save/profile flow, controller support, localization, and playtest-readiness infrastructure without offering a public build or claiming a Steam release, final balance, or final media pass.",
  roleScope:
    "Solo Godot 4.7 C# gameplay, tooling, UI, save/profile, localization, controller, validation, and production architecture work. This is original systems evidence, not a shipped commercial release: there is no public playable build here yet, Steam integration is not presented as complete, Balance Lab is engineering evidence rather than final balance approval, and all media slots are placeholders pending reviewed captures.",
  sections: tlmSections,
  proof: [
    "Godot 4.7 C# project targeting Godot.NET.Sdk/4.7.0-beta.1 with nullable C#, run-scoped systems, Godot Resource catalogs, and a committed C# source snapshot.",
    "Run composition root wires content validation, localization, profile/settings access, item knowledge, events, commands, pools, combat, spells, projectiles, AoE, statuses, summons, defenses, enemies, waves, market, achievements, reports, and UI.",
    "Production spell/item architecture covers shared effect executors, CombatSystem damage attribution, player attributes, item modifiers, bounded procs, activatable items, and cast-flow behaviors.",
    "Horde/tower evidence includes enemy simulation, spatial registry, tower route decisions, defense targeting, barricade/seal systems, wave director, route debug visuals, and 300-enemy benchmark-oriented infrastructure.",
    "Tooling evidence includes content validation, item authoring, gameplay tag editing, localization tooling, regression runner scenarios, profiling reports, Balance Lab simulations, and project-local saved reports.",
    "Production-shell evidence includes save slots, profile statistics, suspended-run checkpoints, options/pause checkpoint work, controller prompts/rebinding, localization service, and front-end flow.",
  ],
  improvements: [
    "Replace placeholder media with reviewed gameplay, spell/item synergy, horde/tower defense, authoring-tool, save/options/controller/localization, and source/tooling proof captures.",
    "Complete product-owner UI smoke for the current options/pause checkpoint before presenting those surfaces as polished.",
    "Continue Steam playtest integration and packaging validation before making any Steam-release claim.",
    "Use practical playtest feedback before treating Balance Lab outputs as final balance direction.",
    "Keep source references limited to committed C# files and describe Godot Resources, scenes, docs, scripts, and generated reports as supporting evidence text.",
  ],
};
