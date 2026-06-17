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

const readyVideoSlot = (
  id: string,
  caption: string,
  source: string,
): MediaSlot => ({
  id,
  type: "video",
  aspectRatio: "16 / 9",
  status: "ready",
  caption,
  source,
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

export const hospitalFlowMediaSlots: MediaSlot[] = [
  readyVideoSlot(
    "run-progression",
    "Hub to Night Shift run loop and quota resolution",
    "media/hospital-flow/hub-to-night-shift-flow.mp4",
  ),
  readyVideoSlot(
    "sessions",
    "Steam lobby, invite-code, and co-op role flow",
    "media/hospital-flow/online-session.mp4",
  ),
  readyVideoSlot(
    "patient-flow",
    "Patient state, AI routing, queues, and hospital pressure",
    "media/hospital-flow/patient-definition-ai-behavior.mp4",
  ),
  readyVideoSlot(
    "spawning",
    "Difficulty-scaled patient spawning and pooling",
    "media/hospital-flow/night-shift-definition.mp4",
  ),
  readyVideoSlot(
    "exam-treatment",
    "Reception, exams, treatments, cure rewards, and exit routing",
    "media/hospital-flow/patient-flow.mp4",
  ),
  readyVideoSlot(
    "machines",
    "Reusable medical station framework and station-specific gameplay",
    "media/hospital-flow/all-machines-usages.mp4",
  ),
  readyVideoSlot(
    "gas",
    "GAS-driven patient, player, machine, and team state",
    "media/hospital-flow/gas-integration.mp4",
  ),
  readyVideoSlot(
    "upgrades",
    "Hub upgrade voting and runtime upgrade application",
    "media/hospital-flow/hub-upgrade-definition.mp4",
  ),
  readyVideoSlot(
    "events",
    "Special events, alert phase, active pressure, and light feedback",
    "media/hospital-flow/special-event.mp4",
  ),
  readyVideoSlot(
    "ui",
    "CommonUI frontend, HUD, tablet, triage, and patient folder flow",
    "media/hospital-flow/frontend-and-options.mp4",
  ),
  readyVideoSlot(
    "developer-tools",
    "In-game debug harness for run and balance iteration",
    "media/hospital-flow/custom-tools.mp4",
  ),
  readyVideoSlot(
    "localization",
    "Gameplay-tag localization for medical UI presentation",
    "media/hospital-flow/localization.mp4",
  ),
];

export const hospitalFlowSections: ContentSection[] = [
  {
    title: "Hub-to-Night Shift run loop",
    mediaSlotId: "run-progression",
    body:
      "The run structure moves the team from Hub selection into Night Shift missions and back to Hub again. The server owns Night Shift selection, cycle quotas, overtime, failure checks, final mission unlocks, summaries, and the state that must survive map travel.",
    points: [
      "GameMode classes drive authority-only transitions while the shared Night Shift GameState exposes replicated run, summary, vote and travel state to clients.",
      "Run rules are designer-tunable through data assets for quotas, cycle length, Night Shift duration, final unlocks, and failure paths.",
      "A progression subsystem caches active run state across Hub and Night Shift ServerTravel so the two map types do not duplicate state ownership.",
    ],
    sourceReferences: [
      codeReference("AEMRHubGameMode", "Source/LOD/Private/Framework/EMRHubGameMode.cpp", 1),
      codeReference("AEMRNightShiftGameMode", "Source/LOD/Private/Framework/EMRNightShiftGameMode.cpp", 1),
      codeReference("AEMRNightShiftGameState", "Source/LOD/Private/Framework/EMRNightShiftGameState.cpp", 1),
      codeReference("UEMRRunRulesSubsystem", "Source/LOD/Private/Subsystems/EMRRunRulesSubsystem.cpp", 1),
      codeReference("UEMRRunProgressionSubsystem", "Source/LOD/Private/Subsystems/EMRRunProgressionSubsystem.cpp", 1),
    ],
  },
  {
    title: "Networked co-op and sessions",
    mediaSlotId: "sessions",
    body:
      "I designed the hospital around flexible co-op roles instead of fixed professions. Any player can work at reception, move to an exam machine, or help at treatment beds, so the networking layer has to keep patient state, queue and interaction results consistent for the whole team.",
    points: [
      "Steam sessions, direct host/join, Steam invites, invite-code and robust travel handling are part of the implemented flow.",
      "Clients send intent through UI and controller calls while server systems validate selections, interaction results, session controls, and run transitions.",
      "World widgets, materials, and sound cues react to replicated hospital flow so the experience does not depend only on screen-space UI.",
    ],
    sourceReferences: [
      codeReference("UEMRLobbySessionSubsystem", "Source/LOD/Private/Subsystems/EMRLobbySessionSubsystem.cpp", 1),
      codeReference("UEMRLobbyInviteCodeSubsystem", "Source/LOD/Private/Subsystems/EMRLobbyInviteCodeSubsystem.cpp", 1),
      codeReference("AEMRLobbyGameMode", "Source/LOD/Private/Framework/EMRLobbyGameMode.cpp", 1),
      codeReference("AEMRLobbyGameState", "Source/LOD/Private/Framework/EMRLobbyGameState.cpp", 1),
      codeReference("AEMRPlayerController", "Source/LOD/Private/Characters/Player/EMRPlayerController.cpp", 1),
    ],
  },
  {
    title: "Patient flow and AI behavior",
    mediaSlotId: "patient-flow",
    body:
      "Patient flow creates the pressure of the game. Players read the patient context, choose the right exam or treatment path and react as exam and treatment queues become harder to manage.",
    points: [
      "Patient definitions are data-driven through primary data assets and tables for identity, vitals, pathologies, symptoms, exams, and treatments.",
      "Patient phases, pathologies, exam progress, treatment needs, patience, and vitals are represented through GAS attributes, effects, and tags.",
      "Gameplay Message Subsystem publish routing intent as Blackboard key Targets for AI Behavior Tree and a custom snap task handles precise final placement at seats and machines.",
    ],
    sourceReferences: [
      codeReference("AEMRPatient", "Source/LOD/Private/Characters/Patient/EMRPatient.cpp", 1),
      codeReference("UEMRPatientData", "Source/LOD/Public/Characters/Patient/EMRPatientData.h", 1),
      codeReference("UEMRPatientAttributeSet", "Source/LOD/Public/GAS/Attributes/EMRPatientAttributeSet.h", 1),
      codeReference("AEMRAIController", "Source/LOD/Private/Characters/Patient/EMRAIController.cpp", 1),
      codeReference("UEMRBTTask_SnapToTarget", "Source/LOD/Private/Behavior/Tasks/EMRBTTask_SnapToTarget.cpp", 1),
      codeReference("UEMRWaitingRoomManagerComponent", "Source/LOD/Private/Environment/EMRWaitingRoomManagerComponent.cpp", 1),
    ],
  },
  {
    title: "Data-driven spawning and difficulty",
    mediaSlotId: "spawning",
    body:
      "Night Shift pressure scales from data: map definition, difficulty tier, player count, overtime, and active special events all feed the spawn director. Spawning is pooled and staged so high-pressure moments do not rely on cold runtime actor setup.",
    points: [
      "Night Shift definitions configure map, difficulty, tags, spawn modifiers, and event eligibility.",
      "The runtime spawn formula combines base rate, difficulty multipliers, player-count scaling, overtime, and special-event modifiers.",
      "Patients are warmed through a pool and staged across hidden spawn, data setup, mesh and animation priming, AI/collision enable, and reveal.",
    ],
    sourceReferences: [
      codeReference("UEMRNightShiftSpawnSubsystem", "Source/LOD/Private/Subsystems/EMRNightShiftSpawnSubsystem.cpp", 1),
      codeReference("UEMRPatientPoolSubsystem", "Source/LOD/Private/Subsystems/EMRPatientPoolSubsystem.cpp", 1),
      codeReference("UEMRNightShiftDefinition", "Source/LOD/Public/Data/EMRNightShiftDefinition.h", 1),
      codeReference("UEMRDifficultyTuningData", "Source/LOD/Public/Data/EMRDifficultyTuningData.h", 1),
    ],
  },
  {
    title: "Exam and treatment pipeline",
    mediaSlotId: "exam-treatment",
    body:
      "The core hospital workflow moves patients from reception through exams, treatment beds and exit. The important part is that routing is modular and data-driven instead of written as one-off patient scripts.",
    points: [
      "Reception validates selected exams against pathologies requirements before the patient can progress.",
      "Exam and treatment queue subsystems assign machines, waiting seats, beds and treatment stations while keeping ownership and phase changes authoritative.",
      "Completed exams, treatment actions, rewards and exit routing are resolved through shared tags, gameplay effects and AI navigation intent.",
    ],
    sourceReferences: [
      codeReference("AEMRReceptionMachine", "Source/LOD/Private/Interaction/EMRReceptionMachine.cpp", 1),
      codeReference("UEMRExamQueueSubsystem", "Source/LOD/Private/Subsystems/EMRExamQueueSubsystem.cpp", 1),
      codeReference("UEMRTreatmentSubsystem", "Source/LOD/Private/Subsystems/EMRTreatmentSubsystem.cpp", 1),
      codeReference("FEMRExamRequirementsForPathologyMapping", "Source/LOD/Public/Data/EMRExamRequirementsForPathologyMapping.h", 1),
      codeReference("FEMRTreatmentForPathologyMapping", "Source/LOD/Public/Data/EMRTreatmentForPathologyMapping.h", 1),
      codeReference("AEMRTreatmentBed", "Source/LOD/Private/Interaction/EMRTreatmentBed.cpp", 1),
    ],
  },
  {
    title: "Reusable medical station framework",
    mediaSlotId: "machines",
    body:
      "Machines share a common interaction model, gameplay-tag identity, patient assignment flow, replicated state and queue integration. Individual stations then add their own gameplay without breaking the routing contract.",
    points: [
      "A base machine abstraction handles occupancy, assigned patients, interaction payloads, machine tags, wait points, queue notifications, and navigation targets.",
      "Reception, XRay, Ultrasound, CT Scan, Lab Analyzer, Treatment Beds, Oxygen, IV treatment, and Item Dispenser plug into the same broader patient-routing architecture.",
      "Replicated state keeps operators, assigned patients, machine motion, UI feedback, and completion cues synchronized in multiplayer.",
    ],
    sourceReferences: [
      codeReference("AEMRBaseMachine", "Source/LOD/Private/Interaction/EMRBaseMachine.cpp", 1),
      codeReference("UEMRInteractableComponent", "Source/LOD/Private/Interaction/EMRInteractableComponent.cpp", 1),
      codeReference("IEMRInteractableInterface", "Source/LOD/Public/Interfaces/EMRInteractableInterface.h", 1),
      codeReference("AEMRXRayMachine", "Source/LOD/Private/Interaction/EMRXRayMachine.cpp", 1),
      codeReference("AEMRUltrasoundMachine", "Source/LOD/Private/Interaction/EMRUltrasoundMachine.cpp", 1),
      codeReference("AEMRCTScanMachine", "Source/LOD/Private/Interaction/EMRCTScanMachine.cpp", 1),
      codeReference("AEMRLabAnalyzerMachine", "Source/LOD/Private/Interaction/EMRLabAnalyzerMachine.cpp", 1),
    ],
  },
  {
    title: "GAS integration",
    mediaSlotId: "gas",
    body:
      "GAS acts as the shared contract between players, AI patients, machines, UI and run systems. It stores medical state, drives interactions, applies queue and phase tags, drains patience and updates team economy through replicated attributes.",
    points: [
      "Player Ability System Component live on PlayerState, patients own their own ASC and the shared Night Shift GameState owns the team ASC for revenue and reputation.",
      "Gameplay events route interactions from objects and machines into abilities instead of making every station invent its own input path.",
      "Tags model patient phases, pathology, exam queues, treatment queues, completed exams, machine identity, run upgrades and reward state.",
    ],
    sourceReferences: [
      codeReference("UEMRAbilitySystemComponent", "Source/LOD/Public/GAS/EMRAbilitySystemComponent.h", 1),
      codeReference("UEMRPatientAttributeSet", "Source/LOD/Public/GAS/Attributes/EMRPatientAttributeSet.h", 1),
      codeReference("UEMRTeamAttributeSet", "Source/LOD/Public/GAS/Attributes/EMRTeamAttributeSet.h", 1),
      codeReference("EMRTags", "Source/LOD/Public/GAS/EMRTags.h", 1),
      codeReference("AEMRPlayerState", "Source/LOD/Private/Characters/Player/EMRPlayerState.cpp", 1),
    ],
  },
  {
    title: "Hub upgrade voting",
    mediaSlotId: "upgrades",
    body:
      "After players select a Night Shift, the Hub generates upgrade offers from data assets and runs a replicated vote through the tablet UI. The winning upgrade becomes a gameplay-tag stack that persists across travel and is applied to real gameplay actors when the Night Shift starts.",
    points: [
      "The server owns offer generation, vote validation, countdown resolution, winner selection and upgrade application.",
      "Upgrade definitions store display text, gameplay tag identity and max purchase count, while active stacks live in replicated run state.",
      "Upgrades affect placed Night Shift actors such as treatment beds, coffee machines, oxygen machines, snack machines, magic boxes and item dispenser",
    ],
    sourceReferences: [
      codeReference("AEMRHubGameMode", "Source/LOD/Private/Framework/EMRHubGameMode.cpp", 1),
      codeReference("UEMRRunUpgradeDefinition", "Source/LOD/Public/Data/EMRRunUpgradeDefinition.h", 1),
      codeReference("AEMRNightShiftGameState", "Source/LOD/Private/Framework/EMRNightShiftGameState.cpp", 1),
      codeReference("UEMRRunProgressionSubsystem", "Source/LOD/Private/Subsystems/EMRRunProgressionSubsystem.cpp", 1),
      codeReference("AEMRNightShiftGameMode", "Source/LOD/Private/Framework/EMRNightShiftGameMode.cpp", 1),
    ],
  },
  {
    title: "Special events",
    mediaSlotId: "events",
    body:
      "Special events are server-scheduled Night Shift crisis moments. Events move through Alert, Active, and Completed phases that influence spawn pressure, patient pathology weighting, UI alerts, global audio and lights.",
    points: [
      "Event data assets define timing, alert text, spawn multipliers, pathology influence and lighting.",
      "Events are filtered by difficulty, Night Shift tags and event pool before the server schedules one into the active run.",
      "Presentation reacts to replicated GameState event data while gameplay pressure remains integrated with the existing spawn subsystem.",
    ],
    sourceReferences: [
      codeReference("UEMRSpecialEventDefinition", "Source/LOD/Public/Data/EMRSpecialEventDefinition.h", 1),
      codeReference("AEMRNightShiftGameMode", "Source/LOD/Private/Framework/EMRNightShiftGameMode.cpp", 1),
      codeReference("AEMRNightShiftGameState", "Source/LOD/Private/Framework/EMRNightShiftGameState.cpp", 1),
      codeReference("UEMRSpecialEventLightSubsystem", "Source/LOD/Private/Subsystems/EMRSpecialEventLightSubsystem.cpp", 1),
      codeReference("UEMRNightShiftSpawnSubsystem", "Source/LOD/Private/Subsystems/EMRNightShiftSpawnSubsystem.cpp", 1),
    ],
  },
  {
    title: "CommonUI and gameplay UI architecture",
    mediaSlotId: "ui",
    body:
      "The UI architecture uses a LocalPlayer UI manager, CommonUI stacks, async soft-widget loading, reusable confirmation screens, data-driven options, key remapping and gameplay-integrated world widgets.",
    points: [
      "Frontend stacks cover main menu, lobby, options, credits, invite-code joining, confirmation flows, loading and quit handling.",
      "Gameplay stacks and world widgets handle HUD, pause menu, hub selection, upgrade voting, reception, machine screens, treatment displays, patient cards and medical folders.",
      "Widgets bind into replicated GameState data, team GAS attributes, LocalPlayer subsystems, player-controller RPCs and patient UI controllers.",
    ],
    sourceReferences: [
      codeReference("UEMRUIManagerSubsystem", "Source/LOD/Private/UI/Common/Subsystems/EMRUIManagerSubsystem.cpp", 1),
      codeReference("UEMRCommonPrimaryLayoutWidget", "Source/LOD/Private/UI/Frontend/Core/EMRCommonPrimaryLayoutWidget.cpp", 1),
      codeReference("UEMROptionsDataRegistry", "Source/LOD/Private/UI/Frontend/Data/EMROptionsDataRegistry.cpp", 1),
      codeReference("UEMRGameUserSettings", "Source/LOD/Public/UI/Frontend/EMRGameUserSettings.h", 1),
      codeReference("UEMRPatientUIController", "Source/LOD/Private/UI/Patient/EMRPatientUIController.cpp", 1),
    ],
  },
  {
    title: "Developer tools harness",
    mediaSlotId: "developer-tools",
    body:
      "I built an in-game developer tools harness to speed up long loop testing without creating a parallel cheat path. UI actions route through server RPCs and then into the same production systems used by normal gameplay.",
    points: [
      "Testers can spawn patients, force Night Shift success or reputation failure, change revenue and reputation, adjust simulation speed, reroll upgrades or grant specific upgrades.",
      "Patient spawning uses the real spawn subsystem, patient pool, waiting seats and movement events.",
      "Win/loss and economy actions reuse GameMode, GameState, GAS effects, summaries, progression and travel flow.",
    ],
    sourceReferences: [
      codeReference("UEMRDeveloperToolsWidget", "Source/LOD/Private/UI/Dev/EMRDeveloperToolsWidget.cpp", 1),
      codeReference("AEMRPlayerController", "Source/LOD/Private/Characters/Player/EMRPlayerController.cpp", 1),
      codeReference("AEMRNightShiftGameMode", "Source/LOD/Private/Framework/EMRNightShiftGameMode.cpp", 1),
      codeReference("AEMRHubGameMode", "Source/LOD/Private/Framework/EMRHubGameMode.cpp", 1),
      codeReference("UEMRNightShiftSpawnSubsystem", "Source/LOD/Private/Subsystems/EMRNightShiftSpawnSubsystem.cpp", 1),
    ],
  },
  {
    title: "Localization and tag presentation",
    mediaSlotId: "localization",
    body:
      "Gameplay systems use tags for blood types, symptoms, pathologies, exams, and treatments. The localization layer maps those tags to string-table entries so the same gameplay data can become readable UI in any language.",
    points: [
      "The subsystem caches gameplay-tag to localization-key mappings from a configured data table.",
      "Patient folders, triage cards and generated medical reference-paper UI can use the same exam and treatment mappings as gameplay.",
    ],
    sourceReferences: [
      codeReference("UEMRLocalizationSubsystem", "Source/LOD/Private/Subsystems/EMRLocalizationSubsystem.cpp", 1),
      codeReference("UEMRLocalizationDeveloperSettings", "Source/LOD/Private/Subsystems/EMRLocalizationDeveloperSettings.cpp", 1),
      codeReference("FEMRTagsLocalizationTable", "Source/LOD/Public/Data/EMRTagsLocalizationTable.h", 1),
      codeReference("UEMRLocalizationLibrary", "Source/LOD/Public/LocalizationLibrary.h", 1),
    ],
  },
];

export const hospitalFlowPage: ProjectPageContent = {
  template: "original",
  overview:
    "I built a co-op \"Friend Slop\" Emergency Room simulator around one shared loop: receive patients, assign them to the right exam, complete the exam and try to cure them while managing queues and hospital pressure. It is playable with up to eight players and has a working Steam Online Subsystem session flow. Development is paused, and the project still needs a lot of polish before I present it as a demo.",
  roleScope:
    "Solo Unreal Engine 5 project focused on listen-server co-op with server-authoritative validations, replicated AI patients and queue state, GAS-driven interactions, controller-friendly UI using CommonUI, and in-game debug tools to speed up development.",
  sections: hospitalFlowSections,
  proof: [],
  improvements: [],
};
