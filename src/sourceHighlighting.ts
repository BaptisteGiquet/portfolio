import type { ThemedToken, ThemeRegistration, TokensResult } from "@shikijs/types";

export type SourceHighlightToken = {
  color?: string;
  content: string;
  fontStyle?: number;
  semanticKind?: SourceHighlightSemanticKind;
};

export type SourceHighlightLine = {
  tokens: SourceHighlightToken[];
};

export type SourceHighlightSemanticKind =
  | "unreal-constant"
  | "unreal-export-macro"
  | "unreal-helper-macro"
  | "unreal-reflection-macro"
  | "unreal-reflection-specifier"
  | "unreal-type";

export type SourceHighlightResult =
  | { kind: "plain-text"; lines: string[] }
  | { kind: "highlighted"; lines: SourceHighlightLine[] };

const riderLightThemeName = "portfolio-rider-light";
const riderLightCommentColor = "#6f7782";
const riderLightStringColor = "#067d17";

const unrealSemanticColors: Record<SourceHighlightSemanticKind, string> = {
  "unreal-constant": "#1750eb",
  "unreal-export-macro": "#7055a4",
  "unreal-helper-macro": "#5f3dc4",
  "unreal-reflection-macro": "#871094",
  "unreal-reflection-specifier": "#8a5a00",
  "unreal-type": "#00627a",
};

const unrealReflectionMacros = new Set([
  "GENERATED_BODY",
  "GENERATED_UCLASS_BODY",
  "GENERATED_USTRUCT_BODY",
  "UCLASS",
  "UDELEGATE",
  "UENUM",
  "UFUNCTION",
  "UINTERFACE",
  "UPROPERTY",
  "USTRUCT",
]);

const unrealHelperMacros = new Set([
  "DOREPLIFETIME",
  "IMPLEMENT_GAME_MODULE",
  "IMPLEMENT_MODULE",
  "LOCTEXT",
  "NSLOCTEXT",
  "SCENE_QUERY_STAT",
  "TEXT",
  "UE_LOG",
]);

const unrealReflectionSpecifiers = new Set([
  "AdvancedDisplay",
  "AllowPrivateAccess",
  "AutoCreateRefTerm",
  "BlueprintAssignable",
  "BlueprintAuthorityOnly",
  "BlueprintCallable",
  "BlueprintImplementableEvent",
  "BlueprintNativeEvent",
  "BlueprintPure",
  "BlueprintReadOnly",
  "BlueprintReadWrite",
  "BlueprintSpawnableComponent",
  "BlueprintType",
  "Category",
  "ClampMax",
  "ClampMin",
  "Client",
  "DefaultToSelf",
  "DisplayName",
  "EditAnywhere",
  "EditDefaultsOnly",
  "EditFixedSize",
  "EditInstanceOnly",
  "ExactClass",
  "ExposeOnSpawn",
  "HideCategories",
  "Instanced",
  "meta",
  "NetMulticast",
  "NotBlueprintable",
  "Replicated",
  "ReplicatedUsing",
  "Reliable",
  "Server",
  "UIMax",
  "UIMin",
  "Unreliable",
  "VisibleAnywhere",
  "VisibleDefaultsOnly",
  "VisibleInstanceOnly",
  "WorldContext",
]);

const unrealConstants = new Set(["INDEX_NONE", "NAME_None"]);

const identifierPattern = /^[A-Za-z_][A-Za-z0-9_]*$/;
const unrealTypePattern = /^[UAFTEI][A-Z][A-Za-z0-9_]*$/;

const riderLightTheme: ThemeRegistration = {
  name: riderLightThemeName,
  bg: "#ffffff",
  fg: "#1f2328",
  settings: [
    {
      settings: {
        background: "#ffffff",
        foreground: "#1f2328",
      },
    },
    {
      scope: ["comment", "punctuation.definition.comment"],
      settings: {
        foreground: "#6f7782",
        fontStyle: "italic",
      },
    },
    {
      scope: ["keyword", "storage", "storage.type", "storage.modifier"],
      settings: {
        foreground: "#0033b3",
      },
    },
    {
      scope: ["string", "string.quoted", "constant.character"],
      settings: {
        foreground: "#067d17",
      },
    },
    {
      scope: ["entity.name.type", "support.type", "support.class"],
      settings: {
        foreground: "#00627a",
      },
    },
    {
      scope: ["entity.name.function", "support.function", "variable.function"],
      settings: {
        foreground: "#00627a",
      },
    },
    {
      scope: ["constant.numeric", "constant.language", "variable.other.enummember"],
      settings: {
        foreground: "#1750eb",
      },
    },
    {
      scope: ["invalid", "markup.deleted"],
      settings: {
        foreground: "#b00020",
      },
    },
  ],
};

type SourceHighlighter = {
  codeToTokens: (
    code: string,
    options: { lang: "cpp" | "csharp"; theme: typeof riderLightThemeName },
  ) => TokensResult;
};

let highlighterPromise: Promise<SourceHighlighter> | undefined;

export function resetSourceHighlighterForTests() {
  highlighterPromise = undefined;
}

