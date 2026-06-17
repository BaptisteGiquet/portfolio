import { agassMediaSlots, agassPage } from "./agass";
import { hospitalFlowMediaSlots, hospitalFlowPage } from "./hospitalFlow";
import { tlmMediaSlots, tlmPage } from "./tlm";
import {
  defaultRoleVariant,
  defaultLanguage,
  type CodeReference,
  type ContentSection,
  type Language,
  type MediaSlot,
  type PortfolioContent,
  type PortfolioProject,
  type PortfolioUiLabels,
  type ProjectAction,
  type ProjectPageContent,
  type ProjectSlug,
  type RoleVariant,
  type RichTextContent,
  type RuntimeSourceMetadata,
  type SiteProfile,
} from "./types";

export const projectOrder = [
  "hospital-flow",
  "agass",
  "tlm",
  "moba-gas",
  "commonui-frontend",
  "gas-crash-course",
  "multiplayer-crash-course",
  "unreal-2d",
  "slash-cpp",
  "cpp-game-development",
  "blueprint-prototypes",
] as const;

type RoleVariantProfile = {
  role: string;
  cvHref: `cv/${string}.pdf`;
};

const roleVariantProfiles = {
  "unreal-engine-programmer": {
    en: {
      role: "Junior Unreal Engine Programmer",
      cvHref: "cv/baptiste-giquet-cv-unreal-programmer.pdf",
    },
    fr: {
      role: "Junior Unreal Engine Programmer",
      cvHref: "cv/baptiste-giquet-cv-unreal-programmer.pdf",
    },
  },
  "qa-tester": {
    en: {
      role: "Junior QA Tester",
      cvHref: "cv/baptiste-giquet-cv-qa-tester.pdf",
    },
    fr: {
      role: "Junior QA Tester",
      cvHref: "cv/baptiste-giquet-cv-qa-tester.pdf",
    },
  },
  "ai-game-designer": {
    en: {
      role: "AI Game Designer",
      cvHref: "cv/baptiste-giquet-cv-ai-game-designer.pdf",
    },
    fr: {
      role: "AI Game Designer",
      cvHref: "cv/baptiste-giquet-cv-ai-game-designer.pdf",
    },
  },
} satisfies Record<RoleVariant, Record<Language, RoleVariantProfile>>;

const cvActionLabel = {
  en: "Download CV",
  fr: "Télécharger le CV",
} satisfies Record<Language, string>;

const cvDownloadFilename = (href: RoleVariantProfile["cvHref"]) =>
  href.split("/").at(-1) ?? "cv.pdf";

const cvAction = (
  roleVariant: RoleVariant,
  language: Language,
): ProjectAction => {
  const href = roleVariantProfiles[roleVariant][language].cvHref;

  return {
    id: "cv",
    label: cvActionLabel[language],
    kind: "download",
    status: "ready",
    href,
    download: cvDownloadFilename(href),
  };
};

const applyProfileCvAction = (
  actions: ProjectAction[],
  roleVariant: RoleVariant,
  language: Language,
) => {
  const action = cvAction(roleVariant, language);
  const cvActionIndex = actions.findIndex((profileAction) => profileAction.id === "cv");

  if (cvActionIndex === -1) {
    return [action, ...actions];
  }

  return actions.map((profileAction, index) =>
    index === cvActionIndex ? action : profileAction,
  );
};

export type {
  CodeReference,
  ContentSection,
  Language,
  MediaSlot,
  PortfolioContent,
  PortfolioProject,
  ProjectAction,
  ProjectPageContent,
  ProjectSlug,
  RichTextContent,
};

export const siteProfile = {
  name: "Baptiste Giquet",
  role: roleVariantProfiles[defaultRoleVariant].en.role,
  location: "Caen, France",
  availability: "Immediately, France-Based or remote",
  contact: {
    email: "baptiste-giquet@hotmail.fr",
    phone: "06.85.80.43.44",
  },
  positioning:
    "Networked gameplay systems, GAS interactions, controller-friendly UI, and debugging tools for Unreal Engine projects.",
  actions: [cvAction(defaultRoleVariant, "en")] satisfies ProjectAction[],
};

const sourceMetadata = (slug: ProjectSlug): RuntimeSourceMetadata => ({
  status: "available",
  manifestPath: `source/${slug}/manifest.json`,
});

const browseSourceAction = (slug: ProjectSlug): ProjectAction => ({
  id: "source",
  label: "Browse source",
  kind: "source",
  status: "ready",
  href: `#/projects/${slug}/source`,
});

const oneDriveDownloadUrls = {
  "hospital-flow": "https://1drv.ms/u/c/0d1dc734bef710b1/IQBhM6MKmk0hSahb_wP9viclASQFZIg9JqBeH1Z7t_AEEaM?e=vl3ndN",
  "moba-gas": "https://1drv.ms/u/c/0d1dc734bef710b1/IQBWe2-BjG8fTp9REvcCmRE6AUT5yXSiET9uWq7wraBoGrY?e=LKulqR",
  "slash-cpp": "https://1drv.ms/u/c/0d1dc734bef710b1/IQCpyArlXGTkSII2YsQLcRBdAQRdFY6aFzeRsVl4eMF8vxE?e=X7IObC",
} satisfies Partial<Record<ProjectSlug, `https://${string}`>>;

const unrealDownloadAction = (
  slug: keyof typeof oneDriveDownloadUrls,
  metaLabel: string,
): ProjectAction => ({
  id: "download",
  label: "Download Unreal project",
  kind: "download",
  status: "ready",
  href: oneDriveDownloadUrls[slug],
  opensInNewTab: true,
  metaLabel,
});

const projectActions = (
  slug: ProjectSlug,
  source?: RuntimeSourceMetadata,
  primaryAction?: ProjectAction,
): ProjectAction[] => [
  ...(primaryAction ? [primaryAction] : []),
  ...(source ? [browseSourceAction(slug)] : []),
];

const courseAction = (
  url: string,
  label = "Course",
): ProjectAction => ({
  id: "course",
  label,
  kind: "course",
  status: "ready",
  href: url,
  opensInNewTab: true,
});

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

const readyImageSlot = (
  id: string,
  caption: string,
  source: string,
): MediaSlot => ({
  id,
  type: "image",
  aspectRatio: "16 / 9",
  status: "ready",
  caption,
  source,
});

const hospitalSource = sourceMetadata("hospital-flow");
const agassSource = sourceMetadata("agass");
const tlmSource = sourceMetadata("tlm");
const mobaGasSource = sourceMetadata("moba-gas");
const slashCppSource = sourceMetadata("slash-cpp");

