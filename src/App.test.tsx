// @vitest-environment jsdom
import "@testing-library/jest-dom/vitest";
import {
  cleanup,
  fireEvent,
  render,
  screen,
  waitFor,
  within,
} from "@testing-library/react";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import App from "./App";
import { resetSourceHighlighterForTests } from "./sourceHighlighting";

const shikiMock = vi.hoisted(() => {
  let highlighterGate: Promise<void> | undefined;
  let initializationError: Error | undefined;
  let tokenizationError: Error | undefined;
  const codeToTokens = vi.fn((code: string) => ({
    tokens: (() => {
      if (tokenizationError) {
        throw tokenizationError;
      }

      return code
        .replace(/\r\n/g, "\n")
        .replace(/\r/g, "\n")
        .split("\n")
        .filter((_, index, lines) => index < lines.length - 1 || lines[index] !== "")
        .map((line) =>
          line
            ? [
                {
                  color: "#0033b3",
                  content: line,
                  fontStyle: 0,
                },
              ]
            : [],
        );
    })(),
  }));
  const createHighlighter = vi.fn(async () => {
    await highlighterGate;
    if (initializationError) {
      throw initializationError;
    }

    return { codeToTokens };
  });
  const createBundledHighlighter = vi.fn(() => createHighlighter);
  const createJavaScriptRegexEngine = vi.fn(() => ({}));
  const delayHighlighter = () => {
    let releaseHighlighter = () => {};

    highlighterGate = new Promise<void>((resolve) => {
      releaseHighlighter = resolve;
    });

    return releaseHighlighter;
  };
  const resetHighlighterGate = () => {
    highlighterGate = undefined;
  };
  const failInitialization = () => {
    initializationError = new Error("Shiki failed to initialize.");
  };
  const failTokenization = () => {
    tokenizationError = new Error("Shiki failed to tokenize.");
  };
  const resetFailures = () => {
    initializationError = undefined;
    tokenizationError = undefined;
  };

  return {
    codeToTokens,
    createBundledHighlighter,
    createHighlighter,
    createJavaScriptRegexEngine,
    delayHighlighter,
    failInitialization,
    failTokenization,
    resetFailures,
    resetHighlighterGate,
  };
});

vi.mock("shiki/core", () => ({
  createBundledHighlighter: shikiMock.createBundledHighlighter,
}));

vi.mock("shiki/engine/javascript", () => ({
  createJavaScriptRegexEngine: shikiMock.createJavaScriptRegexEngine,
}));

const sourceManifestFixture = {
  exportedAt: "2026-06-01T09:38:14.699Z",
  entryFile: "Source/AGASS/Public/AGASSPlayerController.h",
  metrics: {
    fileCount: 3,
    totalLines: 139,
    ownershipCounts: {
      "project-code": 1,
      "project-plugin": 1,
      "third-party-plugin": 1,
    },
  },
  files: [
    {
      path: "Source/AGASS/Public/AGASSPlayerController.h",
      snapshotPath: "files/Source/AGASS/Public/AGASSPlayerController.h",
      language: "cpp-header",
      ownership: "project-code",
      lines: 123,
    },
    {
      path: "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp",
      snapshotPath:
        "files/Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp",
      language: "cpp",
      ownership: "project-plugin",
      lines: 4,
    },
    {
      path: "Plugins/GameSettings/Source/Public/GameSetting.h",
      snapshotPath: "files/Plugins/GameSettings/Source/Public/GameSetting.h",
      language: "cpp-header",
      ownership: "third-party-plugin",
      lines: 12,
    },
  ],
};

const hospitalSourceManifestFixture = {
  exportedAt: "2026-06-01T09:38:14.699Z",
  entryFile: "Source/LOD/Private/Interaction/EMRReceptionMachine.cpp",
  metrics: {
    fileCount: 1,
    totalLines: 3,
    ownershipCounts: {
      "project-code": 1,
      "project-plugin": 0,
      "third-party-plugin": 0,
    },
  },
  files: [
    {
      path: "Source/LOD/Private/Interaction/EMRReceptionMachine.cpp",
      snapshotPath:
        "files/Source/LOD/Private/Interaction/EMRReceptionMachine.cpp",
      language: "cpp",
      ownership: "project-code",
      lines: 3,
    },
  ],
};

const tlmSourceManifestFixture = {
  exportedAt: "2026-06-05T17:00:00.000Z",
  entryFile: "src/Gameplay/Run/RunControllerNode.cs",
  metrics: {
    fileCount: 2,
    totalLines: 8,
    ownershipCounts: {
      "project-code": 1,
      "project-plugin": 1,
      "third-party-plugin": 0,
    },
  },
  files: [
    {
      path: "src/Gameplay/Run/RunControllerNode.cs",
      snapshotPath: "files/src/Gameplay/Run/RunControllerNode.cs",
      language: "csharp",
      ownership: "project-code",
      lines: 4,
    },
    {
      path: "addons/the_last_mage_tools/TheLastMageToolsPlugin.cs",
      snapshotPath:
        "files/addons/the_last_mage_tools/TheLastMageToolsPlugin.cs",
      language: "csharp",
      ownership: "project-plugin",
      lines: 4,
    },
  ],
};

const mobaSourceManifestFixture = {
  exportedAt: "2026-06-06T09:00:00.000Z",
  entryFile: "Source/LOD/Private/Player/MobaPlayerController.h",
  metrics: {
    fileCount: 1,
    totalLines: 4,
    ownershipCounts: {
      "project-code": 1,
      "project-plugin": 0,
      "third-party-plugin": 0,
    },
  },
  files: [
    {
      path: "Source/LOD/Private/Player/MobaPlayerController.h",
      snapshotPath: "files/Source/LOD/Private/Player/MobaPlayerController.h",
      language: "cpp-header",
      ownership: "project-code",
      lines: 4,
    },
  ],
};

const slashSourceManifestFixture = {
  exportedAt: "2026-06-06T09:30:00.000Z",
  entryFile: "Source/Slash/Public/Characters/SlashMyCharacter.h",
  metrics: {
    fileCount: 1,
    totalLines: 4,
    ownershipCounts: {
      "project-code": 1,
      "project-plugin": 0,
      "third-party-plugin": 0,
    },
  },
  files: [
    {
      path: "Source/Slash/Public/Characters/SlashMyCharacter.h",
      snapshotPath: "files/Source/Slash/Public/Characters/SlashMyCharacter.h",
      language: "cpp-header",
      ownership: "project-code",
      lines: 4,
    },
  ],
};

type SourceManifestFixture = Omit<typeof sourceManifestFixture, "entryFile"> & {
  entryFile?: string;
};

function stubSourceManifestFetch({
  missingFiles = [],
  manifest = sourceManifestFixture,
  manifests = {},
}: {
  missingFiles?: string[];
  manifest?: SourceManifestFixture;
  manifests?: Record<string, SourceManifestFixture>;
} = {}) {
  const manifestBySlug: Record<string, SourceManifestFixture | undefined> = {
    agass: manifest,
    ...manifests,
  };
  const fileContents: Record<string, string> = {
    "/source/agass/files/Source/AGASS/Public/AGASSPlayerController.h":
      "#pragma once\n\nclass AAGASSPlayerController;\n",
    "/source/agass/files/Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp":
      '#include "AGASSCommonModule.h"\n\nvoid FAGASSCommonModule::StartupModule() {}\n',
    "/source/agass/files/Plugins/GameSettings/Source/Public/GameSetting.h":
      "#pragma once\n\nclass UGameSetting;\n",
    "/source/hospital-flow/files/Source/LOD/Private/Interaction/EMRReceptionMachine.cpp":
      '#include "EMRReceptionMachine.h"\n\nvoid AEMRReceptionMachine::BeginPlay() {}\n',
    "/source/tlm/files/src/Gameplay/Run/RunControllerNode.cs":
      "namespace TheLastMage.Gameplay.Run;\n\npublic partial class RunControllerNode\n{\n}\n",
    "/source/tlm/files/addons/the_last_mage_tools/TheLastMageToolsPlugin.cs":
      "namespace TheLastMage.EditorTools;\n\npublic partial class TheLastMageToolsPlugin\n{\n}\n",
    "/source/moba-gas/files/Source/LOD/Private/Player/MobaPlayerController.h":
      "#pragma once\n\nclass AMobaPlayerController;\n",
    "/source/slash-cpp/files/Source/Slash/Public/Characters/SlashMyCharacter.h":
      "#pragma once\n\nclass ASlashMyCharacter;\n",
  };
  const fetchMock = vi.fn(async (input: RequestInfo | URL) => {
    const url = input.toString();

    for (const [slug, sourceManifest] of Object.entries(manifestBySlug)) {
      if (sourceManifest && url.endsWith(`/source/${slug}/manifest.json`)) {
        return new Response(JSON.stringify(sourceManifest), {
          headers: { "Content-Type": "application/json" },
          status: 200,
        });
      }
    }

    if (missingFiles.some((fileUrl) => url.endsWith(fileUrl))) {
      return new Response("Not found", { status: 404 });
    }

    if (fileContents[url]) {
      return new Response(fileContents[url], {
        headers: { "Content-Type": "text/plain" },
        status: 200,
      });
    }

    return new Response("Not found", { status: 404 });
  });
  vi.stubGlobal("fetch", fetchMock);

  return fetchMock;
}

function navigateToHash(hash: string) {
  window.location.hash = hash;
  window.dispatchEvent(new Event("hashchange"));
}

function expandSourceFolders(tree: HTMLElement, paths: string[]) {
  for (const path of paths) {
    const folder = within(tree).getByRole("button", { name: `Toggle ${path}` });

    if (folder.getAttribute("aria-expanded") !== "true") {
      fireEvent.click(folder);
    }
  }
}