export function resolveSourceHighlightLanguage(language?: string) {
  switch (language) {
    case "cpp":
    case "cpp-header":
      return "cpp";
    case "csharp":
      return "csharp";
    default:
      return undefined;
  }
}

export async function highlightSource(
  content: string,
  language?: string,
): Promise<SourceHighlightResult> {
  const plainLines = splitSourceLines(content);
  const shikiLanguage = resolveSourceHighlightLanguage(language);

  if (!shikiLanguage) {
    return { kind: "plain-text", lines: plainLines };
  }

  try {
    const highlighter = await getSourceHighlighter();
    const result = highlighter.codeToTokens(content, {
      lang: shikiLanguage,
      theme: riderLightThemeName,
    });

    return {
      kind: "highlighted",
      lines: applyUnrealHighlighting(
        normalizeHighlightedLines(result.tokens, plainLines),
        language,
      ),
    };
  } catch {
    return { kind: "plain-text", lines: plainLines };
  }
}

export function resolveUnrealTokenKind(
  identifier: string,
): SourceHighlightSemanticKind | undefined {
  if (
    unrealReflectionMacros.has(identifier) ||
    identifier.startsWith("GENERATED_")
  ) {
    return "unreal-reflection-macro";
  }

  if (
    unrealHelperMacros.has(identifier) ||
    identifier.startsWith("DECLARE_") ||
    identifier.startsWith("DEFINE_LOG_CATEGORY") ||
    identifier.startsWith("DOREPLIFETIME_") ||
    identifier.startsWith("UE_DEFINE_")
  ) {
    return "unreal-helper-macro";
  }

  if (unrealReflectionSpecifiers.has(identifier)) {
    return "unreal-reflection-specifier";
  }

  if (identifier.endsWith("_API")) {
    return "unreal-export-macro";
  }

  if (
    unrealConstants.has(identifier) ||
    identifier.startsWith("ECC_") ||
    identifier.startsWith("ECR_")
  ) {
    return "unreal-constant";
  }

  if (unrealTypePattern.test(identifier)) {
    return "unreal-type";
  }

  return undefined;
}

export function applyUnrealHighlighting(
  lines: SourceHighlightLine[],
  language?: string,
): SourceHighlightLine[] {
  if (resolveSourceHighlightLanguage(language) !== "cpp") {
    return lines;
  }

  return lines.map((line) => ({
    tokens: line.tokens.flatMap(applyUnrealTokenHighlighting),
  }));
}

export function splitSourceLines(content: string) {
  const normalizedContent = content.replace(/\r\n/g, "\n").replace(/\r/g, "\n");

  if (normalizedContent.length === 0) {
    return [];
  }

  const lines = normalizedContent.split("\n");

  if (normalizedContent.endsWith("\n")) {
    lines.pop();
  }

  return lines;
}

async function getSourceHighlighter() {
  highlighterPromise ??= createSourceHighlighter().catch((error: unknown) => {
    highlighterPromise = undefined;
    throw error;
  });

  return highlighterPromise;
}

async function createSourceHighlighter() {
  const [{ createBundledHighlighter }, { createJavaScriptRegexEngine }] =
    await Promise.all([import("shiki/core"), import("shiki/engine/javascript")]);
  const createHighlighter = createBundledHighlighter({
    langs: {
      cpp: () => import("@shikijs/langs/cpp"),
      csharp: () => import("@shikijs/langs/csharp"),
    },
    themes: {
      [riderLightThemeName]: riderLightTheme,
    },
    engine: () => createJavaScriptRegexEngine(),
  });

  return createHighlighter({
    langs: ["cpp", "csharp"],
    themes: [riderLightThemeName],
  });
}

function normalizeHighlightedLines(
  tokenLines: ThemedToken[][],
  plainLines: string[],
): SourceHighlightLine[] {
  return plainLines.map((plainLine, index) => {
    const tokens = tokenLines[index] ?? [];
    const normalizedTokens = tokens
      .filter((token) => token.content.length > 0)
      .map((token) => ({
        color: token.color,
        content: token.content,
        fontStyle: token.fontStyle,
      }));

    return {
      tokens:
        normalizedTokens.length > 0
          ? normalizedTokens
          : [{ content: plainLine || " " }],
    };
  });
}

function applyUnrealTokenHighlighting(
  token: SourceHighlightToken,
): SourceHighlightToken[] {
  if (isCommentOrStringToken(token)) {
    return [token];
  }

  return splitTokenContent(token).map((content) => {
    const semanticKind = identifierPattern.test(content)
      ? resolveUnrealTokenKind(content)
      : undefined;

    if (!semanticKind) {
      return { ...token, content };
    }

    return {
      ...token,
      color: unrealSemanticColors[semanticKind],
      content,
      semanticKind,
    };
  });
}

function isCommentOrStringToken(token: SourceHighlightToken) {
  const color = token.color?.toLowerCase();

  return (
    color === riderLightCommentColor || color === riderLightStringColor
  );
}

function splitTokenContent(token: SourceHighlightToken): string[] {
  const parts = token.content.match(
    /[A-Za-z_][A-Za-z0-9_]*|\s+|[^A-Za-z_\s]+/g,
  );

  return parts ?? [token.content];
}
