import { access, readFile } from "node:fs/promises";
import path from "node:path";
import { describe, expect, it } from "vitest";
import {
  featuredOriginalProjects,
  getAdjacentProjects,
  getPortfolioContent,
  learningProjects,
  projectOrder,
  projectRegistry,
  projects,
  siteProfile,
  type CodeReference,
  type ProjectAction,
  type RichTextContent,
  type ProjectSlug,
} from "./portfolio";
import { roleVariants } from "./types";

type SourceManifest = {
  schemaVersion: number;
  projectSlug: string;
  exportedAt: string;
  entryFile?: string;
  metrics: {
    fileCount: number;
    ownershipCounts: Record<string, number>;
  };
  files: Array<{
    path: string;
    snapshotPath: string;
    language: string;
    ownership: string;
    lines: number;
  }>;
};

function collectCodeReferences(content: RichTextContent | undefined) {
  if (!content || typeof content === "string") {
    return [] satisfies CodeReference[];
  }

  return content.filter(
    (run): run is CodeReference =>
      typeof run !== "string" && run.kind === "code-reference",
  );
}

function collectProjectCodeReferences(project: (typeof projects)[number]) {
  const references: CodeReference[] = [];
  const page = project.page;

  references.push(...collectCodeReferences(page.overview));
  references.push(...collectCodeReferences(page.roleScope));
  references.push(...collectCodeReferences(page.courseContext));

  for (const section of page.sections) {
    references.push(...collectCodeReferences(section.body));
    references.push(...(section.sourceReferences ?? []));

    for (const point of section.points ?? []) {
      references.push(...collectCodeReferences(point));
    }
  }

  for (const item of [
    ...page.proof,
    ...page.improvements,
    ...(page.whatBuilt ?? []),
    ...(page.whatLearned ?? []),
    ...(page.scopeNotes ?? []),
  ]) {
    references.push(...collectCodeReferences(item));
  }

  return references;
}

