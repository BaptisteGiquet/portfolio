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

export const agassMediaSlots: MediaSlot[] = [
  mediaSlot("architecture", "video", "Plugin architecture and root game-module overview"),
  mediaSlot("sessions", "video", "Host, browser join, invite-code join, and frontend recovery"),
  mediaSlot("compatibility", "video", "Map/mod compatibility gate and rejected join states"),
  mediaSlot("interaction", "video", "Pickup, carried source hiding, preview movement, rotation, and placement"),
  mediaSlot("items", "video", "Data assets, random junk spawners, scaffolding, and held usable tools"),
  mediaSlot("economy", "video", "Shared wallet, physical shop purchase, HUD update, and sell zone"),
  mediaSlot("timed-run", "video", "Timer start, objective completion, summary, best time, and frontend return"),
  mediaSlot("events", "video", "Mad Cops and Mad Plane session events running in multiplayer"),
  mediaSlot("modding", "video", "Mods/maps screen, mod export, install layout, and session metadata"),
  mediaSlot("modding-kit", "video", "Modding Kit manifest authoring, validation, export, and install packaging"),
  mediaSlot("frontend", "video", "CommonUI main menu, session browser, mods/maps, settings, and rebinding"),
  mediaSlot("steam", "video", "Steam invite UI, accepted invite join, stats, and timed-run leaderboards"),
];

