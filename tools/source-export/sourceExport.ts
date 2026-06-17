import {
  cp,
  mkdir,
  readdir,
  readFile,
  rename,
  rm,
  stat,
  writeFile,
} from "node:fs/promises";
import path from "node:path";

export const SOURCE_MANIFEST_SCHEMA_VERSION = 1;
export const LARGE_SOURCE_FILE_BYTES = 1024 * 1024;

export type SourceLanguage = "cpp" | "cpp-header" | "csharp";
export type SourceOwnership =
  | "project-code"
  | "project-plugin"
  | "third-party-plugin";

export interface SourceOwnershipOverride {
  pathPrefix: string;
  ownership: SourceOwnership;
}

export interface SourceExportProjectConfig {
  slug: string;
  projectPath: string;
  sourceRoots: string[];
  entryFile?: string;
  ownershipOverrides?: SourceOwnershipOverride[];
}

export interface SourceManifestFile {
  path: string;
  snapshotPath: string;
  language: SourceLanguage;
  ownership: SourceOwnership;
  bytes: number;
  lines: number;
}

export interface SourceManifest {
  schemaVersion: typeof SOURCE_MANIFEST_SCHEMA_VERSION;
  projectSlug: string;
  exportedAt: string;
  sourceRoots: string[];
  entryFile?: string;
  metrics: {
    fileCount: number;
    totalBytes: number;
    totalLines: number;
    languageCounts: Record<SourceLanguage, number>;
    ownershipCounts: Record<SourceOwnership, number>;
  };
  files: SourceManifestFile[];
}

export interface SkippedSourceFile {
  path: string;
  reason: string;
  bytes?: number;
}

export interface SourceExportSummary {
  projectSlug: string;
  copiedFiles: number;
  skippedFiles: SkippedSourceFile[];
  warnings: string[];
  ownershipCounts: Record<SourceOwnership, number>;
  outputPath: string;
}

export interface SourceExportRunOptions {
  projects: SourceExportProjectConfig[];
  outputRoot: string;
  projectSlug?: string;
  now?: Date;
}

interface ValidatedProject {
  config: SourceExportProjectConfig;
  projectPath: string;
  sourceRoots: string[];
  outputPath: string;
}

const CODE_EXTENSIONS = new Set([".h", ".cpp", ".cs"]);
const EXCLUDED_DIRECTORY_NAMES = new Set([
  ".git",
  ".idea",
  ".vs",
  ".vscode",
  "binaries",
  "build",
  "cache",
  "config",
  "content",
  "cooked",
  "deriveddatacache",
  "dist",
  "docs",
  "documentation",
  "generated",
  "ide",
  "intermediate",
  "node_modules",
  "output",
  "outputs",
  "package",
  "packaged",
  "packages",
  "release",
  "releases",
  "saved",
]);

const createEmptyLanguageCounts = (): Record<SourceLanguage, number> => ({
  cpp: 0,
  "cpp-header": 0,
  csharp: 0,
});

const createEmptyOwnershipCounts = (): Record<SourceOwnership, number> => ({
  "project-code": 0,
  "project-plugin": 0,
  "third-party-plugin": 0,
});

export async function exportSourceProjects(
  options: SourceExportRunOptions,
): Promise<SourceExportSummary[]> {
  const selectedProjects = selectProjects(options.projects, options.projectSlug);
  const outputRoot = path.resolve(options.outputRoot);

  await mkdir(outputRoot, { recursive: true });

  const validatedProjects: ValidatedProject[] = [];
  for (const project of selectedProjects) {
    validatedProjects.push(await validateProject(project, outputRoot));
  }

  const summaries: SourceExportSummary[] = [];
  for (const project of validatedProjects) {
    summaries.push(await exportValidatedProject(project, outputRoot, options.now));
  }

  return summaries;
}

function selectProjects(
  projects: SourceExportProjectConfig[],
  projectSlug: string | undefined,
): SourceExportProjectConfig[] {
  if (!projectSlug) {
    return projects;
  }

  const project = projects.find((candidate) => candidate.slug === projectSlug);
  if (!project) {
    throw new Error(`Unknown source export project: ${projectSlug}`);
  }

  return [project];
}

