# Project Context for Portfolio

This document summarizes project evidence gathered from local repo inspection. Use it as source context for portfolio copy, project cards, media planning, and future case studies.

## Positioning Rules

- Lead with original or heavily self-directed systems work.
- Clearly label course-based projects and describe the specific learning value or custom extension.
- Do not imply original art, shipped commercial work, professional studio experience, or production-grade online infrastructure unless the repo evidence supports it.
- Show only cleaned, attributed, media-backed source artifacts; do not expose build, GitHub, email, or missing-source placeholder actions.

## Showcase Priority

1. Networked Co-op Hospital Flow System: current flagship in the portfolio plan.
2. A Game About Stacking Shit: strong original supporting case study, especially for multiplayer physics and tower placement.
3. Multiplayer MOBA GAS Prototype: strong course-based supporting card because of C++, GAS, replication, AI, shop, and data-driven refactors.
4. Advanced CommonUI Frontend Course: course-based supporting card because it explains the CommonUI/CommonInput frontend path used for EMR.
5. Unreal 2D Course Prototypes: compact learning card for Paper2D and PaperZD breadth.
6. Slash - Unreal C++ Learning Project: timeline item for Unreal C++ gameplay foundations.
7. UE5 GAS Crash Course Prototype: timeline item for GAS consolidation.
8. UE5 Multiplayer Crash Course: timeline item for replication/session consolidation.
9. Learn C++ for Game Development: timeline item for first C++ language foundation before Unreal C++.
10. Early Unreal Blueprint Prototypes: timeline item for first Unreal learning path.

## A Game About Stacking Shit

Path: `K:\GameDev\Projets\AGASS`

Status: original/self-directed project; strong supporting project or possible secondary case study.

Evidence found:

- UE 5.7 C++ project with modular plugin layout.
- Main module: `Source\AGASS`.
- Gameplay split across `AGASSCommon`, `AGASSFrontend`, `AGASSOnline`, `AGASSSteam`, `AGASSInteraction`, `AGASSTower`, `AGASSEconomy`, `AGASSMods`, and `AGASSEditor`.
- CommonUI, Enhanced Input, Steam/Null online subsystems, SteamSockets, StateTree, PCG, Lumen/Nanite/Ray Tracing are enabled through config.
- Packaged/release evidence exists under `Packaged\AGASS_0.1.9` and `Releases\AGASS_0.1.x`.
- `ARCHITECTURE.md` describes a listen-server Steam co-op game.
- The codebase includes a real modding pipeline: `AGASSMods` runtime discovery/selection/compatibility, `AGASSEditor` mod export tooling, `AGASSModdingKitSupport` for a separate binary-only modding-kit workflow, `docs/MODDING_GUIDE.md`, `docs/adr/ADR-003-content-plugin-modding.md`, and a sample content-plugin mod under `Samples\Mods\AGASSSampleTowerMod`.

Systems found:

- Falling item spawner using data/mesh pools.
- Carry, pickup, placement preview, and placeable item flow.
- Tower objective, timed run state, tower analysis, top/bottom/height detection.
- Replicated shared wallet, shop terminal, shop catalog data asset, and sell zone.
- Random hazards including Mad Plane and Mad Cops.
- Scaffolding support, stacking, unsupported-fall, and stabilization behavior.
- CommonUI frontend screens for menus, session browser, maps/mods, settings, and timer HUD.
- Fully moddable content pipeline for maps and session events using content-only plugins, `UAGASSModManifest`, `UAGASSMapDefinition`, `UAGASSSessionEventDefinition`, mod-global/map-scoped event references, and controlled Blueprint extension points.
- Runtime mod discovery from project/local/Workshop-configured roots, cooked plugin pak mounting, plugin `AssetRegistry.bin` loading, manifest validation, active mod toggles, selected map resolution, and content-compatibility hashing.
- Frontend mod and map flow: discovered mods appear in the main menu mod screen, map selection can auto-enable the owning mod, and session browser/invite-code/Steam joins gate against selected map, active mod IDs, and content compatibility.
- Separate modding-kit workflow: a shareable Unreal project lets creators author a content-only plugin, create maps/events, then use `Tools > AGASS > Export Mod...` to cook and export a drop-in `Mods/<PluginName>` folder with the descriptor, paks/IoStore containers, root `AssetRegistry.bin`, and install notes for `AGASS/Mods/<PluginName>/`.

Portfolio angle:

