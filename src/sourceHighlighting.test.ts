import { beforeEach, describe, expect, it, vi } from "vitest";

const shikiMock = vi.hoisted(() => {
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
    if (initializationError) {
      throw initializationError;
    }

    return { codeToTokens };
  });
  const createBundledHighlighter = vi.fn(() => createHighlighter);
  const createJavaScriptRegexEngine = vi.fn(() => ({}));
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
    failInitialization,
    failTokenization,
    resetFailures,
  };
});

vi.mock("shiki/core", () => ({
  createBundledHighlighter: shikiMock.createBundledHighlighter,
}));

vi.mock("shiki/engine/javascript", () => ({
  createJavaScriptRegexEngine: shikiMock.createJavaScriptRegexEngine,
}));

describe("source highlighting", () => {
  beforeEach(() => {
    vi.resetModules();
    shikiMock.resetFailures();
    shikiMock.codeToTokens.mockClear();
    shikiMock.createBundledHighlighter.mockClear();
    shikiMock.createHighlighter.mockClear();
    shikiMock.createJavaScriptRegexEngine.mockClear();
  });

  it("maps manifest languages to supported Shiki languages", async () => {
    const { resolveSourceHighlightLanguage } = await import("./sourceHighlighting");

    expect(resolveSourceHighlightLanguage("cpp")).toBe("cpp");
    expect(resolveSourceHighlightLanguage("cpp-header")).toBe("cpp");
    expect(resolveSourceHighlightLanguage("csharp")).toBe("csharp");
    expect(resolveSourceHighlightLanguage(undefined)).toBeUndefined();
    expect(resolveSourceHighlightLanguage("python")).toBeUndefined();
  });

  it("keeps missing and unknown languages as plain text without loading Shiki", async () => {
    const { highlightSource } = await import("./sourceHighlighting");

    await expect(highlightSource("const int32 Count = 1;\n", undefined)).resolves.toEqual({
      kind: "plain-text",
      lines: ["const int32 Count = 1;"],
    });
    await expect(highlightSource("print('hello')", "python")).resolves.toEqual({
      kind: "plain-text",
      lines: ["print('hello')"],
    });
    expect(shikiMock.createBundledHighlighter).not.toHaveBeenCalled();
  });

  it("returns normalized token lines for supported languages", async () => {
    const { highlightSource } = await import("./sourceHighlighting");

    const result = await highlightSource(
      "#pragma once\n\nclass AAGASSPlayerController;\n",
      "cpp-header",
    );

    expect(shikiMock.createBundledHighlighter).toHaveBeenCalledWith({
      engine: expect.any(Function),
      langs: {
        cpp: expect.any(Function),
        csharp: expect.any(Function),
      },
      themes: {
        "portfolio-rider-light": expect.objectContaining({
          name: "portfolio-rider-light",
        }),
      },
    });
    expect(shikiMock.createHighlighter).toHaveBeenCalledWith({
      langs: ["cpp", "csharp"],
      themes: ["portfolio-rider-light"],
    });
    expect(shikiMock.codeToTokens).toHaveBeenCalledWith(
      "#pragma once\n\nclass AAGASSPlayerController;\n",
      {
        lang: "cpp",
        theme: "portfolio-rider-light",
      },
    );
    expect(result.kind).toBe("highlighted");
    if (result.kind !== "highlighted") {
      throw new Error("Expected highlighted source.");
    }
    expect(
      result.lines.map((line) =>
        line.tokens.map((token) => token.content).join(""),
      ),
    ).toEqual(["#pragma once", " ", "class AAGASSPlayerController;"]);
    expect(result.lines[2].tokens).toEqual(
      expect.arrayContaining([
        expect.objectContaining({
          color: "#00627a",
          content: "AAGASSPlayerController",
          fontStyle: 0,
          semanticKind: "unreal-type",
        }),
      ]),
    );
  });

  it("resolves Unreal-specific identifier categories", async () => {
    const { resolveUnrealTokenKind } = await import("./sourceHighlighting");

    expect(resolveUnrealTokenKind("UCLASS")).toBe("unreal-reflection-macro");
    expect(resolveUnrealTokenKind("GENERATED_BODY")).toBe(
      "unreal-reflection-macro",
    );
    expect(resolveUnrealTokenKind("BlueprintCallable")).toBe(
      "unreal-reflection-specifier",
    );
    expect(resolveUnrealTokenKind("meta")).toBe("unreal-reflection-specifier");
    expect(resolveUnrealTokenKind("FName")).toBe("unreal-type");
    expect(resolveUnrealTokenKind("FVector")).toBe("unreal-type");
    expect(resolveUnrealTokenKind("TObjectPtr")).toBe("unreal-type");
    expect(resolveUnrealTokenKind("TSubclassOf")).toBe("unreal-type");
    expect(resolveUnrealTokenKind("UAGASSInventoryComponent")).toBe(
      "unreal-type",
    );
    expect(resolveUnrealTokenKind("AAGASSPlayerController")).toBe("unreal-type");
    expect(resolveUnrealTokenKind("AGASS_API")).toBe("unreal-export-macro");
    expect(resolveUnrealTokenKind("UE_LOG")).toBe("unreal-helper-macro");
    expect(resolveUnrealTokenKind("DEFINE_LOG_CATEGORY_STATIC")).toBe(
      "unreal-helper-macro",
    );
    expect(resolveUnrealTokenKind("DECLARE_DYNAMIC_MULTICAST_DELEGATE")).toBe(
      "unreal-helper-macro",
    );
    expect(resolveUnrealTokenKind("UE_DEFINE_GAMEPLAY_TAG")).toBe(
      "unreal-helper-macro",
    );
    expect(resolveUnrealTokenKind("ECC_Visibility")).toBe("unreal-constant");
    expect(resolveUnrealTokenKind("INDEX_NONE")).toBe("unreal-constant");
    expect(resolveUnrealTokenKind("UnknownIdentifier")).toBeUndefined();
  });

  it("applies Unreal highlighting only to C++ token identifiers", async () => {
    const { applyUnrealHighlighting } = await import("./sourceHighlighting");

    const sourceLine =
      "UPROPERTY(BlueprintReadOnly, meta=(ClampMin=\"0\")) TObjectPtr<UAGASSInventoryComponent> Inventory;";
    const lines = applyUnrealHighlighting(
      [
        {
          tokens: [
            {
              color: "#0033b3",
              content: sourceLine,
              fontStyle: 0,
            },
          ],
        },
      ],
      "cpp-header",
    );

    expect(lines[0].tokens.map((token) => token.content).join("")).toBe(
      sourceLine,
    );
    expect(lines[0].tokens).toEqual(
      expect.arrayContaining([
        expect.objectContaining({
          color: "#871094",
          content: "UPROPERTY",
          semanticKind: "unreal-reflection-macro",
        }),
        expect.objectContaining({
          color: "#8a5a00",
          content: "BlueprintReadOnly",
          semanticKind: "unreal-reflection-specifier",
        }),
        expect.objectContaining({
          color: "#8a5a00",
          content: "meta",
          semanticKind: "unreal-reflection-specifier",
        }),
        expect.objectContaining({
          color: "#8a5a00",
          content: "ClampMin",
          semanticKind: "unreal-reflection-specifier",
        }),
        expect.objectContaining({
          color: "#00627a",
          content: "TObjectPtr",
          semanticKind: "unreal-type",
        }),
        expect.objectContaining({
          color: "#00627a",
          content: "UAGASSInventoryComponent",
          semanticKind: "unreal-type",
        }),
      ]),
    );
  });

  it("keeps comments, strings, and unsupported languages on their Shiki tokens", async () => {
    const { applyUnrealHighlighting } = await import("./sourceHighlighting");
    const lines = [
      {
        tokens: [
          {
            color: "#6f7782",
            content: "// UCLASS TObjectPtr BlueprintCallable",
            fontStyle: 1,
          },
          {
            color: "#067d17",
            content: '"UPROPERTY FName"',
            fontStyle: 0,
          },
          {
            color: "#6F7782",
            content: "// GENERATED_BODY AAGASSPlayerController",
            fontStyle: 1,
          },
          {
            color: "#067D17",
            content: '"AGASS|Timed Run"',
            fontStyle: 0,
          },
        ],
      },
    ];

    expect(applyUnrealHighlighting(lines, "cpp")).toEqual(lines);
    expect(
      applyUnrealHighlighting(
        [
          {
            tokens: [
              {
                color: "#0033b3",
                content: "UCLASS FName",
                fontStyle: 0,
              },
            ],
          },
        ],
        "csharp",
      ),
    ).toEqual([
      {
        tokens: [
          {
            color: "#0033b3",
            content: "UCLASS FName",
            fontStyle: 0,
          },
        ],
      },
    ]);
  });

  it("keeps source as plain text when Shiki initialization fails", async () => {
    shikiMock.failInitialization();
    const { highlightSource } = await import("./sourceHighlighting");

    await expect(
      highlightSource("const int32 Count = 1;\n", "cpp"),
    ).resolves.toEqual({
      kind: "plain-text",
      lines: ["const int32 Count = 1;"],
    });
    expect(shikiMock.createBundledHighlighter).toHaveBeenCalledOnce();
    expect(shikiMock.codeToTokens).not.toHaveBeenCalled();

    shikiMock.resetFailures();

    const result = await highlightSource("const int32 Count = 1;\n", "cpp");

    expect(result.kind).toBe("highlighted");
    if (result.kind !== "highlighted") {
      throw new Error("Expected highlighted source.");
    }
    expect(result.lines[0].tokens.map((token) => token.content).join("")).toBe(
      "const int32 Count = 1;",
    );
    expect(shikiMock.createHighlighter).toHaveBeenCalledTimes(2);
  });

  it("keeps source as plain text when tokenization fails", async () => {
    shikiMock.failTokenization();
    const { highlightSource } = await import("./sourceHighlighting");

    await expect(
      highlightSource("public class HospitalFlow {}\n", "csharp"),
    ).resolves.toEqual({
      kind: "plain-text",
      lines: ["public class HospitalFlow {}"],
    });
    expect(shikiMock.codeToTokens).toHaveBeenCalledWith(
      "public class HospitalFlow {}\n",
      {
        lang: "csharp",
        theme: "portfolio-rider-light",
      },
    );
  });
});