export const projectRegistry: Record<ProjectSlug, PortfolioProject> = {
  "hospital-flow": {
    slug: "hospital-flow",
    visibility: "public",
    category: "original",
    title: "Emergency Room simulator",
    shortTitle: "Hospital Flow",
    route: "#/projects/hospital-flow",
    summary:
      "Online Co-op Emergency Room simulator using host/client server authoritative model, heavy Gameplay Ability System usage and CommonUI for all the frontend.",
    technologies: [
      "Unreal Engine 5.7",
      "C++",
      "Replication",
      "AI behavior",
      "Gameplay Ability System",
      "CommonUI",
      "Enhanced Input",
    ],
    mediaSlots: hospitalFlowMediaSlots,
    source: hospitalSource,
    actions: projectActions(
      "hospital-flow",
      hospitalSource,
      unrealDownloadAction("hospital-flow", "UE 5.7, 20.68 GiB ZIP"),
    ),
    includeInPreviousNext: true,
    page: hospitalFlowPage,
  },
  agass: {
    slug: "agass",
    visibility: "archived",
    category: "original",
    title: "A Game About Stacking Sh*t",
    shortTitle: "AGASS",
    route: "#/projects/agass",
    summary:
      "Online Co-op game with Host/Client server-authoritative model, built with modding in mind with custom modding kit and Unreal tools.",
    technologies: [
      "Unreal Engine 5.7",
      "C++",
      "Replication",
      "Physics",
      "Content-plugin mods",
      "Editor Tooling",
    ],
    mediaSlots: agassMediaSlots,
    source: agassSource,
    actions: projectActions("agass", agassSource),
    includeInPreviousNext: false,
    page: agassPage,
  },
  tlm: {
    slug: "tlm",
    visibility: "archived",
    category: "original",
    title: "The Last Mage",
    shortTitle: "TLM",
    route: "#/projects/tlm",
    summary:
      "FPS horde-survivor roguelite prototype made to learn Godot with data-driven architecture, editor tooling, automated tests, controller support, localization, and save system.",
    technologies: [
      "Godot 4.7",
      "C#",
      "Data-driven architecture",
      "Editor tooling",
      "Automated tests",
      "Controller support",
      "Localization",
      "Save system",
    ],
    mediaSlots: tlmMediaSlots,
    source: tlmSource,
    actions: projectActions("tlm", tlmSource),
    includeInPreviousNext: false,
    page: tlmPage,
  },
  "moba-gas": {
    slug: "moba-gas",
    visibility: "public",
    category: "learning",
    title: "Multiplayer MOBA Prototype",
    shortTitle: "MOBA GAS",
    route: "#/projects/moba-gas",
    summary:
      "Complete prototype for a competitive MOBA game with dedicated server model. Heavy use of Gameplay Ability System, AI behavior and state trees on a data-driven architecture.",
    technologies: [
      "Unreal Engine 5.6",
      "C++",
      "Gameplay Ability System",
      "Replication",
      "AI Behavior",
      "UMG",
    ],
    mediaSlots: [
      readyImageSlot(
        "course-image",
        "Udemy course image for Multiplayer in Unreal with GAS and AWS Dedicated Servers",
        "media/courses/moba-gas.jpg",
      ),
    ],
    source: mobaGasSource,
    actions: [
      unrealDownloadAction("moba-gas", "UE 5.7, 3.48 GiB ZIP"),
      browseSourceAction("moba-gas"),
      courseAction("https://www.udemy.com/course/multiplayer-in-unreal-with-gas-and-aws-dedicated-servers/"),
    ],
    includeInPreviousNext: true,
    page: {
      template: "learning",
      overview:
        "I built this multiplayer MOBA Unreal project by following a course project and writing the code in my own local Unreal project. Much of the baseline follows the course implementation, but the downloadable Unreal project and source snapshot show the code I wrote and the refactors I made, especially around replacing hardcoded gameplay values with more data-driven assets.",
      roleScope:
        "My Unreal C++ implementation and refactor work inside a multiplayer GAS curriculum project, with source shown through a reviewed portfolio snapshot.",
      course: {
        title: "Multiplayer in Unreal with GAS and AWS Dedicated Servers",
        provider: "Udemy",
        instructor: "Jingtian Li",
        url: "https://www.udemy.com/course/multiplayer-in-unreal-with-gas-and-aws-dedicated-servers/",
      },
      courseContext:
        "I wrote this Unreal project while following Jingtian Li's Udemy course on multiplayer Unreal development with GAS. The course provided the learning path, structure, and most of the baseline implementation. The project and source code available here are my own completed local project files and code I wrote while following that curriculum, with additional refactors where I changed hardcoded systems into data-driven ones.",
      whatBuilt: [
        "I implemented GAS combat foundations with a custom Ability System Component, Attribute Sets, Gameplay Effects, Gameplay Cues, Gameplay Tags and C++ ability classes.",
        "I worked through replicated player systems for attributes, abilities, death/respawn, XP, levels, upgrade points, gold and stat scaling from data tables.",
        "I implemented team-aware AI minions with perception, behavior trees, blackboards, pooled wave spawning, team skins and objective routing.",
        "I built a replicated match objective that moves toward team goals based on unit dominance and drives win/loss camera and UI flow.",
        "I implemented a networked inventory and shop flow with owner-only inventory, stackable and consumable items, server purchase/sell/activate RPCs, item combinations, cooldown UI and item-granted abilities.",
      ],
      whatLearned: [
        "How to structure server-authoritative gameplay around GAS instead of treating abilities, attributes, inventory and UI as separate local-only systems.",
        "How replicated Ability System state, Gameplay Tags, Gameplay Effects, cues and UI feedback fit together in an Unreal multiplayer project.",
        "How to move hard-coded systems into data driven systems using Primary Data Assets, data tables, tag-to-value map and asset manager.",
      ],
      sections: [
        {
          title: "Custom extensions and refactors",
          body:
            "The most useful part of this project for me was moving course-style implementation toward data-driven architecture. A custom asset manager loads ShopItem primary assets, item definitions carry prices, effects, granted abilities, stack/consumable flags, ingredient recipes, and tag-to-value maps, while a separate generic ability-system data asset stores shared effects, passive abilities, base stats and experience curves.",
          points: [
            "Base attributes are read from data table rows matched by character class.",
            "Item behavior is driven through primary data assets instead of being hardcoded directly into widgets or characters.",
          ],
        },
      ],
      proof: [],
      improvements: [],
    },
  },
  "commonui-frontend": {
    slug: "commonui-frontend",
    visibility: "public",
    category: "learning",
    title: "CommonUI Frontend Course",
    shortTitle: "CommonUI Frontend",
    route: "#/projects/commonui-frontend",
    summary:
      "CommonUI course project I used to learn the frontend patterns I later applied to my own EMR UI: widget stacks, controller navigation, options screens, input remapping and loading screens.",
    technologies: [
      "Unreal Engine 5.7",
      "C++",
      "CommonUI",
      "Widget Stack",
      "Controller navigation",
      "Game User Settings",
    ],
    mediaSlots: [
      readyImageSlot(
        "course-image",
        "Udemy course image for Unreal Engine 5 C++: Advanced Frontend UI Programming",
        "media/courses/commonui-frontend.jpg",
      ),
    ],
    actions: [
      courseAction("https://www.udemy.com/course/ureal-engine-5-cpp-advanced-frontend-ui-programming/"),
    ],
    includeInPreviousNext: true,
    page: {
      template: "learning",
      overview:
        "I completed this advanced frontend UI course so I could implement my own CommonUI-based frontend inside the Emergency Room project. The course project gave me the patterns for screen stacks, async widget loading, focus behavior, options screens, input remapping, and loading screens; I then applied those ideas to EMR's main menu, lobby, options, loading flow, and controller-friendly gameplay UI.",
      roleScope:
        "Course implementation and applied learning focused on Unreal C++ frontend architecture, then reused in my own EMR UI work with CommonUI, CommonInput, UMG, settings data, controller-friendly navigation and reusable screen flow.",
      course: {
        title: "Unreal Engine 5 C++: Advanced Frontend UI Programming",
        provider: "Udemy",
        instructor: "Vince Petrelli",
        url: "https://www.udemy.com/course/ureal-engine-5-cpp-advanced-frontend-ui-programming/",
      },
      courseContext:
        "I followed the course to understand how an Unreal frontend can be organized with CommonUI using Common Activatable Widgets, widget stacks, gameplay tags, async loading, CommonInput, options data, input remapping, and loading screens. I then used that knowledge to build the CommonUI frontend architecture in my own Emergency Room project.",
      whatBuilt: [
        "A modular CommonUI frontend setup with a frontend player controller, primary layout widget, tagged widget stacks, a UI subsystem, developer settings and async actions for pushing soft widget classes.",
        "Main menu and modal flows using Common Activatable Widget patterns, common buttons, bound actions, confirmation screens and gamepad ready UI.",
        "Options screens with tab navigation, data objects, list entry widgets, dynamic details panels, reset logic and Game User Settings integration.",
        "Input remapping and input detection work using CommonInput-style presentation.",
      ],
      whatLearned: [
        "How to structure a controller-friendly Unreal frontend around CommonUI screen stacks and gameplay tags.",
        "How menu widgets, modal widgets, options data, settings persistence, and focus recovery need to cooperate for gamepad navigation.",
        "How to translate course patterns into my own EMR frontend, adapting them to the project's hospital gameplay, multiplayer lobby, options, loading and controller-navigation needs.",
      ],
      sections: [],
      proof: [],
      improvements: [],
    },
  },
  "unreal-2d": {
    slug: "unreal-2d",
    visibility: "public",
    category: "learning",
    title: "Unreal 2D Prototypes",
    shortTitle: "Unreal 2D",
    route: "#/projects/unreal-2d",
    summary:
      "2D and 2.5D Unreal prototypes covering sprites, tilemaps, PaperZD animations, Enhanced Input, combat, enemies, pickups, projectiles.",
    technologies: ["Unreal Engine 5.6", "Blueprints", "Paper2D", "PaperZD", "Tilemaps", "UMG"],
    mediaSlots: [
      readyImageSlot(
        "course-image",
        "Udemy course image for The Ultimate 2D Top Down Unreal Engine Course",
        "media/courses/unreal-2d.jpg",
      ),
    ],
    actions: [
      courseAction("https://www.udemy.com/course/unreal-2d-top-down/"),
    ],
    includeInPreviousNext: true,
    page: {
      template: "learning",
      overview:
        "I grouped these Unreal 2D exercises because together they show how I learned Unreal's 2D and 2.5D workflow: sprites, tilemaps, PaperZD animation, Enhanced Input, Blueprint combat, enemies, pickups, projectiles, and UMG.",
      roleScope:
        "Blueprint-focused learning work across two local course projects: DungeonAdventure as the stronger PaperZD top-down prototype and MonsterWorld as a smaller interaction and overworld exercise.",
      course: {
        title: "The Ultimate 2D Top Down Unreal Engine Course",
        provider: "Udemy",
        instructor: "Cobra Code",
        url: "https://www.udemy.com/course/unreal-2d-top-down/",
      },
      courseContext:
        "I used this course to learn 2D and 2.D workflows in Unreal Engine: Paper2D, PaperZD, sprites, tile maps, animations and more.",
      whatBuilt: [
        "A set of Blueprint-based 2D learning projects and exercises around Paper2D, PaperZD, sprite and flipbook preparation, tile sets, tile maps, 'pixels perfect' setup, orthographic top-down cameras and 2D-friendly project/post-process settings.",
        "A top-down prototype with imported sprites and tilemaps, directional idle/walk animation, Enhanced Input movement, foreground/background sorting, collision setup, pickup interactions, items, simple HUD widgets, NPC dialogue and Blueprint interfaces.",
        "A Dungeon Adventure-style action prototype with PaperZD animation blueprints, inherited character Blueprints, health components, player/enemy states, AI following, overlap damage, sword attacks, bow and arrow projectiles, hitboxes, knockback, invincibility, hit stop, pickups, health UI, sound cues and restart flow.",
        "Spawner and wave-system with spawn managers, enemy/pickup spawning, wave data structures, wave tracking, wave counter UI and runtime feedback for top-down combat loops.",
        "Hybrid 2.5D RPG covering sprite characters in a 3D environment, camera/post-process tuning, dialogue icons, speech bubbles, Data Table driven dialogue, NPC-facing behavior, roaming NPCs through nav meshes, follower recruitment and 3D sound cues.",
      ],
      whatLearned: [
        "How Unreal's 2D stack fits together: sprites, tilemaps, animation assets, PaperZD, Blueprints, UMG and input mapping.",
        "How to build simple combat, pickups, health feedback, and enemy loops in a 2D workflow.",
      ],
      sections: [],
      proof: [],
      improvements: [],
    },
  },
  "gas-crash-course": {
    slug: "gas-crash-course",
    visibility: "public",
    category: "learning",
    title: "Gameplay Ability System Crash Course",
    shortTitle: "GAS Crash Course",
    route: "#/projects/gas-crash-course",
    summary:
      "Crash course used to consolidate my Gameplay Ability System fundamentals after the larger MOBA GAS prototype.",
    technologies: ["Unreal Engine 5.7", "C++", "Gameplay Abilities", "Gameplay Tags", "Ability Tasks", "Gameplay Cues"],
    mediaSlots: [
      readyImageSlot(
        "course-image",
        "Udemy course image for Unreal Engine 5 Gameplay Ability System (GAS) Crash Course",
        "media/courses/gas-crash-course.jpg",
      ),
    ],
    actions: [
      courseAction("https://www.udemy.com/course/ue5-gas-crash-course/"),
    ],
    includeInPreviousNext: true,
    page: {
      template: "learning",
      overview:
        "I used this smaller GAS project to isolate the fundamentals after working through the larger MOBA GAS prototype. It helped me focus on the core pieces: Ability System Components, AttributeSets, tags, effects, abilities, UI reactions, damage, hit reactions, and death handling.",
      roleScope:
        "Course implementation in a UE 5.7 C++ project used to consolidate Gameplay Ability System fundamentals after the larger MOBA GAS prototype.",
      course: {
        title: "Unreal Engine 5 Gameplay Ability System (GAS) Crash Course",
        provider: "Udemy",
        instructor: "Stephen Ulibarri",
        url: "https://www.udemy.com/course/ue5-gas-crash-course/",
      },
      courseContext:
        "I completed this course after the MOBA GAS course to consolidate what I had learned: Ability System Components, AttributeSets, Gameplay Abilities, Gameplay Tags, Gameplay Effects, Gameplay Cues, Ability Tasks, UI responses to attribute changes, combat, AI, cost/cooldown behavior, death/respawn and more.",
      whatBuilt: [
        "A GAS foundation with Ability System Components, a custom ASC, Attribute Sets, startup abilities, native Gameplay Tags, ability activation by tag, a base ability class and montage Ability Tasks.",
        "Gameplay event and hit reaction systems using custom Anim Notifies and Notify States, overlap tests, collision filtering, directional hit calculation, attack montages, Niagara particles and Gameplay Cues.",
        "Replicated health and mana attributes with initialization, damage, reset, cost, and cooldown Gameplay Effects, Curve Table scaling, Set By Caller magnitudes, attribute-change listeners and progress-bar widgets.",
        "Character death and respawn flow with native health-change handling, a custom attribute-change async task, death abilities, Gameplay Effects that apply state tags, player input blocking while dead and respawn handling for player and enemy characters.",
        "Enemy combat covering target search, AI movement and stopping, melee and ranged attacks, projectiles, search range, knockback, airborne enemy states and health and mana pickups.",
      ],
      whatLearned: [
        "How ASC ownership, Attribute Sets, Gameplay Tags, Effects, Abilities and Cues cooperates.",
        "How replicated attributes and UI listeners turn GAS state changes into readable player feedback.",
      ],
      sections: [],
      proof: [],
      improvements: [],
    },
  },
  "multiplayer-crash-course": {
    slug: "multiplayer-crash-course",
    visibility: "public",
    category: "learning",
    title: "UE5 Multiplayer Crash Course",
    shortTitle: "Multiplayer Crash Course",
    route: "#/projects/multiplayer-crash-course",
    summary:
      "Another crash course, this time to consolidate my Unreal Multiplayer Framework fundamentals with replication, listen-server model, session creation, PlayerState and RPCs.",
    technologies: ["Unreal Engine 5.7", "C++", "Replication", "OnlineSubsystem", "SteamSockets", "RPCs", "UMG"],
    mediaSlots: [
      readyImageSlot(
        "course-image",
        "Udemy course image for Unreal Engine 5 C++ Multiplayer CRASH COURSE",
        "media/courses/multiplayer-crash-course.jpg",
      ),
    ],
    actions: [
      courseAction("https://www.udemy.com/course/ue5-multiplayer-crash-course/"),
    ],
    includeInPreviousNext: true,
    page: {
      template: "learning",
      overview:
        "I used this project to practice Unreal multiplayer fundamentals in a smaller C++ codebase before applying the same ideas to larger co-op projects. The focus is listen-server flow, session creation and joining, replicated gameplay examples, PlayerState values, and authority debugging.",
      roleScope:
        "Course implementation using C++, OnlineSubsystem, SteamSockets, and replicated gameplay examples to consolidate Unreal multiplayer basics.",
      course: {
        title: "Unreal Engine 5 C++ Multiplayer CRASH COURSE",
        provider: "Udemy",
        instructor: "Stephen Ulibarri",
        url: "https://www.udemy.com/course/ue5-multiplayer-crash-course/",
      },
      courseContext:
        "I completed this course after the MOBA GAS course to consolidate my Unreal multiplayer fundamentals on: client-server model, listen-server testing, LAN and Steam session workflows, actor and component replication, RepNotifies, RPCs, ownership, core framework classes and multiplayer travel.",
      whatBuilt: [
        "A C++ multiplayer project around Unreal's client-server model, PIE multiplayer testing, LAN connection, and listen-server sessions through Steam.",
        "Session and travel flow using a MultiplayerSessions plugin, OnlineSubsystem, OnlineSubsystemSteam, SteamSockets, host/join menu behavior, session create/find/join/destroy, listen-server travel, client travel and seamless travel concepts.",
        "Actor replication examples covering replicated actors, movement replication, authority and net role checks, actor/component attachment, replicated variables, RepNotifies and replication conditions.",
        "Remote function for Client RPCs, Server RPCs, Multicast RPCs, relevancy, and priority, with authority debugging examples to show which machine owns and executes gameplay code.",
      ],
      whatLearned: [
        "How Unreal's multiplayer framework divides responsibilities across GameMode, GameState, PlayerState, PlayerController, Pawn/Character, HUD, widgets and replicated actors.",
        "How listen-server hosting, session search/join, travel, RPCs, RepNotifies, ownership and authority checks affect gameplay code structure.",
      ],
      sections: [],
      proof: [],
      improvements: [],
    },
  },
  "slash-cpp": {
    slug: "slash-cpp",
    visibility: "public",
    category: "learning",
    title: "Unreal C++ Course",
    shortTitle: "Slash C++",
    route: "#/projects/slash-cpp",
    summary:
      "First Unreal C++ project I wrote after the C++ fundamentals and Blueprint courses, following an action-RPG course to learn Unreal C++ gameplay structure, editor workflow, combat, enemy AI, UI, effects and reusable gameplay classes.",
    technologies: ["Unreal Engine 5.6", "C++", "Enhanced Input", "AI Behaviors", "UMG", "Niagara", "Chaos"],
    mediaSlots: [
      readyImageSlot(
        "course-image",
        "Udemy course image for Unreal Engine 5 C++ The Ultimate Game Developer Course",
        "media/courses/slash-cpp.jpg",
      ),
    ],
    source: slashCppSource,
    actions: [
      unrealDownloadAction("slash-cpp", "UE 5.6, 23.61 GiB ZIP"),
      browseSourceAction("slash-cpp"),
      courseAction("https://www.udemy.com/course/unreal-engine-5-the-ultimate-game-developer-course/"),
    ],
    includeInPreviousNext: true,
    page: {
      template: "learning",
      overview:
        "This project marks my transition from Blueprint-only Unreal learning into hands-on C++ gameplay programming. I wrote my own local Unreal project while following the course project, using it to practice Unreal's C++ class hierarchy, character movement, weapon equip flow, melee traces, enemy AI, attributes, UI, pickups, loot, effects, and destructible objects in a third-person action prototype.",
      roleScope:
        "Course-based Unreal C++ implementation written in my own local project, useful as the foundation step before later GAS, multiplayer, CommonUI, and original gameplay systems work.",
      course: {
        title: "Unreal Engine 5 C++ The Ultimate Game Developer Course",
        provider: "Udemy",
        instructor: "Stephen Ulibarri",
        url: "https://www.udemy.com/course/unreal-engine-5-the-ultimate-game-developer-course/",
      },
      courseContext:
        "I completed this course after my C++ fundamentals and Blueprint courses to move from visual scripting into Unreal C++ programming. The course provided the action-RPG project structure and baseline implementation, the Unreal project and source available here are my own local course project files and code I wrote while following that curriculum.",
      whatBuilt: [
        "A C++ third-person action-RPG prototype built from an empty/open-world setup, with landscapes, lighting, foliage, Packed Level Actors, Level Instances, and imported dungeon content.",
        "Core Unreal C++ exercises around Actor, Pawn, and Character classes using components, UPROPERTY/UFUNCTION exposure, debug helpers, templates, forward declarations, Enhanced Input, camera setup and controller-relative movement.",
        "Animation and character work including an Animation Blueprint based on a C++ class, jumping, IK foot placement, weapon sockets, montages, sound notifies, MetaSounds and equip/unequip animation flow.",
        "Combat systems with weapon pickup/equip, character and action state enums, melee box traces, dynamic arrays for one-hit-per-swing behavior, Unreal interfaces, hit sounds, hit particles, root-motion hit reactions, dot/cross-product directional reactions and enemy damage.",
        "Gameplay systems for Chaos breakable actors, Field System weapon impacts, treasure spawning, Niagara effects, health bars, damage, death poses, HUD updates, soul/gold attributes, soul pickups, dodge/stamina behavior and player death.",
        "AI architecture covering patrol targets, Pawn Sensing, enemy states, aggro/lost-interest behavior, attack radius, inherited enemy attack logic, timers, Motion Warping and root-motion attacks.",
      ],
      whatLearned: [
        "How to use Unreal C++ for components, inheritance, reflection, garbage collection, input binding, traces, collision, delegates, Blueprint exposure, animation events, UI, AI behavior and more.",
        "Best coding practices and architecture design decisions for a clean and healthy project code base."
      ],
      sections: [],
      proof: [],
      improvements: [],
    },
  },
  "cpp-game-development": {
    slug: "cpp-game-development",
    visibility: "public",
    category: "learning",
    title: "Learn C++ for Game Development",
    shortTitle: "C++ Fundamentals",
    route: "#/projects/cpp-game-development",
    summary:
      "The first C++ course I completed, using written C++ exercises to learn syntax, functions, references, pointers, object-oriented programming, dynamic memory, polymorphism, casting, and header/source organization.",
    technologies: ["C++", "Object-oriented programming", "Pointers", "Inheritance", "Casting", "Headers"],
    mediaSlots: [
      readyImageSlot(
        "course-image",
        "Udemy course image for Learn C++ for Game Development",
        "media/courses/cpp-game-development.jpg",
      ),
    ],
    actions: [
      courseAction("https://www.udemy.com/course/learn-cpp-for-ue4-unit-1/"),
    ],
    includeInPreviousNext: true,
    page: {
      template: "learning",
      overview:
        "This was my first C++ course and the foundation I used before moving into Unreal C++ gameplay projects. The course itself is not an Unreal Engine project; I use it as a learning milestone because I wrote the C++ practice code while learning the language basics needed to understand later Unreal classes, headers, pointers, inheritance, and object-oriented programming.",
      roleScope:
        "Course-based C++ fundamentals practice used to prepare for Unreal Engine programming, with console-style exercises and language work rather than a portfolio game prototype.",
      course: {
        title: "Learn C++ for Game Development",
        provider: "Udemy",
        instructor: "Stephen Ulibarri",
        url: "https://www.udemy.com/course/learn-cpp-for-ue4-unit-1/",
      },
      courseContext:
        "I completed this course before the Unreal C++ course to learn C++ from the ground up for game development programming. It gave me the language foundation I needed before writing later Unreal C++ projects.",
      whatBuilt: [
        "C++ practice projects covering input/output streams, variables, data types, statements, expressions, truth values, relational operators, scope, identifiers, keywords and functions.",
        "Hands-on exercises for function reuse, computations, while/do-while/for loops, references, function overloading, strings, constants, truth tables, arrays, enums, switch statements, and structs.",
        "Object-oriented programming exercises using classes, constructors, member variables/functions, access modifiers, inheritance, pointers, dynamic memory, stack/heap allocation, destructors, static variables, virtual functions, polymorphism, multiple inheritance, casting and header files.",
        "A C++ foundation specifically aimed at later game-engine work, so Unreal C++ class structure, header/source separation, inheritance hierarchies, pointers/references and object lifetime.",
      ],
      whatLearned: [
        "How basic C++ syntax, expressions, functions, control flow, loops, arrays, enums, structs, strings, constants and object-oriented programming work.",
        "Why pointers, references, constructors, inheritance, virtual functions and headers matter when reading or writing Unreal gameplay code.",
      ],
      sections: [],
      proof: [],
      improvements: [],
    },
  },
  "blueprint-prototypes": {
    slug: "blueprint-prototypes",
    visibility: "public",
    category: "learning",
    title: "Unreal Blueprint Course",
    shortTitle: "Blueprint Prototypes",
    route: "#/projects/blueprint-prototypes",
    summary:
      "First Unreal Engine course I completed, using Blueprint-only prototypes to learn the editor, visual scripting, materials, vectors, Enhanced Input, collision, UI, AI, Niagara, MetaSounds, Paper2D/PaperZD, and Chaos Vehicles.",
    technologies: ["Unreal Engine 5.6", "Blueprints", "AI", "UMG", "Materials", "Collisions", "Chaos Vehicles"],
    mediaSlots: [
      readyImageSlot(
        "course-image",
        "Udemy course image for Unreal Engine 5 Blueprints - The Ultimate Developer Course",
        "media/courses/blueprint-prototypes.jpg",
      ),
    ],
    actions: [
      courseAction("https://www.udemy.com/course/ue5-ultimate-bp-course/"),
    ],
    includeInPreviousNext: true,
    page: {
      template: "learning",
      overview:
        "These prototypes represent my first structured Unreal learning path. I wrote my own local Blueprint course projects while following Stephen Ulibarri's curriculum, using them to learn Unreal Engine and Blueprint visual scripting before moving into C++.",
      roleScope:
        "Course-based Blueprint-only Unreal projects grouped into one early learning page instead of separate thin project entries.",
      course: {
        title: "Unreal Engine 5 Blueprints - The Ultimate Developer Course",
        provider: "Udemy",
        instructor: "Stephen Ulibarri",
        url: "https://www.udemy.com/course/ue5-ultimate-bp-course/",
      },
      courseContext:
        "I completed this as my first Unreal Engine course, before the C++ fundamentals and Unreal C++ courses. The course provided the learning path, assets, and project designs, I've wrote my own Blueprint implementations while following that curriculum.",
      whatBuilt: [
        "Unreal editor exercises covering project setup, Starter Content, editor layout, PIE testing, object manipulation, perspective, Unreal file types, mesh scale, materials, Marketplace/FAB assets, and asset migration.",
        "Blueprint fundamentals for Level Blueprints, Blueprint classes, components, strings, coordinates, debug shapes, local/global space, vectors, rotators, booleans, branches, vector magnitude/normalization and rotator practice.",
        "Bad Bot, a drone flying shooter prototype with Pawn references, interpolation, sockets, spawned projectiles, projectile movement, Blueprint functions, Niagara hit effects, MetaSounds, timers, hit events, Expose on Spawn values, pickups, overlap events, GameMode score, Widget Blueprints, line traces for aiming, spawn volumes and a boss fight.",
        "Jetpack Journey, a 3D platformer prototype with Character class setup, custom PlayerController input, WASD/controller movement, animation Blendspaces, state machines, skeletal mesh sockets, Niagara, sound notifies, Character Movement flying mode, jet fuel resources, fuel pickups, UMG fuel bar, moving platforms, pressure plates, interact interfaces, Packed Level Actors, win/lose flow, end-game menu, event dispatchers, construction scripts, soft references, async loading, texture compression and dependency/memory awareness.",
        "Red Hood, a 2D dungeon crawler prototype with Paper2D/PaperZD, sprites, flipbooks, Paper Character setup, 2D camera settings, locomotion, transitional animations, tile sets/layers, sort priority, Behavior Trees, Blackboards, tasks, decorators, services, custom decorators, box traces, custom Anim Notifies, hit reactions, health bars, damage numbers, structs for attack data, combat effects and enemy death.",
        "Chaos Vehicle work using Unreal rigging/skinning tools, a vehicle skeleton and physics asset, Chaos Vehicle settings, Enhanced Input vehicle controls, and possession changes so a character can enter and leave the vehicle.",
      ],
      whatLearned: [
        "How to use Blueprints for gameplay programming and fast prototyping before moving into C++.",
        "How Unreal's editor, assets, class hierarchy, input, UI, animation, AI, collision, physics, effects, audio, and project structure fit together across several small games.",
        "Why Blueprint architecture still needs engineering discipline: clean nodes, interfaces, event dispatchers, dependency management, soft references, async loading, texture compression and awareness of Tick/casting costs.",
      ],
      sections: [],
      proof: [],
      improvements: [],
    },
  },
};

