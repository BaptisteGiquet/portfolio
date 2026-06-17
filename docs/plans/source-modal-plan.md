# Source Browser Modal Plan

Status: converted to PRD issue https://github.com/LocmaSaaS/portfolio/issues/33.

## Summary
Change the source browser from a separate page into a modal overlay that opens above the current project detail page. Keep the existing source hash routes so `Browse source`, inline code references, selected files, selected lines, browser back/forward, and direct deep links continue to work.

## Key Changes
- Treat `#/projects/<slug>/source?...` as a “project page with source modal open” route.
- Render `ProjectDetailPage` behind the modal when `route.name === "source"`.
- Render the existing source UI inside a new modal shell with backdrop, close control, and constrained full-screen layout.
- Change the source header action from `Back to project` to a `Close source` action linking to `#/projects/<slug>`.
- Keep `projectSourcePath(...)` unchanged so all current source links and line links remain valid.
- Keep source routes for unknown project slugs as not-found routes; source recovery applies only to file and line query state for known projects.
- Closing the modal discards `file` and `line` query state by returning to `#/projects/<slug>`.

## Implementation Changes
- In `src/App.tsx`, replace the current source-route rendering branch:
  - Instead of rendering only `SourceSnapshotPage`, render `ProjectDetailPage project={route.project}` plus a `SourceModal`.
  - `SourceModal` wraps the existing `SourceSnapshotPage` content or a refactored inner source component.
- Refactor `SourceSnapshotPage` lightly:
  - Remove standalone page-mode assumptions unless a temporary display prop is simpler during the refactor; the project source route is modal-only.
  - Remove page-style padding/back-page behavior and expose a `Close source` link to `projectPath(project.slug)`.
  - Keep manifest loading, source file loading, filtering, mobile switcher, and code pane logic unchanged.
- Add modal behavior:
  - Backdrop covers the viewport.
  - Dialog uses `role="dialog"` and `aria-modal="true"`.
  - Close button/link returns to the owning project route.
  - Escape key also closes by setting `window.location.hash = projectPath(project.slug)`.
  - Body scroll is locked while the modal is open.
  - Escape handling and body scroll lock are lifecycle effects tied to the mounted modal, with cleanup on unmount.
  - The project detail page behind the modal renders normally, but only the modal content loads source manifests and source files.
  - Focus restoration to the original opener is optional for this slice because source routes can be opened by direct deep link.
  - A React portal is optional; use one only if stacking, inert, or focus handling becomes awkward.
  - Keep the source modal mounted across `file` and `line` query changes so filters, tree expansion, and mobile pane state are not reset by source navigation.
  - Reset transient source browser state when the modal closes/reopens or opens for a different project.
  - Preserve the existing mobile behavior where selecting a file switches back to the code pane.
- Add modal CSS in `src/styles.css`:
  - Fixed viewport overlay above the full app shell, including header and footer, with subdued backdrop.
  - Modal panel sized for code browsing: near full viewport on desktop, full-screen or near full-screen on mobile.
  - Source browser height calculated inside the modal so tree and code panes scroll internally.
  - Reuse existing source browser styling where possible; add modal wrapper/layout styles without redesigning the tree, filters, code pane, mobile switcher, or labels.
  - Preserve existing mobile `Code` / `Files` switcher behavior.

## Test Plan
- Update existing source-route test to expect both:
  - the project detail page is still rendered underneath, using a stable project-detail signal, and
  - the source browser appears inside an accessible dialog named from the project source title.
- Add/adjust tests for:
  - `Browse source` opens the source modal route.
  - Inline code references open the same modal route with file and line query preserved.
  - Close control returns to `#/projects/<slug>` and removes the dialog.
  - Escape closes the modal.
  - Opening the modal focuses the `Close source` control.
  - Body scroll is locked while the modal is open.
  - Line-number clicks still update the hash with `file` and `line`.
  - Browser back/forward still moves across explicit line selections.
  - Direct deep links like `#/projects/agass/source?file=...&line=42` open the project page with the modal already visible.
  - Source unavailable state still appears inside the modal for projects without a committed source snapshot.

Avoid overfitting unit tests to a full focus-trap implementation unless one is custom-built; prefer browser/e2e coverage for full keyboard trapping if needed.

## Manual / Browser Verification
- Run the app and verify the source modal layout on desktop and mobile viewports.
- Verify `Browse source` opens the modal above the project detail page.
- Verify a direct deep link like `#/projects/agass/source?file=...&line=42` loads with the modal already visible and selected line behavior intact.
- Verify a source-unavailable project route opens the modal unavailable state.
- Verify keyboard behavior: initial focus on `Close source`, Escape closes to the project route, and browser back/forward moves through explicit source file/line selections while the modal remains open.

## Assumptions
- Keep source routes public and deep-linkable rather than using private component state only.
- The modal is opened only on project source routes, not from the homepage project cards.
- Existing source browser functionality should be reused, not redesigned.
- Closing the modal always returns to the owning project page and drops source query state.
