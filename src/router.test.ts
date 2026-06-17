import { describe, expect, it } from "vitest";
import { parseHashRoute, projectPath, projectSourcePath } from "./router";

describe("hash routing", () => {
  it("supports the homepage route", () => {
    expect(parseHashRoute("").name).toBe("home");
    expect(parseHashRoute("#/").name).toBe("home");
  });

  it("parses language from hash query params and defaults invalid values to English", () => {
    const frenchHome = parseHashRoute("#/?lang=fr");

    expect(frenchHome.name).toBe("home");
    expect(frenchHome.language).toBe("fr");
    expect(frenchHome.key).toBe("#/?lang=fr");

    const invalidProject = parseHashRoute("#/projects/hospital-flow?lang=de");

    expect(invalidProject.name).toBe("project");
    expect(invalidProject.language).toBe("en");
    expect(invalidProject.key).toBe("#/projects/hospital-flow");
  });

  it("supports project detail routes", () => {
    const route = parseHashRoute("#/projects/hospital-flow");

    expect(route.name).toBe("project");
    expect(route.name === "project" ? route.project.slug : undefined).toBe(
      "hospital-flow",
    );
    expect(projectPath("agass")).toBe("#/projects/agass");
    expect(projectPath("agass", "fr")).toBe("#/projects/agass?lang=fr");
  });

  it("supports role-specific home and project routes", () => {
    const homeRoute = parseHashRoute("#/roles/qa-tester");

    expect(homeRoute.name).toBe("home");
    expect(homeRoute.language).toBe("en");
    expect(homeRoute.roleVariant).toBe("qa-tester");
    expect(homeRoute.key).toBe("#/roles/qa-tester");

    const projectRoute = parseHashRoute("#/roles/qa-tester/projects/hospital-flow");

    expect(projectRoute.name).toBe("project");
    expect(projectRoute.roleVariant).toBe("qa-tester");
    expect(
      projectRoute.name === "project" ? projectRoute.project.slug : undefined,
    ).toBe("hospital-flow");
    expect(projectPath("agass", "en", "qa-tester")).toBe(
      "#/roles/qa-tester/projects/agass",
    );
    expect(projectPath("agass", "fr", "qa-tester")).toBe(
      "#/roles/qa-tester/projects/agass?lang=fr",
    );
  });

  it("supports project source routes", () => {
    const route = parseHashRoute("#/projects/hospital-flow/source");

    expect(route.name).toBe("source");
    expect(route.name === "source" ? route.project.slug : undefined).toBe(
      "hospital-flow",
    );
    expect(projectSourcePath("agass")).toBe("#/projects/agass/source");
    expect(projectSourcePath("agass", {}, "fr")).toBe("#/projects/agass/source?lang=fr");
    expect(projectSourcePath("agass", {}, "en", "qa-tester")).toBe(
      "#/roles/qa-tester/projects/agass/source",
    );

    const sourceUnavailableRoute = parseHashRoute("#/projects/moba-gas/source");

    expect(sourceUnavailableRoute.name).toBe("source");
    expect(
      sourceUnavailableRoute.name === "source"
        ? sourceUnavailableRoute.project.slug
        : undefined,
    ).toBe("moba-gas");
  });

  it("writes and parses encoded source address state", () => {
    const sourcePath = projectSourcePath("agass", {
      file: "Source/AGASS/Public/AGASSPlayerController.h",
      line: 42,
    });

    expect(sourcePath).toBe(
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=42",
    );

    const route = parseHashRoute(sourcePath);

    expect(route.name).toBe("source");
    expect(route.name === "source" ? route.sourceAddress : undefined).toEqual({
      file: "Source/AGASS/Public/AGASSPlayerController.h",
      line: 42,
    });
  });

  it("preserves language with source file and line query state", () => {
    const sourcePath = projectSourcePath(
      "agass",
      {
        file: "Source/AGASS/Public/AGASSPlayerController.h",
        line: 42,
      },
      "fr",
    );

    expect(sourcePath).toBe(
      "#/projects/agass/source?lang=fr&file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=42",
    );

    const route = parseHashRoute(sourcePath);

    expect(route.name).toBe("source");
    expect(route.language).toBe("fr");
    expect(route.name === "source" ? route.project.summary : undefined).toMatch(
      /Jeu co-op en ligne/i,
    );
    expect(route.name === "source" ? route.sourceAddress : undefined).toEqual({
      file: "Source/AGASS/Public/AGASSPlayerController.h",
      line: 42,
    });
  });

  it("normalizes source file paths to forward slashes when writing and parsing", () => {
    const sourcePath = projectSourcePath("agass", {
      file: "Source\\AGASS\\Public\\AGASSPlayerController.h",
      line: 7,
    });

    expect(sourcePath).toBe(
      "#/projects/agass/source?file=Source%2FAGASS%2FPublic%2FAGASSPlayerController.h&line=7",
    );

    const route = parseHashRoute(
      "#/projects/agass/source?file=Source%5CAGASS%5CPublic%5CAGASSPlayerController.h&line=7",
    );

    expect(route.name).toBe("source");
    expect(route.name === "source" ? route.sourceAddress : undefined).toEqual({
      file: "Source/AGASS/Public/AGASSPlayerController.h",
      line: 7,
    });
  });

  it("decodes source file queries exactly once", () => {
    const route = parseHashRoute("#/projects/agass/source?file=Source%252FEncoded.cpp");

    expect(route.name).toBe("source");
    expect(route.name === "source" ? route.sourceAddress.file : undefined).toBe(
      "Source%2FEncoded.cpp",
    );
  });

  it("ignores invalid optional line query values", () => {
    const zeroLineRoute = parseHashRoute(
      "#/projects/agass/source?file=Source%2FAGASS.cpp&line=0",
    );
    const textLineRoute = parseHashRoute(
      "#/projects/agass/source?file=Source%2FAGASS.cpp&line=abc",
    );

    expect(zeroLineRoute.name).toBe("source");
    expect(
      zeroLineRoute.name === "source" ? zeroLineRoute.sourceAddress : undefined,
    ).toEqual({ file: "Source/AGASS.cpp" });
    expect(textLineRoute.name).toBe("source");
    expect(
      textLineRoute.name === "source" ? textLineRoute.sourceAddress : undefined,
    ).toEqual({ file: "Source/AGASS.cpp" });
  });

  it("does not write invalid line query values", () => {
    expect(projectSourcePath("agass", { file: "Source/AGASS.cpp", line: 0 })).toBe(
      "#/projects/agass/source?file=Source%2FAGASS.cpp",
    );
    expect(projectSourcePath("agass", { file: "Source/AGASS.cpp", line: 1.5 })).toBe(
      "#/projects/agass/source?file=Source%2FAGASS.cpp",
    );
    expect(projectSourcePath("agass", { file: "Source/AGASS.cpp", line: -1 })).toBe(
      "#/projects/agass/source?file=Source%2FAGASS.cpp",
    );
  });

  it("returns not-found for unknown routes and unknown slugs", () => {
    expect(parseHashRoute("#/missing").name).toBe("not-found");

    const route = parseHashRoute("#/projects/nope");

    expect(route.name).toBe("not-found");
    expect(route.name === "not-found" ? route.slug : undefined).toBe("nope");

    const sourceRoute = parseHashRoute("#/projects/nope/source");

    expect(sourceRoute.name).toBe("not-found");
    expect(sourceRoute.name === "not-found" ? sourceRoute.slug : undefined).toBe(
      "nope",
    );

    const sourceRouteWithQuery = parseHashRoute(
      "#/projects/nope/source?file=Source%2FExample.cpp&line=10",
    );

    expect(sourceRouteWithQuery.name).toBe("not-found");
    expect(
      sourceRouteWithQuery.name === "not-found"
        ? sourceRouteWithQuery.slug
        : undefined,
    ).toBe("nope");
  });
});