export const projects = projectOrder.map((slug) => projectRegistry[slug]);

export const publicProjects = projects.filter((project) => project.visibility === "public");

export const featuredOriginalProjects = publicProjects.filter(
  (project) => project.category === "original",
);

export const learningProjects = publicProjects.filter(
  (project) => project.category === "learning",
);

export const getProjectBySlug = (
  slug: string,
): PortfolioProject | undefined =>
  projectOrder.includes(slug as ProjectSlug)
    ? projectRegistry[slug as ProjectSlug]
    : undefined;

export const getAdjacentProjects = (slug: ProjectSlug) => {
  const navigableProjects = publicProjects.filter((project) => project.includeInPreviousNext);
  const currentIndex = navigableProjects.findIndex((project) => project.slug === slug);

  return {
    previous: currentIndex > 0 ? navigableProjects[currentIndex - 1] : undefined,
    next:
      currentIndex >= 0 && currentIndex < navigableProjects.length - 1
        ? navigableProjects[currentIndex + 1]
        : undefined,
  };
};

const englishUi: PortfolioUiLabels = {
  profileActions: "Profile actions",
  projectActions: "Project actions",
  profileFacts: "Profile facts",
  contact: "Contact",
  location: "Location",
  availability: "Availability",
  personalProjects: "Personal projects",
  learningProjects: "Completed Courses Projects",
  learningProjectsIntro:
    "These are Unreal projects I implemented while completing courses. The course context is credited, but the available project files and source snapshots show my own hands-on implementation work, not just watched lessons.",
  viewProject: "View project",
  technologies: (projectTitle) => `${projectTitle} technologies`,
  mediaSlots: (projectTitle) => `${projectTitle} media slots`,
  placeholder: (type, caption) => `${type} placeholder: ${caption}`,
  backToProjects: "Back to projects",
  courseContext: "Course context",
  whatBuilt: "What I built",
  whatLearned: "What I learned",
  attributionAndScope: "Attribution and scope",
  technicalProof: "Technical proof",
  improveNext: "What I would improve next",
  overview: "Overview",
  sourceReferences: (sectionTitle) => `${sectionTitle} source references`,
  sourceUnavailable: "Source unavailable",
  sourceUnavailableBody: "This project does not have a committed source snapshot yet.",
  loadingSourceSnapshot: "Loading source snapshot",
  loadingSourceManifest: "Loading committed source manifest.",
  sourceManifestUnavailable: "Source manifest unavailable",
  sourceUnknownManifestError: "Unknown manifest error.",
  sourceBrowser: "Source browser",
  sourceFiles: "Source files",
  sourceFilesEmpty: "No source files are listed in this manifest.",
  codeReader: "Code reader",
  loadingSourceFile: "Loading source file.",
  sourceUnknownFileError: "Unknown source file error.",
  closeSource: "Close source",
  sourceTitle: (projectShortTitle) => `${projectShortTitle} source`,
  line: (lineNumber) => `Line ${lineNumber}`,
  toggleFolder: (path) => `Toggle ${path}`,
  lines: (count) => `${count} lines`,
  ownershipLabels: {
    "project-code": "Project code",
    "project-plugin": "Project plugin",
    "third-party-plugin": "Third-party plugin",
  },
  notFoundPage: "Page not found",
  projectRouteUnavailable: "Project route unavailable",
  noPublicProject: (slug) => `No public project page is registered for "${slug}".`,
  unmatchedRoute: "The route does not match this portfolio.",
  returnToProjects: "Return to projects",
  documentTitle: {
    home: (profile) => `${profile.name} - ${profile.role}`,
    project: (project, profile) => `${project.title} - ${profile.name}`,
    source: (project, profile) => `${project.shortTitle} source - ${profile.name}`,
    notFound: (profile) => `Project not found - ${profile.name}`,
  },
  languageToggle: "Portfolio language",
};

