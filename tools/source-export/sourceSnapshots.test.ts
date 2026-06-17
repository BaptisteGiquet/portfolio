import { access, readFile } from "node:fs/promises";
import path from "node:path";
import { describe, expect, it } from "vitest";
import {
  SOURCE_MANIFEST_SCHEMA_VERSION,
  type SourceManifest,
  type SourceOwnership,
} from "./sourceExport";

const snapshotRoot = path.resolve("public", "source");

const expectedSnapshots = [
  {
    slug: "hospital-flow",
    sourceRoots: ["Source", "Plugins"],
    entryFile: "Source/LOD/Public/Characters/Player/EMRPlayerController.h",
    ownershipForPath(path: string): SourceOwnership {
      return path.startsWith("Plugins/") ? "third-party-plugin" : "project-code";
    },
  },
  {
    slug: "agass",
    sourceRoots: ["Source", "Plugins"],
    entryFile: "Source/AGASS/Public/AGASSPlayerController.h",
    ownershipForPath(path: string): SourceOwnership {
      return path.startsWith("Plugins/") ? "project-plugin" : "project-code";
    },
  },
  {
    slug: "tlm",
    sourceRoots: [
      "src",
      "addons/the_last_mage_tools",
      "addons/TLMRegressionTools",
    ],
    entryFile: "src/Gameplay/Run/RunControllerNode.cs",
    ownershipForPath(path: string): SourceOwnership {
      return path.startsWith("addons/") ? "project-plugin" : "project-code";
    },
  },
  {
    slug: "moba-gas",
    sourceRoots: ["Source"],
    entryFile: "Source/LOD/Private/Player/MobaPlayerController.h",
    ownershipForPath(): SourceOwnership {
      return "project-code";
    },
  },
  {
    slug: "slash-cpp",
    sourceRoots: ["Source"],
    entryFile: "Source/Slash/Public/Characters/SlashMyCharacter.h",
    ownershipForPath(): SourceOwnership {
      return "project-code";
    },
  },
] satisfies Array<{
  slug: string;
  sourceRoots: string[];
  entryFile: string;
  ownershipForPath(path: string): SourceOwnership;
}>;

describe("committed source snapshots", () => {
  for (const expected of expectedSnapshots) {
    it(`validates the ${expected.slug} source manifest and files`, async () => {
      const manifest = await readManifest(expected.slug);

      expect(manifest.schemaVersion).toBe(SOURCE_MANIFEST_SCHEMA_VERSION);
      expect(manifest.projectSlug).toBe(expected.slug);
      expect(manifest.sourceRoots).toEqual(expected.sourceRoots);
      expect(manifest.entryFile).toBe(expected.entryFile);
      expect(manifest.files.length).toBe(manifest.metrics.fileCount);
      expect(manifest.files.some((file) => file.path === expected.entryFile)).toBe(
        true,
      );

      let totalBytes = 0;
      let totalLines = 0;
      const languageCounts = Object.fromEntries(
        Object.keys(manifest.metrics.languageCounts).map((language) => [
          language,
          0,
        ]),
      );
      const ownershipCounts = {
        "project-code": 0,
        "project-plugin": 0,
        "third-party-plugin": 0,
      };

      for (const file of manifest.files) {
        expect(file).not.toHaveProperty("content");
        expect(file.path).not.toContain("\\");
        expect(file.snapshotPath).not.toContain("\\");
        expect(file.snapshotPath).toBe(`files/${file.path}`);
        expect(file.path).not.toMatch(/(^|\/)(Engine|Intermediate|Saved)\//i);
        expect(file.path).not.toMatch(/\.(Build|Target)\.cs$/);
        expect(file.path).not.toMatch(/(\.generated\.h|\.gen\.cpp)$/i);
        expect(["cpp", "cpp-header", "csharp"]).toContain(file.language);
        expect(file.ownership).toBe(expected.ownershipForPath(file.path));

        totalBytes += file.bytes;
        totalLines += file.lines;
        languageCounts[file.language] = (languageCounts[file.language] ?? 0) + 1;
        ownershipCounts[file.ownership] += 1;

        await expect(
          access(path.join(snapshotRoot, expected.slug, file.snapshotPath)),
        ).resolves.toBeUndefined();
      }

      expect(manifest.metrics.totalBytes).toBe(totalBytes);
      expect(manifest.metrics.totalLines).toBe(totalLines);
      expect(manifest.metrics.languageCounts).toEqual(languageCounts);
      expect(manifest.metrics.ownershipCounts).toEqual(ownershipCounts);
    });
  }

  it("validates the TLM C# snapshot scope", async () => {
    const manifest = await readManifest("tlm");
    const paths = manifest.files.map((file) => file.path);

    expect(manifest.metrics.languageCounts.csharp).toBeGreaterThan(0);
    expect(manifest.metrics.languageCounts.cpp).toBe(0);
    expect(manifest.metrics.languageCounts["cpp-header"]).toBe(0);
    expect(manifest.metrics.ownershipCounts["project-code"]).toBeGreaterThan(0);
    expect(manifest.metrics.ownershipCounts["project-plugin"]).toBeGreaterThan(0);
    expect(paths).toContain("src/Gameplay/Run/RunControllerNode.cs");
    expect(paths).toContain("addons/the_last_mage_tools/TheLastMageToolsPlugin.cs");
    expect(paths).toContain("addons/TLMRegressionTools/TLMRegressionToolsPlugin.cs");
    expect(paths.some((file) => file.startsWith("addons/CodexBridge/"))).toBe(false);
    expect(paths.some((file) => file.startsWith("docs/"))).toBe(false);
    expect(paths.some((file) => file.startsWith("Saved/"))).toBe(false);
    expect(paths.some((file) => file === "test.cs")).toBe(false);
  });

  it("validates the learning Unreal C++ snapshot scopes", async () => {
    const mobaManifest = await readManifest("moba-gas");
    const mobaPaths = mobaManifest.files.map((file) => file.path);
    expect(mobaManifest.metrics.languageCounts.cpp).toBeGreaterThan(0);
    expect(mobaManifest.metrics.languageCounts["cpp-header"]).toBeGreaterThan(0);
    expect(mobaManifest.metrics.languageCounts.csharp).toBe(0);
    expect(mobaPaths).toContain("Source/LOD/Private/GAS/MobaAbilitySystemComponent.h");
    expect(mobaPaths).toContain("Source/LOD/Private/Inventory/MobaInventoryComponent.cpp");
    expect(mobaPaths.some((file) => file.startsWith("Plugins/"))).toBe(false);

    const slashManifest = await readManifest("slash-cpp");
    const slashPaths = slashManifest.files.map((file) => file.path);
    expect(slashManifest.metrics.languageCounts.cpp).toBeGreaterThan(0);
    expect(slashManifest.metrics.languageCounts["cpp-header"]).toBeGreaterThan(0);
    expect(slashManifest.metrics.languageCounts.csharp).toBe(0);
    expect(slashPaths).toContain("Source/Slash/Public/Characters/SlashMyCharacter.h");
    expect(slashPaths).toContain("Source/Slash/Private/Enemy/Enemy.cpp");
    expect(slashPaths.some((file) => file.startsWith("Content/"))).toBe(false);
  });
});

async function readManifest(slug: string): Promise<SourceManifest> {
  const text = await readFile(
    path.join(snapshotRoot, slug, "manifest.json"),
    "utf8",
  );

  return JSON.parse(text) as SourceManifest;
}
