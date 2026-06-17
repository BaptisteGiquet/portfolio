import { mkdtemp, mkdir, readFile, rm, stat, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { describe, expect, it } from "vitest";
import {
  LARGE_SOURCE_FILE_BYTES,
  countSourceLines,
  exportSourceProjects,
  type SourceExportProjectConfig,
  type SourceManifest,
} from "./sourceExport";

describe("source export", () => {
  it("exports eligible C++ source files with manifest metadata and preserved content", async () => {
    const workspace = await createWorkspace();
    const projectPath = path.join(workspace, "Project");
    const outputRoot = path.join(workspace, "public", "source");

    await writeProjectFile(projectPath, "Source/Game/Player.h", "line 1\r\nline 2\r\n");
    await writeProjectFile(projectPath, "Source/Game/Player.cpp", "one\ntwo");
    await writeProjectFile(projectPath, "Source/Game/Player.generated.h", "generated");
    await writeProjectFile(projectPath, "Source/Game/Game.Build.cs", "build file");
    await writeProjectFile(projectPath, "Source/Game/Game.Target.cs", "target file");
    await writeProjectFile(projectPath, "Source/Game/Build/Build.cpp", "build dir");
    await writeProjectFile(projectPath, "Source/Game/Cache/Cache.cpp", "cache dir");
    await writeProjectFile(projectPath, "Source/Game/Generated/Noise.cpp", "generated dir");
    await writeProjectFile(projectPath, "Source/Game/IDE/Workspace.cpp", "IDE dir");
    await writeProjectFile(projectPath, "Source/Game/Intermediate/Cache.cpp", "cache");
    await writeProjectFile(projectPath, "Source/Game/Output/Output.cpp", "output dir");
    await writeProjectFile(projectPath, "Source/Game/Package/Package.cpp", "package dir");
    await writeProjectFile(projectPath, "Source/Game/Release/Release.cpp", "release dir");
    await writeProjectFile(projectPath, "Content/Accidental.cpp", "asset folder");

    const [summary] = await exportSourceProjects({
      projects: [createProjectConfig(projectPath)],
      outputRoot,
      now: new Date("2026-06-01T10:00:00.000Z"),
    });

    expect(summary.copiedFiles).toBe(2);
    expect(summary.skippedFiles).toEqual([]);

    const copiedHeader = await readFile(
      path.join(outputRoot, "hospital-flow", "files", "Source", "Game", "Player.h"),
      "utf8",
    );
    expect(copiedHeader).toBe("line 1\r\nline 2\r\n");

    const manifest = await readManifest(outputRoot, "hospital-flow");
    expect(manifest).toMatchObject({
      schemaVersion: 1,
      projectSlug: "hospital-flow",
      exportedAt: "2026-06-01T10:00:00.000Z",
      sourceRoots: ["Source"],
      entryFile: "Source/Game/Player.cpp",
      metrics: {
        fileCount: 2,
        languageCounts: { cpp: 1, "cpp-header": 1 },
        ownershipCounts: {
          "project-code": 2,
          "project-plugin": 0,
          "third-party-plugin": 0,
        },
      },
    });
    expect(manifest.files.map((file) => file.path)).toEqual([
      "Source/Game/Player.cpp",
      "Source/Game/Player.h",
    ]);
    expect(manifest.files[0]).not.toHaveProperty("content");
    expect(manifest.files[0].path).not.toContain("\\");
    expect(manifest.files[0].snapshotPath).toBe("files/Source/Game/Player.cpp");
    expect(manifest.metrics.totalLines).toBe(4);
  });

  it("exports eligible C# source files while preserving Unreal metadata exclusions", async () => {
    const workspace = await createWorkspace();
    const projectPath = path.join(workspace, "Project");
    const outputRoot = path.join(workspace, "public", "source");

    await writeProjectFile(projectPath, "src/Gameplay/Run/RunControllerNode.cs", "class RunControllerNode {}\n");
    await writeProjectFile(projectPath, "src/Gameplay/Combat/CombatSystem.cs", "class CombatSystem {}");
    await writeProjectFile(projectPath, "src/Gameplay/Game.Build.cs", "build metadata");
    await writeProjectFile(projectPath, "src/Gameplay/Game.Target.cs", "target metadata");

    const [summary] = await exportSourceProjects({
      projects: [
        createProjectConfig(projectPath, {
          slug: "tlm",
          sourceRoots: ["src"],
          entryFile: "src/Gameplay/Run/RunControllerNode.cs",
        }),
      ],
      outputRoot,
      now: new Date("2026-06-02T12:00:00.000Z"),
    });

    expect(summary.copiedFiles).toBe(2);
    expect(summary.skippedFiles).toEqual([]);

    const manifest = await readManifest(outputRoot, "tlm");
    expect(manifest).toMatchObject({
      schemaVersion: 1,
      projectSlug: "tlm",
      sourceRoots: ["src"],
      entryFile: "src/Gameplay/Run/RunControllerNode.cs",
      metrics: {
        fileCount: 2,
        languageCounts: { cpp: 0, "cpp-header": 0, csharp: 2 },
        ownershipCounts: {
          "project-code": 2,
          "project-plugin": 0,
          "third-party-plugin": 0,
        },
      },
    });
    expect(manifest.files.map((file) => file.path)).toEqual([
      "src/Gameplay/Combat/CombatSystem.cs",
      "src/Gameplay/Run/RunControllerNode.cs",
    ]);
    expect(manifest.files.every((file) => file.language === "csharp")).toBe(true);
    expect(
      manifest.files.some((file) => /\.(Build|Target)\.cs$/.test(file.path)),
    ).toBe(false);
  });

  it("applies ownership overrides before plugin defaults", async () => {
    const workspace = await createWorkspace();
    const projectPath = path.join(workspace, "Project");
    const outputRoot = path.join(workspace, "public", "source");

    await writeProjectFile(projectPath, "Source/Game/Core.cpp", "project");
    await writeProjectFile(
      projectPath,
      "Plugins/AuthoredPlugin/Source/Authored.cpp",
      "authored",
    );
    await writeProjectFile(
      projectPath,
      "Plugins/UnknownPlugin/Source/Unknown.cpp",
      "third party",
    );

    const config = createProjectConfig(projectPath, {
      sourceRoots: ["Source", "Plugins"],
      ownershipOverrides: [
        {
          pathPrefix: "Plugins/AuthoredPlugin",
          ownership: "project-plugin",
        },
      ],
    });

    await exportSourceProjects({ projects: [config], outputRoot });

    const manifest = await readManifest(outputRoot, "hospital-flow");
    const ownershipByPath = Object.fromEntries(
      manifest.files.map((file) => [file.path, file.ownership]),
    );

    expect(ownershipByPath["Source/Game/Core.cpp"]).toBe("project-code");
    expect(ownershipByPath["Plugins/AuthoredPlugin/Source/Authored.cpp"]).toBe(
      "project-plugin",
    );
    expect(ownershipByPath["Plugins/UnknownPlugin/Source/Unknown.cpp"]).toBe(
      "third-party-plugin",
    );
    expect(manifest.metrics.ownershipCounts).toEqual({
      "project-code": 1,
      "project-plugin": 1,
      "third-party-plugin": 1,
    });
  });

  it("validates source paths before replacing an existing snapshot", async () => {
    const workspace = await createWorkspace();
    const projectPath = path.join(workspace, "Project");
    const outputRoot = path.join(workspace, "public", "source");
    const existingFile = path.join(outputRoot, "hospital-flow", "files", "keep.cpp");

    await mkdir(path.dirname(existingFile), { recursive: true });
    await writeFile(existingFile, "existing");
    await mkdir(projectPath, { recursive: true });

    await expect(
      exportSourceProjects({
        projects: [createProjectConfig(projectPath, { sourceRoots: ["Missing"] })],
        outputRoot,
      }),
    ).rejects.toThrow(/Source root does not exist/);

    await expect(readFile(existingFile, "utf8")).resolves.toBe("existing");
  });

  it("replaces a project snapshot instead of leaving stale copied files", async () => {
    const workspace = await createWorkspace();
    const projectPath = path.join(workspace, "Project");
    const outputRoot = path.join(workspace, "public", "source");
    const oldSourcePath = path.join(projectPath, "Source", "Game", "Old.cpp");

    await writeProjectFile(projectPath, "Source/Game/Old.cpp", "old");
    await exportSourceProjects({
      projects: [createProjectConfig(projectPath)],
      outputRoot,
    });

    await rm(oldSourcePath);
    await writeProjectFile(projectPath, "Source/Game/New.cpp", "new");
    await exportSourceProjects({
      projects: [createProjectConfig(projectPath)],
      outputRoot,
    });

    await expect(
      stat(path.join(outputRoot, "hospital-flow", "files", "Source", "Game", "Old.cpp")),
    ).rejects.toThrow();
    await expect(
      readFile(
        path.join(outputRoot, "hospital-flow", "files", "Source", "Game", "New.cpp"),
        "utf8",
      ),
    ).resolves.toBe("new");
  });

  it("skips files larger than one megabyte with terminal summary warnings", async () => {
    const workspace = await createWorkspace();
    const projectPath = path.join(workspace, "Project");
    const outputRoot = path.join(workspace, "public", "source");
    const largeSource = Buffer.alloc(LARGE_SOURCE_FILE_BYTES + 1, "a");

    await writeProjectFile(projectPath, "Source/Game/Huge.cpp", largeSource);
    await writeProjectFile(projectPath, "Source/Game/Small.cpp", "small");

    const [summary] = await exportSourceProjects({
      projects: [createProjectConfig(projectPath)],
      outputRoot,
    });

    expect(summary.copiedFiles).toBe(1);
    expect(summary.skippedFiles).toEqual([
      {
        path: "Source/Game/Huge.cpp",
        reason: "Source file is larger than 1 MB.",
        bytes: LARGE_SOURCE_FILE_BYTES + 1,
      },
    ]);
    expect(summary.warnings[0]).toContain("Source/Game/Huge.cpp skipped");
    await expect(
      stat(path.join(outputRoot, "hospital-flow", "files", "Source", "Game", "Huge.cpp")),
    ).rejects.toThrow();
  });

  it("can export every configured project or one requested project", async () => {
    const workspace = await createWorkspace();
    const hospitalPath = path.join(workspace, "Hospital");
    const agassPath = path.join(workspace, "AGASS");
    const outputRoot = path.join(workspace, "public", "source");

    await writeProjectFile(hospitalPath, "Source/Hospital/Hospital.cpp", "hospital");
    await writeProjectFile(agassPath, "Source/AGASS/AGASS.cpp", "agass");

    const projects: SourceExportProjectConfig[] = [
      createProjectConfig(hospitalPath, { slug: "hospital-flow" }),
      createProjectConfig(agassPath, { slug: "agass" }),
    ];

    const selectedSummaries = await exportSourceProjects({
      projects,
      outputRoot,
      projectSlug: "agass",
    });

    expect(selectedSummaries.map((summary) => summary.projectSlug)).toEqual(["agass"]);
    await expect(stat(path.join(outputRoot, "agass", "manifest.json"))).resolves.toBeTruthy();
    await expect(stat(path.join(outputRoot, "hospital-flow", "manifest.json"))).rejects.toThrow();

    const allSummaries = await exportSourceProjects({ projects, outputRoot });
    expect(allSummaries.map((summary) => summary.projectSlug)).toEqual([
      "hospital-flow",
      "agass",
    ]);
  });

  it("rejects unsafe project slugs and source roots outside the project", async () => {
    const workspace = await createWorkspace();
    const projectPath = path.join(workspace, "Project");
    const outsidePath = path.join(workspace, "Outside");
    const outputRoot = path.join(workspace, "public", "source");

    await writeProjectFile(projectPath, "Source/Game/Core.cpp", "project");
    await writeProjectFile(outsidePath, "Engine.cpp", "outside");

    await expect(
      exportSourceProjects({
        projects: [createProjectConfig(projectPath, { slug: "../bad" })],
        outputRoot,
      }),
    ).rejects.toThrow(/Invalid source export project slug/);

    await expect(
      exportSourceProjects({
        projects: [
          createProjectConfig(projectPath, {
            sourceRoots: [path.relative(projectPath, outsidePath)],
          }),
        ],
        outputRoot,
      }),
    ).rejects.toThrow(/Unsafe source root/);
  });

  it("counts CRLF and LF source lines without rewriting text", () => {
    expect(countSourceLines(Buffer.from(""))).toBe(0);
    expect(countSourceLines(Buffer.from("one"))).toBe(1);
    expect(countSourceLines(Buffer.from("one\n"))).toBe(1);
    expect(countSourceLines(Buffer.from("one\ntwo"))).toBe(2);
    expect(countSourceLines(Buffer.from("one\r\ntwo\r\n"))).toBe(2);
  });
});

async function createWorkspace() {
  return mkdtemp(path.join(os.tmpdir(), "portfolio-source-export-"));
}

function createProjectConfig(
  projectPath: string,
  overrides: Partial<SourceExportProjectConfig> = {},
): SourceExportProjectConfig {
  return {
    slug: "hospital-flow",
    projectPath,
    sourceRoots: ["Source"],
    entryFile: "Source/Game/Player.cpp",
    ...overrides,
  };
}

async function writeProjectFile(
  projectPath: string,
  relativePath: string,
  content: string | Buffer,
) {
  const filePath = path.join(projectPath, relativePath);
  await mkdir(path.dirname(filePath), { recursive: true });
  await writeFile(filePath, content);
}

async function readManifest(
  outputRoot: string,
  slug: string,
): Promise<SourceManifest> {
  const manifestText = await readFile(
    path.join(outputRoot, slug, "manifest.json"),
    "utf8",
  );
  return JSON.parse(manifestText) as SourceManifest;
}
