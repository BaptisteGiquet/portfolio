import {
  getPortfolioContent,
  type PortfolioProject,
  type ProjectSlug,
} from "./content/portfolio";
import {
  defaultLanguage,
  defaultRoleVariant,
  type Language,
  type RoleVariant,
} from "./content/types";

export type AppRoute =
  | {
      name: "home";
      key: string;
      language: Language;
      roleVariant: RoleVariant;
    }
  | {
      name: "project";
      key: string;
      language: Language;
      roleVariant: RoleVariant;
      project: PortfolioProject;
    }
  | {
      name: "source";
      key: string;
      language: Language;
      roleVariant: RoleVariant;
      project: PortfolioProject;
      sourceAddress: SourceAddress;
    }
  | {
      name: "not-found";
      key: string;
      language: Language;
      roleVariant: RoleVariant;
      slug?: string;
    };

export type SourceAddress = {
  file?: string;
  line?: number;
};

function isSupportedLanguage(value: string | null): value is Language {
  return value === "en" || value === "fr";
}

function isSupportedRoleVariant(value: string): value is RoleVariant {
  return (
    value === "unreal-engine-programmer" ||
    value === "qa-tester" ||
    value === "ai-game-designer"
  );
}

function parseLanguage(query = ""): Language {
  const params = new URLSearchParams(query);
  const rawLanguage = params.get("lang");

  return isSupportedLanguage(rawLanguage) ? rawLanguage : defaultLanguage;
}

function appendLanguageQuery(params: URLSearchParams, language: Language) {
  if (language === "fr") {
    params.set("lang", language);
  }
}

function hashQuery(params: URLSearchParams) {
  const query = params.toString();

  return query ? `?${query}` : "";
}

export const projectPath = (
  slug: ProjectSlug,
  language: Language = defaultLanguage,
  roleVariant: RoleVariant = defaultRoleVariant,
) => {
  const params = new URLSearchParams();

  appendLanguageQuery(params, language);

  const basePath =
    roleVariant === defaultRoleVariant
      ? `#/projects/${slug}`
      : `#/roles/${roleVariant}/projects/${slug}`;

  return `${basePath}${hashQuery(params)}` as const;
};

export const roleHomePath = (
  roleVariant: RoleVariant = defaultRoleVariant,
  language: Language = defaultLanguage,
) => {
  const params = new URLSearchParams();

  appendLanguageQuery(params, language);

  const basePath =
    roleVariant === defaultRoleVariant ? "#/" : `#/roles/${roleVariant}`;

  return `${basePath}${hashQuery(params)}` as const;
};

function normalizeSourceFilePath(file: string): string {
  return file.replace(/\\/g, "/");
}

function isValidLineNumber(line: number | undefined): line is number {
  return (
    typeof line === "number" &&
    Number.isSafeInteger(line) &&
    line > 0
  );
}

export const projectSourcePath = (
  slug: ProjectSlug,
  sourceAddress: SourceAddress = {},
  language: Language = defaultLanguage,
  roleVariant: RoleVariant = defaultRoleVariant,
) => {
  const params = new URLSearchParams();

  appendLanguageQuery(params, language);

  if (sourceAddress.file) {
    params.set(
      "file",
      normalizeSourceFilePath(sourceAddress.file),
    );
  }

  if (isValidLineNumber(sourceAddress.line)) {
    params.set("line", String(sourceAddress.line));
  }

  const basePath =
    roleVariant === defaultRoleVariant
      ? `#/projects/${slug}/source`
      : `#/roles/${roleVariant}/projects/${slug}/source`;

  return `${basePath}${hashQuery(params)}` as const;
};

function parseLine(value: string | null): number | undefined {
  if (!value || !/^[1-9]\d*$/.test(value)) {
    return undefined;
  }

  const line = Number(value);
  return Number.isSafeInteger(line) ? line : undefined;
}

function parseSourceAddress(query = ""): SourceAddress {
  const params = new URLSearchParams(query);
  const rawFile = params.get("file") ?? undefined;
  const file = rawFile ? normalizeSourceFilePath(rawFile) : undefined;
  const line = parseLine(params.get("line"));

  return {
    ...(file ? { file } : {}),
    ...(line ? { line } : {}),
  };
}

function decodeRouteSegment(segment: string): string {
  try {
    return decodeURIComponent(segment);
  } catch {
    return segment;
  }
}