export const agassSections: ContentSection[] = [
  {
    title: "Modular Unreal architecture",
    mediaSlotId: "architecture",
    body:
      "AGASS is structured as a small UE 5.7 root game module plus focused internal plugins for common settings, frontend UI, online sessions, Steam, interaction, tower gameplay, economy, mods, and editor tooling. The split keeps platform, runtime, and authoring concerns separate instead of concentrating every feature in the root module.",
    points: [
      "The root AGASS module stays close to framework glue: game instance, game mode, game state, player controller, character, and cross-plugin composition.",
      [
        "Runtime plugins own gameplay and UX domains, while ",
        codeReference("AGASSEditorModule", "Plugins/AGASS/AGASSEditor/Source/AGASSEditor/Private/AGASSEditorModule.cpp", 1),
        " and ",
        codeReference("AGASSModdingKitSupportModule", "Plugins/AGASS/AGASSModdingKitSupport/Source/AGASSModdingKitSupport/Private/AGASSModdingKitSupportModule.cpp", 1),
        " keep mod export and creator-facing concerns out of shipped gameplay.",
      ],
      "C++ owns authority, replication, validation, and subsystem contracts; Blueprints, Widget Blueprints, Data Assets, maps, input assets, meshes, materials, and audio stay in the authored content layer.",
    ],
    sourceReferences: [
      codeReference("AAGASSGameModeBase", "Source/AGASS/Private/AGASSGameModeBase.cpp", 1),
      codeReference("AAGASSGameStateBase", "Source/AGASS/Private/AGASSGameStateBase.cpp", 1),
      codeReference("AGASSCommonModule", "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp", 1),
      codeReference("AGASSOnlineModule", "Plugins/AGASS/AGASSOnline/Source/AGASSOnline/Private/AGASSOnlineModule.cpp", 1),
      codeReference("AGASSTowerModule", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/AGASSTowerModule.cpp", 1),
      codeReference("AGASSEditorModule", "Plugins/AGASS/AGASSEditor/Source/AGASSEditor/Private/AGASSEditorModule.cpp", 1),
    ],
  },
  {
    title: "Multiplayer session flow",
    mediaSlotId: "sessions",
    body:
      "The online flow is built around listen-server co-op. Players host directly into the selected tower map, join through the browser, invite code, or Steam invite, and return safely to FrontendMap after failures or run completion.",
    points: [
      "Browser joins, invite-code joins, and Steam accepted invites converge into the same generic join path, so compatibility and travel handling are not duplicated per entry point.",
      "Hosting publishes build, selected map, active mod, invite-code, and compatibility metadata before opening the live gameplay map.",
      "Network failures, travel failures, closed runs, and return-to-frontend flows update UI-facing status instead of leaving players in a broken map state.",
    ],
    sourceReferences: [
      codeReference("UAGASSSessionSubsystem", "Plugins/AGASS/AGASSOnline/Source/AGASSOnline/Private/Subsystems/AGASSSessionSubsystem.cpp", 1),
      codeReference("UAGASSInviteCodeSubsystem", "Plugins/AGASS/AGASSOnline/Source/AGASSOnline/Private/Subsystems/AGASSInviteCodeSubsystem.cpp", 1),
      codeReference("UAGASSSessionInfoComponent", "Plugins/AGASS/AGASSOnline/Source/AGASSOnline/Private/Components/AGASSSessionInfoComponent.cpp", 1),
      codeReference("AAGASSGameModeBase", "Source/AGASS/Private/AGASSGameModeBase.cpp", 1),
      codeReference("UAGASSRunStateComponent", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Components/AGASSRunStateComponent.cpp", 1),
      codeReference("UAGASSSteamPlatformSubsystem", "Plugins/AGASS/AGASSSteam/Source/AGASSSteam/Private/Subsystems/AGASSSteamPlatformSubsystem.cpp", 1),
    ],
  },
  {
    title: "Session compatibility contract",
    mediaSlotId: "compatibility",
    body:
      "Joining is treated as a content contract, not only a network address. The host advertises build, selected map, active mods, and a stable content hash; clients check that state before travel, and the server revalidates key data in PreLogin.",
    points: [
      "The compatibility hash is based on stable map/mod IDs, compatibility versions, and tags rather than local install paths or timestamps.",
      "The session browser can surface disabled rows before travel, while PreLogin still protects the server from stale clients, direct invites, or malformed travel options.",
      "Join-in-progress remains available during active runs, then closes when the run moves into completion or frontend return.",
    ],
    sourceReferences: [
      codeReference("UAGASSSessionSubsystem", "Plugins/AGASS/AGASSOnline/Source/AGASSOnline/Private/Subsystems/AGASSSessionSubsystem.cpp", 1),
      codeReference("UAGASSModsSubsystem", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Private/Subsystems/AGASSModsSubsystem.cpp", 1),
      codeReference("AGASSModsTypes", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Types/AGASSModsTypes.h", 1),
      codeReference("AAGASSGameModeBase", "Source/AGASS/Private/AGASSGameModeBase.cpp", 1),
      codeReference("UAGASSSessionBrowserEntryWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/AGASSSessionBrowserEntryWidget.cpp", 1),
      codeReference("UAGASSMapDefinition", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSMapDefinition.h", 1),
    ],
  },
  {
    title: "Interaction, carry, preview, and placement",
    mediaSlotId: "interaction",
    body:
      "The core interaction loop lets players pick up items, move a responsive placement preview, rotate in controlled steps, adjust preview distance, and commit final placement through server validation. Placeables use a hidden source actor plus a replicated preview actor, while held-use tools attach to the character and use a separate action path.",
    points: [
      "Owner-local preview prediction keeps aiming responsive, while the server approves the final transform and replicates placement state to everyone.",
      "Validation covers range, overlap, actor-specific rules, item-definition hooks, touch support, snapping tolerance, and special scaffolding behavior.",
      "Usable items such as the Fumigene do not go through placement; they attach to character meshes, replicate carry presentation, and execute server-owned use logic.",
    ],
    sourceReferences: [
      codeReference("UAGASSInteractionComponent", "Plugins/AGASS/AGASSInteraction/Source/AGASSInteraction/Private/Components/AGASSInteractionComponent.cpp", 1),
      codeReference("AAGASSPlacementPreviewActor", "Plugins/AGASS/AGASSInteraction/Source/AGASSInteraction/Private/Actors/AGASSPlacementPreviewActor.cpp", 1),
      codeReference("AAGASSCarryableItemActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSCarryableItemActor.cpp", 1),
      codeReference("AAGASSPlaceableItemActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSPlaceableItemActor.cpp", 1),
      codeReference("AAGASSScaffoldingPlaceableActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSScaffoldingPlaceableActor.cpp", 1),
      codeReference("AAGASSUsableItemActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSUsableItemActor.cpp", 1),
    ],
  },
  {
    title: "Content-driven item framework",
    mediaSlotId: "items",
    body:
      "Items are authored through Data Assets and Blueprint subclasses, while C++ owns multiplayer rules for pickup, carry, placement, shops, selling, random junk spawning, scaffolding, and held usable tools. This lets special items stay richly authored and lets random junk be generated cheaply from mesh pools.",
    points: [
      [
        codeReference("UAGASSItemDefinition", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Public/Data/AGASSItemDefinition.h", 1),
        " and ",
        codeReference("UAGASSUsableItemDefinition", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Public/Data/AGASSUsableItemDefinition.h", 1),
        " expose meshes, actor classes, costs, sell values, placement tuning, carry sockets, montage data, and consume-on-use behavior.",
      ],
      "Spawner junk uses replicated runtime item data, so a mesh pool can generate varied props without requiring one Data Asset for every loose object.",
      "Scaffolding overrides generic placement with stack and support rules, while Fumigene demonstrates a replicated held-use item with timed server-authoritative activation.",
    ],
    sourceReferences: [
      codeReference("UAGASSItemDefinition", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Public/Data/AGASSItemDefinition.h", 1),
      codeReference("UAGASSUsableItemDefinition", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Public/Data/AGASSUsableItemDefinition.h", 1),
      codeReference("UAGASSItemSpawnerPoolData", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Public/Data/AGASSItemSpawnerPoolData.h", 1),
      codeReference("AAGASSItemSpawnerActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSItemSpawnerActor.cpp", 1),
      codeReference("AAGASSFumigeneUsableItemActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSFumigeneUsableItemActor.cpp", 1),
      codeReference("UAGASSShopCatalog", "Plugins/AGASS/AGASSEconomy/Source/AGASSEconomy/Public/Data/AGASSShopCatalog.h", 1),
    ],
  },
  {
    title: "Shared economy, shop, and sell flow",
    mediaSlotId: "economy",
    body:
      "The co-op team shares one replicated wallet on GameState. World shop actors build visible catalog slots with item meshes, prices, triggers, and spawn points, while sell zones convert valid placed items back into team money.",
    points: [
      "Purchases are accepted only on the server: overlap, held-item state, affordability, spawn class, item spawn, immediate carry claim, refund-on-error, and audio feedback all resolve through authority.",
      "Catalog entries define item references, price overrides, spawn offsets, and whether the bought item should auto-claim into the buyer's carry flow.",
      "Sell zones reject held-hidden items, credit the shared wallet, play feedback, and destroy the sold placeable so the economy works with physical world objects.",
    ],
    sourceReferences: [
      codeReference("UAGASSTeamWalletComponent", "Plugins/AGASS/AGASSEconomy/Source/AGASSEconomy/Private/Components/AGASSTeamWalletComponent.cpp", 1),
      codeReference("AAGASSShopTerminal", "Plugins/AGASS/AGASSEconomy/Source/AGASSEconomy/Private/Actors/AGASSShopTerminal.cpp", 1),
      codeReference("AAGASSSellZone", "Plugins/AGASS/AGASSEconomy/Source/AGASSEconomy/Private/Actors/AGASSSellZone.cpp", 1),
      codeReference("UAGASSShopCatalog", "Plugins/AGASS/AGASSEconomy/Source/AGASSEconomy/Public/Data/AGASSShopCatalog.h", 1),
      codeReference("UAGASSEconomyHUDSubsystem", "Plugins/AGASS/AGASSEconomy/Source/AGASSEconomy/Private/Subsystems/AGASSEconomyHUDSubsystem.cpp", 1),
      codeReference("IAGASSSharedTeamMoneyInterface", "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Public/Interfaces/AGASSSharedTeamMoneyInterface.h", 1),
    ],
  },
  {
    title: "Timed run, objective, and end-of-run loop",
    mediaSlotId: "timed-run",
    body:
      "The timed-run mode turns the tower sandbox into a repeatable objective loop. Players intentionally start one shared official timer, build upward, complete the objective, see a replicated summary, save local best times, submit Steam leaderboard results when available, and return to the frontend.",
    points: [
      "RunPhase controls session lifecycle and join closure, while TimedRunState controls WaitingToStart, Active, and Completed timer state.",
      "The timer display derives from replicated server time, and the final result is frozen as an official millisecond value when the objective is accepted.",
      "Objective actors and Blueprint/manual completion hooks share the same completion interface, which keeps base maps and mod-authored triggers on one path.",
    ],
    sourceReferences: [
      codeReference("AAGASSGameModeBase", "Source/AGASS/Private/AGASSGameModeBase.cpp", 1),
      codeReference("UAGASSRunStateComponent", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Components/AGASSRunStateComponent.cpp", 1),
      codeReference("AAGASSTimedRunStartActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSTimedRunStartActor.cpp", 1),
      codeReference("AAGASSObjectiveActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSObjectiveActor.cpp", 1),
      codeReference("UAGASSTimedRunBlueprintLibrary", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Blueprint/AGASSTimedRunBlueprintLibrary.cpp", 1),
      codeReference("UAGASSEndOfRunWidget", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/UI/AGASSEndOfRunWidget.cpp", 1),
    ],
  },
  {
    title: "Session event framework",
    mediaSlotId: "events",
    body:
      "Session events are content-driven gameplay moments contributed by the selected map and active content-only mods. The server loads event definition assets, schedules randomized activation, spawns replicated event actors, and cleans them up when events finish or the run ends.",
    points: [
      "Map definitions can contribute map-scoped events, while mod manifests can contribute global event definitions even when the mod does not provide a playable map.",
      "The manager owns activation timing, same-event concurrency, active-event replication, destruction handling, and shutdown during frontend return.",
      "Mad Cops and Mad Plane prove the framework with two different event shapes: route/AI/bribe/tower-scan gameplay and height-gated aircraft pressure with Fumigene counterplay.",
    ],
    sourceReferences: [
      codeReference("UAGASSSessionEventManagerComponent", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Components/AGASSSessionEventManagerComponent.cpp", 1),
      codeReference("UAGASSSessionEventDefinition", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Public/Data/AGASSSessionEventDefinition.h", 1),
      codeReference("AAGASSSessionEventActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSSessionEventActor.cpp", 1),
      codeReference("AAGASSMadCopsSessionEventActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSMadCopsSessionEventActor.cpp", 1),
      codeReference("AAGASSMadPlaneSessionEventActor", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/Actors/AGASSMadPlaneSessionEventActor.cpp", 1),
      codeReference("AGASSTowerStructureAnalysis", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Private/SessionEvents/AGASSTowerStructureAnalysis.cpp", 1),
    ],
  },
  {
    title: "Modding and map-selection pipeline",
    mediaSlotId: "modding",
    body: [
      "AGASS supports cooked content-plugin mods with asset-authored ",
      codeReference("UAGASSModManifest", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSModManifest.h", 1),
      " assets, selectable ",
      codeReference("UAGASSMapDefinition", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSMapDefinition.h", 1),
      " assets, active mod toggles, and multiplayer compatibility metadata. The same data model covers base-game maps, modded maps, and event-only mods.",
    ],
    points: [
      [
        codeReference("UAGASSModsSubsystem", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Private/Subsystems/AGASSModsSubsystem.cpp", 1),
        " scans local/project roots and Workshop-ready roots, mounts valid cooked plugins, loads manifests, resolves selected map state, aggregates events, and builds compatibility hashes.",
      ],
      "The frontend lists maps and mods, auto-enables a mod when its map is selected, and prevents disabling a required owning mod while that map is active.",
      "The runtime path stays content-only: it loads cooked assets and metadata, but it does not execute arbitrary native code from mods.",
    ],
    sourceReferences: [
      codeReference("UAGASSModsSubsystem", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Private/Subsystems/AGASSModsSubsystem.cpp", 1),
      codeReference("UAGASSModManifest", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSModManifest.h", 1),
      codeReference("UAGASSMapDefinition", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSMapDefinition.h", 1),
      codeReference("UAGASSModsAndMapsScreenWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/AGASSModsAndMapsScreenWidget.cpp", 1),
      codeReference("UAGASSMapSelectionEntryWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/AGASSMapSelectionEntryWidget.cpp", 1),
      codeReference("AGASSEditorModule", "Plugins/AGASS/AGASSEditor/Source/AGASSEditor/Private/AGASSEditorModule.cpp", 1),
      codeReference("AGASSModdingKitSupportModule", "Plugins/AGASS/AGASSModdingKitSupport/Source/AGASSModdingKitSupport/Private/AGASSModdingKitSupportModule.cpp", 1),
    ],
  },
  {
    title: "Modding Kit authoring workflow",
    mediaSlotId: "modding-kit",
    body: [
      "The Modding Kit is the creator-facing side of the mod pipeline. It gives creators a controlled Unreal editor environment for authoring content-only plugin mods with ",
      codeReference("UAGASSModManifest", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSModManifest.h", 1),
      ", ",
      codeReference("UAGASSMapDefinition", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSMapDefinition.h", 1),
      ", and ",
      codeReference("UAGASSSessionEventDefinition", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Public/Data/AGASSSessionEventDefinition.h", 1),
      " assets before exporting a folder that the packaged game can discover.",
    ],
    points: [
      [
        codeReference("AGASSModdingKitSupportModule", "Plugins/AGASS/AGASSModdingKitSupport/Source/AGASSModdingKitSupport/Private/AGASSModdingKitSupportModule.cpp", 1),
        " keeps creator-kit support isolated from live gameplay modules while still exposing the asset types and extension points that mod authors need.",
      ],
      [
        codeReference("AGASSEditorModule", "Plugins/AGASS/AGASSEditor/Source/AGASSEditor/Private/AGASSEditorModule.cpp", 1),
        " validates the plugin descriptor, manifest identity, map/event IDs, compatibility versions, asset ownership, cooked containers, AssetRegistry.bin, and install layout before producing an export.",
      ],
      [
        codeReference("UAGASSModExportDeveloperSettings", "Plugins/AGASS/AGASSEditor/Source/AGASSEditor/Public/Settings/AGASSModExportDeveloperSettings.h", 1),
        " centralizes export roots, naming policy, and validation switches so the workflow is repeatable instead of a manual copy step.",
      ],
      "The result is a scoped modding workflow for maps and session events: creators can add playable content, while multiplayer compatibility and runtime loading stay predictable.",
    ],
    sourceReferences: [
      codeReference("AGASSModdingKitSupportModule", "Plugins/AGASS/AGASSModdingKitSupport/Source/AGASSModdingKitSupport/Private/AGASSModdingKitSupportModule.cpp", 1),
      codeReference("AGASSEditorModule", "Plugins/AGASS/AGASSEditor/Source/AGASSEditor/Private/AGASSEditorModule.cpp", 1),
      codeReference("UAGASSModExportDeveloperSettings", "Plugins/AGASS/AGASSEditor/Source/AGASSEditor/Public/Settings/AGASSModExportDeveloperSettings.h", 1),
      codeReference("UAGASSModManifest", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSModManifest.h", 1),
      codeReference("UAGASSMapDefinition", "Plugins/AGASS/AGASSMods/Source/AGASSMods/Public/Data/AGASSMapDefinition.h", 1),
      codeReference("UAGASSSessionEventDefinition", "Plugins/AGASS/AGASSTower/Source/AGASSTower/Public/Data/AGASSSessionEventDefinition.h", 1),
    ],
  },
  {
    title: "CommonUI frontend and settings UX",
    mediaSlotId: "frontend",
    body:
      "The frontend is a production-shaped CommonUI stack, not a one-off menu widget. It handles startup into FrontendMap, main menu flow, session browsing, invite-code joining, maps/mods selection, loading recovery, in-session game menu, options, local settings, startup benchmark policy, and key rebinding.",
    points: [
      [
        codeReference("UAGASSUIManagerSubsystem", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/Subsystems/AGASSUIManagerSubsystem.cpp", 1),
        " and ",
        codeReference("UAGASSUIPolicy", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/Subsystems/AGASSUIPolicy.cpp", 1),
        " own persistent root layout lifetime, tagged layers, back handling, focus recovery, controller replacement recovery, and frontend/game-menu separation.",
      ],
      "GameSettings pages bind to local graphics, audio, gameplay, controls, interface, placement-tuning, benchmark, and timed-run best-time state.",
      "The rebinding UX adds custom raw key capture, duplicate-binding confirmation, Enhanced Input persistence, keyboard text fallback, and CommonInput gamepad glyph lookup.",
    ],
    sourceReferences: [
      codeReference("UAGASSUIManagerSubsystem", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/Subsystems/AGASSUIManagerSubsystem.cpp", 1),
      codeReference("UAGASSUIPolicy", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/Subsystems/AGASSUIPolicy.cpp", 1),
      codeReference("UAGASSFrontendPrimaryLayoutWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/AGASSFrontendPrimaryLayoutWidget.cpp", 1),
      codeReference("UAGASSMainMenuScreenWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/AGASSMainMenuScreenWidget.cpp", 1),
      codeReference("UAGASSSessionBrowserScreenWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/AGASSSessionBrowserScreenWidget.cpp", 1),
      codeReference("UAGASSGameSettingRegistry", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/Settings/AGASSGameSettingRegistry.cpp", 1),
      codeReference("UAGASSGameSettingPressAnyKeyWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/Settings/AGASSGameSettingPressAnyKeyWidget.cpp", 1),
    ],
  },
  {
    title: "Steam platform layer",
    mediaSlotId: "steam",
    body:
      "Steam-specific behavior lives in AGASSSteam while AGASSOnline remains responsible for the generic session lifecycle, invite codes, compatibility checks, travel, and recovery. Steam becomes a platform surface rather than the place where game-specific join policy lives.",
    points: [
      [
        "The Steam invite button routes through a generic platform-menu interface, then ",
        codeReference("UAGASSSteamPlatformSubsystem", "Plugins/AGASS/AGASSSteam/Source/AGASSSteam/Private/Subsystems/AGASSSteamPlatformSubsystem.cpp", 1),
        " checks Steam activity, overlay availability, named-session state, and lobby ID before opening the focused friend invite dialog.",
      ],
      "Accepted Steam invites hand a resolved search result back into the shared join path, preserving the same compatibility and PreLogin checks as browser and invite-code joins.",
      "Stats, achievements, playtime, timed-run uploads, and friend leaderboard queries are driven from gameplay events and map/mod-derived leaderboard keys.",
    ],
    sourceReferences: [
      codeReference("UAGASSSteamPlatformSubsystem", "Plugins/AGASS/AGASSSteam/Source/AGASSSteam/Private/Subsystems/AGASSSteamPlatformSubsystem.cpp", 1),
      codeReference("UAGASSSteamFriendsUISubsystem", "Plugins/AGASS/AGASSSteam/Source/AGASSSteam/Private/Subsystems/AGASSSteamFriendsUISubsystem.cpp", 1),
      codeReference("UAGASSPlatformMenuSubsystem", "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/Subsystems/AGASSPlatformMenuSubsystem.cpp", 1),
      codeReference("UAGASSGameMenuScreenWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/AGASSGameMenuScreenWidget.cpp", 1),
      codeReference("UAGASSMainMenuScreenWidget", "Plugins/AGASS/AGASSFrontend/Source/AGASSFrontend/Private/UI/AGASSMainMenuScreenWidget.cpp", 1),
      codeReference("UAGASSSessionSubsystem", "Plugins/AGASS/AGASSOnline/Source/AGASSOnline/Private/Subsystems/AGASSSessionSubsystem.cpp", 1),
    ],
  },
];

export const agassPage: ProjectPageContent = {
  template: "original",
  overview:
    "AGASS is a Co-op tower-building prototype. Players stack falling junk and buy utility items to reach the objective in a minimum time. The game supports Steam sessions, invite codes, content-plugin mods, map selection, and a modding kit with a custom Unreal export tool.",
  roleScope:
    "Solo C++ gameplay, online, frontend, economy, Steam, modding, editor tooling, and systems architecture work. I present it as an original systems case study with a committed source snapshot, while final media captures stay pending until the artifact review is complete.",
  sections: agassSections,
  proof: [
    "Modular UE 5.7 architecture with separate plugins for common settings/events, frontend, online, Steam, interaction, tower gameplay, economy, runtime mods, and Modding Kit support.",
    "Listen-server session flow with browser joins, invite-code joins, Steam invite handoff, selected map/mod metadata, compatibility hashing, PreLogin rejection, and frontend recovery.",
    "Server-authoritative pickup, carry, placement preview, placement validation, usable-item flow, random item spawners, scaffolding, and Fumigene gameplay.",
    "Replicated shared team wallet, physical shop lineup, server-side purchase/refund paths, sell zones, and HUD balance updates.",
    "Timed-run loop with explicit start actor, objective completion, replicated summary, local best-time persistence, Steam leaderboard hooks, and return-to-frontend scheduling.",
    "Content-plugin mod pipeline with manifests, map definitions, event-only mods, runtime discovery, frontend selection, Modding Kit export validation, and multiplayer compatibility checks.",
    "CommonUI frontend with persistent layers, session browser, maps/mods screen, options, key rebinding, loading recovery, in-session menu, and Steam friend timed-run display.",
  ],
  improvements: [
    "I would capture the page media system by system: architecture, session flow, compatibility rejection, placement, items, economy, timed run, events, runtime modding, Modding Kit export, frontend, and Steam.",
    "I would strengthen automated tests around multiplayer compatibility mismatches, mod manifest validation, shop purchase/refund cases, timed-run completion, and packaged Steam smoke testing.",
    "I would keep mod support focused on content-plugin maps and session events rather than expanding into arbitrary native-code mods.",
    "I would clean remaining localization and config drift so user-facing text and legacy map/mod defaults follow the same asset-driven conventions.",
    "I would keep distribution decisions separate from this portfolio and focus the page on reviewed media, source evidence, asset attribution, packaged Steam behavior, and mod-export validation.",
  ],
};