- Best deeper case study: "Making chaotic physics reliable enough for multiplayer tower building."
- Strong secondary case study angle: "Designing a moddable Unreal co-op game with a separate creator kit, runtime discovery, compatibility checks, and one-click mod export."
- Strong technical story: falling items need physics and collision reliability, carried items use a preview/source split, placed items become stable support-validated tower pieces, and random events can reactivate physics to collapse sections.
- Modding story: creators can build a new map or event/content mod in a separate kit, export it as a player-installable folder, and have AGASS discover it in the frontend before hosting or joining a compatible session.

Portfolio-safe copy:

> UE 5.7 co-op tower-building game where players catch falling junk, buy utility items, and stack high enough to reach the objective while chaotic events try to destroy the tower. Built primarily in C++ with server-authoritative pickup, replicated placement previews, physics-aware tower stability, shared economy, Steam sessions, and content-driven hazards.

> The project also includes a creator-facing modding pipeline: a separate binary-only Unreal modding kit, content-only plugin mods, asset-driven map and event manifests, editor export tooling, runtime mod discovery in the main menu, and session compatibility checks so players can install a mod folder and host/join matching modded content.

Proof bullets:

- Modular UE 5.7 C++ co-op architecture with separate plugins for interaction, tower gameplay, economy, online sessions, Steam integration, frontend UI, and mod/map support.
- Server-authoritative item pickup and placement with replicated carry state, client-side placement preview, and validated server placement.
- Physics pipeline for falling items, stable stacked pieces, and event-driven collapse.
- Collision reliability checks using overlap tests, downward sweeps, support-touch validation, placement insets, snapping tolerance, and final encroachment resolution.
- Support graph logic lets placed tower items know their supporting and supported items.
- Content-driven hazards analyze the tower and collapse affected items with impulses.
- Replicated shared economy with team wallet, shop terminal, item catalog, purchase flow, and sell-zone support.
- Steam/Null online paths, SteamSockets networking, session browser, invite-code joining, and frontend-to-gameplay travel flow.
- Creator-facing modding kit and editor export workflow for content-only map/event mods, including validation that each mod has a stable manifest, map/event IDs, and same-plugin asset references.
- Runtime mod system discovers installed mod folders, mounts cooked plugin paks, loads plugin asset registries, exposes mods/maps in the frontend, and publishes selected map, active mod IDs, and compatibility hashes through the online session path.

Caveats and media needs:

- Static repo inspection only; runtime behavior still needs capture.
- Inspected path was not a git repository, so commit history is not proof here.
- Avoid claiming a finished generic structural stability subsystem; repo evidence supports support-validated placement, support graphs, event collapse, and scaffolding stabilization.
- Modding support is content-plugin/map/event focused with controlled Blueprint extension points; do not claim arbitrary native-code mods.
- Automated test coverage appears thin.
- Need media showing falling item collision, placement preview, tower reaching objective, plane/cops collapsing sections, shop/economy, two-client co-op, main-menu mod discovery, a modded map selection, and the modding-kit export/install flow.

## Multiplayer MOBA GAS Prototype

Path: `D:\GameDev\Anciens Projets\Apprentissage\MOBA GAS\LOD`

Status: course-based project with meaningful custom architecture/refactor evidence. Strong supporting card, not flagship.

Course: https://www.udemy.com/course/multiplayer-in-unreal-with-gas-and-aws-dedicated-servers/

Evidence found:

- UE 5.6 C++ project.
- Runtime module `LOD`, about 137 C++ header/source files and roughly 13k source lines under `Source/LOD`.
- Dependencies include GameplayAbilities, GameplayTasks, GameplayTags, Enhanced Input, UMG, AIModule, Niagara, and AnimGraphRuntime.
- Content includes AI, Characters, Framework, GameplayAbilities, GameplayCueNotifies, Inputs, Maps, Player, Widget, Paragon Crunch, and Paragon Minions.

Systems found:

- MOBA character, player controller, camera aiming, Enhanced Input, HUD, stun/death/respawn hooks.
- GameMode team assignment, start spots, central win object, match-end notifications.
- Replicated objective that moves toward team goals based on unit dominance and drives win/loss camera/UI flow.
- AI minions, behavior tree/blackboard integration, AI perception, hostile sensing, pooled wave respawn.
- Replicated inventory/shop with owner-only inventory, stackable/consumable items, server purchase/sell/activate RPCs, item combinations, item-granted abilities, and cooldown UI.
- GAS combat with custom ASC, attribute sets, gameplay tags, gameplay effects, cues, target actors, and ability classes.

