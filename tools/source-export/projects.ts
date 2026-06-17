import type { SourceExportProjectConfig } from "./sourceExport";

export const sourceExportProjects: SourceExportProjectConfig[] = [
  {
    slug: "hospital-flow",
    projectPath: "K:/GameDev/Projets/EmergencyRoom",
    sourceRoots: ["Source", "Plugins"],
    entryFile: "Source/LOD/Public/Characters/Player/EMRPlayerController.h",
  },
  {
    slug: "agass",
    projectPath: "K:/GameDev/Projets/AGASS",
    sourceRoots: ["Source", "Plugins"],
    entryFile: "Source/AGASS/Public/AGASSPlayerController.h",
    ownershipOverrides: [
      {
        pathPrefix: "Plugins",
        ownership: "project-plugin",
      },
    ],
  },
  {
    slug: "tlm",
    projectPath: "K:/GameDev/Godot/tlm",
    sourceRoots: [
      "src",
      "addons/the_last_mage_tools",
      "addons/TLMRegressionTools",
    ],
    entryFile: "src/Gameplay/Run/RunControllerNode.cs",
    ownershipOverrides: [
      {
        pathPrefix: "addons/the_last_mage_tools",
        ownership: "project-plugin",
      },
      {
        pathPrefix: "addons/TLMRegressionTools",
        ownership: "project-plugin",
      },
    ],
  },
  {
    slug: "moba-gas",
    projectPath: "D:/GameDev/Anciens Projets/Apprentissage/MOBA GAS/LOD",
    sourceRoots: ["Source"],
    entryFile: "Source/LOD/Private/Player/MobaPlayerController.h",
  },
  {
    slug: "slash-cpp",
    projectPath:
      "D:/GameDev/Anciens Projets/Apprentissage/Tutoriels Stephen Ulibarri/Learn UE 5 C++ Programming/Slash",
    sourceRoots: ["Source"],
    entryFile: "Source/Slash/Public/Characters/SlashMyCharacter.h",
  },
];