describe("project registry", () => {
  it("contains every canonical project with route, metadata, content, and navigation eligibility", () => {
    expect(projects).toHaveLength(projectOrder.length);

    for (const slug of projectOrder) {
      const project = projectRegistry[slug];

      expect(project.slug).toBe(slug);
      expect(project.route).toBe(`#/projects/${slug}`);
      expect(project.title).toBeTruthy();
      expect(project.visibility).toMatch(/public|archived/);
      expect(project.category).toMatch(/original|learning/);
      expect(project.summary).toBeTruthy();
      expect(project.technologies.length).toBeGreaterThan(0);
      expect(project.mediaSlots.length).toBeGreaterThan(0);
      expect(project.actions.length).toBeGreaterThan(0);
      expect(project.page.overview).toBeTruthy();
      expect(project.page.roleScope).toBeTruthy();
      expect(Array.isArray(project.page.proof)).toBe(true);
      expect(Array.isArray(project.page.improvements)).toBe(true);
      expect(typeof project.includeInPreviousNext).toBe("boolean");
    }
  });

  it("keeps localized registries structurally aligned with English", () => {
    const english = getPortfolioContent("en");
    const french = getPortfolioContent("fr");

    expect(english.projects.map((project) => project.slug)).toEqual(
      french.projects.map((project) => project.slug),
    );

    for (const slug of projectOrder) {
      const englishProject = english.projectRegistry[slug];
      const frenchProject = french.projectRegistry[slug];

      expect(frenchProject.slug).toBe(englishProject.slug);
      expect(frenchProject.title).toBe(englishProject.title);
      expect(frenchProject.shortTitle).toBe(englishProject.shortTitle);
      expect(frenchProject.visibility).toBe(englishProject.visibility);
      expect(frenchProject.category).toBe(englishProject.category);
      expect(frenchProject.technologies).toEqual(englishProject.technologies);
      expect(frenchProject.source).toEqual(englishProject.source);
      expect(frenchProject.includeInPreviousNext).toBe(
        englishProject.includeInPreviousNext,
      );
      expect(frenchProject.mediaSlots.map((slot) => slot.id)).toEqual(
        englishProject.mediaSlots.map((slot) => slot.id),
      );
      expect(frenchProject.mediaSlots.map((slot) => slot.source)).toEqual(
        englishProject.mediaSlots.map((slot) => slot.source),
      );
      expect(frenchProject.actions.map((action) => action.id)).toEqual(
        englishProject.actions.map((action) => action.id),
      );
      expect(frenchProject.actions.map((action) => action.kind)).toEqual(
        englishProject.actions.map((action) => action.kind),
      );
      expect(frenchProject.page.sections.map((section) => section.mediaSlotId)).toEqual(
        englishProject.page.sections.map((section) => section.mediaSlotId),
      );
      expect(collectProjectCodeReferences(frenchProject)).toEqual(
        collectProjectCodeReferences(englishProject),
      );
    }
  });

  it("provides required French strings without translating stable technical identifiers", () => {
    const french = getPortfolioContent("fr");

    expect(french.siteProfile.role).toBe("Junior Unreal Engine Programmer");
    expect(french.ui.viewProject).toBe("Voir le projet");
    expect(french.ui.closeSource).toBe("Fermer la source");
    expect(french.projectRegistry["hospital-flow"].summary).toMatch(
      /Simulateur d'urgences/i,
    );
    expect(french.projectRegistry["hospital-flow"].page.sections[0].title).toBe(
      "Boucle Hub vers Gardes",
    );
    expect(french.projectRegistry.agass.page.sections[9].title).toBe(
      "Workflow d'auteur Modding Kit",
    );
    expect(french.projectRegistry.tlm.page.sections[5].title).toBe(
      "Debugging, profiling, régression et Balance Lab",
    );
    expect(french.projectRegistry["moba-gas"].page.course?.title).toBe(
      "Multiplayer in Unreal with GAS and AWS Dedicated Servers",
    );
    expect(french.projectRegistry.tlm.technologies).toContain("Godot 4.7");
    expect(
      french.projectRegistry["hospital-flow"].page.sections[0].sourceReferences?.[0],
    ).toMatchObject({
      label: "AEMRHubGameMode",
      file: "Source/LOD/Private/Framework/EMRHubGameMode.cpp",
    });
  });

  it("supports a QA Tester role variant without changing project content", () => {
    const defaultContent = getPortfolioContent("en");
    const qaContent = getPortfolioContent("en", "qa-tester");
    const frenchContent = getPortfolioContent("fr");

    expect(defaultContent.siteProfile.role).toBe("Junior Unreal Engine Programmer");
    expect(qaContent.siteProfile.role).toBe("Junior QA Tester");
    expect(frenchContent.siteProfile.availability).toBe("Immédiatement");
    for (const roleVariant of roleVariants) {
      expect(getPortfolioContent("fr", roleVariant).siteProfile.availability).toBe(
        frenchContent.siteProfile.availability,
      );
    }
    expect(qaContent.projects.map((project) => project.slug)).toEqual(
      defaultContent.projects.map((project) => project.slug),
    );
    expect(qaContent.projectRegistry["hospital-flow"].summary).toBe(
      defaultContent.projectRegistry["hospital-flow"].summary,
    );
  });

  it("maps every role variant to a downloadable CV asset", async () => {
    const expectedCvByRole = {
      "unreal-engine-programmer": "cv/baptiste-giquet-cv-unreal-programmer.pdf",
      "qa-tester": "cv/baptiste-giquet-cv-qa-tester.pdf",
      "ai-game-designer": "cv/baptiste-giquet-cv-ai-game-designer.pdf",
    } satisfies Record<(typeof roleVariants)[number], string>;

    for (const roleVariant of roleVariants) {
      const englishCv = getPortfolioContent("en", roleVariant).siteProfile.actions.find(
        (action) => action.id === "cv",
      );
      const frenchCv = getPortfolioContent("fr", roleVariant).siteProfile.actions.find(
        (action) => action.id === "cv",
      );

      expect(englishCv).toMatchObject({
        download: path.basename(expectedCvByRole[roleVariant]),
        href: expectedCvByRole[roleVariant],
        kind: "download",
        label: "Download CV",
        status: "ready",
      });
      expect(frenchCv).toMatchObject({
        download: path.basename(expectedCvByRole[roleVariant]),
        href: expectedCvByRole[roleVariant],
        kind: "download",
        label: "Télécharger le CV",
        status: "ready",
      });
      expect(englishCv?.metaLabel).toBeUndefined();
      expect(frenchCv?.metaLabel).toBeUndefined();
      await access(path.resolve("public", expectedCvByRole[roleVariant]));
    }
  });

  it("keeps media metadata content-driven for placeholder and future ready assets", () => {
    for (const project of projects) {
      const mediaSlotIds = new Set(project.mediaSlots.map((slot) => slot.id));

      for (const slot of project.mediaSlots) {
        expect(slot.id).toBeTruthy();
        expect(slot.type).toMatch(/image|video/);
        expect(slot.aspectRatio).toMatch(/16 \/ 9|4 \/ 3/);
        expect(slot.status).toMatch(/placeholder|ready/);
        expect(slot.caption).toBeTruthy();

        if (slot.status === "ready") {
          expect(slot.source).toBeTruthy();
        }

        if (slot.status === "placeholder") {
          expect(slot.source).toBeUndefined();
          expect(slot.poster).toBeUndefined();
        }
      }

      for (const section of project.page.sections) {
        if (section.mediaSlotId) {
          expect(
            mediaSlotIds.has(section.mediaSlotId),
            `${project.slug}: ${section.title} references missing media slot ${section.mediaSlotId}`,
          ).toBe(true);
        }
      }
    }
  });

  it("keeps ready actions linkable and pending actions non-navigable across project and profile content", () => {
    const actionGroups: ProjectAction[][] = [
      siteProfile.actions,
      ...projects.map((project) => project.actions),
    ];

    for (const actions of actionGroups) {
      for (const action of actions) {
        expect(action.id).toBeTruthy();
        expect(action.label).toBeTruthy();

        if (action.status === "ready") {
          expect(action.href).toMatch(/^(#\/|https:\/\/|cv\/)/);
        }

        if (action.status === "pending") {
          expect(action.href).toBeUndefined();
          expect(action.pendingReason).toBeTruthy();
        }

        if (action.opensInNewTab) {
          expect(action.href).toMatch(/^https:\/\//);
        }
      }
    }
  });

  it("derives Browse source actions only from runtime source metadata", () => {
    const sourceEnabledSlugs = projects
      .filter((project) => project.source?.status === "available")
      .map((project) => project.slug);

    expect(sourceEnabledSlugs).toEqual([
      "hospital-flow",
      "agass",
      "tlm",
      "moba-gas",
      "slash-cpp",
    ]);

    for (const project of projects) {
      const sourceAction = project.actions.find((action) => action.id === "source");

      if (project.source) {
        expect(project.source.manifestPath).toBe(
          `source/${project.slug}/manifest.json`,
        );
        expect(sourceAction).toMatchObject({
          href: `#/projects/${project.slug}/source`,
          label: "Browse source",
          status: "ready",
        });
      } else {
        expect(sourceAction).toBeUndefined();
      }
    }
  });

  it("validates runtime source metadata against committed manifests", async () => {
    const allowedLanguages = new Set(["cpp", "cpp-header", "csharp"]);
    const allowedOwnership = new Set([
      "project-code",
      "project-plugin",
      "third-party-plugin",
    ]);

    for (const project of projects.filter((project) => project.source)) {
      const manifestPath = path.resolve("public", project.source!.manifestPath);
      const manifest = JSON.parse(
        await readFile(manifestPath, "utf8"),
      ) as SourceManifest;

      expect(manifest.schemaVersion).toBe(1);
      expect(manifest.projectSlug).toBe(project.slug);
      expect(Date.parse(manifest.exportedAt)).not.toBeNaN();
      expect(manifest.files.length).toBe(manifest.metrics.fileCount);
      expect(
        manifest.entryFile
          ? manifest.files.some((file) => file.path === manifest.entryFile)
          : true,
      ).toBe(true);

      const ownershipCounts = {
        "project-code": 0,
        "project-plugin": 0,
        "third-party-plugin": 0,
      };

      for (const file of manifest.files) {
        expect(file.path).not.toContain("\\");
        expect(file.snapshotPath).toBe(`files/${file.path}`);
        expect(allowedLanguages.has(file.language)).toBe(true);
        expect(allowedOwnership.has(file.ownership)).toBe(true);
        ownershipCounts[file.ownership as keyof typeof ownershipCounts] += 1;

        await expect(
          access(
            path.resolve(
              "public",
              "source",
              project.slug,
              file.snapshotPath,
            ),
          ),
        ).resolves.toBeUndefined();
      }

      expect(manifest.metrics.ownershipCounts).toEqual(ownershipCounts);
    }
  });

  it("validates code references against committed source manifests", async () => {
    for (const project of projects) {
      const references = collectProjectCodeReferences(project);

      if (!project.source) {
        expect(references).toHaveLength(0);
        continue;
      }

      const manifestPath = path.resolve("public", project.source.manifestPath);
      const manifest = JSON.parse(
        await readFile(manifestPath, "utf8"),
      ) as SourceManifest;
      const filesByPath = new Map(manifest.files.map((file) => [file.path, file]));

      for (const reference of references) {
        const file = filesByPath.get(reference.file);

        expect(file, `${project.slug}: ${reference.file}`).toBeTruthy();
        expect(reference.file).not.toContain("\\");

        if (reference.line) {
          expect(reference.line).toBeGreaterThan(0);
          expect(reference.line).toBeLessThanOrEqual(file?.lines ?? 0);
        }
      }
    }
  });

  it("derives public homepage groups from the canonical order", () => {
    expect(featuredOriginalProjects.map((project) => project.slug)).toEqual([
      "hospital-flow",
    ]);
    expect(learningProjects.map((project) => project.slug)).toEqual([
      "moba-gas",
      "commonui-frontend",
      "gas-crash-course",
      "multiplayer-crash-course",
      "unreal-2d",
      "slash-cpp",
      "cpp-game-development",
      "blueprint-prototypes",
    ]);
    expect(projectRegistry.agass.visibility).toBe("archived");
    expect(projectRegistry.tlm.visibility).toBe("archived");
  });

  it("keeps Unreal project downloads scoped to the requested projects", () => {
    const downloadableSlugs = projects
      .filter((project) =>
        project.actions.some((action) => action.kind === "download"),
      )
      .map((project) => project.slug);

    expect(downloadableSlugs).toEqual([
      "hospital-flow",
      "moba-gas",
      "slash-cpp",
    ]);

    for (const slug of downloadableSlugs) {
      const project = projectRegistry[slug];
      const [downloadAction, sourceAction] = project.actions;

      expect(downloadAction).toMatchObject({
        id: "download",
        label: "Download Unreal project",
        kind: "download",
        status: "ready",
        opensInNewTab: true,
      });
      expect(downloadAction.href).toMatch(/^https:\/\/1drv\.ms\//);
      expect(downloadAction.metaLabel).toMatch(/^UE 5\.[67], \d+\.\d{2} GiB ZIP$/);
      expect(sourceAction).toMatchObject({
        id: "source",
        label: "Browse source",
        kind: "source",
        status: "ready",
        href: `#/projects/${slug}/source`,
      });
    }
  });

  it("derives previous and next projects from public navigable projects", () => {
    expect(getAdjacentProjects("hospital-flow").previous).toBeUndefined();
    expect(getAdjacentProjects("hospital-flow").next?.slug).toBe("moba-gas");
    expect(getAdjacentProjects("agass").previous).toBeUndefined();
    expect(getAdjacentProjects("agass").next).toBeUndefined();
    expect(getAdjacentProjects("tlm").previous).toBeUndefined();
    expect(getAdjacentProjects("tlm").next).toBeUndefined();
    expect(getAdjacentProjects("moba-gas").previous?.slug).toBe("hospital-flow");
    expect(getAdjacentProjects("moba-gas").next?.slug).toBe("commonui-frontend");
    expect(getAdjacentProjects("commonui-frontend").next?.slug).toBe("gas-crash-course");
    expect(getAdjacentProjects("multiplayer-crash-course").next?.slug).toBe("unreal-2d");
    expect(getAdjacentProjects("unreal-2d").next?.slug).toBe("slash-cpp");
    expect(getAdjacentProjects("cpp-game-development").previous?.slug).toBe("slash-cpp");
    expect(getAdjacentProjects("cpp-game-development").next?.slug).toBe("blueprint-prototypes");
    expect(getAdjacentProjects("blueprint-prototypes").next).toBeUndefined();
  });

  it("keeps TLM as a complete production-evidence page with source-backed C# references", () => {
    const tlm = projectRegistry.tlm;

    expect(tlm.page.template).toBe("original");
    expect(tlm.actions.find((action) => action.id === "source")).toMatchObject({
      href: "#/projects/tlm/source",
      label: "Browse source",
      status: "ready",
    });
    expect(tlm.actions.find((action) => action.id === "build")).toBeUndefined();
    expect(tlm.mediaSlots.map((slot) => slot.id)).toEqual([
      "gameplay-loop",
      "spell-item-synergy",
      "horde-tower-defense",
      "day-night-market",
      "authoring-tools",
      "production-shell",
      "source-tooling-proof",
    ]);
    expect(tlm.mediaSlots.every((slot) => slot.status === "placeholder")).toBe(true);
    expect(tlm.page.sections.map((section) => section.title)).toEqual([
      "Production gameplay architecture",
      "Combat, spells, items, and build synergies",
      "Horde enemies, tower routing, and defenses",
      "Day/night run loop and market progression",
      "Content authoring and validation tools",
      "Debugging, profiling, regression, and Balance Lab",
      "Production shell, save slots, options, controller, and localization",
    ]);
    expect(tlm.page.overview).toMatch(/Godot 4\.7 C# FPS horde-survivor roguelite/);
    expect(tlm.page.roleScope).toMatch(/no public playable build/i);
    expect(tlm.page.roleScope).toMatch(/Steam integration is not presented as complete/i);
    expect(tlm.page.roleScope).toMatch(/Balance Lab is engineering evidence/i);
    expect(tlm.page.proof.join(" ")).toMatch(/Godot\.NET\.Sdk\/4\.7\.0-beta\.1/);
    expect(tlm.page.improvements.join(" ")).toMatch(/source references limited to committed C# files/i);
  });

  it("keeps every learning project complete and attributed", () => {
    const verifiedCourseTitles: Record<ProjectSlug, string | undefined> = {
      "hospital-flow": undefined,
      agass: undefined,
      tlm: undefined,
      "moba-gas": "Multiplayer in Unreal with GAS and AWS Dedicated Servers",
      "commonui-frontend": "Unreal Engine 5 C++: Advanced Frontend UI Programming",
      "unreal-2d": "The Ultimate 2D Top Down Unreal Engine Course",
      "gas-crash-course": "Unreal Engine 5 Gameplay Ability System (GAS) Crash Course",
      "multiplayer-crash-course": "Unreal Engine 5 C++ Multiplayer CRASH COURSE",
      "slash-cpp": "Unreal Engine 5 C++ The Ultimate Game Developer Course",
      "cpp-game-development": "Learn C++ for Game Development",
      "blueprint-prototypes": "Unreal Engine 5 Blueprints - The Ultimate Developer Course",
    };

    for (const project of learningProjects) {
      expect(project.page.template).toBe("learning");
      expect(project.page.course?.title).toBe(verifiedCourseTitles[project.slug]);
      expect(project.page.course?.provider).toBe("Udemy");
      expect(project.page.course?.instructor).toBeTruthy();
      expect(project.page.course?.url).toMatch(/^https:\/\/www\.udemy\.com\/course\//);
      expect(project.page.courseContext).toBeTruthy();
      expect(project.page.whatBuilt?.length).toBeGreaterThan(0);
      expect(project.page.whatLearned?.length).toBeGreaterThan(0);
      if (
        project.slug !== "moba-gas" &&
        project.slug !== "commonui-frontend" &&
        project.slug !== "unreal-2d" &&
        project.slug !== "gas-crash-course" &&
        project.slug !== "multiplayer-crash-course" &&
        project.slug !== "slash-cpp" &&
        project.slug !== "cpp-game-development" &&
        project.slug !== "blueprint-prototypes"
      ) {
        expect(project.page.scopeNotes?.length).toBeGreaterThan(0);
        expect(project.page.improvements.length).toBeGreaterThan(0);
      }
    }
  });
});