Custom/data-driven evidence:

- Custom asset manager registered in config to load `ShopItem` primary assets.
- `UMobaShopItem` primary data assets hold icon refs, effects, granted abilities, prices, stack/consumable flags, ingredient items, and tag-to-value maps.
- `UMobaAbilitySystemGenerics` primary data asset holds generic effects, passive abilities, base stat table, and experience curve.
- Base attributes read from data table rows matched by character class.
- Core redirect history supports substantial refactoring away from original course names and hardcoded systems.

Portfolio-safe copy:

> Course-based multiplayer project extended into a server-authoritative MOBA prototype with GAS combat, replicated attributes, team AI minion waves, item shop/inventory, ability progression, and a data-driven asset architecture.

Proof bullets:

- UE 5.6 multiplayer MOBA prototype built around GAS, replicated attributes, gameplay tags, gameplay effects, cues, and custom C++ ability classes.
- Server-authoritative combat and progression: ability grants/upgrades, death/respawn, XP/level thresholds, upgrade points, gold, and stat scaling from data tables.
- Networked inventory/shop systems with primary data assets, item recipes, stackable/consumable behavior, server-side purchase/sell validation, and GAS-powered item effects.
- Team-aware AI minions with perception, behavior trees, blackboards, pooled wave spawning, team skins, and objective routing.
- Replicated match objective that moves based on team dominance and drives win/loss state across clients.
- Refactored course-style systems toward data-driven C++ architecture using asset manager, primary data assets, gameplay tag maps, data tables, and Unreal core redirects.

Caveats and media needs:

- Must be attributed as course-based work.
- Avoid implying original character art or environment art; Paragon/Megascans/imported assets are present.
- No dedicated `Server.Target.cs` was found, so avoid claiming a packaged dedicated-server build for this inspected repo.
- Static analysis only; 5v5 runtime behavior was not verified.
- Need media showing two-client local session, ability replication, shop purchase/combine flow, minion waves, objective win/loss, and relevant code/data asset overlays.

## Unreal 2D Course Prototypes

Path: `D:\GameDev\Anciens Projets\Apprentissage\Tuto cobra code`

Status: course-based learning card.

Course: https://www.udemy.com/course/unreal-2d-top-down/

Inventory:

- `DungeonAdventure`: UE 5.5 project; PaperZD enabled; more complete top-down prototype.
- `MonsterWorld`: UE 5.6 project; smaller top-down interaction/overworld prototype.
- Raw course/media asset packs are present.

Systems and techniques:

- Paper2D sprite and tilemap import.
- Aseprite-style frame imports and flipbook-style animation assets.
- Enhanced Input movement/actions.
- Directional 2D animation sets.
- PaperZD animation setup in `DungeonAdventure`.
- Tilemap environments, sword/bow actions, projectile base/arrow, health component, HUD hearts, pickups, enemy controller, spawn manager, and interaction UI.
- Public course curriculum also covers Unreal editor and Blueprint fundamentals, Paper2D/PaperZD concepts, Monster World, Dungeon Adventure, Hybrid RPG, sprite characters in 3D environments, camera/post-process tuning, dialogue icons, speech bubbles, typewriter text, Data Table-driven dialogue, roaming NPCs with nav meshes, follower recruitment, 3D sound cues, pixel-art texture setup, foreground masking, Sequencer cutscenes, gamepad support, and retro axis-locked movement.

Portfolio-safe copy:

> Course-based Unreal 5 Blueprint learning work exploring 2D and 2.5D top-down workflows: sprite import, tilemaps, PaperZD animation, Enhanced Input, Blueprint combat, enemies, waves, pickups, projectiles, UMG health/dialogue UI, and hybrid 2D/3D RPG presentation.

Caveats and media needs:

- Attribute as Cobra Code course exercises.
- Blueprint-only; no C++ source found.
- Do not present art, sound, or exercise design as original work.
- Need short `DungeonAdventure` clip with movement, sword, bow, enemies, health loss, and pickup.

## UE5 GAS Crash Course Prototype

Path: `D:\GameDev\Anciens Projets\Apprentissage\Tutoriels Stephen\GAS Crash Course\Learn GAS\LearnGAS`

Status: learning timeline item.

Course: https://www.udemy.com/course/ue5-gas-crash-course/

Evidence found:

- UE 5.6 C++ project.
- Single runtime module `Source/LearnGas`.
- GameplayAbilities, GameplayTasks, GameplayTags, Enhanced Input, and UMG dependencies.
- Custom ASC, AttributeSet, replicated health/mana attributes, native gameplay tags, Gameplay Effects, Gameplay Abilities, hit reactions, death handling, and attribute-driven UI.
- Public course curriculum also covers blank project setup, player/enemy character classes, Player Controller and input mappings, ability activation by tag, montage tasks, Gameplay Events, custom Anim Notifies and Notify States, overlap tests, collision filtering, Curve Tables, Set By Caller magnitudes, death/respawn flow, enemy target search, AI movement, melee/ranged attacks, projectiles, Gameplay Cues, camera shakes, cost/cooldown effects, pickups, knockback, airborne states, and damage numbers.

Portfolio-safe copy:

> Course-based Unreal Engine 5.6 project completed after the MOBA GAS course to consolidate Gameplay Ability System fundamentals in a smaller focused project. Implemented a replicated GAS foundation with custom ASC/AttributeSet classes, native gameplay tags, ability input routing, health/mana UI binding, damage effects, hit reactions, and death handling for player, melee enemy, and ranged enemy characters.

Caveats and media needs:

- Attribute as Stephen Ulibarri GAS Crash Course work.
- Best as a learning milestone, not a case study.
- Paragon assets dominate content; avoid art claims.
- Need short clip showing ability activation, health/mana updates, hit reaction, damage/death, and enemy variants.

## UE5 Multiplayer Crash Course

Path: `D:\GameDev\Anciens Projets\Apprentissage\Tutoriels Stephen\Multiplayer Crash Course\MP_CrashCourse`

Status: learning timeline item.

Course: https://www.udemy.com/course/ue5-multiplayer-crash-course/

Evidence found:

- UE 5.6 C++ project.
- Runtime `MultiplayerSessions` plugin from Stephen Ulibarri.
- OnlineSubsystem, OnlineSubsystemSteam, SteamSockets, Enhanced Input, UMG, Slate, NetCore.
- Packaged Windows build under `Builds/Build x64`.
- LAN host/join menu, OnlineSubsystem session create/find/join/destroy, listen-server travel, client travel.
- Replicated PlayerState gold, replicated health component, replicated character armor/gold, replicated GameState team arrays, and replicated pickup actors.
- Public course curriculum also covers the client-server model, PIE multiplayer testing, standalone/listen/dedicated server modes, LAN connection, listen servers via Steam, actor replication, movement replication, authority/net roles, attachment, variable replication, RepNotifies, replication conditions, custom runtime replication conditions, ownership, replicated Actor Components, Client/Server/Multicast RPCs, RPC validation, relevancy, priority, GameMode, GameState, PlayerState, PlayerController, Pawn/Character, HUD/widgets, static accessor caveats, hard/seamless travel, and implementing seamless travel.

Portfolio-safe copy:

> Course-based Unreal C++ project completed after the MOBA GAS course to consolidate Unreal multiplayer fundamentals in a smaller focused project. Implemented listen-server hosting, LAN/IP join, OnlineSubsystem session creation/search/join, actor replication, replicated pickups, PlayerState gold tracking, replicated health/armor state, simple team assignment, RPC/authority debugging examples, framework-class boundaries, and multiplayer travel.

Caveats and media needs:

- Attribute as Stephen Ulibarri multiplayer crash course work.
- Keep as learning milestone; several RPC paths are educational/debug examples.
- UE template/sample content is present, so avoid presenting variants as original systems.
- Need two-client clip showing host/join, replicated pickup collection, and gold UI update.

## Slash - Unreal C++ Learning Project

Path: `D:\GameDev\Anciens Projets\Apprentissage\Tutoriels Stephen Ulibarri\Learn UE 5 C++ Programming\Slash`

Status: learning timeline item for first C++ foundation.

Course: https://www.udemy.com/course/unreal-engine-5-the-ultimate-game-developer-course/

Positioning:

- This is Baptiste's own local Unreal course project/source implementation, written while following Stephen Ulibarri's course.
- The course provided the action-RPG project structure and baseline implementation.
- Present it as the transition from C++ fundamentals and Blueprint-only Unreal learning into hands-on Unreal C++ gameplay programming.
- Do not present the action-RPG design, art direction, imported assets, animations, or environment as original work.

Evidence found:

- UE 5.6 C++ project.
- Dependencies include Enhanced Input, HairStrandsCore, GeometryCollectionEngine, Niagara, AIModule.
- Third-person melee character, weapon pickup/equip, melee box traces, hit reactions, death montage, dodge/stamina, health/stamina/gold/souls attributes, enemy AI, pickups, Chaos breakables, UMG HUD, and Niagara effects.
- C++ class layering includes `ABaseCharacter`, `ASlashMyCharacter`, `AEnemy`, `AItem`, `AWeapon`, `ASoul`, `ATreasure`, and `UAttributeComponent`.
- Public course curriculum also covers editor setup, realistic landscapes, open-world setup, Quixel/Megascans replacement assets, lighting, foliage, Packed Level Actors, Level Instances, vectors/rotators/trigonometry, Unreal C++ reflection and garbage collection, Actor/Pawn/Character classes, C++ components, Blueprint exposure, Enhanced Input, Animation Blueprint C++ bases, IK, collision/overlap delegates, sockets, IK Rig/Retargeter, montages, MetaSounds, melee traces, Unreal interfaces, directional hit reactions, weapon trails, Chaos destructibles, Field System Actors, treasure spawning, Niagara effects/components, health bars, HUD updates, enemy patrol/sensing/states, inherited enemy attacks, root-motion attacks, Motion Warping, soul/gold/stamina attributes, dodge behavior, Animation Blueprint Templates, and reusable enemy variants.

Portfolio-safe copy:

> Course-based UE5 C++ action prototype written in Baptiste's own local project after the C++ fundamentals and Blueprint courses. Implemented Unreal C++ class hierarchy exercises, Enhanced Input character control, animation/IK setup, weapon equipping, melee traces, enemy patrol/chase/attack behavior, Motion Warping, health/stamina/soul/gold attributes, HUD updates, pickups, loot drops, Niagara/MetaSounds feedback, and Chaos destructibles.

Caveats and media needs:

- Attribute as course-based C++ learning project.
- Do not present as original game design or original art direction.
- Repo includes generated/local Unreal folders and is not clean as a public source repo.
- Need 30-45 second clip showing equip, attack, enemy chase, dodge, HUD updates, pickup, and destructible pot/treasure.

## Early Unreal Blueprint Prototypes

Path: `D:\GameDev\Anciens Projets\Apprentissage\Tutoriels Stephen Ulibarri\Ultimate BP course`

Status: early learning timeline item.

Course: https://www.udemy.com/course/ue5-ultimate-bp-course/

Positioning:

- This is Baptiste's own local Blueprint coursework while following Stephen Ulibarri's course.
- The course provided the learning path, assets, and project designs.
- Present it as the first structured Unreal Engine learning path before C++ fundamentals, Unreal C++, GAS, multiplayer, CommonUI, and original systems work.
- Do not present course assets, project designs, imported art, or exercise concepts as original portfolio work.

Inventory:

- `FirstProject`: basic UE learning project.
- `CollisionTests\CollisionTests`: collision/physics sandbox.
- `BadBot`: drone shooter/combat prototype.
- `RedHood`: 2D platformer/combat prototype using PaperZD.
- `Vehicle`: Chaos vehicle physics prototype.
- `Course Assets`: reference/import assets and sample projects.

Public course curriculum:

- 11 sections, 210 lectures, 41h 33m total length, last updated 5/2026.
- Covers Unreal editor basics, project setup, Starter Content, PIE testing, object manipulation, file types, mesh scale, materials, Marketplace/FAB assets, asset migration, Blueprint classes/components, coordinates, debug shapes, local/global space, vectors, rotators, booleans, branches, delta seconds, Bad Bot drone shooter, Pawns, Enhanced Input, Floating Pawn Movement, collision framework, line traces, sweeps, physics collisions, collision callbacks, custom channels/presets, Blueprint Interfaces, Widget Blueprints, spawn volumes, boss fight, Jetpack Journey 3D platformer, Character class, PlayerController input, animation Blendspaces, state machines, sockets, Niagara thrusters, MetaSounds/sound notifies, flying mode, jet fuel pickups, UMG fuel bar, class dependencies, casting/memory, texture compression, soft references, async loading, platforms, pressure plates, interact interfaces, Packed Level Actors, event dispatchers, construction scripts, Red Hood 2D dungeon crawler, Paper2D/PaperZD, sprites, flipbooks, tile sets/maps, Behavior Trees, Blackboards, tasks, decorators, services, box traces, custom Anim Notifies, hit reactions, health bars, damage numbers, combo attacks, structs, enemy death, and Chaos Vehicles with rigging/skinning, physics assets, vehicle settings, Enhanced Input controls, and possession changes.