async function validateProject(
  config: SourceExportProjectConfig,
  outputRoot: string,
): Promise<ValidatedProject> {
  if (!/^[a-z0-9][a-z0-9-]*$/.test(config.slug)) {
    throw new Error(`Invalid source export project slug: ${config.slug}`);
  }

  if (config.sourceRoots.length === 0) {
    throw new Error(`Source export project has no source roots: ${config.slug}`);
  }

  const projectPath = path.resolve(config.projectPath);
  const projectStats = await stat(projectPath).catch(() => undefined);
  if (!projectStats?.isDirectory()) {
    throw new Error(`Source project path does not exist: ${projectPath}`);
  }

  const sourceRoots: string[] = [];
  for (const sourceRoot of config.sourceRoots) {
    const resolvedRoot = path.resolve(projectPath, sourceRoot);
    ensureInside(projectPath, resolvedRoot, "source root", config.slug);

    const sourceRootStats = await stat(resolvedRoot).catch(() => undefined);
    if (!sourceRootStats?.isDirectory()) {
      throw new Error(`Source root does not exist: ${resolvedRoot}`);
    }

    sourceRoots.push(resolvedRoot);
  }

  const outputPath = path.resolve(outputRoot, config.slug);
  ensureInside(outputRoot, outputPath, "output target", config.slug);

  return { config, projectPath, sourceRoots, outputPath };
}

async function exportValidatedProject(
  project: ValidatedProject,
  outputRoot: string,
  now = new Date(),
): Promise<SourceExportSummary> {
  const tempOutputPath = path.resolve(
    outputRoot,
    `.${project.config.slug}.tmp-${process.pid}-${Date.now()}`,
  );
  ensureInside(outputRoot, tempOutputPath, "temporary output target", project.config.slug);

  const files: SourceManifestFile[] = [];
  const skippedFiles: SkippedSourceFile[] = [];
  const warnings: string[] = [];

  try {
    await mkdir(path.join(tempOutputPath, "files"), { recursive: true });

    for (const sourceRoot of project.sourceRoots) {
      await collectSourceFiles({
        directory: sourceRoot,
        projectPath: project.projectPath,
        outputPath: tempOutputPath,
        config: project.config,
        files,
        skippedFiles,
        warnings,
      });
    }

    files.sort((left, right) => left.path.localeCompare(right.path));

    const manifest = createManifest(
      project.config,
      project.projectPath,
      project.sourceRoots,
      files,
      now,
    );
    await writeFile(
      path.join(tempOutputPath, "manifest.json"),
      `${JSON.stringify(manifest, null, 2)}\n`,
      "utf8",
    );

    await rm(project.outputPath, { recursive: true, force: true });
    await mkdir(path.dirname(project.outputPath), { recursive: true });
    await renameDirectory(tempOutputPath, project.outputPath);

    return {
      projectSlug: project.config.slug,
      copiedFiles: files.length,
      skippedFiles,
      warnings,
      ownershipCounts: manifest.metrics.ownershipCounts,
      outputPath: project.outputPath,
    };
  } catch (error) {
    await rm(tempOutputPath, { recursive: true, force: true });
    throw error;
  }
}

async function collectSourceFiles(params: {
  directory: string;
  projectPath: string;
  outputPath: string;
  config: SourceExportProjectConfig;
  files: SourceManifestFile[];
  skippedFiles: SkippedSourceFile[];
  warnings: string[];
}) {
  const entries = await readdir(params.directory, { withFileTypes: true });

  for (const entry of entries) {
    const absolutePath = path.join(params.directory, entry.name);
    const logicalPath = toLogicalPath(params.projectPath, absolutePath);

    if (entry.isDirectory()) {
      if (isExcludedDirectory(entry.name)) {
        continue;
      }

      await collectSourceFiles({ ...params, directory: absolutePath });
      continue;
    }

    if (!entry.isFile()) {
      continue;
    }

    if (!isEligibleCodeFile(entry.name)) {
      continue;
    }

    const fileStats = await stat(absolutePath);
    if (fileStats.size > LARGE_SOURCE_FILE_BYTES) {
      const skippedFile = {
        path: logicalPath,
        reason: "Source file is larger than 1 MB.",
        bytes: fileStats.size,
      };
      params.skippedFiles.push(skippedFile);
      params.warnings.push(`${logicalPath} skipped: ${skippedFile.reason}`);
      continue;
    }

    const sourceBytes = await readFile(absolutePath);
    const manifestFile: SourceManifestFile = {
      path: logicalPath,
      snapshotPath: `files/${logicalPath}`,
      language: getLanguage(entry.name),
      ownership: classifyOwnership(logicalPath, params.config.ownershipOverrides ?? []),
      bytes: sourceBytes.byteLength,
      lines: countSourceLines(sourceBytes),
    };

    const outputFilePath = path.resolve(params.outputPath, manifestFile.snapshotPath);
    ensureInside(params.outputPath, outputFilePath, "source file output", params.config.slug);
    await mkdir(path.dirname(outputFilePath), { recursive: true });
    await writeFile(outputFilePath, sourceBytes);
    params.files.push(manifestFile);
  }
}

