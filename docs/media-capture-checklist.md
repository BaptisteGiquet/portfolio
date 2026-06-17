# Media Capture Checklist

Use this as the one-by-one capture list for the portfolio media pass. Keep clips short, muted-friendly, and focused on proof of systems. Prefer 1080p or higher for desktop captures, 16:9 for main videos, and avoid showing private notes, license keys, paid asset library folders, or messy editor scratch content.

## Priority 1: Networked Co-op Hospital Flow System

Main walkthrough clip:

- [ ] `hospital-main-walkthrough.mp4`: 60-90 seconds, understandable without audio. Show the co-op loop in order: patient arrival/reception, routing decision, replicated patient or queue info visible to another player, machine exam, treatment interaction, payment/exit or completion, and one brief UI/controller/debug proof moment.

Focused clips:

- [ ] `hospital-reception-validation.mp4`: show a reception decision being accepted server-side and patient state advancing.
- [ ] `hospital-invalid-feedback.mp4`: show an invalid/wrong interaction returning clear client feedback without advancing the patient.
- [ ] `hospital-two-client-replication.mp4`: host and client views side by side or sequentially showing the same patient/queue/machine state updating.
- [ ] `hospital-machine-exam.mp4`: show a patient entering an exam queue, using the required machine, and producing the next routing/treatment context.
- [ ] `hospital-treatment-flow.mp4`: show diagnosis/treatment queue, treatment bed interaction, and patient completion.
- [ ] `hospital-commonui-controller-flow.mp4`: show controller navigation through frontend/lobby or in-game CommonUI flow, including focus behavior.
- [ ] `hospital-world-widget-interaction.mp4`: show an in-world UI/widget interaction used during gameplay.

Screenshots:

- [ ] Reception/intake UI with patient need, urgency, or pathology context visible.
- [ ] Queue view showing patient membership/destination/pressure.
- [ ] Machine exam UI or interaction state.
- [ ] Treatment bed or treatment routing state.
- [ ] Client feedback for failed/wrong action.
- [ ] Host/client replicated state comparison if visually available.
- [ ] CommonUI frontend/lobby controller-focused screen.
- [ ] World-space widget interaction in context.
- [ ] Editor screenshot of relevant C++/Blueprint/GAS assets only if clean and readable.

Diagrams:

- [ ] Reception -> Exam Queue -> Machine Exam -> Treatment Queue -> Treatment Bed -> Payment/Exit, annotated with server validation, replicated queue state, patient state/tag updates, and client UI feedback.

## Priority 2: Patient Behavior & Gameplay Pressure

Clips:

- [ ] `patient-state-flow.mp4`: short clip showing a patient moving through arrival, triage, exam, diagnosis, treatment, and exit/recovery states.
- [ ] `patient-waiting-pressure.mp4`: show queue buildup, waiting pressure, urgency, or patience pressure becoming visible to the player.
- [ ] `patient-treatment-routing.mp4`: show pathology/diagnosis causing a specific treatment destination.
- [ ] `hospital-conditions-pressure.mp4`: if implemented/visible, show hospital condition or spawn-rate pressure changing the ward state.

Screenshots:

- [ ] Patient state/urgency display.
- [ ] Queue pressure with multiple patients waiting.
- [ ] Diagnosis or exam result context.
- [ ] Treatment destination context.
- [ ] Hospital condition/spawn pressure setting or debug view, if available.

Diagrams:

- [ ] Arrival -> Waiting for Triage -> Queued for Exam -> Being Examined -> Queued for Treatment -> Treated/Exiting, annotated with player-readable trigger for each transition.

## Priority 3: Runtime Debug Tools & Automated Gameplay Validation

Clips:

- [ ] `debug-scenario-spawner.mp4`: show a debug tool spawning targeted patients with selected urgency/pathology/queue pressure.
- [ ] `debug-state-inspector.mp4`: show patient state, queue membership, required exam/treatment, and waiting values updating during play.
- [ ] `debug-replicated-snapshot.mp4`: show host/client or server/client state comparison for patient, queue, machine, or interaction state.
- [ ] `debug-automation-runner.mp4`: show a deterministic validation or automation run producing a pass/fail result.
- [ ] `debug-bug-repro-workflow.mp4`: show the example workflow: spawn setup, perform interaction, capture/inspect state, run validation.

Screenshots:

- [ ] Scenario spawner panel.
- [ ] Patient state inspector.
- [ ] Replicated snapshot panel.
- [ ] Automation result output.
- [ ] Telemetry/log output for intake, routing, exam completion, treatment assignment, or exit.
- [ ] A clean example bug report or validation matrix if available.

Diagrams:

- [ ] Select Scenario -> Spawn Setup -> Capture Snapshot -> Log Result -> Run Regression, annotated with what state is recorded at each step.

## Priority 4: Co-op Physics Tower Builder / AGASS

Main clip:

