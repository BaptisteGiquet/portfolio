import { readFileSync } from "node:fs";

import { describe, expect, it } from "vitest";

const styles = readFileSync(new URL("./styles.css", import.meta.url), "utf8").replace(
  /\r\n/g,
  "\n",
);

function ruleFor(selector: string) {
  const escapedSelector = selector.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const match = styles.match(new RegExp(`${escapedSelector}\\s*\\{([^}]*)\\}`));

  return match?.[1] ?? "";
}

describe("source code pane styles", () => {
  it("right-aligns the compact project-page identity header", () => {
    expect(ruleFor(".site-header")).toContain("justify-content: flex-end");
    expect(ruleFor(".identity-link")).toContain("text-align: right");
    expect(styles).not.toContain(".site-nav");
    expect(styles).not.toContain(".project-type");
  });

  it("uses Unreal brand chrome while keeping the Rider Light code pane", () => {
    expect(ruleFor(".source-code-block")).toContain("background: #ffffff");
    expect(ruleFor(".source-code-block")).toContain("color: #1f2328");
    expect(ruleFor(".source-code-block")).toContain(
      'font-family: "JetBrains Mono", "Cascadia Mono", Consolas, monospace',
    );
    expect(ruleFor(".source-code-block")).toContain("tab-size: 4");
    expect(ruleFor(".source-line-number")).toContain("background: #f3f4f6");
    expect(ruleFor(".source-code-line.selected")).toContain("background: #e8f2ff");
    expect(ruleFor(".source-tree-pane")).toContain(
      "border-right: 1px solid var(--ue-border)",
    );
    expect(styles).toContain(
      ".source-tree-file a:hover,\n.source-tree-file a.selected {\n  background: var(--ue-control)",
    );
  });

  it("keeps source lines horizontally scrollable instead of wrapped", () => {
    expect(ruleFor(".source-code-block")).toContain("overflow: auto");
    expect(ruleFor(".source-code-block code")).toContain("width: max-content");
    expect(ruleFor(".source-code-line")).toContain(
      "grid-template-columns: 58px max-content",
    );
    expect(ruleFor(".source-code-line")).toContain("min-width: 100%");
    expect(ruleFor(".source-line-text")).toContain("white-space: pre");
    expect(ruleFor(".source-line-text")).toContain("min-width: max-content");
  });
});