function getSourceLineTextElement(text: string) {
  return Array.from(document.querySelectorAll<HTMLElement>(".source-line-text")).find(
    (line) => line.textContent === text,
  );
}

async function findSourceLineTextElement(text: string) {
  let lineElement: HTMLElement | undefined;

  await waitFor(() => {
    lineElement = getSourceLineTextElement(text);
    expect(lineElement).toBeInTheDocument();
  });

  return lineElement!;
}

function recordSelectedTreeScrolls() {
  const calls: { label: string; options?: ScrollIntoViewOptions }[] = [];

  Object.defineProperty(window.HTMLElement.prototype, "scrollIntoView", {
    configurable: true,
    value: function scrollIntoView(this: HTMLElement, options?: ScrollIntoViewOptions) {
      if (this.matches(".source-tree-file a.selected")) {
        calls.push({
          label: this.getAttribute("aria-label") ?? "",
          options,
        });
      }
    },
  });

  return calls;
}

function recordSelectedCodeLineScrolls() {
  const calls: { lineNumber: string | null; options?: ScrollIntoViewOptions }[] = [];

  Object.defineProperty(window.HTMLElement.prototype, "scrollIntoView", {
    configurable: true,
    value: function scrollIntoView(this: HTMLElement, options?: ScrollIntoViewOptions) {
      if (this.matches(".source-code-line.selected")) {
        calls.push({
          lineNumber: this.getAttribute("data-line-number"),
          options,
        });
      }
    },
  });

  return calls;
}