- [ ] `agass-two-client-systems.mp4`: 45-75 seconds showing two-client co-op, falling items, pickup/carry, placement preview, validated placement, tower progress, shop/economy, and one hazard collapse.

Focused clips:

- [ ] `agass-falling-item-collision.mp4`: falling items landing/colliding reliably enough to be picked up.
- [ ] `agass-pickup-placement-preview.mp4`: pickup/carry state and client-side placement preview.
- [ ] `agass-server-placement.mp4`: server-validated placement becoming a stable tower piece.
- [ ] `agass-tower-objective.mp4`: tower height/progress/objective feedback.
- [ ] `agass-shop-economy.mp4`: shared wallet, shop terminal, purchase, sell zone, or item catalog.
- [ ] `agass-hazard-collapse.mp4`: plane/cops/hazard destabilizing or collapsing part of the tower.
- [ ] `agass-session-flow.mp4`: frontend session browser, host/join, Steam/Null flow, or invite-code flow.
- [ ] `agass-modding-kit-export.mp4`: separate modding-kit project showing a content-only mod plugin, map or event assets, and `Tools > AGASS > Export Mod...` producing a `Mods/<PluginName>` export folder.
- [ ] `agass-mod-discovery-selection.mp4`: packaged/main game discovering an installed mod from the mods folder, showing it in the frontend mod/map screen, selecting a modded map, and hosting with that content.

Screenshots:

- [ ] Placement preview with valid/invalid state.
- [ ] Stable tower piece/support validation in gameplay.
- [ ] Tower objective/progress UI.
- [ ] Shop terminal/catalog/shared wallet.
- [ ] Hazard aftermath/collapsed tower section.
- [ ] Frontend session browser/host/join screen.
- [ ] Frontend mods/maps screen showing a discovered mod and a modded map.
- [ ] Modding-kit editor screenshot showing `UAGASSModManifest`, `UAGASSMapDefinition`, or `UAGASSSessionEventDefinition` assets.
- [ ] Exported mod folder layout showing `Mods/<PluginName>/<PluginName>.uplugin`, cooked containers, root `AssetRegistry.bin`, and install notes.
- [ ] Clean editor/code screenshot showing modular plugin layout or relevant systems.

## Priority 5: Multiplayer MOBA GAS Prototype

Clip:

- [ ] `moba-gas-overview.mp4`: 45-60 seconds showing two-client or networked play, ability activation/replication, attributes changing, minion waves, objective movement or win/loss, and shop/inventory use.

Screenshots:

- [ ] GAS ability activation with HUD/attributes visible.
- [ ] Gameplay effects/tags/cues or ability Blueprint/C++ asset view.
- [ ] Shop/inventory item purchase, sell, combine, or consume flow.
- [ ] Primary data asset for shop item or ability system generics.
- [ ] AI minion wave/pathing/objective state.
- [ ] Match objective/win-loss UI.

## Priority 6: Additional Learning Projects

Unreal 2D / Paper2D / PaperZD:

- [ ] `unreal-2d-overview.mp4`: 20-40 seconds showing DungeonAdventure movement, sword, bow/projectile, enemies, health loss, pickup, and tilemap environment.
- [ ] Screenshots: PaperZD animation setup, tilemap level, combat/HUD, interaction UI.

UE5 GAS Crash Course:

- [ ] `gas-crash-course-overview.mp4`: 20-40 seconds showing ability activation, health/mana updates, hit reaction, damage/death, and enemy variants.
- [ ] Screenshots: custom ASC/AttributeSet or GameplayEffect/GameplayAbility setup, health/mana UI.

UE5 Multiplayer Crash Course:

- [ ] `multiplayer-crash-course-overview.mp4`: 20-40 seconds showing host/join, replicated pickup collection, gold/health/armor UI update, and two-client visibility.
- [ ] Screenshots: session menu, replicated PlayerState or GameState evidence, pickup/health UI.

Slash Unreal C++ Learning Project:

- [ ] `slash-cpp-overview.mp4`: 30-45 seconds showing weapon pickup/equip, melee attack, enemy chase/combat, dodge/stamina, HUD update, pickup/loot, and Chaos destructible.
- [ ] Screenshots: C++ class/source view, combat/HUD, enemy AI or patrol setup, destructible/loot moment.

Early Unreal Blueprint Prototypes:

- [ ] Optional small screenshot set only if the timeline becomes visual: BadBot, RedHood, Vehicle, and one Blueprint graph. These should stay clearly labeled as early coursework.

## General Capture Rules

- [ ] Keep all course projects visibly attributed in filenames/notes so they do not get mistaken for original game concepts.
- [ ] Avoid claiming original art, professional QA, shipped commercial games, dedicated servers, or production infrastructure unless the specific capture proves it.
- [ ] Prefer web-safe proof first: clips, screenshots, captions, and diagrams should explain the work without requiring a build download.
- [ ] For every clip, write a one-sentence caption explaining the exact system being proven.
- [ ] Show only cleaned and verified source links; keep build, GitHub, email, and missing-source placeholder actions hidden.