const frenchUi: PortfolioUiLabels = {
  ...englishUi,
  profileActions: "Actions du profil",
  projectActions: "Actions du projet",
  profileFacts: "Informations du profil",
  contact: "Contact",
  location: "Localisation",
  availability: "Disponibilité",
  personalProjects: "Projets personnels",
  learningProjects: "Projets de cours terminés",
  learningProjectsIntro:
    "",
  viewProject: "Voir le projet",
  technologies: (projectTitle) => `Technologies de ${projectTitle}`,
  mediaSlots: (projectTitle) => `Médias de ${projectTitle}`,
  placeholder: (type, caption) => `Emplacement ${type} : ${caption}`,
  backToProjects: "Retour aux projets",
  courseContext: "Contexte du cours",
  whatBuilt: "Ce que j'ai construit",
  whatLearned: "Ce que j'ai appris",
  attributionAndScope: "Attribution et périmètre",
  technicalProof: "Preuves techniques",
  improveNext: "Ce que j'améliorerais ensuite",
  overview: "Vue d'ensemble",
  sourceReferences: (sectionTitle) => `Références source : ${sectionTitle}`,
  sourceUnavailable: "Source indisponible",
  sourceUnavailableBody: "Ce projet n'a pas encore de snapshot source validé.",
  loadingSourceSnapshot: "Chargement de la capture de source",
  loadingSourceManifest: "Chargement du manifeste source.",
  sourceManifestUnavailable: "Manifeste source indisponible",
  sourceUnknownManifestError: "Erreur de manifeste inconnue.",
  sourceBrowser: "Navigateur de source",
  sourceFiles: "Fichiers source",
  sourceFilesEmpty: "Aucun fichier source n'est listé dans ce manifeste.",
  codeReader: "Lecteur de code",
  loadingSourceFile: "Chargement du fichier source.",
  sourceUnknownFileError: "Erreur de fichier source inconnue.",
  closeSource: "Fermer la source",
  sourceTitle: (projectShortTitle) => `Source ${projectShortTitle}`,
  line: (lineNumber) => `Ligne ${lineNumber}`,
  toggleFolder: (path) => `Basculer ${path}`,
  lines: (count) => `${count} lignes`,
  ownershipLabels: {
    "project-code": "Code du projet",
    "project-plugin": "Plugin du projet",
    "third-party-plugin": "Plugin tiers",
  },
  notFoundPage: "Page introuvable",
  projectRouteUnavailable: "Route de projet indisponible",
  noPublicProject: (slug) => `Aucune page de projet publique n'est enregistrée pour "${slug}".`,
  unmatchedRoute: "Cette route ne correspond pas à ce portfolio.",
  returnToProjects: "Retour aux projets",
  documentTitle: {
    home: (profile) => `${profile.name} - ${profile.role}`,
    project: (project, profile) => `${project.title} - ${profile.name}`,
    source: (project, profile) => `Source ${project.shortTitle} - ${profile.name}`,
    notFound: (profile) => `Projet introuvable - ${profile.name}`,
  },
  languageToggle: "Langue du portfolio",
};

const frenchSiteProfile: SiteProfile = {
  ...siteProfile,
  role: roleVariantProfiles[defaultRoleVariant].fr.role,
  availability: "Immédiatement",
  positioning:
    "Systèmes de gameplay réseau, interactions GAS, UI adaptée à la manette et outils de débogage pour projets Unreal Engine.",
  actions: applyProfileCvAction(siteProfile.actions, defaultRoleVariant, "fr"),
};

const localizedProjectRoute = (
  slug: ProjectSlug,
  language: Language,
): PortfolioProject["route"] =>
  language === "fr" ? `#/projects/${slug}?lang=fr` : `#/projects/${slug}`;

const localizedProjectSourceRoute = (slug: ProjectSlug, language: Language) =>
  language === "fr" ? `#/projects/${slug}/source?lang=fr` : `#/projects/${slug}/source`;

function localizeProjectAction(
  action: ProjectAction,
  slug: ProjectSlug,
  language: Language,
): ProjectAction {
  if (language === "en") {
    return action;
  }

  const labelByKind: Partial<Record<ProjectAction["kind"], string>> = {
    course: "Cours",
    download: "Télécharger le projet Unreal",
    source: "Voir le code source",
  };

  return {
    ...action,
    label: labelByKind[action.kind] ?? action.label,
    href: action.kind === "source" ? localizedProjectSourceRoute(slug, language) : action.href,
    pendingReason: action.pendingReason,
  };
}

type ProjectFrenchText = {
  summary: string;
  mediaCaptions?: string[];
  overview: RichTextContent;
  roleScope: RichTextContent;
  sections?: Array<{
    title: string;
    body: RichTextContent;
    points?: RichTextContent[];
  }>;
  proof?: RichTextContent[];
  improvements?: RichTextContent[];
  courseContext?: RichTextContent;
  whatBuilt?: RichTextContent[];
  whatLearned?: RichTextContent[];
  scopeNotes?: RichTextContent[];
};

