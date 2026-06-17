# Source Browser Rider Light Syntax Highlighting Plan

## Summary

Add Rider-light-style syntax highlighting to the existing source browser code reader only. Keep the current modal, file tree, line-number links, selected-line deep links, and mobile horizontal scrolling behavior intact.

## Key Changes

- Add `shiki` as a runtime syntax-highlighting dependency.
- Add a small source-highlighting helper that:
  - lazy-loads Shiki only when the code viewer has a fetched file to highlight;
  - creates one cached Shiki highlighter for `cpp` and `csharp`;
  - maps manifest languages `cpp` and `cpp-header` to Shiki `cpp`;
  - maps future C# manifests to Shiki `csharp`;
  - returns normalized line/token arrays so `SourceCodePane` does not depend on Shiki internals;
  - falls back to plain text for missing, unknown, or unsupported manifest languages;
  - falls back to plain text if highlighting initialization or tokenization fails;
  - uses a local `portfolio-rider-light` Shiki theme approximating Rider Light: white editor background, muted gray comments, blue/purple keywords, green strings, teal types/functions, red/orange constants/errors.
- Update `SourceCodePane` so fetched file content can be tokenized with Shiki and rendered as React spans per token, not injected HTML.
- Show plain text as soon as file content is available, then replace line text with token spans when highlighting finishes.
- Ignore stale asynchronous highlighting results when the selected file changes before highlighting completes.
- Keep file fetch state separate from highlighting state:
  - fetch failures still show the existing code pane error;
  - highlighting failures do not show a visible error and instead keep plain text.
- Preserve current line rendering:
  - each code line remains addressable with its line-number link;
  - code text remains inert for navigation;
  - selected line still receives `.selected`, `data-line-number`, and scrolls into view;
  - selected-line scrolling reruns after highlighted rendering settles;
  - empty lines still keep stable height.
- Do not change the source manifest schema or source export behavior for highlighting.
- Restyle only the editor pane:
  - white/light editor background instead of dark gray;
  - pale gutter behind line numbers;
  - Rider-like monospace stack: `JetBrains Mono`, `Cascadia Mono`, `Consolas`, monospace;
  - selected line uses a subtle light IDE highlight;
  - mobile keeps readable font size, compact gutter, and horizontal scrolling with no wrapping.

## Test Plan

- Add focused tests for language mapping: `cpp`, `cpp-header`, future `csharp`, missing language, and unknown language.
- Add focused tests for plain-text fallback behavior when highlighting cannot run.
- Add or adjust source modal tests to confirm highlighted code renders token spans while line links, selected-line behavior, empty lines, and raw source accessibility still work.
- Avoid exact token color assertions; test behavior and structure instead.
- Keep existing source browser route, modal, backdrop, close, and deep-link tests passing.
- Run:
  - `npm test`
  - `npm run build`
- After implementation, verify in browser on desktop and mobile width:
  - AGASS source opens and highlights C++;
  - Hospital source opens and highlights C++;
  - long lines scroll horizontally on mobile;
  - selected line deep links remain visible and readable.

## Assumptions

- We will not add full-text code search, copy buttons, copy-to-clipboard, wrapping controls, theme switching, or new source-browser features.
- The file tree and modal shell keep their current portfolio styling.
- Shiki is preferred over a custom lexer because it supports accurate C++ now and C# later without maintaining our own parser.
- Exact byte-for-byte Rider theme parity is not required; the target is a close Rider Light reading feel.
- Client-side Shiki highlighting is a reversible implementation choice and does not need an ADR.
- C# support is limited to language mapping and tests for this feature; the exporter remains scoped to current `.h` and `.cpp` source snapshots.