function createManifest(
  config: SourceExportProjectConfig,
  projectPath: string,
  sourceRoots: string[],
  files: SourceManifestFile[],
  now: Date,
): SourceManifest {
  const languageCounts = createEmptyLanguageCounts();
  const ownershipCounts = createEmptyOwnershipCounts();
  let totalBytes = 0;
  let totalLines = 0;

  for (const file of files) {
    languageCounts[file.language] += 1;
    ownershipCounts[file.ownership] += 1;
    totalBytes += file.bytes;
    totalLines += file.lines;
  }

  const normalizedEntryFile = config.entryFile
    ? normalizeLogicalPath(config.entryFile)
    : undefined;

  return {
    schemaVersion: SOURCE_MANIFEST_SCHEMA_VERSION,
    projectSlug: config.slug,
    exportedAt: now.toISOString(),
    sourceRoots: sourceRoots.map((sourceRoot) => toLogicalPath(projectPath, sourceRoot)),
    ...(normalizedEntryFile ? { entryFile: normalizedEntryFile } : {}),
    metrics: {
      fileCount: files.length,
      totalBytes,
      totalLines,
      languageCounts,
      ownershipCounts,
    },
    files,
  };
}

function isEligibleCodeFile(fileName: string): boolean {
  const extension = path.extname(fileName).toLowerCase();
  if (!CODE_EXTENSIONS.has(extension)) {
    return false;
  }

  return !/(\.generated\.h|\.Build\.cs|\.Target\.cs)$/i.test(fileName);
}

function isExcludedDirectory(directoryName: string): boolean {
  const normalizedName = directoryName.toLowerCase();
  return EXCLUDED_DIRECTORY_NAMES.has(normalizedName);
}

function getLanguage(fileName: string): SourceLanguage {
  const extension = path.extname(fileName).toLowerCase();

  if (extension === ".cpp") {
    return "cpp";
  }

  if (extension === ".cs") {
    return "csharp";
  }

  return "cpp-header";
}

function classifyOwnership(
  logicalPath: string,
  overrides: SourceOwnershipOverride[],
): SourceOwnership {
  const normalizedPath = normalizeLogicalPath(logicalPath).toLowerCase();

  for (const override of overrides) {
    const prefix = normalizeLogicalPath(override.pathPrefix).toLowerCase();
    if (normalizedPath === prefix || normalizedPath.startsWith(`${prefix}/`)) {
      return override.ownership;
    }
  }

  return normalizedPath.startsWith("plugins/")
    ? "third-party-plugin"
    : "project-code";
}

export function countSourceLines(sourceBytes: Buffer): number {
  if (sourceBytes.byteLength === 0) {
    return 0;
  }

  let lineBreaks = 0;
  for (const byte of sourceBytes) {
    if (byte === 10) {
      lineBreaks += 1;
    }
  }

  return sourceBytes[sourceBytes.byteLength - 1] === 10
    ? lineBreaks
    : lineBreaks + 1;
}

function toLogicalPath(rootPath: string, absolutePath: string): string {
  return normalizeLogicalPath(path.relative(rootPath, absolutePath));
}

function normalizeLogicalPath(inputPath: string): string {
  return inputPath.split(path.sep).join("/").replaceAll("\\", "/");
}

function ensureInside(
  rootPath: string,
  targetPath: string,
  targetLabel: string,
  projectSlug: string,
) {
  const relativePath = path.relative(rootPath, targetPath);
  if (
    relativePath === "" ||
    (!relativePath.startsWith("..") && !path.isAbsolute(relativePath))
  ) {
    return;
  }

  throw new Error(
    `Unsafe ${targetLabel} for source export project ${projectSlug}: ${targetPath}`,
  );
}

async function renameDirectory(from: string, to: string) {
  try {
    await rename(from, to);
  } catch (error) {
    if (!isRecoverableRenameError(error)) {
      throw error;
    }

    await cp(from, to, { recursive: true });
    await rm(from, { recursive: true, force: true });
  }
}

function isRecoverableRenameError(error: unknown) {
  return (
    error instanceof Error &&
    "code" in error &&
    (error.code === "EPERM" || error.code === "EXDEV")
  );
}