const frenchProjectText: Record<ProjectSlug, ProjectFrenchText> = {
  "hospital-flow": {
    summary:
      "Simulateur d'urgences en coopération en ligne, avec modèle hôte/client autoritaire, usage intensif du Gameplay Ability System et CommonUI pour tout le frontend.",
    mediaCaptions: [
      "Boucle Hub vers Night Shift et résolution des quotas",
      "Lobby Steam, code d'invitation et répartition des rôles co-op",
      "État patient, IA, files d'attente et pression hospitalière",
      "Spawn de patients avec difficulté et pooling",
      "Réception, examens, traitements, récompenses de guérison et sortie",
      "Framework réutilisable de stations médicales et gameplay spécifique",
      "États des patients, des joueurs, des machines et de l'équipe pilotés par GAS",
      "Vote d'améliorations du Hub et application en runtime",
      "Événements spéciaux, phase d'alerte, pression active et retours lumineux",
      "Frontend CommonUI, HUD, tablette, triage et dossier patient",
      "Outils de débogage en jeu pour itérer sur les runs et l'équilibrage",
      "Localisation par gameplay tags pour l'UI médicale",
    ],
    overview:
      "J'ai construit un simulateur d'urgences co-op autour d'une boucle partagée : recevoir des patients, les envoyer vers le bon examen, terminer l'examen puis tenter de les soigner tout en gérant les files d'attentes et la pression de l'hôpital. Le projet est jouable jusqu'à huit joueurs et possède un flux de sessions Steam Online Subsystem fonctionnel. Le développement est en pause et le projet demande encore beaucoup de polish avant d'être présenté en démo.",
    roleScope:
      "Projet solo Unreal Engine 5 centré sur la co-op listen-server, les validations autoritaires serveur, les patients IA répliqués, l'état des files, les interactions GAS, l'UI compatible manette avec CommonUI et les outils de débogage en jeu.",
    sections: [
      {
        title: "Boucle Hub vers Gardes",
        body:
          "La structure de run fait passer l'équipe du Hub vers les Gardes, puis revient au Hub. Le serveur gère la sélection, les quotas, l'overtime, les échecs, les missions finales, les résumés et l'état qui doit survivre aux changements de carte.",
        points: [
          "Les GameMode pilotent les transitions côté serveur tandis que le NightShift GameState réplique l'état de run, de résumé, de vote et de travel aux clients.",
          "Les règles de run sont paramétrées par data assets : quotas, durée de cycle, durée de garde.",
          "Un subsystem conserve l'état actif entre Hub et Garde, utilisation du ServerTravel afin d'éviter une duplication d'état.",
        ],
      },
      {
        title: "Co-op et sessions",
        body:
          "J'ai conçu l'hôpital autour de rôles flexibles plutôt que de professions fixes. Chaque joueur peut aider à la réception, à une machine d'examen ou aux lits de traitement, donc le réseau doit garder l'état des patients et les files d'attentes cohérentes pour toute l'équipe.",
        points: [
          "Les sessions, les invitations Steam, les codes d'invitation et le travel robuste font partie de l'implémentation réseau.",
          "Les clients envoient des intentions via UI et contrôleur pendant que les systèmes valident sélections, interactions, contrôles de session et transitions côté serveur.",
          "Les world widgets, matériaux et sons réagissent au flux hospitalier répliqué pour ne pas dépendre sur une interface écran.",
        ],
      },
      {
        title: "Flux patient et comportement IA",
        body:
          "Le flux des patients crées la pression principale du jeu. Les joueurs lisent le dossier médical, choisissent le bon examen ou traitement et doivent réagir quand les files d'attentes deviennent difficiles à tenir.",
        points: [
          "Les définitions des patients sont data-driven : identité, constantes, pathologies, symptômes, examens et traitements.",
          "Les phases, pathologies, examens, besoins de traitement, patience et constantes sont modélisés par attributs, effets et tags GAS.",
          "Le Gameplay Message Subsystem publie les intentions de déplacement avec des Blackboard Key, et une task de snap gère le placement final précis.",
        ],
      },
      {
        title: "Spawn et difficulté data-driven",
        body:
          "La pression des gardes est dirigée par les données : carte, difficulté, nombre de joueurs, overtime et événements spéciaux alimentent le spawner.",
        points: [
          "Les data assets NightShift configurent carte, difficulté, tags, modificateurs de spawn et éligibilité aux événements.",
          "Au runtime un calcul combine taux de base, multiplicateurs de difficulté, scaling joueurs, overtime et événements.",
          "Les patients sont préchauffés dans une pool puis préparés par étapes avant d'être spawn pour éviter des pics de consommation.",
        ],
      },
      {
        title: "Pipeline examens et traitements",
        body:
          "Le système déplace les patients de la réception vers les examens, les lits de traitement et la sortie. Le point important est que le routage reste modulaire et data-driven.",
        points: [
          "La réception valide côté serveur les examens choisis par rapport aux pathologies avant de faire progresser le patient.",
          "Les sous-systèmes de files d'attentes assignent machines, sièges, lits et stations tout en gardant l'autorité serveur.",
          "Les examens terminés, traitements, récompenses et sorties passent par des tags, Gameplay Effects et intention de navigation IA.",
        ],
      },
      {
        title: "Framework réutilisable de stations médicales",
        body:
          "Les machines partagent un modèle d'interaction, une identité par gameplay tag, une assignation patient, un état répliqué et une intégration aux files d'attentes. Chaque station ajoute ensuite son propre gameplay.",
        points: [
          "Une classe parente de machine gère occupation, patients assignés, payloads d'interaction, tags, points d'attente, notifications de file et cibles de navigation.",
          "Réception, XRay, Ultrasound, CT Scan, Lab Analyzer, Treatment beds, oxygen, IV et Item Dispenser se branchent sur la même architecture.",
          "L'état répliqué synchronise opérateurs, patients, machine et feedback UI.",
        ],
      },
      {
        title: "Intégration GAS",
        body:
          "GAS sert de contrat partagé entre joueurs, patients IA, machines, UI et systèmes de run. Il stocke l'état médical, pilote les interactions, applique les tags de file d'attente et met à jour l'économie d'équipe.",
        points: [
          "L'ASC joueur vit sur le PlayerState, les patients possèdent leur propre ASC, et le NightShift GameState possède l'ASC d'équipe.",
          "Les gameplay events routent les interactions depuis objets et machines vers les gameplay abilities.",
          "Les tags modélisent phases patient, pathologies, files, examens, machine, améliorations et récompenses.",
        ],
      },
      {
        title: "Vote d'améliorations du Hub",
        body:
          "Après la sélection d'une Garde, le Hub génère des offres d'amélioration et lance un vote répliqué via la tablette. L'amélioration gagnante devient une pile de gameplay tags conservée pendant le travel.",
        points: [
          "Le serveur possède génération, validation des votes, compte à rebours, choix gagnant et application.",
          "Les définitions stockent texte, tag d'identité et achat maximum, tandis que les piles actives vivent dans l'état de run répliqué.",
          "Les améliorations modifient des acteurs placés comme les lits, machines à café, machine à oxygen, boîte magique et distributeurs.",
        ],
      },
      {
        title: "Événements spéciaux",
        body:
          "Les événements spéciaux sont des crises programmées par le serveur. Ils passent par les états Alerte, Actif et Terminé, ils influencent spawn, pondération des pathologies, alertes UI, audio global et lumières.",
        points: [
          "Les data assets définissent les timing, texte d'alerte, multiplicateurs de spawn, influence pathologique et éclairage.",
          "Les événements sont filtrés par difficulté et tags de garde avant planification serveur.",
        ],
      },
      {
        title: "Architecture CommonUI et UI de gameplay",
        body:
          "L'architecture UI utilise un manager LocalPlayer, des stacks CommonUI, le chargement async de soft widgets, des écrans de confirmations réutilisables, des options data-driven et des world widgets liés au gameplay.",
        points: [
          "Les stacks frontend couvrent menu principal, lobby, options, crédits, invitation, confirmations et chargement.",
          "Les stacks gameplay couvrent HUD, pause, sélection Hub, votes, réception, machines, traitements, cartes patient et dossiers médicaux.",
          "Les widgets se branchent sur: GameState répliqué, attributs GAS d'équipe, subsystem LocalPlayer et controllers UI patient.",
        ],
      },
      {
        title: "Outils développeur",
        body:
          "J'ai construit un outil de débogage en jeu pour accélérer les tests sans 'tricher'. Les actions UI passent par des RPC serveur puis par les mêmes systèmes que le gameplay normal.",
        points: [
          "L'outil peut spawn des patients, forcer succès ou échec des gardes, modifier les revenus et réputation, changer la vitesse du jeu, relancer les améliorations ou en accorder une.",
          "Le spawn utilise le vrai subsystem, le pool des patients, les sièges et les intentions de navigation avec le gameplay message subsystem.",
        ],
      },
      {
        title: "Localisation et présentation par tags",
        body:
          "Les systèmes gameplay utilisent des tags pour: groupes sanguins, symptômes, pathologies, examens et traitements. Mon système de localisation mappe ces tags vers des entrées de string table pour rendre pouvoir utiliser ces gameplay tags dans le code et conserver une traduction complète du jeu.",
        points: [
          "Le subsystem met en cache les correspondances gameplay-tag vers clé de localisation depuis une data table configurée.",
        ],
      },
    ],
    proof: [],
    improvements: [],
  },
  agass: {
    summary:
      "Jeu co-op en ligne avec modèle hôte/client autoritaire, pensé pour le modding avec kit de modding personnalisé et outils Unreal.",
    mediaCaptions: [
      "Architecture de plugins et module racine",
      "Hébergement, navigateur de sessions, code d'invitation et reprise frontend",
      "Contrat de compatibilité carte/mod et refus de connexion",
      "Pickup, portage, preview, rotation et placement",
      "Data assets, spawners d'objets, échafaudage et outils utilisables",
      "Portefeuille partagé, achat physique, HUD et zone de vente",
      "Timer, objectif, résumé, meilleur temps et retour frontend",
      "Événements Mad Cops et Mad Plane en multijoueur",
      "Écran mods/cartes, export de mod et métadonnées de session",
      "Workflow Modding Kit, validation, export et packaging",
      "CommonUI, navigateur de sessions, options et rebinding",
      "Invitations Steam, stats et leaderboards de run",
    ],
    overview:
      "AGASS est un prototype co-op de construction de tour. Les joueurs empilent des objets qui tombent et achètent des outils pour atteindre l'objectif le plus vite possible. Le jeu supporte les sessions Steam, les codes d'invitation, les mods de contenu, la sélection de cartes et un kit de modding avec outil d'export Unreal.",
    roleScope:
      "Travail solo en C++ sur gameplay, online, frontend, économie, Steam, modding, outils éditeur et architecture système. Je le présente comme une étude de cas originale avec snapshot source versionné, tandis que les médias finaux restent en attente de revue.",
    sections: [
      {
        title: "Architecture Unreal modulaire",
        body:
          "AGASS est structuré autour d'un petit module racine UE 5.7 et de plugins internes spécialisés pour settings, frontend, online, Steam, interaction, gameplay de tour, économie, mods et tooling éditeur.",
        points: [
          "Le module racine reste proche de la glue framework : game instance, game mode, game state, player controller, character et composition inter-plugin.",
          "Les plugins runtime possèdent les domaines gameplay et UX, tandis que les modules éditeur isolent l'export de mods et les aspects créateur.",
          "Le C++ possède autorité, réplication, validation et contrats de sous-systèmes ; le contenu créé reste dans les assets.",
        ],
      },
      {
        title: "Flux de sessions multijoueur",
        body:
          "Le online repose sur de la co-op listen-server. Les joueurs hébergent directement une carte, rejoignent via navigateur, code d'invitation ou invitation Steam, puis reviennent proprement au FrontendMap.",
        points: [
          "Les chemins navigateur, code et invitation Steam convergent vers le même join générique.",
          "L'hébergement publie build, carte, mod actif, invite-code et métadonnées de compatibilité avant d'ouvrir la carte.",
          "Les échecs réseau/travel et retours frontend mettent à jour l'UI au lieu de bloquer les joueurs.",
        ],
      },
      {
        title: "Contrat de compatibilité de session",
        body:
          "Rejoindre une session est traité comme un contrat de contenu, pas seulement une adresse réseau. L'hôte annonce build, carte, mods et hash stable ; les clients vérifient avant travel et le serveur revalide en PreLogin.",
        points: [
          "Le hash repose sur IDs stables de cartes/mods, versions de compatibilité et tags.",
          "Le navigateur peut désactiver des lignes avant travel, tandis que PreLogin protège encore le serveur.",
          "Le join-in-progress reste ouvert pendant les runs actifs puis se ferme lors de la complétion.",
        ],
      },
      {
        title: "Interaction, portage, preview et placement",
        body:
          "La boucle centrale permet de ramasser, de viser une preview réactive, de tourner par pas contrôlés, d'ajuster la distance et de confirmer via validation serveur.",
        points: [
          "La prédiction locale garde la visée responsive pendant que le serveur approuve la transformation finale.",
          "La validation couvre portée, overlap, règles acteur, hooks de définition, snapping et échafaudage.",
          "Les objets utilisables attachés au personnage suivent un chemin d'action séparé du placement.",
        ],
      },
      {
        title: "Framework d'objets piloté par contenu",
        body:
          "Les objets sont créés dans des Data Assets et Blueprints, tandis que le C++ possède les règles multijoueur de pickup, portage, placement, shops, vente, spawn aléatoire et outils tenus.",
        points: [
          "Les définitions exposent meshes, classes, coûts, valeurs de vente, tuning de placement, sockets et comportement consume-on-use.",
          "Les objets aléatoires utilisent des données runtime répliquées pour générer des props variés sans un asset par objet.",
          "L'échafaudage spécialise les règles de pile/support, et le Fumigène prouve un outil tenu répliqué.",
        ],
      },
      {
        title: "Économie partagée, shop et vente",
        body:
          "L'équipe partage un portefeuille répliqué sur GameState. Les shops physiques construisent leurs slots avec meshes, prix, triggers et spawn points ; les zones de vente convertissent les objets valides en argent d'équipe.",
        points: [
          "Les achats sont acceptés uniquement serveur : overlap, état tenu, prix, spawn, claim, refund et feedback.",
          "Le catalogue définit références, overrides de prix, offsets et claim automatique.",
          "Les zones de vente rejettent les items cachés, créditent le wallet et détruisent le placeable vendu.",
        ],
      },
      {
        title: "Run chrono, objectif et fin de boucle",
        body:
          "Le timed-run transforme le bac à sable de tour en boucle répétable : démarrage intentionnel, construction, objectif, résumé répliqué, meilleurs temps locaux, leaderboard Steam si disponible et retour frontend.",
        points: [
          "RunPhase contrôle le cycle de session, TimedRunState contrôle l'état WaitingToStart, Active et Completed.",
          "L'affichage dérive du temps serveur répliqué et le résultat officiel est figé en millisecondes.",
          "Les objectifs et hooks Blueprint/manuels partagent la même interface de complétion.",
        ],
      },
      {
        title: "Framework d'événements de session",
        body:
          "Les événements de session sont des moments gameplay data-driven fournis par la carte et les mods de contenu actifs. Le serveur charge, planifie, spawn et nettoie les acteurs d'événement répliqués.",
        points: [
          "Les définitions de carte peuvent fournir des événements locaux et les manifests de mods des événements globaux.",
          "Le manager possède timing, concurrence, réplication, destruction et arrêt.",
          "Mad Cops et Mad Plane prouvent deux formes différentes de pression multiplayer.",
        ],
      },
      {
        title: "Pipeline modding et sélection de cartes",
        body:
          "AGASS supporte des mods de plugins de contenu cuits avec manifests, définitions de cartes, toggles de mods actifs et métadonnées de compatibilité multijoueur.",
        points: [
          "Le subsystème scanne les racines locales, monte les plugins valides, charge les manifests, résout la carte sélectionnée et construit les hashes.",
          "Le frontend liste cartes et mods, active automatiquement le mod requis par une carte et empêche de le désactiver.",
          "Le chemin runtime reste content-only et ne charge pas de code natif arbitraire.",
        ],
      },
      {
        title: "Workflow d'auteur Modding Kit",
        body:
          "Le Modding Kit est le côté créateur du pipeline. Il donne un environnement Unreal contrôlé pour créer des mods de contenu avec manifests, cartes et événements avant export.",
        points: [
          "Le support du kit reste isolé des modules gameplay live tout en exposant les types d'assets utiles.",
          "Le module éditeur valide descriptor, identité, IDs, versions, propriété d'assets, containers cuits, AssetRegistry et layout.",
          "Les settings d'export centralisent racines, nommage et validations pour rendre le workflow répétable.",
          "Le résultat est un workflow borné pour cartes et événements de session.",
        ],
      },
      {
        title: "Frontend CommonUI et UX settings",
        body:
          "Le frontend est une pile CommonUI de forme production : démarrage, menu, sessions, codes, cartes/mods, recovery de chargement, menu en session, options et rebinding.",
        points: [
          "Le manager UI et la policy possèdent lifetime, layers tagués, back handling, focus recovery et séparation frontend/game-menu.",
          "Les pages GameSettings couvrent graphiques, audio, gameplay, contrôles, interface, tuning placement, benchmark et meilleurs temps.",
          "Le rebinding ajoute capture brute, confirmation de doublons, persistance Enhanced Input et glyphes gamepad.",
        ],
      },
      {
        title: "Couche plateforme Steam",
        body:
          "Le comportement Steam vit dans AGASSSteam pendant qu'AGASSOnline garde le cycle générique de sessions, codes, compatibilité, travel et recovery.",
        points: [
          "Le bouton d'invitation passe par une interface de menu plateforme puis ouvre le dialogue d'amis ciblé.",
          "Les invitations acceptées reviennent dans le chemin de join partagé avec les mêmes vérifications.",
          "Stats, succès, temps de jeu, uploads timed-run et leaderboards amis viennent des événements gameplay.",
        ],
      },
    ],
    proof: [
      "Architecture UE 5.7 modulaire avec plugins séparés pour settings, frontend, online, Steam, interaction, tower, économie, mods runtime et Modding Kit.",
      "Flux listen-server avec navigateur, codes, invitations Steam, métadonnées de carte/mod, hash de compatibilité, rejet PreLogin et recovery frontend.",
      "Pickup, portage, preview, validation, objets utilisables, spawners, échafaudage et Fumigène autoritaires serveur.",
      "Wallet d'équipe répliqué, shop physique, achats/refunds serveur, zones de vente et HUD.",
      "Boucle timed-run avec start actor, objectif, résumé répliqué, best-time local, hooks Steam et retour frontend.",
      "Pipeline de mods de contenu avec manifests, cartes, événements, discovery runtime, sélection frontend et validation d'export.",
      "Frontend CommonUI avec layers persistants, sessions, mods/cartes, options, rebinding, loading recovery, menu en session et affichage Steam.",
    ],
    improvements: [
      "Capturer les médias système par système : architecture, sessions, compatibilité, placement, objets, économie, timed run, événements, modding, frontend et Steam.",
      "Renforcer les tests autour des incompatibilités, validations de manifest, achats/refunds, complétion de timed-run et smoke tests Steam packagés.",
      "Garder le support mod centré sur cartes et événements de session en plugins de contenu.",
      "Nettoyer la localisation et la dérive de configuration pour que le texte public suive les conventions data-driven.",
      "Séparer les décisions de distribution de cette page et concentrer le portfolio sur médias revus, source, attribution et validation d'export.",
    ],
  },
  tlm: {
    summary:
      "Prototype FPS horde-survivor roguelite créé pour apprendre Godot avec architecture data-driven, tooling éditeur, tests automatisés, support manette, localisation et sauvegarde.",
    mediaCaptions: [
      "Capture gameplay en attente : choix du mage, run, jour, marché de nuit et résumé",
      "Capture sorts, objets, procs et synergies en attente",
      "Capture horde, pression de tour, barricade et sceau explosif en attente",
      "Capture jour/nuit, offres, reroll et progression en attente",
      "Capture authoring d'items, validation, tags et localisation en attente",
      "Capture sauvegardes, options, manette, localisation et frontend en attente",
      "Capture régression, Balance Lab, rapports et source-browser en attente",
    ],
    overview:
      "The Last Mage est mon projet personnel Godot 4.7 C# de FPS horde-survivor roguelite. Je l'utilise pour montrer un travail de systèmes autour de l'architecture de run, des synergies sorts/objets, de la pression horde/tour, du contenu data-driven, du tooling éditeur, de la validation de régression, de la sauvegarde, du support manette, de la localisation et de la préparation playtest sans proposer de build public.",
    roleScope:
      "Travail solo Godot 4.7 C# sur gameplay, tooling, UI, sauvegarde/profil, localisation, manette, validation et architecture production. C'est une preuve système originale, pas une sortie commerciale : pas encore de build public, intégration Steam non présentée comme complète, Balance Lab comme preuve d'ingénierie et médias en attente.",
    sections: [
      {
        title: "Architecture gameplay de production",
        body:
          "TLM est construit comme une colonne vertébrale Godot 4.7 C# plutôt qu'un prototype jetable. La racine de composition charge les catalogues Resource, valide le contenu, crée un contexte de run et branche commandes, events, sauvegarde, localisation, pools et systèmes gameplay.",
        points: [
          "RunControllerNode est la racine inspectable pour chargement, validation, état de run, event bus, dispatcher, profil, localisation et systèmes ordonnés.",
          "Les scènes Godot, Resources, docs et rapports soutiennent la preuve, mais les liens de source restent sur les fichiers C# versionnés.",
          "La page montre l'architecture et la préparation playtest sans revendiquer de build public, Steam fini, balance finale ou médias finaux.",
        ],
      },
      {
        title: "Combat, sorts, objets et synergies de build",
        body:
          "Le combat passe par des systèmes partagés pour slots de sorts, cast flow, exécution d'effets, projectiles, AoE, statuts, dégâts, modificateurs d'objets et chaînes de procs.",
        points: [
          "SpellSystem distribue des executors communs au lieu de traiter chaque sort comme un script de scène unique.",
          "ProcSystem borne les chaînes d'objets avec récursion et budgets par frame.",
          "Les Resources d'items et contenus Godot sont décrits comme preuves, tandis que les références source restent sur C#.",
        ],
      },
      {
        title: "Hordes, routage de tour et défenses",
        body:
          "Le travail de horde repose sur de vrais ennemis actifs avec vie, attaques, routage, queries spatiales, présentation et métriques debug. La tour devient une pression gameplay, pas un simple fail state.",
        points: [
          "EnemySystem possède simulation, attaques, ciblage défenses, séparation, décisions de route, binding acteur et métriques.",
          "TowerNavigationAdapter rend explicites l'entrée, les escaliers et la route vers la base du mage.",
          "Scènes de benchmark et visuels debug soutiennent la preuve sans devenir des cibles de source.",
        ],
      },
      {
        title: "Boucle jour/nuit et progression marché",
        body:
          "La boucle sépare les jours de combat, pression maximale, nettoyage, marché de nuit, défense nocturne, victoire et défaite via des permissions de phase explicites.",
        points: [
          "Les offres de nuit sont des récompenses choose-one déterministes avec filtrage, exclusion en run, reroll et connaissance persistante.",
          "Succès et données de profil prouvent une méta-progression sans prétendre à un tuning final.",
          "Le travail Steam reste positionné comme préparation, pas comme distribution complète.",
        ],
      },
      {
        title: "Authoring de contenu et outils de validation",
        body:
          "La preuve tooling la plus forte est le pipeline autour des Godot Resources, de la validation, de l'authoring d'items, des gameplay tags, de la localisation et des rapports répétables.",
        points: [
          "ItemAuthoringDock fournit un workflow éditeur pour IDs, noms, poids, tags, unlocks, textes, icônes, effets, previews et validation.",
          "ContentValidator vérifie IDs, références, effets, unlocks, catalogues et hypothèses runtime.",
          "Resources, scènes, CSV et rapports sont cités comme preuves textuelles plutôt que cibles source.",
        ],
      },
      {
        title: "Debugging, profiling, régression et Balance Lab",
        body:
          "TLM a une infrastructure de régression et d'analyse couvrant spine du projet, combat, stats, portée, durée, marché, tour, défenses, ennemis, méta-progression, items, localisation, input, sauvegarde et options.",
        points: [
          "RegressionRunnerNode centralise la couverture visible/headless et enregistre les résultats.",
          "BalanceSimulationRunner produit des rapports déterministes pour signaux d'ingénierie.",
          "Les rapports locaux sont décrits comme preuves, pas comme liens de source.",
        ],
      },
      {
        title: "Shell production, sauvegardes, options, manette et localisation",
        body:
          "Le shell actuel dépasse un démarrage debug : sélection de slot, choix du mage, stats, checkpoints de run, settings, pause/options, prompts manette, rebinding et services de localisation.",
        points: [
          "FrontEndPanel possède le front-end orienté save-slot, menu, sélection de mage, stats, options, crédits, feedback et continue/new-run.",
          "SaveServiceNode et RunCheckpointService séparent persistance profil/settings et checkpoints de la scène live.",
          "Manette, localisation, options et sauvegarde sont présentés comme preuves de shell, avec polish public futur.",
        ],
      },
    ],
    proof: [
      "Projet Godot 4.7 C# visant Godot.NET.Sdk/4.7.0-beta.1 avec C# nullable, systèmes scoped par run, catalogues Resource et snapshot C# versionné.",
      "Racine de composition branchant validation, localisation, profil/settings, event bus, commandes, pools, combat, sorts, AoE, statuts, invocations, défenses, ennemis, marché, achievements, rapports et UI.",
      "Architecture sorts/items couvrant executors partagés, attribution des dégâts, attributs joueur, modificateurs, procs bornés, items activables et cast-flow.",
      "Preuve horde/tour avec simulation ennemie, spatial registry, routes de tour, défenses, wave director et infrastructure benchmark.",
      "Tooling : validation de contenu, authoring items, tags, localisation, regression runner, profiling, Balance Lab et rapports.",
      "Shell production : slots de sauvegarde, stats, checkpoints, options/pause, prompts manette, rebinding, localisation et frontend.",
    ],
    improvements: [
      "Remplacer les médias placeholder par des captures revues de gameplay, synergies, horde/tour, tooling, sauvegarde/options/manette/localisation et preuve source.",
      "Terminer le smoke UI product-owner pour options/pause avant de présenter ces surfaces comme polies.",
      "Continuer l'intégration playtest Steam et la validation packaging avant toute revendication Steam.",
      "Utiliser les retours playtest avant de traiter Balance Lab comme direction finale de balance.",
      "Limiter les références source aux fichiers C# versionnés et décrire Resources, scènes, docs et rapports comme preuves textuelles.",
    ],
  },
  "moba-gas": {
    summary:
      "Prototype complet de MOBA compétitif avec modèle serveur dédié, usage intensif du Gameplay Ability System, IA et architecture data-driven.",
    mediaCaptions: [
      "Image du cours Udemy Multiplayer in Unreal with GAS and AWS Dedicated Servers",
    ],
    overview:
      "J'ai construit ce projet MOBA Unreal en suivant un projet de cours et en écrivant le code dans mon propre projet local. La base suit largement le cours, mais le projet téléchargeable et la capture de source montrent le code que j'ai écrit et mes refactors vers des assets plus data-driven.",
    roleScope:
      "Implémentation Unreal C++ et travail de refactor dans un projet de curriculum multiplayer GAS, avec source présentée via une capture de portfolio revue.",
    courseContext:
      "J'ai réalisé ce projet Unreal en suivant le cours Udemy de Jingtian Li sur le multiplayer Unreal avec le Gameplay Ability System. Le cours a fourni une grande partie de la base ; les fichiers disponibles ici sont mon projet local terminé avec mes propres modifications.",
    whatBuilt: [
      "Fondations d'un sytème de combat utilisant GAS avec ASC personnalisé, Attribute Sets, Gameplay Effects, Cues, Tags et classes d'abilities en C++.",
      "Tous les systèmes répliqués pour attributs, abilities, mort/respawn, XP, niveaux, points d'upgrade, or et shop.",
      "IA des minions par équipe avec perception, behavior trees, blackboards, pooling, skins d'équipe et routage des objectifs.",
      "Objectif de match répliqué qui avance selon la dominance d'unités et pilote la caméra de victoire ou défaite.",
      "Inventaire et shop répliqués avec RPC serveur, objets stackables/consommables, combinaisons, cooldowns et abilities accordées.",
    ],
    whatLearned: [
      "Structurer du gameplay serveur-autoritaire à l'aide du Gameplay Ability System.",
      "Comprendre comment l'ASC répliqué, tags, effets, cues et feedback UI coopèrent ensemble.",
      "Passer des systèmes hardcodés du cours vers des Primary Data Assets, data tables, tag-to-value maps et asset manager.",
    ],
    sections: [
      {
        title: "Extensions et refactors personnels",
        body:
          "La partie la plus utile a été de pousser l'implémentation de cours vers une architecture data-driven : asset manager custom, ShopItem primary assets, data asset pour: items, recettes, flags et effets.",
        points: [
          "Les attributs de base sont lus depuis des data table par classe de personnage.",
          "Le comportement d'item passe par des primary data assets au lieu d'être hardcodé dans les widgets ou characters.",
        ],
      },
    ],
    proof: [],
    improvements: [],
  },
  "commonui-frontend": {
    summary:
      "Projet de cours CommonUI utilisé pour apprendre les patterns frontend appliqués ensuite à mon UI EMR : stacks, navigation manette, options, rebinding et loading.",
    mediaCaptions: [
      "Image du cours Udemy Unreal Engine 5 C++: Advanced Frontend UI Programming",
    ],
    overview:
      "J'ai terminé ce cours frontend avancé afin d'implémenter mon propre frontend CommonUI dans Emergency Room. Il m'a donné les patterns de stacks, chargement async, focus, options, rebinding et loading screens que j'ai ensuite appliqués à EMR.",
    roleScope:
      "Implémentation de cours et apprentissage appliqué à l'architecture frontend Unreal C++, réutilisé ensuite dans EMR avec CommonUI, CommonInput, UMG, settings, navigation manette et flows réutilisables.",
    courseContext:
      "J'ai suivi ce cours pour comprendre comment organiser un frontend Unreal avec CommonUI, Activatable Widgets, widget stacks, gameplay tags, async loading, CommonInput, données d'options et remapping.",
    whatBuilt: [
      "Setup CommonUI modulaire avec frontend player controller, primary layout, stacks tagués, UI subsystem, developer settings et async actions.",
      "Menu principal et modales avec Common Activatable Widgets, boutons communs, actions liées, écrans de confirmations et UI utilisable entièrement à la manette.",
      "Options avec onglets, data objects, list entries, détails dynamiques, reset et Game User Settings.",
      "Remappage des touches et détection d'input avec présentation de style CommonInput.",
    ],
    whatLearned: [
      "Structurer un frontend Unreal manette autour de CommonUI, stacks et gameplay tags.",
      "Faire coopérer menus, modales et options.",
      "Adapter les patterns de cours à EMR : menu, lobby, options, loading et gameplay UI.",
    ],
    proof: [],
    improvements: [],
  },
  "gas-crash-course": {
    summary:
      "Cours intensif utilisé pour consolider mes fondamentaux sur le Gameplay Ability System après le prototype MOBA GAS plus large.",
    mediaCaptions: [
      "Image du cours Udemy Unreal Engine 5 Gameplay Ability System (GAS) Crash Course",
    ],
    overview:
      "J'ai utilisé ce projet GAS plus petit pour isoler les fondamentaux après le prototype MOBA GAS : ASC, AttributeSets, tags, effets, abilities, réactions UI, dégâts, hit réactions et mort.",
    roleScope:
      "Implémentation de cours dans un projet UE 5.7 C++ pour consolider les fondamentaux Gameplay Ability System.",
    courseContext:
      "J'ai terminé ce cours après MOBA GAS afin de consolider: ASC, AttributeSets, Gameplay Abilities, Tags, Effects, Cues, Ability Tasks, UI, combat, IA.",
    whatBuilt: [
      "Fondation GAS avec ASC, Attribute Sets, abilities de départ, tags natifs, activation par tag et base ability.",
      "Événements de gameplay et hit réactions via Anim Notifies, overlap, collision, montages, Niagara et Gameplay Cues.",
      "Santé et mana répliqués avec initialisation, damage, reset, coûts, cooldowns, Curve Tables et widgets.",
      "Mort et respawn avec handlers natifs, async tasks, tags d'état et blocage input.",
      "Combat ennemi : recherche de cible, déplacement, attaques mélées/distances, knockback et pickups.",
    ],
    whatLearned: [
      "Comment ASC, Attribute Sets, Tags, Effects, Abilities et Cues coopèrent.",
      "Transformer les changements d'attributs répliqués en feedback UI lisible.",
    ],
    proof: [],
    improvements: [],
  },
  "multiplayer-crash-course": {
    summary:
      "Cours intensif pour consolider mes bases sur le Framework Unreal Multiplayer: réplication, listen-server, sessions, PlayerState et RPCs.",
    mediaCaptions: [
      "Image du cours Udemy Unreal Engine 5 C++ Multiplayer CRASH COURSE",
    ],
    overview:
      "J'ai utilisé ce projet pour pratiquer les fondamentaux multiplayer Unreal dans une base C++ plus petite avant de les appliquer à des projets co-op plus larges.",
    roleScope:
      "Implémentation de cours avec C++, OnlineSubsystem, SteamSockets et exemples de gameplay répliqué pour consolider le multiplayer Unreal.",
    courseContext:
      "J'ai terminé ce cours après MOBA GAS pour consolider: modèle client-serveur, LAN/Steam sessions, réplication, RepNotifies, RPCs, ownership et map travel.",
    whatBuilt: [
      "Projet C++ multijoueur autour du modèle client-serveur, LAN et sessions Steam.",
      "Flux host/join via MultiplayerSessions, OnlineSubsystem, SteamSockets, map travel et menu.",
      "Exemples d'acteurs répliqués, mouvement, rôles, variables, RepNotifies.",
      "RPC Client, Server et Multicast, priority et debug d'autorité.",
    ],
    whatLearned: [
      "Répartir les responsabilités entre GameMode, GameState, PlayerState, PlayerController, Pawn, HUD et widgets.",
      "Comprendre: session hosting, search/join, travel, RPCs, RepNotifies, ownership et authority.",
    ],
    proof: [],
    improvements: [],
  },
  "unreal-2d": {
    summary:
      "Prototypes 2D et 2.5D Unreal couvrant sprites, tilemaps, PaperZD, Enhanced Input, combat, ennemis, pickups et projectiles.",
    mediaCaptions: [
      "Image du cours Udemy The Ultimate 2D Top Down Unreal Engine Course",
    ],
    overview:
      "J'ai groupé ces exercices Unreal 2D car ils montrent ensemble mon apprentissage du workflow 2D/2.5D : sprites, tilemaps, PaperZD, Enhanced Input, Blueprint combat, ennemis, pickups, projectiles et UMG.",
    roleScope:
      "Travail d'apprentissage Blueprint sur deux projets de cours locaux : DungeonAdventure comme prototype PaperZD principal et MonsterWorld comme exercice plus petit.",
    courseContext:
      "J'ai utilisé ce cours pour apprendre les workflows 2D et 2.5D dans Unreal Engine : Paper2D, PaperZD, sprites, tile maps, animations et plus.",
    whatBuilt: [
      "Exercices Blueprint autour de Paper2D, PaperZD, sprites, tile sets, tile maps, setup pixel-perfect et caméras orthographiques.",
      "Prototype top-down avec sprites, animations, Enhanced Input, sorting, collisions, pickups, items, HUD, dialogue NPC et interfaces.",
      "Prototype action Dungeon Adventure avec animation, états, IA, dégâts, attaques, projectiles, hitboxes, knockback, UI, sons et restart.",
      "Spawner et waves avec managers, structures de données, suivi de vague et UI.",
      "RPG hybride 2.5D avec personnages sprite dans un environnement 3D, dialogue via data table.",
    ],
    whatLearned: [
      "L'ensemble de la stack 2D Unreal : sprites, tilemaps, animation, PaperZD, Blueprints, UMG et input.",
    ],
    proof: [],
    improvements: [],
  },
  "slash-cpp": {
    summary:
      "Premier projet en C++ sur Unreal après les cours C++ basiques et les cours Blueprint, ce cours sert à créer un action-RPG pour apprendre: animations, combat, IA, UI et classes réutilisables.",
    mediaCaptions: [
      "Image du cours Udemy Unreal Engine 5 C++ The Ultimate Game Developer Course",
    ],
    overview:
      "Ce projet marque mon passage de l'apprentissage Blueprint vers la programmation gameplay C++ Unreal. J'ai écrit mon projet local en suivant le cours afin de pratiquer hiérarchie C++, movement, équipement, traces melee, IA, attributs, UI, pickups et loot.",
    roleScope:
      "Implémentation de cours Unreal C++ dans mon projet local, utile comme fondation avant GAS, multiplayer, CommonUI et systèmes originaux.",
    courseContext:
      "J'ai terminé ce cours après mes fondamentaux C++ et Blueprint pour passer du visual scripting à Unreal C++.",
    whatBuilt: [
      "Prototype d'un action-RPG troisième personne fait en C++ avec: paysages, lumière, foliage, Level Instances et donjon.",
      "Exercices C++ autour d'Actor, Pawn, Character, composants, UPROPERTY/UFUNCTION debug, input et caméra.",
      "Animation et personnage: Animation Blueprint basée C++, IK, sockets, montages, MetaSounds.",
      "Combat avec: pickup/equip, enums d'état, traces mélée, interfaces, effets, hit réactions et damage.",
      "Systèmes de gameplay: breakables Chaos, Field System, Niagara, health bars, HUD, pickups, dodge et mort.",
      "IA avec: patrouille, Pawn Sensing, états, aggro, rayons, timers, Motion Warping et attaques root-motion.",
    ],
    whatLearned: [
      "Utiliser Unreal C++ avec: composants, héritage, reflection, GC, input, traces, delegates, UI, animation et IA.",
      "Bonnes pratiques de code et décisions d'architecture pour garder un projet sain.",
    ],
    proof: [],
    improvements: [],
  },
  "cpp-game-development": {
    summary:
      "Premier cours C++ terminé, avec des exercices pour apprendre: syntaxe, fonctions, références, pointeurs, OOP, mémoire dynamique, polymorphisme et headers.",
    mediaCaptions: [
      "Image du cours Udemy Learn C++ for Game Development",
    ],
    overview:
      "Ce cours a été ma première base C++ avant Unreal. Je l'utilise comme jalon d'apprentissage car j'y ai écrit le code nécessaire pour comprendre ensuite classes Unreal, headers, pointeurs, héritage et OOP.",
    roleScope:
      "Pratique de fondamentaux C++ orientée game development, avec exercices console et langage plutôt qu'un prototype Unreal.",
    courseContext:
      "J'ai terminé ce cours avant d'apprendre le C++ pour Unreal afin d'obtenir la base et les fondations de langage nécessaires.",
    whatBuilt: [
      "Exercices C++ sur: variables, types, statements, expressions, booléens, scope et fonctions.",
      "Boucles, références, overloads, strings, constantes, arrays, enums, switch et structs.",
      "OOP avec classes, constructeurs, membres, modifiers, héritage, pointeurs, stack/heap, GC, static, virtual, polymorphisme et headers.",
      "Fondation C++ pour comprendre plus tard la hiérarchie Unreal, séparation header/source et pointeurs.",
    ],
    whatLearned: [
      "Syntaxe C++, expressions, fonctions, contrôles, loops, arrays, enums, structs, strings, constantes et OOP.",
    ],
    proof: [],
    improvements: [],
  },
  "blueprint-prototypes": {
    summary:
      "Premier cours Unreal Engine, avec des prototypes Blueprint pour apprendre: éditeur, visual scripting, matériaux, Enhanced Input, collision, UI, IA, Niagara, MetaSounds, Paper2D/PaperZD et Chaos Vehicles.",
    mediaCaptions: [
      "Image du cours Udemy Unreal Engine 5 Blueprints - The Ultimate Developer Course",
    ],
    overview:
      "Ces prototypes représentent mon premier parcours Unreal structuré. J'ai écrit mes propres projets Blueprint en suivant le curriculum pour apprendre Unreal Engine avant C++.",
    roleScope:
      "Projets Unreal Blueprint-only de cours, regroupés en une page d'apprentissage plutôt que plusieurs petites entrées.",
    courseContext:
      "J'ai terminé ce cours comme premier cours sur Unreal Engine.",
    whatBuilt: [
      "Exercices éditeur : setup projet, Starter Content, layout, PIE, formats Unreal, meshes, materials, FAB et migration.",
      "Fondamentaux Blueprint : Level Blueprints, classes, composants, strings, debug, vectors, rotators et fonctions.",
      "Bad Bot, shooter drone avec références Pawn, interpolation, sockets, projectiles, Niagara, timers, pickups, score, widgets et boss.",
      "Jetpack Journey, plateforme 3D avec Character, PlayerController, WASD/controller, animations, jet fuel, UMG, plateformes et soft références.",
      "Red Hood, dungeon crawler 2D avec Paper2D/PaperZD, caméra, locomotion, Behavior Trees, combat, damage numbers et effets.",
      "Chaos Vehicle avec rigging/skinning Unreal, squelette, physics asset, input véhicule et possession.",
    ],
    whatLearned: [
      "Utiliser Blueprints pour programmer du gameplay et prototyper rapidement avant C++.",
      "Relier éditeur, assets, hiérarchie, input, UI, animation, IA, collision, physique, effets, audio et structure projet.",
      "Garder de la discipline d'architecture même en Blueprint : interfaces, dispatchers, dépendances, soft références, async loading et coûts Tick/cast.",
    ],
    proof: [],
    improvements: [],
  },
};

