import path from "node:path";
import { pathToFileURL } from "node:url";
import { exportSourceProjects } from "./sourceExport.ts";
import { sourceExportProjects } from "./projects.ts";

interface CliOptions {
  projectSlug?: string;
  outputRoot: string;
  help: boolean;
}

export async function runSourceExportCli(
  argv = process.argv.slice(2),
  cwd = process.cwd(),
) {
  const options = parseArgs(argv, cwd);

  if (options.help) {
    printHelp();
    return;
  }

  if (sourceExportProjects.length === 0) {
    console.log(
      "No source export projects are configured yet. Confirm project roots in issue #23 before exporting real snapshots.",
    );
    return;
  }

  const summaries = await exportSourceProjects({
    projects: sourceExportProjects,
    outputRoot: options.outputRoot,
    projectSlug: options.projectSlug,
  });

  for (const summary of summaries) {
    console.log(`\n${summary.projectSlug}`);
    console.log(`  copied: ${summary.copiedFiles}`);
    console.log(`  skipped: ${summary.skippedFiles.length}`);
    console.log(`  output: ${summary.outputPath}`);
    console.log(
      `  ownership: project-code=${summary.ownershipCounts["project-code"]}, project-plugin=${summary.ownershipCounts["project-plugin"]}, third-party-plugin=${summary.ownershipCounts["third-party-plugin"]}`,
    );

    for (const warning of summary.warnings) {
      console.warn(`  warning: ${warning}`);
    }
  }
}

function parseArgs(argv: string[], cwd: string): CliOptions {
  const options: CliOptions = {
    outputRoot: path.resolve(cwd, "public", "source"),
    help: false,
  };

  for (let index = 0; index < argv.length; index += 1) {
    const arg = argv[index];

    if (arg === "--help" || arg === "-h") {
      options.help = true;
      continue;
    }

    if (arg === "--project") {
      options.projectSlug = readValue(argv, index, arg);
      index += 1;
      continue;
    }

    if (arg === "--output") {
      options.outputRoot = path.resolve(cwd, readValue(argv, index, arg));
      index += 1;
      continue;
    }

    throw new Error(`Unknown source export argument: ${arg}`);
  }

  return options;
}

function readValue(argv: string[], index: number, optionName: string): string {
  const value = argv[index + 1];
  if (!value || value.startsWith("-")) {
    throw new Error(`${optionName} requires a value.`);
  }

  return value;
}

function printHelp() {
  console.log(`Manual source snapshot export

Usage:
  npm run export:source
  npm run export:source -- --project <slug>
  npm run export:source -- --output <directory>

Snapshots are written to public/source/<project-slug>/ by default.`);
}

if (import.meta.url === pathToFileURL(process.argv[1]).href) {
  runSourceExportCli().catch((error: unknown) => {
    console.error(error instanceof Error ? error.message : error);
    process.exitCode = 1;
  });
}