export function parseHashRoute(hash: string): AppRoute {
  const normalizedHash = hash || "#/";

  const homeMatch = normalizedHash.match(/^#\/?(?:\?([^#]*))?$/);

  if (normalizedHash === "#" || normalizedHash === "" || homeMatch) {
    const language = parseLanguage(homeMatch?.[1]);

    return {
      name: "home",
      key: roleHomePath(defaultRoleVariant, language),
      language,
      roleVariant: defaultRoleVariant,
    };
  }

  const roleHomeMatch = normalizedHash.match(/^#\/roles\/([^/?#]+)(?:\?([^#]*))?$/);

  if (roleHomeMatch) {
    const roleVariant = decodeRouteSegment(roleHomeMatch[1]);
    const language = parseLanguage(roleHomeMatch[2]);

    if (isSupportedRoleVariant(roleVariant)) {
      return {
        name: "home",
        key: roleHomePath(roleVariant, language),
        language,
        roleVariant,
      };
    }

    return {
      name: "not-found",
      key: normalizedHash,
      language,
      roleVariant: defaultRoleVariant,
      slug: roleVariant,
    };
  }

  const sourceMatch = normalizedHash.match(
    /^#\/projects\/([^/?#]+)\/source(?:\?([^#]*))?$/,
  );
  const roleSourceMatch = normalizedHash.match(
    /^#\/roles\/([^/?#]+)\/projects\/([^/?#]+)\/source(?:\?([^#]*))?$/,
  );

  if (sourceMatch || roleSourceMatch) {
    const rawRoleVariant = roleSourceMatch
      ? decodeRouteSegment(roleSourceMatch[1])
      : defaultRoleVariant;
    const slug = decodeRouteSegment(roleSourceMatch?.[2] ?? sourceMatch?.[1] ?? "");
    const query = roleSourceMatch?.[3] ?? sourceMatch?.[2];
    const language = parseLanguage(query);

    if (!isSupportedRoleVariant(rawRoleVariant)) {
      return {
        name: "not-found",
        key: normalizedHash,
        language,
        roleVariant: defaultRoleVariant,
        slug: rawRoleVariant,
      };
    }

    const content = getPortfolioContent(language, rawRoleVariant);
    const project = content.getProjectBySlug(slug);

    if (project) {
      const sourceAddress = parseSourceAddress(query);

      return {
        name: "source",
        key: projectSourcePath(project.slug, sourceAddress, language, rawRoleVariant),
        language,
        roleVariant: rawRoleVariant,
        project,
        sourceAddress,
      };
    }

    return {
      name: "not-found",
      key: normalizedHash,
      language,
      roleVariant: rawRoleVariant,
      slug,
    };
  }

  const projectMatch = normalizedHash.match(/^#\/projects\/([^/?#]+)(?:\?([^#]*))?$/);
  const roleProjectMatch = normalizedHash.match(
    /^#\/roles\/([^/?#]+)\/projects\/([^/?#]+)(?:\?([^#]*))?$/,
  );

  if (projectMatch || roleProjectMatch) {
    const rawRoleVariant = roleProjectMatch
      ? decodeRouteSegment(roleProjectMatch[1])
      : defaultRoleVariant;
    const slug = decodeRouteSegment(roleProjectMatch?.[2] ?? projectMatch?.[1] ?? "");
    const query = roleProjectMatch?.[3] ?? projectMatch?.[2];
    const language = parseLanguage(query);

    if (!isSupportedRoleVariant(rawRoleVariant)) {
      return {
        name: "not-found",
        key: normalizedHash,
        language,
        roleVariant: defaultRoleVariant,
        slug: rawRoleVariant,
      };
    }

    const content = getPortfolioContent(language, rawRoleVariant);
    const project = content.getProjectBySlug(slug);

    if (project) {
      return {
        name: "project",
        key: projectPath(project.slug, language, rawRoleVariant),
        language,
        roleVariant: rawRoleVariant,
        project,
      };
    }

    return {
      name: "not-found",
      key: normalizedHash,
      language,
      roleVariant: rawRoleVariant,
      slug,
    };
  }

  return {
    name: "not-found",
    key: normalizedHash,
    language: defaultLanguage,
    roleVariant: defaultRoleVariant,
  };
}