function localizeMediaSlots(
  slots: MediaSlot[],
  captions: string[] | undefined,
): MediaSlot[] {
  return slots.map((slot, index) => ({
    ...slot,
    caption: captions?.[index] ?? slot.caption,
  }));
}

function preserveCodeReferences(
  original: RichTextContent | undefined,
  localized: RichTextContent | undefined,
): RichTextContent | undefined {
  if (!localized || !Array.isArray(original) || Array.isArray(localized)) {
    return localized;
  }

  const references = original.filter(
    (run): run is CodeReference =>
      typeof run !== "string" && run.kind === "code-reference",
  );

  if (references.length === 0) {
    return localized;
  }

  return [
    localized,
    " ",
    ...references.flatMap((reference, index) =>
      index === references.length - 1 ? [reference] : [reference, " "],
    ),
  ];
}

function preserveCodeReferencesInList(
  original: RichTextContent[] | undefined,
  localized: RichTextContent[] | undefined,
): RichTextContent[] | undefined {
  if (!localized) {
    return localized;
  }

  return localized.map(
    (item, index) => preserveCodeReferences(original?.[index], item) ?? item,
  );
}

function localizeProjectPage(
  page: ProjectPageContent,
  text: ProjectFrenchText,
): ProjectPageContent {
  return {
    ...page,
    overview: preserveCodeReferences(page.overview, text.overview) ?? text.overview,
    roleScope:
      preserveCodeReferences(page.roleScope, text.roleScope) ?? text.roleScope,
    sections: page.sections.map((section, index) => {
      const translatedSection = text.sections?.[index];

      return {
        ...section,
        ...(translatedSection
          ? {
              title: translatedSection.title,
              body:
                preserveCodeReferences(section.body, translatedSection.body) ??
                translatedSection.body,
              points:
                preserveCodeReferencesInList(
                  section.points,
                  translatedSection.points,
                ) ??
                section.points,
            }
          : {}),
      };
    }),
    proof: preserveCodeReferencesInList(page.proof, text.proof) ?? page.proof,
    improvements:
      preserveCodeReferencesInList(page.improvements, text.improvements) ??
      page.improvements,
    courseContext:
      preserveCodeReferences(page.courseContext, text.courseContext) ??
      page.courseContext,
    whatBuilt:
      preserveCodeReferencesInList(page.whatBuilt, text.whatBuilt) ??
      page.whatBuilt,
    whatLearned:
      preserveCodeReferencesInList(page.whatLearned, text.whatLearned) ??
      page.whatLearned,
    scopeNotes:
      preserveCodeReferencesInList(page.scopeNotes, text.scopeNotes) ??
      page.scopeNotes,
  };
}