describe("App routing", () => {
  beforeEach(() => {
    resetSourceHighlighterForTests();
    shikiMock.resetFailures();
    shikiMock.resetHighlighterGate();
    shikiMock.codeToTokens.mockClear();
    shikiMock.createBundledHighlighter.mockClear();
    shikiMock.createHighlighter.mockClear();
    shikiMock.createJavaScriptRegexEngine.mockClear();
    document.body.style.overflow = "";
    window.location.hash = "#/";
    window.scrollTo = vi.fn();
    Object.defineProperty(window.HTMLElement.prototype, "scrollIntoView", {
      configurable: true,
      value: vi.fn(),
    });
  });

  afterEach(() => {
    cleanup();
    document.body.style.overflow = "";
    vi.restoreAllMocks();
    vi.unstubAllGlobals();
  });

  it("renders homepage groups from the registry", () => {
    const { container } = render(<App />);

    const homeLanguageToggle = container.querySelector<HTMLElement>(
      ".home-language-toggle",
    );
    const profileFacts = within(screen.getByLabelText("Profile facts"));

    expect(container.querySelector(".site-header")).not.toBeInTheDocument();
    expect(homeLanguageToggle).toBeInTheDocument();
    expect(
      within(homeLanguageToggle as HTMLElement).getByLabelText("Portfolio language"),
    ).toBeInTheDocument();
    expect(
      within(homeLanguageToggle as HTMLElement).queryByText("Baptiste Giquet"),
    ).not.toBeInTheDocument();
    expect(
      within(homeLanguageToggle as HTMLElement).queryByText(
        "Junior Unreal Engine Programmer",
      ),
    ).not.toBeInTheDocument();
    expect(
      within(screen.getByLabelText("Profile actions")).queryByLabelText(
        "Portfolio language",
      ),
    ).not.toBeInTheDocument();
    expect(container.querySelector(".hero-profile-name")).toHaveTextContent(
      "Baptiste Giquet",
    );
    expect(profileFacts.getByText("Contact")).toBeInTheDocument();
    expect(
      profileFacts.getByRole("link", { name: "baptiste-giquet@hotmail.fr" }),
    ).toHaveAttribute("href", "mailto:baptiste-giquet@hotmail.fr");
    expect(profileFacts.getByRole("link", { name: "06.85.80.43.44" })).toHaveAttribute(
      "href",
      "tel:0685804344",
    );
    expect(screen.queryByText(/Networked gameplay systems, GAS interactions/i)).not.toBeInTheDocument();
    expect(screen.getByRole("heading", { name: /personal projects/i })).toBeInTheDocument();
    expect(screen.queryByText(/These are the projects that best show my solo systems work in Unreal/i)).not.toBeInTheDocument();
    expect(screen.queryByText("Original systems work")).not.toBeInTheDocument();
    const hospitalCard = screen
      .getByRole("heading", { name: /emergency room simulator/i })
      .closest(".project-row") as HTMLElement;

    expect(hospitalCard).toBeInTheDocument();
    expect(within(hospitalCard).getByText(/Online Co-op Emergency Room simulator/i)).toBeInTheDocument();
    expect(within(hospitalCard).getByText("AI behavior")).toBeInTheDocument();
    expect(within(hospitalCard).queryByText("Blueprints")).not.toBeInTheDocument();
    expect(screen.queryByText(/learning project/i)).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: /a game about stacking sh\*t/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: /the last mage/i })).not.toBeInTheDocument();
    expect(screen.getByRole("heading", { name: /completed courses projects/i })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: /multiplayer moba prototype/i })).toBeInTheDocument();
  });

  it("renders the QA Tester role variant without changing the project list", () => {
    window.location.hash = "#/roles/qa-tester";

    render(<App />);

    expect(document.title).toBe("Baptiste Giquet - Junior QA Tester");
    expect(screen.getByRole("heading", { name: "Junior QA Tester" })).toBeInTheDocument();
    expect(
      screen.queryByRole("heading", { name: "Junior Unreal Engine Programmer" }),
    ).not.toBeInTheDocument();
    expect(screen.getByRole("heading", { name: /personal projects/i })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: /emergency room simulator/i })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: /multiplayer moba prototype/i })).toBeInTheDocument();
    expect(screen.getByRole("link", { name: /download cv/i })).toHaveAttribute(
      "href",
      "cv/baptiste-giquet-cv-qa-tester.pdf",
    );
    expect(screen.getAllByRole("link", { name: "View project" })[0]).toHaveAttribute(
      "href",
      "#/roles/qa-tester/projects/hospital-flow",
    );
    expect(
      within(screen.getByLabelText("Portfolio language")).getByRole("button", {
        name: "FR",
      }),
    ).toHaveAttribute("href", "#/roles/qa-tester?lang=fr");
  });

  it("renders French home routes with translated UI and document language", () => {
    window.location.hash = "#/?lang=fr";

    render(<App />);

    expect(document.documentElement.lang).toBe("fr");
    expect(document.title).toBe(
      "Baptiste Giquet - Junior Unreal Engine Programmer",
    );
    expect(
      screen.getByRole("heading", {
        name: "Junior Unreal Engine Programmer",
      }),
    ).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Projets personnels" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Projets de cours terminés" })).toBeInTheDocument();
    expect(screen.getAllByRole("link", { name: "Voir le projet" })).toHaveLength(9);
    expect(
      screen.getByText(/Simulateur d'urgences en coopération en ligne/i),
    ).toBeInTheDocument();
    expect(screen.getByRole("link", { name: /télécharger le cv/i })).toHaveAttribute(
      "href",
      "cv/baptiste-giquet-cv-unreal-programmer.pdf",
    );
    expect(
      within(screen.getByLabelText("Langue du portfolio")).getByRole("button", {
        name: "FR",
      }),
    ).toHaveAttribute("aria-pressed", "true");
  });

  it("renders French project routes and preserves French state in project links", () => {
    window.location.hash = "#/projects/hospital-flow?lang=fr";

    render(<App />);

    expect(document.documentElement.lang).toBe("fr");
    expect(document.title).toBe("Emergency Room simulator - Baptiste Giquet");
    expect(
      screen.getByRole("heading", { name: "Boucle Hub vers Gardes" }),
    ).toBeInTheDocument();
    expect(screen.getByText(/J'ai construit un simulateur d'urgences co-op/i)).toBeInTheDocument();
    expect(screen.getByRole("link", { name: /voir le code source/i })).toHaveAttribute(
      "href",
      "#/projects/hospital-flow/source?lang=fr",
    );
    expect(screen.getByRole("link", { name: "AEMRReceptionMachine" })).toHaveAttribute(
      "href",
      "#/projects/hospital-flow/source?lang=fr&file=Source%2FLOD%2FPrivate%2FInteraction%2FEMRReceptionMachine.cpp&line=1",
    );

    const languageToggle = screen.getByLabelText("Langue du portfolio");

    expect(within(languageToggle).getByRole("button", { name: "EN" })).toHaveAttribute(
      "href",
      "#/projects/hospital-flow",
    );
    expect(within(languageToggle).getByRole("button", { name: "FR" })).toHaveAttribute(
      "href",
      "#/projects/hospital-flow?lang=fr",
    );
  });

  it("does not load the source highlighter on the main portfolio route", () => {
    render(<App />);

    expect(screen.getByRole("heading", { name: /personal projects/i })).toBeInTheDocument();
    expect(shikiMock.createBundledHighlighter).not.toHaveBeenCalled();
  });

  it("keeps homepage project actions scoped to view-project links", () => {
    render(<App />);

    expect(screen.getAllByRole("link", { name: /view project/i })).toHaveLength(9);
    expect(screen.queryByRole("link", { name: /source/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: /build/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: /course/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: "GitHub" })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: "Email" })).not.toBeInTheDocument();
    expect(screen.getByRole("link", { name: /download cv/i })).toHaveAttribute(
      "href",
      "cv/baptiste-giquet-cv-unreal-programmer.pdf",
    );
    expect(screen.getByRole("link", { name: /download cv/i })).toHaveAttribute(
      "download",
      "baptiste-giquet-cv-unreal-programmer.pdf",
    );
  });

  it("orders original and learning projects for recruiter scanning", () => {
    render(<App />);

    const hospital = screen.getByRole("heading", {
      name: /emergency room simulator/i,
    });
    const moba = screen.getByRole("heading", {
      name: /multiplayer moba prototype/i,
    });
    const commonui = screen.getByRole("heading", {
      name: /commonui frontend course/i,
    });
    const gasCrashCourse = screen.getByRole("heading", {
      name: /gameplay ability system crash course/i,
    });
    const multiplayerCrashCourse = screen.getByRole("heading", {
      name: /ue5 multiplayer crash course/i,
    });
    const unreal2d = screen.getByRole("heading", {
      name: /unreal 2d prototypes/i,
    });
    const slashCpp = screen.getByRole("heading", {
      name: /unreal c\+\+ course/i,
    });

    expect(screen.queryByRole("heading", { name: /a game about stacking sh\*t/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: /the last mage/i })).not.toBeInTheDocument();
    expect(Boolean(hospital.compareDocumentPosition(moba) & Node.DOCUMENT_POSITION_FOLLOWING)).toBe(true);
    expect(Boolean(moba.compareDocumentPosition(commonui) & Node.DOCUMENT_POSITION_FOLLOWING)).toBe(true);
    expect(Boolean(commonui.compareDocumentPosition(gasCrashCourse) & Node.DOCUMENT_POSITION_FOLLOWING)).toBe(true);
    expect(
      Boolean(
        gasCrashCourse.compareDocumentPosition(multiplayerCrashCourse) &
          Node.DOCUMENT_POSITION_FOLLOWING,
      ),
    ).toBe(true);
    expect(
      Boolean(
        multiplayerCrashCourse.compareDocumentPosition(unreal2d) &
          Node.DOCUMENT_POSITION_FOLLOWING,
      ),
    ).toBe(true);
    expect(Boolean(unreal2d.compareDocumentPosition(slashCpp) & Node.DOCUMENT_POSITION_FOLLOWING)).toBe(true);
  });

  it("renders a project route and updates document title", () => {
    window.location.hash = "#/projects/agass";

    const { container } = render(<App />);
    const header = container.querySelector<HTMLElement>(".site-header");

    expect(screen.getByRole("heading", { name: "A Game About Stacking Sh*t" })).toBeInTheDocument();
    expect(header).toBeInTheDocument();
    expect(within(header as HTMLElement).getByText("Baptiste Giquet")).toBeInTheDocument();
    expect(
      within(header as HTMLElement).getByText("Junior Unreal Engine Programmer"),
    ).toBeInTheDocument();
    expect(header?.querySelector(".site-nav")).not.toBeInTheDocument();
    expect(screen.queryByText("Original systems work")).not.toBeInTheDocument();
    expect(document.title).toBe("A Game About Stacking Sh*t - Baptiste Giquet");
    expect(window.scrollTo).toHaveBeenCalledWith({ top: 0, left: 0 });
  });

  it("preserves the QA Tester role variant on project pages", () => {
    window.location.hash = "#/roles/qa-tester/projects/hospital-flow";

    const { container } = render(<App />);
    const header = container.querySelector<HTMLElement>(".site-header");

    expect(screen.getByRole("heading", { name: "Emergency Room simulator" })).toBeInTheDocument();
    expect(header).toBeInTheDocument();
    expect(within(header as HTMLElement).getByText("Junior QA Tester")).toBeInTheDocument();
    expect(screen.getByRole("link", { name: "Back to projects" })).toHaveAttribute(
      "href",
      "#/roles/qa-tester",
    );
    expect(screen.getByRole("link", { name: /browse source/i })).toHaveAttribute(
      "href",
      "#/roles/qa-tester/projects/hospital-flow/source",
    );
  });

  it("renders the hospital detail page with its required case-study sections", () => {
    window.location.hash = "#/projects/hospital-flow";

    render(<App />);

    expect(screen.getByRole("heading", { name: "Emergency Room simulator" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Role / Scope" })).not.toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Hub-to-Night Shift run loop" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Networked co-op and sessions" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Patient flow and AI behavior" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Exam and treatment pipeline" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "CommonUI and gameplay UI architecture" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Telemetry and automated tests" })).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Loading and map travel" })).not.toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Localization and tag presentation" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Technical proof" })).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "What I would improve next" })).not.toBeInTheDocument();
    expect(screen.getByLabelText("Gameplay-tag localization for medical UI presentation")).toHaveAttribute(
      "class",
      "media-asset",
    );
    expect(
      within(screen.getByLabelText("Exam and treatment pipeline source references")).getByRole(
        "link",
        { name: "AEMRReceptionMachine" },
      ),
    ).toHaveAttribute(
      "href",
      "#/projects/hospital-flow/source?file=Source%2FLOD%2FPrivate%2FInteraction%2FEMRReceptionMachine.cpp&line=1",
    );
    expect(screen.queryByRole("navigation", { name: /previous and next projects/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: "Contact" })).not.toBeInTheDocument();
  });

  it("renders the AGASS detail page with scoped original systems sections", () => {
    window.location.hash = "#/projects/agass";

    render(<App />);

    expect(screen.getByRole("heading", { name: "A Game About Stacking Sh*t" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Role / Scope" })).not.toBeInTheDocument();
    expect(screen.getByText(/Players stack falling junk and buy utility items/i)).toBeInTheDocument();

    for (const heading of [
      "Modular Unreal architecture",
      "Multiplayer session flow",
      "Session compatibility contract",
      "Interaction, carry, preview, and placement",
      "Content-driven item framework",
      "Shared economy, shop, and sell flow",
      "Timed run, objective, and end-of-run loop",
      "Session event framework",
      "Modding and map-selection pipeline",
      "Modding Kit authoring workflow",
      "CommonUI frontend and settings UX",
      "Steam platform layer",
    ]) {
      expect(screen.getByRole("heading", { name: heading })).toBeInTheDocument();
    }

    expect(
      screen.queryByRole("heading", { name: "Production tooling and content pipeline" }),
    ).not.toBeInTheDocument();
    expect(screen.getAllByRole("link", { name: "UAGASSItemDefinition" })[0]).toHaveAttribute(
      "href",
      "#/projects/agass/source?file=Plugins%2FAGASS%2FAGASSTower%2FSource%2FAGASSTower%2FPublic%2FData%2FAGASSItemDefinition.h&line=1",
    );
    expect(screen.getAllByRole("link", { name: "UAGASSModManifest" }).length).toBeGreaterThan(0);
    expect(screen.getByText(/creator-facing side of the mod pipeline/i)).toBeInTheDocument();
    expect(screen.getByText(/content-plugin maps and session events rather than expanding into arbitrary native-code mods/i)).toBeInTheDocument();
    expect(
      screen.getByRole("img", {
        name: "video placeholder: Modding Kit manifest authoring, validation, export, and install packaging",
      }),
    ).toBeInTheDocument();
    expect(screen.queryByRole("link", { name: /previous:/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: /next:/i })).not.toBeInTheDocument();
  });

  it("renders the TLM detail page with production-evidence sections", () => {
    window.location.hash = "#/projects/tlm";

    render(<App />);

    expect(screen.getByRole("heading", { name: "The Last Mage" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Role / Scope" })).not.toBeInTheDocument();
    expect(screen.getAllByText(/Godot 4\.7 C# FPS horde-survivor roguelite/i).length).toBeGreaterThan(0);
    expect(screen.getAllByText(/Godot 4\.7/i).length).toBeGreaterThan(0);
    expect(screen.getAllByText(/C#/i).length).toBeGreaterThan(0);
    for (const heading of [
      "Production gameplay architecture",
      "Combat, spells, items, and build synergies",
      "Horde enemies, tower routing, and defenses",
      "Day/night run loop and market progression",
      "Content authoring and validation tools",
      "Debugging, profiling, regression, and Balance Lab",
      "Production shell, save slots, options, controller, and localization",
    ]) {
      expect(screen.getByRole("heading", { name: heading })).toBeInTheDocument();
    }
    expect(screen.getByText(/without offering a public build or claiming a Steam release/i)).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Course source" })).not.toBeInTheDocument();
    expect(screen.getByRole("link", { name: /browse source/i })).toHaveAttribute(
      "href",
      "#/projects/tlm/source",
    );
    expect(screen.queryByRole("button", { name: /source pending/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("button", { name: /build pending/i })).not.toBeInTheDocument();
    expect(
      screen.getByRole("img", {
        name: "video placeholder: Gameplay loop capture pending: mage selection, run start, day combat, night market, and run summary",
      }),
    ).toBeInTheDocument();
    expect(
      screen.getByRole("img", {
        name: "video placeholder: Spell, item, proc, and build-synergy capture pending",
      }),
    ).toBeInTheDocument();
    expect(
      within(screen.getByLabelText("Production gameplay architecture source references")).getByRole(
        "link",
        { name: "RunControllerNode" },
      ),
    ).toHaveAttribute(
      "href",
      "#/projects/tlm/source?file=src%2FGameplay%2FRun%2FRunControllerNode.cs&line=1",
    );
    expect(
      within(screen.getByLabelText("Content authoring and validation tools source references")).getByRole(
        "link",
        { name: "ItemAuthoringDock" },
      ),
    ).toHaveAttribute(
      "href",
      "#/projects/tlm/source?file=addons%2Fthe_last_mage_tools%2Fdocks%2FItemAuthoringDock.cs&line=1",
    );
    expect(screen.queryByRole("link", { name: /previous:/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: /next:/i })).not.toBeInTheDocument();
  });

  it("renders the MOBA GAS learning page with attribution and local implementation scope", () => {
    window.location.hash = "#/projects/moba-gas";

    render(<App />);

    expect(screen.getByRole("heading", { name: "Multiplayer MOBA Prototype" })).toBeInTheDocument();
    expect(screen.getByText(/Complete prototype for a competitive MOBA game/i)).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Course source" })).not.toBeInTheDocument();
    expect(screen.queryAllByText(/Jingtian Li.*Udemy/i).length).toBeGreaterThan(0);
    expect(screen.getByText(/most of the baseline implementation/i)).toBeInTheDocument();
    expect(screen.getByText(/code I wrote while following that curriculum/i)).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Course context" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "What I built" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "What I learned" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Custom extensions and refactors" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Attribution and scope" })).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Technical proof" })).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Project scope" })).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "What I would improve next" })).not.toBeInTheDocument();
    expect(screen.getByText(/custom Ability System Component/i)).toBeInTheDocument();
    expect(screen.getByText(/moving course-style implementation toward data-driven architecture/i)).toBeInTheDocument();
    expect(screen.queryByText(/not presenting my local version as a shipped backend/i)).not.toBeInTheDocument();
    expect(screen.queryByText(/Paragon and other imported assets/i)).not.toBeInTheDocument();
    expect(screen.getByRole("link", { name: "Course" })).toHaveAttribute(
      "href",
      "https://www.udemy.com/course/multiplayer-in-unreal-with-gas-and-aws-dedicated-servers/",
    );
    expect(screen.getByRole("link", { name: "Course" })).toHaveAttribute("rel", "noreferrer noopener");
    expect(screen.getByRole("link", { name: /download unreal project/i })).toHaveAttribute(
      "href",
      expect.stringMatching(/^https:\/\/1drv\.ms\//),
    );
    expect(screen.getByRole("link", { name: /browse source/i })).toHaveAttribute(
      "href",
      "#/projects/moba-gas/source",
    );
    expect(screen.queryByRole("navigation", { name: /previous and next projects/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: "Contact" })).not.toBeInTheDocument();

    expect(
      screen.getByRole("img", {
        name: "Udemy course image for Multiplayer in Unreal with GAS and AWS Dedicated Servers",
      }),
    ).toHaveAttribute("src", "media/courses/moba-gas.jpg");
  });

  it("uses the approved learning-project section order for MOBA GAS", () => {
    window.location.hash = "#/projects/moba-gas";

    render(<App />);

    const orderedHeadings = [
      screen.getByRole("heading", { name: "Course context" }),
      screen.getByRole("heading", { name: "What I built" }),
      screen.getByRole("heading", { name: "What I learned" }),
      screen.getByRole("heading", { name: "Custom extensions and refactors" }),
    ];

    for (let index = 0; index < orderedHeadings.length - 1; index += 1) {
      expect(
        Boolean(
          orderedHeadings[index].compareDocumentPosition(orderedHeadings[index + 1]) &
            Node.DOCUMENT_POSITION_FOLLOWING,
        ),
      ).toBe(true);
    }
  });

  it.each([
    {
      course: "Unreal Engine 5 C++: Advanced Frontend UI Programming",
      route: "#/projects/commonui-frontend",
      title: "CommonUI Frontend Course",
      uniqueText: /own EMR frontend/i,
    },
    {
      course: "The Ultimate 2D Top Down Unreal Engine Course",
      route: "#/projects/unreal-2d",
      title: "Unreal 2D Prototypes",
      uniqueText: /learn 2D and 2\.D workflows/i,
    },
    {
      course: "Unreal Engine 5 Gameplay Ability System (GAS) Crash Course",
      route: "#/projects/gas-crash-course",
      title: "Gameplay Ability System Crash Course",
      uniqueText: /after the MOBA GAS course to consolidate/i,
    },
    {
      course: "Unreal Engine 5 C++ Multiplayer CRASH COURSE",
      route: "#/projects/multiplayer-crash-course",
      title: "UE5 Multiplayer Crash Course",
      uniqueText: /after the MOBA GAS course to consolidate my Unreal multiplayer fundamentals/i,
    },
    {
      course: "Unreal Engine 5 C++ The Ultimate Game Developer Course",
      route: "#/projects/slash-cpp",
      title: "Unreal C++ Course",
      uniqueText: /own local course project files/i,
    },
    {
      course: "Learn C++ for Game Development",
      route: "#/projects/cpp-game-development",
      title: "Learn C++ for Game Development",
      uniqueText: /before the Unreal C\+\+ course to learn C\+\+ from the ground up/i,
    },
    {
      course: "Unreal Engine 5 Blueprints - The Ultimate Developer Course",
      route: "#/projects/blueprint-prototypes",
      title: "Unreal Blueprint Course",
      uniqueText: /first Unreal Engine course/i,
    },
  ])(
    "renders $title as a complete attributed learning page",
    ({ course, route, title, uniqueText }) => {
      window.location.hash = route;

      render(<App />);

      expect(screen.getByRole("heading", { name: title })).toBeInTheDocument();
      expect(
        screen.getByRole("img", { name: `Udemy course image for ${course}` }),
      ).toBeInTheDocument();
      expect(screen.queryByRole("heading", { name: "Course source" })).not.toBeInTheDocument();
      expect(screen.getByRole("heading", { name: "Course context" })).toBeInTheDocument();
      expect(screen.getByRole("heading", { name: "What I built" })).toBeInTheDocument();
      expect(screen.getByRole("heading", { name: "What I learned" })).toBeInTheDocument();
      if (
        route === "#/projects/commonui-frontend" ||
        route === "#/projects/unreal-2d" ||
        route === "#/projects/gas-crash-course" ||
        route === "#/projects/multiplayer-crash-course" ||
        route === "#/projects/slash-cpp" ||
        route === "#/projects/cpp-game-development" ||
        route === "#/projects/blueprint-prototypes"
      ) {
        expect(screen.queryByRole("heading", { name: "Connection to EMR" })).not.toBeInTheDocument();
        expect(screen.queryByRole("heading", { name: "Project scope" })).not.toBeInTheDocument();
        expect(screen.queryByRole("heading", { name: "Attribution and scope" })).not.toBeInTheDocument();
        expect(screen.queryByRole("heading", { name: "Technical proof" })).not.toBeInTheDocument();
        expect(screen.queryByRole("heading", { name: "What I would improve next" })).not.toBeInTheDocument();
      } else {
        expect(screen.getByRole("heading", { name: "Attribution and scope" })).toBeInTheDocument();
        expect(screen.getByRole("heading", { name: "Technical proof" })).toBeInTheDocument();
        expect(screen.getByRole("heading", { name: "What I would improve next" })).toBeInTheDocument();
        expect(screen.getByRole("heading", { name: "Project scope" })).toBeInTheDocument();
      }
      expect(screen.queryAllByText(uniqueText).length).toBeGreaterThan(0);
      if (route === "#/projects/commonui-frontend") {
        expect(screen.getByText(/frontend patterns I later applied to my own EMR UI/i)).toBeInTheDocument();
        expect(screen.getByText(/used that knowledge to build the CommonUI frontend architecture/i)).toBeInTheDocument();
      }
      if (route === "#/projects/unreal-2d") {
        expect(screen.getByText(/used this course to learn 2D and 2\.D workflows/i)).toBeInTheDocument();
        expect(screen.getByText(/Data Table driven dialogue/i)).toBeInTheDocument();
        expect(screen.getByText(/Spawner and wave-system with spawn managers/i)).toBeInTheDocument();
      }
      if (route === "#/projects/gas-crash-course") {
        expect(screen.getByText(/completed this course after the MOBA GAS course/i)).toBeInTheDocument();
        expect(screen.getByText(/custom Anim Notifies and Notify States/i)).toBeInTheDocument();
        expect(screen.getByText(/cost, and cooldown Gameplay Effects/i)).toBeInTheDocument();
      }
      if (route === "#/projects/multiplayer-crash-course") {
        expect(screen.getByText(/completed this course after the MOBA GAS course/i)).toBeInTheDocument();
        expect(screen.getByText(/Client RPCs, Server RPCs, Multicast RPCs/i)).toBeInTheDocument();
        expect(screen.getByText(/divides responsibilities across GameMode, GameState/i)).toBeInTheDocument();
      }
      if (route === "#/projects/slash-cpp") {
        expect(screen.getByText(/completed this course after my C\+\+ fundamentals and Blueprint courses/i)).toBeInTheDocument();
        expect(screen.queryAllByText(/Motion Warping/i).length).toBeGreaterThan(0);
        expect(screen.queryAllByText(/root-motion attacks/i).length).toBeGreaterThan(0);
      }
      if (route === "#/projects/cpp-game-development") {
        expect(screen.getByText(/language foundation I needed before writing later Unreal C\+\+ projects/i)).toBeInTheDocument();
        expect(screen.getByText(/stack\/heap allocation/i)).toBeInTheDocument();
        expect(screen.getByText(/multiple inheritance/i)).toBeInTheDocument();
      }
      if (route === "#/projects/blueprint-prototypes") {
        expect(screen.getByText(/my own Blueprint implementations/i)).toBeInTheDocument();
        expect(screen.getByText(/Bad Bot, a drone flying shooter prototype/i)).toBeInTheDocument();
        expect(screen.getByText(/Jetpack Journey, a 3D platformer prototype/i)).toBeInTheDocument();
        expect(screen.getByText(/Red Hood, a 2D dungeon crawler prototype/i)).toBeInTheDocument();
        expect(screen.getByText(/Chaos Vehicle work using Unreal rigging\/skinning tools/i)).toBeInTheDocument();
      }
      if (route === "#/projects/slash-cpp") {
        expect(screen.getByRole("link", { name: /download unreal project/i })).toHaveAttribute(
          "href",
          expect.stringMatching(/^https:\/\/1drv\.ms\//),
        );
        expect(screen.getByRole("link", { name: /browse source/i })).toHaveAttribute(
          "href",
          "#/projects/slash-cpp/source",
        );
      } else {
        expect(screen.queryByRole("button", { name: /source pending/i })).not.toBeInTheDocument();
        expect(screen.queryByRole("link", { name: /download unreal project/i })).not.toBeInTheDocument();
      }
      expect(screen.getByRole("link", { name: "Course" })).toHaveAttribute("rel", "noreferrer noopener");
      expect(screen.queryByRole("navigation", { name: /previous and next projects/i })).not.toBeInTheDocument();
    },
  );

  it("renders Unreal download as the first action for the direct project downloads", () => {
    for (const [route, sourceHref] of [
      ["#/projects/hospital-flow", "#/projects/hospital-flow/source"],
      ["#/projects/moba-gas", "#/projects/moba-gas/source"],
      ["#/projects/slash-cpp", "#/projects/slash-cpp/source"],
    ] as const) {
      cleanup();
      window.location.hash = route;
      render(<App />);

      const actions = within(screen.getByLabelText("Project actions")).getAllByRole(
        "link",
      );

      expect(actions[0]).toHaveTextContent("Download Unreal project");
      expect(actions[0]).toHaveAttribute("href", expect.stringMatching(/^https:\/\/1drv\.ms\//));
      expect(actions[0]).toHaveAttribute("target", "_blank");
      expect(actions[0]).toHaveAttribute("rel", "noreferrer noopener");
      expect(actions[1]).toHaveTextContent("Browse source");
      expect(actions[1]).toHaveAttribute("href", sourceHref);
    }
  });

  it("renders AGASS with a derived source action and no build action", () => {
    window.location.hash = "#/projects/agass";

    render(<App />);

    expect(screen.getByRole("link", { name: /browse source/i })).toHaveAttribute(
      "href",
      "#/projects/agass/source",
    );
    expect(screen.queryByRole("link", { name: /build pending/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("button", { name: /build pending/i })).not.toBeInTheDocument();

    for (const placeholder of screen.getAllByRole("img", { name: /placeholder/i })) {
      expect(placeholder).toBeEmptyDOMElement();
    }
  });

  it("renders project copy code references as source-route links", async () => {
    window.location.hash = "#/projects/hospital-flow";

    render(<App />);

    expect(screen.getByRole("link", { name: "AEMRReceptionMachine" })).toHaveAttribute(
      "href",
      "#/projects/hospital-flow/source?file=Source%2FLOD%2FPrivate%2FInteraction%2FEMRReceptionMachine.cpp&line=1",
    );

    window.location.hash = "#/projects/agass";
    window.dispatchEvent(new Event("hashchange"));

    expect(
      await screen.findByRole("heading", { name: "A Game About Stacking Sh*t" }),
    ).toBeInTheDocument();
    expect(
      within(
        screen.getByLabelText("Interaction, carry, preview, and placement source references"),
      ).getByRole("link", { name: "AAGASSPlaceableItemActor" }),
    ).toHaveAttribute(
      "href",
      "#/projects/agass/source?file=Plugins%2FAGASS%2FAGASSTower%2FSource%2FAGASSTower%2FPrivate%2FActors%2FAGASSPlaceableItemActor.cpp&line=1",
    );
  });

  it("ignores stale highlighted output when the active source file changes", async () => {
    const releaseHighlighter = shikiMock.delayHighlighter();

    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    expect(await screen.findByText("class AAGASSPlayerController;")).toBeInTheDocument();
    expect(container.querySelector(".source-token")).not.toBeInTheDocument();

    const pluginFile =
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp";
    const tree = screen.getByRole("complementary", { name: "Source files" });
    expandSourceFolders(tree, [
      "Plugins",
      "Plugins/AGASS",
      "Plugins/AGASS/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private",
    ]);
    const sourceLink = await screen.findByRole("link", { name: pluginFile });

    fireEvent.click(sourceLink);
    navigateToHash(sourceLink.getAttribute("href") ?? "");

    expect(await screen.findByText('#include "AGASSCommonModule.h"')).toBeInTheDocument();

    releaseHighlighter();

    await waitFor(() =>
      expect(container.querySelectorAll(".source-token").length).toBeGreaterThan(0),
    );

    const tokenText = Array.from(container.querySelectorAll(".source-line-text"))
      .map((line) => line.textContent)
      .join("\n");

    expect(tokenText).toContain('#include "AGASSCommonModule.h"');
    expect(tokenText).not.toContain("class AAGASSPlayerController;");
    expect(screen.queryByText("class AAGASSPlayerController;")).not.toBeInTheDocument();
  });

  it("keeps fetched plain source visible while the source highlighter loads", async () => {
    const releaseHighlighter = shikiMock.delayHighlighter();

    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    expect(await screen.findByText("class AAGASSPlayerController;")).toBeInTheDocument();
    expect(container.querySelector(".source-token")).not.toBeInTheDocument();

    releaseHighlighter();

    await waitFor(() =>
      expect(container.querySelectorAll(".source-token").length).toBeGreaterThan(0),
    );
  });

  it("keeps highlighted source lines addressable with selected line state", async () => {
    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3";

    const { container } = render(<App />);

    const codeReader = await screen.findByRole("region", { name: "Code reader" });

    await waitFor(() =>
      expect(container.querySelectorAll(".source-token").length).toBeGreaterThan(0),
    );

    const selectedLine = container.querySelector<HTMLElement>(
      '[data-line-number="3"]',
    );

    expect(selectedLine).toHaveClass("source-code-line", "selected");
    expect(within(selectedLine!).getByRole("link", { name: "Line 3" })).toHaveAttribute(
      "href",
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3",
    );
    expect(within(codeReader).getAllByRole("link", { name: /^Line \d+$/ })).toHaveLength(
      3,
    );
  });

  it("keeps empty highlighted lines addressable with stable placeholder text", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    await waitFor(() =>
      expect(container.querySelectorAll(".source-token").length).toBeGreaterThan(0),
    );

    const emptyLine = container.querySelector<HTMLElement>('[data-line-number="2"]');

    expect(emptyLine).toHaveClass("source-code-line");
    expect(within(emptyLine!).getByRole("link", { name: "Line 2" })).toHaveAttribute(
      "href",
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=2",
    );
    expect(emptyLine?.querySelector(".source-line-text")?.textContent).toBe(" ");
  });

  it("keeps highlighted code text inert while line numbers update source address", async () => {
    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h";

    const { container } = render(<App />);

    const codeReader = await screen.findByRole("region", { name: "Code reader" });

    await waitFor(() =>
      expect(container.querySelectorAll(".source-token").length).toBeGreaterThan(0),
    );

    const highlightedToken = await findSourceLineTextElement(
      "class AAGASSPlayerController;",
    );

    fireEvent.click(highlightedToken);
    fireEvent.mouseUp(highlightedToken);

    expect(window.location.hash).toBe(
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h",
    );

    const lineLink = within(codeReader).getByRole("link", { name: "Line 3" });

    fireEvent.click(lineLink);
    navigateToHash(lineLink.getAttribute("href") ?? "");

    expect(window.location.hash).toBe(
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3",
    );
  });

  it("reruns selected-line scrolling after highlighted output settles", async () => {
    const scrollCalls = recordSelectedCodeLineScrolls();

    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3";

    const { container } = render(<App />);

    await screen.findByText("class AAGASSPlayerController;");
    await waitFor(() =>
      expect(container.querySelectorAll(".source-token").length).toBeGreaterThan(0),
    );

    await waitFor(() => expect(scrollCalls.length).toBeGreaterThanOrEqual(2));
    expect(scrollCalls).toEqual(
      expect.arrayContaining([
        {
          lineNumber: "3",
          options: {
            block: "center",
            inline: "nearest",
          },
        },
      ]),
    );
  });

  it("renders a source route from committed manifest metadata", async () => {
    const fetchMock = stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    expect(await screen.findByRole("region", { name: "Source browser" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: /page not found|project route unavailable/i })).not.toBeInTheDocument();
    expect(
      screen.getByRole("heading", {
        hidden: true,
        name: "A Game About Stacking Sh*t",
      }),
    ).toBeInTheDocument();
    const dialog = screen.getByRole("dialog", { name: "AGASS source" });
    expect(within(dialog).getByRole("heading", { name: "AGASS source" })).toBeInTheDocument();
    expect(screen.queryByText(/3 files, 139 lines/i)).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Snapshot summary" })).not.toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: "Source address" })).not.toBeInTheDocument();
    expect(
      screen.getByRole("link", {
        name: "Source/AGASS/Public/AGASSPlayerController.h",
      }),
    ).toHaveAttribute("aria-current", "page");
    const tree = screen.getByRole("complementary", { name: "Source files" });
    expect(
      within(tree).getByRole("button", { name: "Toggle Source" }),
    ).toHaveAttribute("aria-expanded", "true");
    expect(
      within(tree).getByRole("button", { name: "Toggle Source/AGASS" }),
    ).toHaveAttribute("aria-expanded", "true");
    expect(
      within(tree).getByRole("button", { name: "Toggle Source/AGASS/Public" }),
    ).toHaveAttribute("aria-expanded", "true");
    expect(
      within(tree).getByRole("button", { name: "Toggle Plugins" }),
    ).toHaveAttribute("aria-expanded", "false");
    expect(screen.getAllByText(/Project code - \d+ lines/).length).toBeGreaterThan(0);
    expect(screen.queryByText(/Project plugin - \d+ lines/)).not.toBeInTheDocument();
    expect(screen.queryByText(/Third-party plugin - \d+ lines/)).not.toBeInTheDocument();
    expect(await findSourceLineTextElement("#pragma once")).toBeInTheDocument();
    await waitFor(() =>
      expect(container.querySelectorAll(".source-token").length).toBeGreaterThan(0),
    );
    expect(container.querySelector(".source-token")).toHaveStyle({
      color: "rgb(0, 51, 179)",
    });
    expect(container.querySelectorAll(".source-code-line")).toHaveLength(3);
    expect(within(dialog).getByRole("link", { name: "Close source" })).toHaveAttribute(
      "href",
      "#/projects/agass",
    );
    expect(tree).toBeInTheDocument();
    expect(screen.getByRole("region", { name: "Code reader" })).toBeInTheDocument();
    expect(screen.queryByRole("navigation", { name: /previous and next projects/i })).not.toBeInTheDocument();
    expect(document.title).toBe("AGASS source - Baptiste Giquet");
    expect(fetchMock).toHaveBeenCalledWith(
      "/source/agass/manifest.json",
      expect.objectContaining({ signal: expect.any(AbortSignal) }),
    );
    expect(
      fetchMock.mock.calls.filter(([input]) =>
        input.toString().endsWith("/source/agass/manifest.json"),
      ),
    ).toHaveLength(1);
    expect(fetchMock).toHaveBeenCalledWith(
      "/source/agass/files/Source/AGASS/Public/AGASSPlayerController.h",
      expect.objectContaining({ signal: expect.any(AbortSignal) }),
    );
    expect(fetchMock).not.toHaveBeenCalledWith(
      "/source/agass/files/Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp",
      expect.anything(),
    );
    expect(fetchMock).not.toHaveBeenCalledWith(
      "/source/agass/files/Plugins/GameSettings/Source/Public/GameSetting.h",
      expect.anything(),
    );
  });

  it("loads French source routes while localizing only surrounding source UI", async () => {
    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?lang=fr&file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3";

    const { container } = render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "Source AGASS" });

    expect(within(dialog).getByRole("link", { name: "Fermer la source" })).toHaveAttribute(
      "href",
      "#/projects/agass?lang=fr",
    );
    expect(screen.getByRole("region", { name: "Navigateur de source" })).toBeInTheDocument();
    expect(screen.getByRole("complementary", { name: "Fichiers source" })).toBeInTheDocument();
    expect(screen.getByRole("region", { name: "Lecteur de code" })).toBeInTheDocument();
    expect(screen.getByText("Code du projet - 123 lignes")).toBeInTheDocument();
    expect(await screen.findByText("class AAGASSPlayerController;")).toBeInTheDocument();
    expect(container.querySelector('[data-line-number="3"]')).toHaveClass("selected");

    const toggleHrefs = Array.from(
      container.querySelectorAll<HTMLAnchorElement>(".site-header .language-toggle a"),
    ).map((link) => link.getAttribute("href"));

    expect(toggleHrefs).toContain(
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3",
    );
    expect(toggleHrefs).toContain(
      "#/projects/agass/source?lang=fr&file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3",
    );
  });

  it("renders the TLM source route with C# file recovery and project-owned tooling code", async () => {
    const fetchMock = stubSourceManifestFetch({
      manifests: {
        tlm: tlmSourceManifestFixture,
      },
    });
    window.location.hash = "#/projects/tlm/source?file=src%2FMissing.cs";

    render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "TLM source" });
    const tree = screen.getByRole("complementary", { name: "Source files" });

    expect(within(dialog).getByRole("heading", { name: "TLM source" })).toBeInTheDocument();
    expect(
      within(tree).getByRole("link", {
        name: "src/Gameplay/Run/RunControllerNode.cs",
      }),
    ).toHaveAttribute("aria-current", "page");
    expect(
      within(tree).getByRole("button", { name: "Toggle addons" }),
    ).toHaveAttribute("aria-expanded", "false");
    expect(await screen.findByText("namespace TheLastMage.Gameplay.Run;")).toBeInTheDocument();
    expect(screen.getByText("public partial class RunControllerNode")).toBeInTheDocument();
    expect(shikiMock.codeToTokens).toHaveBeenCalledWith(
      "namespace TheLastMage.Gameplay.Run;\n\npublic partial class RunControllerNode\n{\n}\n",
      {
        lang: "csharp",
        theme: "portfolio-rider-light",
      },
    );

    expandSourceFolders(tree, [
      "addons",
      "addons/the_last_mage_tools",
    ]);
    const pluginFile = "addons/the_last_mage_tools/TheLastMageToolsPlugin.cs";
    const pluginLink = await screen.findByRole("link", { name: pluginFile });

    expect(within(tree).getByText("Project plugin - 4 lines")).toBeInTheDocument();

    fireEvent.click(pluginLink);
    window.location.hash = pluginLink.getAttribute("href") ?? "";
    window.dispatchEvent(new Event("hashchange"));

    expect(await screen.findByText("namespace TheLastMage.EditorTools;")).toBeInTheDocument();
    expect(fetchMock).toHaveBeenCalledWith(
      "/source/tlm/manifest.json",
      expect.objectContaining({ signal: expect.any(AbortSignal) }),
    );
    expect(fetchMock).toHaveBeenCalledWith(
      "/source/tlm/files/src/Gameplay/Run/RunControllerNode.cs",
      expect.objectContaining({ signal: expect.any(AbortSignal) }),
    );
    expect(fetchMock).toHaveBeenCalledWith(
      "/source/tlm/files/addons/the_last_mage_tools/TheLastMageToolsPlugin.cs",
      expect.objectContaining({ signal: expect.any(AbortSignal) }),
    );
  });

  it("renders only source hierarchy and code surfaces in the source modal", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "AGASS source" });

    expect(within(dialog).getByRole("link", { name: "Close source" })).toBeInTheDocument();
    expect(screen.getByRole("complementary", { name: "Source files" })).toBeInTheDocument();
    expect(screen.getByRole("region", { name: "Code reader" })).toBeInTheDocument();
    expect(screen.queryByLabelText("Search source")).not.toBeInTheDocument();
    expect(screen.queryByRole("group", { name: "Mobile source view" })).not.toBeInTheDocument();
    expect(screen.queryByRole("button", { name: "Reset filters" })).not.toBeInTheDocument();
    expect(screen.queryByRole("checkbox", { name: "Project code" })).not.toBeInTheDocument();
    expect(screen.queryByText("Ownership filters")).not.toBeInTheDocument();
  });

  it("resolves encoded source file and line query state from the route", async () => {
    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=42";

    render(<App />);

    expect(await screen.findByRole("region", { name: "Code reader" })).toBeInTheDocument();
    expect(screen.queryByText("Selected file")).not.toBeInTheDocument();
    expect(
      screen.getByRole("link", {
        name: "Source/AGASS/Public/AGASSPlayerController.h",
      }),
    ).toHaveAttribute("aria-current", "page");
    expect(screen.queryByText("Line 42")).not.toBeInTheDocument();
  });

  it("updates source URLs from line-number clicks without reacting to code text selection", async () => {
    const scrollIntoView = vi.fn();

    Object.defineProperty(window.HTMLElement.prototype, "scrollIntoView", {
      configurable: true,
      value: scrollIntoView,
    });

    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h";

    const { container } = render(<App />);

    const codeReader = await screen.findByRole("region", { name: "Code reader" });
    const codeText = await findSourceLineTextElement(
      "class AAGASSPlayerController;",
    );
    const lineLink = within(codeReader).getByRole("link", { name: "Line 3" });

    fireEvent.mouseUp(codeText);

    expect(window.location.hash).toBe(
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h",
    );

    expect(lineLink).toHaveAttribute(
      "href",
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3",
    );

    fireEvent.click(lineLink);
    window.location.hash = lineLink.getAttribute("href") ?? "";
    window.dispatchEvent(new Event("hashchange"));

    await waitFor(() =>
      expect(container.querySelector('[data-line-number="3"]')).toHaveClass(
        "selected",
      ),
    );
    await waitFor(() =>
      expect(scrollIntoView).toHaveBeenCalledWith({
        block: "center",
        inline: "nearest",
      }),
    );
  });

  it("supports browser back and forward across explicit line selections", async () => {
    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h";

    const { container } = render(<App />);

    const codeReader = await screen.findByRole("region", { name: "Code reader" });
    const lineOneHref =
      within(codeReader).getByRole("link", { name: "Line 1" }).getAttribute("href") ??
      "";
    const lineTwoHref =
      within(codeReader).getByRole("link", { name: "Line 2" }).getAttribute("href") ??
      "";

    window.location.hash = lineOneHref;
    window.dispatchEvent(new Event("hashchange"));
    await waitFor(() =>
      expect(container.querySelector('[data-line-number="1"]')).toHaveClass(
        "selected",
      ),
    );

    window.location.hash = lineTwoHref;
    window.dispatchEvent(new Event("hashchange"));
    await waitFor(() =>
      expect(container.querySelector('[data-line-number="2"]')).toHaveClass(
        "selected",
      ),
    );

    window.history.back();

    await waitFor(() => expect(window.location.hash).toBe(lineOneHref));
    await waitFor(() =>
      expect(container.querySelector('[data-line-number="1"]')).toHaveClass(
        "selected",
      ),
    );

    window.history.forward();

    await waitFor(() => expect(window.location.hash).toBe(lineTwoHref));
    await waitFor(() =>
      expect(container.querySelector('[data-line-number="2"]')).toHaveClass(
        "selected",
      ),
    );
  });

  it("recovers stale source file queries to the manifest entry file", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source?file=Source%2FMissing.cpp";

    render(<App />);

    await screen.findByRole("region", { name: "Code reader" });
    expect(
      screen.getByRole("link", {
        name: "Source/AGASS/Public/AGASSPlayerController.h",
      }),
    ).toHaveAttribute("aria-current", "page");
    expect(screen.queryByRole("status")).not.toBeInTheDocument();
  });

  it("recovers stale source file queries to the first sorted source file without an entry file", async () => {
    stubSourceManifestFetch({
      manifest: {
        ...sourceManifestFixture,
        entryFile: undefined,
      },
    });
    window.location.hash = "#/projects/agass/source?file=Source%2FMissing.cpp";

    render(<App />);

    await screen.findByRole("region", { name: "Code reader" });
    expect(
      screen.getByRole("link", {
        name: "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp",
      }),
    ).toHaveAttribute("aria-current", "page");
    expect(screen.queryByRole("status")).not.toBeInTheDocument();
  });

  it("clears line selections when a stale file query recovers to a different file", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source?file=Source%2FMissing.cpp&line=3";

    render(<App />);

    await screen.findByRole("region", { name: "Code reader" });
    expect(screen.queryByRole("status")).not.toBeInTheDocument();
    expect(screen.queryByText("None")).not.toBeInTheDocument();
    expect(
      screen.queryByText("class AAGASSPlayerController;", { selector: ".selected *" }),
    ).not.toBeInTheDocument();
  });

  it("clears source line selections that exceed the selected file range", async () => {
    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=999";

    render(<App />);

    await screen.findByRole("region", { name: "Code reader" });
    expect(screen.queryByRole("status")).not.toBeInTheDocument();
    expect(screen.queryByText("None")).not.toBeInTheDocument();
    expect(document.querySelector(".source-code-line.selected")).not.toBeInTheDocument();
  });

  it("renders source file fetch failures inside the code pane", async () => {
    stubSourceManifestFetch({
      missingFiles: ["/source/agass/files/Source/AGASS/Public/AGASSPlayerController.h"],
    });
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const codeReader = await screen.findByRole("region", { name: "Code reader" });

    expect(within(codeReader).getByRole("alert")).toHaveTextContent(
      "Source file request failed with 404.",
    );
    expect(screen.getByRole("heading", { name: "AGASS source" })).toBeInTheDocument();
    expect(screen.getByRole("complementary", { name: "Source files" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: /page not found|project route unavailable/i })).not.toBeInTheDocument();
  });

  it("keeps highlighting failures as plain source without a code pane alert", async () => {
    shikiMock.failTokenization();
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    const codeReader = await screen.findByRole("region", { name: "Code reader" });

    expect(await within(codeReader).findByText("#pragma once")).toBeInTheDocument();
    expect(within(codeReader).getByText("class AAGASSPlayerController;")).toBeInTheDocument();
    expect(within(codeReader).queryByRole("alert")).not.toBeInTheDocument();
    expect(container.querySelector(".source-token")).not.toBeInTheDocument();
    expect(shikiMock.codeToTokens).toHaveBeenCalled();
  });

  it("uses source file links as browser-history-friendly source navigation state", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const pluginFile =
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp";
    const tree = await screen.findByRole("complementary", { name: "Source files" });
    expandSourceFolders(tree, [
      "Plugins",
      "Plugins/AGASS",
      "Plugins/AGASS/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private",
    ]);
    const sourceLink = await screen.findByRole("link", { name: pluginFile });

    expect(sourceLink).toHaveAttribute(
      "href",
      "#/projects/agass/source?file=Plugins%2FAGASS%2FAGASSCommon%2FSource%2FAGASSCommon%2FPrivate%2FAGASSCommonModule.cpp",
    );

    fireEvent.click(sourceLink);
    window.location.hash = sourceLink.getAttribute("href") ?? "";
    window.dispatchEvent(new Event("hashchange"));

    expect(await screen.findByText('#include "AGASSCommonModule.h"')).toBeInTheDocument();
    expect(window.location.hash).toBe(
      "#/projects/agass/source?file=Plugins%2FAGASS%2FAGASSCommon%2FSource%2FAGASSCommon%2FPrivate%2FAGASSCommonModule.cpp",
    );
  });

  it("centers the selected source file in the hierarchy when source opens", async () => {
    const scrollCalls = recordSelectedTreeScrolls();

    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    await screen.findByRole("link", {
      name: "Source/AGASS/Public/AGASSPlayerController.h",
    });

    await waitFor(() =>
      expect(scrollCalls).toEqual(
        expect.arrayContaining([
          {
            label: "Source/AGASS/Public/AGASSPlayerController.h",
            options: {
              block: "center",
              inline: "nearest",
            },
          },
        ]),
      ),
    );
  });

  it("centers the newly selected source file in the hierarchy after source navigation", async () => {
    const scrollCalls = recordSelectedTreeScrolls();

    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const pluginFile =
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp";
    const tree = await screen.findByRole("complementary", { name: "Source files" });
    expandSourceFolders(tree, [
      "Plugins",
      "Plugins/AGASS",
      "Plugins/AGASS/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private",
    ]);
    const sourceLink = await screen.findByRole("link", { name: pluginFile });

    fireEvent.click(sourceLink);
    window.location.hash = sourceLink.getAttribute("href") ?? "";
    window.dispatchEvent(new Event("hashchange"));

    await screen.findByText('#include "AGASSCommonModule.h"');
    await waitFor(() =>
      expect(scrollCalls).toEqual(
        expect.arrayContaining([
          {
            label: pluginFile,
            options: {
              block: "center",
              inline: "nearest",
            },
          },
        ]),
      ),
    );
  });

  it("preserves source tree expansion across source address changes", async () => {
    const scrollIntoView = vi.fn();

    Object.defineProperty(window.HTMLElement.prototype, "scrollIntoView", {
      configurable: true,
      value: scrollIntoView,
    });

    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "AGASS source" });
    const tree = await screen.findByRole("complementary", { name: "Source files" });
    expandSourceFolders(tree, ["Plugins", "Plugins/GameSettings"]);
    const gameSettingsFolder = within(tree).getByRole("button", {
      name: "Toggle Plugins/GameSettings",
    });

    fireEvent.click(gameSettingsFolder);

    expect(gameSettingsFolder).toHaveAttribute("aria-expanded", "false");

    navigateToHash(
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3",
    );

    await waitFor(() =>
      expect(document.querySelector('[data-line-number="3"]')).toHaveClass(
        "selected",
      ),
    );
    expect(screen.getByRole("dialog", { name: "AGASS source" })).toBe(dialog);
    expect(gameSettingsFolder).toHaveAttribute("aria-expanded", "false");
    expect(
      getSourceLineTextElement("class AAGASSPlayerController;"),
    ).toBeInTheDocument();
    expect(document.querySelector('[data-line-number="3"]')).toHaveClass("selected");
    await waitFor(() =>
      expect(scrollIntoView).toHaveBeenCalledWith({
        block: "center",
        inline: "nearest",
      }),
    );
  });

  it("clears the selected line when selecting a different source file", async () => {
    stubSourceManifestFetch();
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3";

    const { container } = render(<App />);

    await waitFor(() =>
      expect(container.querySelector('[data-line-number="3"]')).toHaveClass(
        "selected",
      ),
    );

    const pluginFile =
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp";
    const tree = screen.getByRole("complementary", { name: "Source files" });
    expandSourceFolders(tree, [
      "Plugins",
      "Plugins/AGASS",
      "Plugins/AGASS/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private",
    ]);
    const sourceLink = await screen.findByRole("link", { name: pluginFile });

    fireEvent.click(sourceLink);
    window.location.hash = sourceLink.getAttribute("href") ?? "";
    window.dispatchEvent(new Event("hashchange"));

    expect(await screen.findByText('#include "AGASSCommonModule.h"')).toBeInTheDocument();
    expect(document.querySelector(".source-code-line.selected")).not.toBeInTheDocument();
  });

  it("resets source tree expansion after closing and reopening source", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "AGASS source" });
    const tree = screen.getByRole("complementary", { name: "Source files" });
    const pluginsFolder = within(tree).getByRole("button", { name: "Toggle Plugins" });

    fireEvent.click(pluginsFolder);
    expect(pluginsFolder).toHaveAttribute("aria-expanded", "true");

    const closeSourceLink = within(dialog).getByRole("link", { name: "Close source" });
    navigateToHash(closeSourceLink.getAttribute("href") ?? "");

    await waitFor(() => expect(screen.queryByRole("dialog")).not.toBeInTheDocument());

    const sourceLink = screen.getByRole("link", { name: /browse source/i });
    navigateToHash(sourceLink.getAttribute("href") ?? "");

    await screen.findByRole("dialog", { name: "AGASS source" });
    expect(
      within(screen.getByRole("complementary", { name: "Source files" })).getByRole(
        "button",
        { name: "Toggle Plugins" },
      ),
    ).toHaveAttribute("aria-expanded", "false");
  });

  it("resets source tree expansion when opening source for a different project", async () => {
    stubSourceManifestFetch({
      manifests: {
        "hospital-flow": hospitalSourceManifestFixture,
      },
    });
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    await screen.findByRole("dialog", { name: "AGASS source" });

    const tree = screen.getByRole("complementary", { name: "Source files" });
    const pluginsFolder = within(tree).getByRole("button", { name: "Toggle Plugins" });
    fireEvent.click(pluginsFolder);
    expect(pluginsFolder).toHaveAttribute("aria-expanded", "true");

    navigateToHash("#/projects/hospital-flow/source");

    await screen.findByRole("dialog", { name: "Hospital Flow source" });
    expect(
      await screen.findByText('#include "EMRReceptionMachine.h"'),
    ).toBeInTheDocument();
  });

  it("lazy-loads selected source files after explicit tree navigation", async () => {
    const fetchMock = stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    await findSourceLineTextElement("#pragma once");

    const thirdPartyFile = "Plugins/GameSettings/Source/Public/GameSetting.h";
    const tree = screen.getByRole("complementary", { name: "Source files" });
    expandSourceFolders(tree, [
      "Plugins",
      "Plugins/GameSettings",
      "Plugins/GameSettings/Source",
      "Plugins/GameSettings/Source/Public",
    ]);
    const thirdPartyLink = await screen.findByRole("link", { name: thirdPartyFile });

    expect(fetchMock).not.toHaveBeenCalledWith(
      "/source/agass/files/Plugins/GameSettings/Source/Public/GameSetting.h",
      expect.anything(),
    );

    fireEvent.click(thirdPartyLink);
    window.location.hash = thirdPartyLink.getAttribute("href") ?? "";
    window.dispatchEvent(new Event("hashchange"));

    expect(await screen.findByText("class UGameSetting;")).toBeInTheDocument();
    expect(fetchMock).toHaveBeenCalledWith(
      "/source/agass/files/Plugins/GameSettings/Source/Public/GameSetting.h",
      expect.objectContaining({ signal: expect.any(AbortSignal) }),
    );
  });

  it("renders source files as a folder-first tree with authorship labels and line counts", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const tree = await screen.findByRole("complementary", { name: "Source files" });

    expect(within(tree).getByText("Plugins")).toBeInTheDocument();
    expect(within(tree).getAllByText("Source").length).toBeGreaterThan(0);
    expect(
      within(tree).getByRole("button", { name: "Toggle Plugins" }),
    ).toHaveAttribute("aria-expanded", "false");
    expect(
      within(tree).getByRole("link", {
        name: "Source/AGASS/Public/AGASSPlayerController.h",
      }),
    ).toHaveAttribute("aria-current", "page");
    expect(within(tree).getByText("Project code - 123 lines")).toBeInTheDocument();
    expect(within(tree).queryByText("Project plugin - 4 lines")).not.toBeInTheDocument();
    expect(
      within(tree).queryByText("Third-party plugin - 12 lines"),
    ).not.toBeInTheDocument();

    expandSourceFolders(tree, [
      "Plugins",
      "Plugins/AGASS",
      "Plugins/AGASS/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private",
      "Plugins/GameSettings",
      "Plugins/GameSettings/Source",
      "Plugins/GameSettings/Source/Public",
    ]);

    expect(within(tree).getByText("Project plugin - 4 lines")).toBeInTheDocument();
    expect(within(tree).getByText("Third-party plugin - 12 lines")).toBeInTheDocument();
  });

  it("preserves folder expansion state across manual toggles", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const tree = await screen.findByRole("complementary", { name: "Source files" });
    const pluginsFolder = within(tree).getByRole("button", { name: "Toggle Plugins" });
    const moduleFile =
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private/AGASSCommonModule.cpp";

    expect(pluginsFolder).toHaveAttribute("aria-expanded", "false");
    fireEvent.click(pluginsFolder);
    expect(pluginsFolder).toHaveAttribute("aria-expanded", "true");

    expandSourceFolders(tree, [
      "Plugins/AGASS",
      "Plugins/AGASS/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon",
      "Plugins/AGASS/AGASSCommon/Source/AGASSCommon/Private",
    ]);
    expect(within(tree).getByRole("link", { name: moduleFile })).toBeInTheDocument();

    fireEvent.click(pluginsFolder);
    expect(pluginsFolder).toHaveAttribute("aria-expanded", "false");
    expect(within(tree).queryByRole("link", { name: moduleFile })).not.toBeInTheDocument();

    fireEvent.click(pluginsFolder);

    expect(within(tree).getByRole("link", { name: moduleFile })).toBeInTheDocument();
  });

  it("navigates from Browse source to the source route", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass";

    render(<App />);

    const sourceLink = screen.getByRole("link", { name: /browse source/i });
    expect(sourceLink).toHaveAttribute("href", "#/projects/agass/source");

    fireEvent.click(sourceLink);
    window.location.hash = sourceLink.getAttribute("href") ?? "";
    window.dispatchEvent(new Event("hashchange"));

    expect(await screen.findByRole("dialog", { name: "AGASS source" })).toBeInTheDocument();
    expect(
      screen.getByRole("heading", {
        hidden: true,
        name: "A Game About Stacking Sh*t",
      }),
    ).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: /page not found|project route unavailable/i })).not.toBeInTheDocument();
  });

  it("renders source-unavailable state for known projects without source metadata", () => {
    window.location.hash = "#/projects/commonui-frontend/source";

    render(<App />);

    const dialog = screen.getByRole("dialog", { name: "CommonUI Frontend source" });
    expect(
      screen.getByRole("heading", {
        hidden: true,
        name: "CommonUI Frontend Course",
      }),
    ).toBeInTheDocument();
    expect(within(dialog).getByRole("heading", { name: "CommonUI Frontend source" })).toBeInTheDocument();
    expect(within(dialog).getByRole("heading", { name: "Source unavailable" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: /page not found|project route unavailable/i })).not.toBeInTheDocument();
  });

  it("moves focus into the source dialog and hides the app background from keyboard access", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "AGASS source" });
    const closeSourceLink = within(dialog).getByRole("link", { name: "Close source" });
    const header = container.querySelector<HTMLElement>(".site-header");
    const backgroundProject = container.querySelector<HTMLElement>("main > .detail-page");

    await waitFor(() => expect(closeSourceLink).toHaveFocus());

    for (const element of [header, backgroundProject]) {
      expect(element).toHaveAttribute("inert");
      expect(element).toHaveAttribute("aria-hidden", "true");
    }
  });

  it("keeps tab focus inside the source dialog", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "AGASS source" });
    const closeSourceLink = within(dialog).getByRole("link", { name: "Close source" });

    await findSourceLineTextElement("#pragma once");
    await waitFor(() => expect(closeSourceLink).toHaveFocus());

    const focusableElements = Array.from(
      dialog.querySelectorAll<HTMLElement>(
        "a[href], button:not([disabled]), input:not([disabled]), [tabindex]:not([tabindex='-1'])",
      ),
    );
    const lastFocusableElement = focusableElements[focusableElements.length - 1];

    fireEvent.keyDown(document, { key: "Tab", shiftKey: true });

    expect(lastFocusableElement).toHaveFocus();

    fireEvent.keyDown(document, { key: "Tab" });

    expect(closeSourceLink).toHaveFocus();
  });

  it("closes source deterministically on Escape and restores body scroll", async () => {
    stubSourceManifestFetch();
    document.body.style.overflow = "clip";
    window.location.hash =
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=3";

    render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "AGASS source" });

    expect(dialog).toBeInTheDocument();
    expect(document.body.style.overflow).toBe("hidden");

    fireEvent.keyDown(document, { key: "Escape" });

    await waitFor(() => expect(screen.queryByRole("dialog")).not.toBeInTheDocument());
    expect(window.location.hash).toBe("#/projects/agass");
    expect(document.body.style.overflow).toBe("clip");
  });

  it("keeps the source modal open when the backdrop is clicked", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    await screen.findByRole("dialog", { name: "AGASS source" });

    fireEvent.click(container.querySelector(".source-modal-backdrop") as Element);

    expect(screen.getByRole("dialog", { name: "AGASS source" })).toBeInTheDocument();
    expect(window.location.hash).toBe("#/projects/agass/source");
  });

  it("cleans up modal lifecycle side effects when navigating away", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    await screen.findByRole("dialog", { name: "AGASS source" });
    expect(document.body.style.overflow).toBe("hidden");

    window.location.hash = "#/";
    window.dispatchEvent(new Event("hashchange"));

    await waitFor(() => expect(screen.queryByRole("dialog")).not.toBeInTheDocument());
    expect(document.body.style.overflow).toBe("");

    fireEvent.keyDown(document, { key: "Escape" });

    expect(window.location.hash).toBe("#/");
    expect(container.querySelector(".site-header")).not.toBeInTheDocument();
    expect(container.querySelector(".home-language-toggle")).toBeInTheDocument();
    expect(container.querySelector(".site-footer")).not.toBeInTheDocument();
  });

  it("restores background accessibility and focuses the source action after closing", async () => {
    stubSourceManifestFetch();
    window.location.hash = "#/projects/agass/source";

    const { container } = render(<App />);

    const dialog = await screen.findByRole("dialog", { name: "AGASS source" });
    const closeSourceLink = within(dialog).getByRole("link", { name: "Close source" });
    const header = container.querySelector<HTMLElement>(".site-header");
    const backgroundProject = container.querySelector<HTMLElement>("main > .detail-page");

    fireEvent.click(closeSourceLink);
    window.location.hash = closeSourceLink.getAttribute("href") ?? "";
    window.dispatchEvent(new Event("hashchange"));

    await waitFor(() => expect(screen.queryByRole("dialog")).not.toBeInTheDocument());
    await waitFor(() =>
      expect(screen.getByRole("link", { name: /browse source/i })).toHaveFocus(),
    );

    for (const element of [header, backgroundProject]) {
      expect(element).not.toHaveAttribute("inert");
      expect(element).not.toHaveAttribute("aria-hidden");
    }
  });

  it("renders not-found for unknown project source routes", () => {
    window.location.hash = "#/projects/unknown/source";

    render(<App />);

    expect(screen.getByRole("heading", { name: /page not found|project route unavailable/i })).toBeInTheDocument();
    expect(screen.queryByRole("dialog")).not.toBeInTheDocument();
  });

  it("does not render previous or next project navigation on detail pages", () => {
    window.location.hash = "#/projects/hospital-flow";

    render(<App />);

    expect(screen.queryByRole("navigation", { name: /previous and next projects/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: /previous:/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: /next:/i })).not.toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Emergency Room simulator" })).toBeInTheDocument();
    expect(screen.queryByRole("heading", { name: /page not found|project route unavailable/i })).not.toBeInTheDocument();
  });

  it("renders hospital with a derived source action and no build action", () => {
    window.location.hash = "#/projects/hospital-flow";

    render(<App />);

    expect(screen.getByRole("link", { name: /browse source/i })).toHaveAttribute(
      "href",
      "#/projects/hospital-flow/source",
    );
    expect(screen.queryByRole("link", { name: /build pending/i })).not.toBeInTheDocument();
    expect(screen.queryByRole("button", { name: /build pending/i })).not.toBeInTheDocument();

    expect(screen.queryByRole("img", { name: /placeholder/i })).not.toBeInTheDocument();
  });

  it("renders not-found for unknown project slugs", () => {
    window.location.hash = "#/projects/unknown";

    render(<App />);

    expect(screen.getByRole("heading", { name: /page not found|project route unavailable/i })).toBeInTheDocument();
    expect(screen.getByText(/No public project page is registered for "unknown"./i)).toBeInTheDocument();
  });
});