Portfolio-safe copy:

> Completed Stephen Ulibarri's Ultimate Blueprint course as the first structured Unreal Engine learning path. Built local Blueprint course projects covering editor basics, Blueprint classes/components, vector/rotator math, Bad Bot drone shooting, Enhanced Input, collision, UI, Blueprint Interfaces, Jetpack Journey platforming, animation Blendspaces, Niagara/MetaSounds, soft references, async loading, Paper2D/PaperZD dungeon combat, Behavior Trees, combo attacks, damage numbers, and Chaos Vehicle possession.

Caveats and media needs:

- Attribute as coursework.
- Blueprint-only; no C++ source found.
- Strongest use is an About/timeline milestone, not a project card.
- Need small media samples only if the timeline becomes visual.

## Advanced CommonUI Frontend Course

Course: https://www.udemy.com/course/ureal-engine-5-cpp-advanced-frontend-ui-programming/

Status: course-based learning card with direct relevance to the hospital frontend.

Instructor: Vince Petrelli.

User context:

- The course was completed to learn CommonUI for the Emergency Room project frontend.
- It should have a dedicated attributed course page instead of being hidden only inside the hospital case study.
- The course page should explain the learning connection to EMR without presenting course UI design or sample assets as original portfolio work.

Course coverage:

- CommonUI setup with C++ frontend player controller, primary layout, tagged widget stacks, UI subsystem, developer settings, and async soft-widget loading.
- Main menu, modal/confirmation flow, bound action prompts, and gamepad-tested focus behavior.
- Options screens with tab navigation, dynamic details panels, settings data objects, Game User Settings integration, and reset behavior.
- Input remapping and input detection using input preprocessors.
- Startup and in-game loading-screen flow.

Portfolio-safe copy:

> Course-based UE5 C++ frontend project completed to learn CommonUI, CommonInput, widget stacks, controller navigation, options screens, input remapping, and loading screens before applying those patterns to the Emergency Room project's frontend and lobby flow.

Caveats and media needs:

- Attribute as Vince Petrelli course work.
- Do not present course exercise structure, sample assets, or UI design as original work.
- Public source remains disabled until a clean source artifact is prepared or the relevant proof is attached to the EMR source page.
- Need media showing gamepad navigation through menu flow, options tabs, remapping, confirmation modal, and loading screen.

## Learn C++ for Game Development

Course: https://www.udemy.com/course/learn-cpp-for-ue4-unit-1/

Status: course-based learning milestone.

Instructor: Stephen Ulibarri.

User context:

- This was the first C++ course completed before Unreal C++ work.
- It should appear on the homepage as a completed course project with its own dedicated page.

Positioning:

- This is Baptiste's written C++ fundamentals practice while following Stephen Ulibarri's course.
- The course does not use Unreal Engine; present it as preparation for Unreal C++ work rather than as an Unreal project.
- It directly supports the later Slash, GAS, multiplayer, CommonUI, and EMR C++ pages.

Course coverage:

- C++ basics for game development preparation: input/output, variables, truth values, conditions, functions, loops, arrays, enums, switches, structs, strings, classes, constructors, access modifiers, inheritance, pointers, dynamic memory, destructors, virtual functions, polymorphism, casting, and header files.
- Public course curriculum also covers Visual Studio setup, errors and warnings, statements and expressions, keywords and identifiers, function reuse, increment operators, computations, references, function overloading, constants, truth tables, stack vs heap memory, static variables, multiple inheritance, and section quizzes/exercises.
- The course prepares students for game development programming but does not use Unreal Engine directly.

Portfolio-safe copy:

> First C++ course completed before Unreal C++ gameplay work. Baptiste wrote console-style C++ practice code covering syntax, functions, loops, references, arrays, enums, structs, classes, inheritance, pointers, dynamic memory, destructors, virtual functions, polymorphism, casting, and header/source organization, giving the foundation needed for later Unreal Engine C++ projects.

Caveats and media needs:

- Attribute as Stephen Ulibarri course work.
- Present as a C++ preparation milestone, not as an Unreal Engine project or original game.
- Keep the page concise so the stronger Unreal systems projects remain the main technical proof.