const frenchProjectRegistry = Object.fromEntries(
  projectOrder.map((slug) => {
    const project = projectRegistry[slug];
    const text = frenchProjectText[slug];

    return [
      slug,
      {
        ...project,
        route: localizedProjectRoute(slug, "fr"),
        summary: text.summary,
        mediaSlots: localizeMediaSlots(project.mediaSlots, text.mediaCaptions),
        actions: project.actions.map((action) =>
          localizeProjectAction(action, slug, "fr"),
        ),
        page: localizeProjectPage(project.page, text),
      },
    ];
  }),
) as Record<ProjectSlug, PortfolioProject>;

function createPortfolioContent(
  language: Language,
  localizedSiteProfile: SiteProfile,
  localizedProjectRegistry: Record<ProjectSlug, PortfolioProject>,
  ui: PortfolioUiLabels,
  roleVariant: RoleVariant = defaultRoleVariant,
): PortfolioContent {
  const localizedProjects = projectOrder.map((slug) => localizedProjectRegistry[slug]);
  const localizedPublicProjects = localizedProjects.filter(
    (project) => project.visibility === "public",
  );
  const localizedFeaturedOriginalProjects = localizedPublicProjects.filter(
    (project) => project.category === "original",
  );
  const localizedLearningProjects = localizedPublicProjects.filter(
    (project) => project.category === "learning",
  );

  return {
    language,
    roleVariant,
    siteProfile: localizedSiteProfile,
    projectRegistry: localizedProjectRegistry,
    projects: localizedProjects,
    publicProjects: localizedPublicProjects,
    featuredOriginalProjects: localizedFeaturedOriginalProjects,
    learningProjects: localizedLearningProjects,
    getProjectBySlug: (slug) =>
      projectOrder.includes(slug as ProjectSlug)
        ? localizedProjectRegistry[slug as ProjectSlug]
        : undefined,
    getAdjacentProjects: (slug) => {
      const navigableProjects = localizedPublicProjects.filter(
        (project) => project.includeInPreviousNext,
      );
      const currentIndex = navigableProjects.findIndex(
        (project) => project.slug === slug,
      );

      return {
        previous: currentIndex > 0 ? navigableProjects[currentIndex - 1] : undefined,
        next:
          currentIndex >= 0 && currentIndex < navigableProjects.length - 1
            ? navigableProjects[currentIndex + 1]
            : undefined,
      };
    },
    ui,
  };
}

export const englishPortfolioContent = createPortfolioContent(
  "en",
  siteProfile,
  projectRegistry,
  englishUi,
);

export const frenchPortfolioContent = createPortfolioContent(
  "fr",
  frenchSiteProfile,
  frenchProjectRegistry,
  frenchUi,
);

function applyRoleVariant(
  content: PortfolioContent,
  roleVariant: RoleVariant,
): PortfolioContent {
  const profile = roleVariantProfiles[roleVariant][content.language];
  const siteProfile = {
    ...content.siteProfile,
    role: profile.role,
    actions: applyProfileCvAction(
      content.siteProfile.actions,
      roleVariant,
      content.language,
    ),
  };

  if (roleVariant === defaultRoleVariant) {
    return {
      ...content,
      siteProfile,
    };
  }

  return {
    ...content,
    roleVariant,
    siteProfile,
  };
}

export function getPortfolioContent(
  language: Language = defaultLanguage,
  roleVariant: RoleVariant = defaultRoleVariant,
): PortfolioContent {
  const content = language === "fr" ? frenchPortfolioContent : englishPortfolioContent;

  return applyRoleVariant(content, roleVariant);
}
