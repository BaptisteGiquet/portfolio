# Portfolio Context

This context defines portfolio-specific language for Baptiste Giquet's game development portfolio.

## Language

**Source Snapshot**:
A committed copy of local project code files that the portfolio can expose for recruiter inspection.
_Avoid_: Repository mirror, build source

**Broad Source Snapshot**:
A source snapshot that initially includes authored code plus third-party plugin code so it can be pruned after review.
_Avoid_: Final public source package, cleaned source

**Ownership Category**:
A label that tells readers whether a code file is authored project code, project plugin code, or third-party code.
_Avoid_: Permission level, license

**Authorship Label**:
A visible source-browser label that helps recruiters distinguish Baptiste's work from integrated dependency or third-party code.
_Avoid_: Credit, badge

**Code Reference**:
A portfolio content link that opens a specific source file, class, symbol, or line inside a project's source browser.
_Avoid_: GitHub link, raw source URL

**Project-Relative Path**:
The original path to a file inside its local Unreal project root, preserved in the source browser.
_Avoid_: Flattened path, display path

**Source Address**:
A source-browser URL target made from a project slug, encoded project-relative file path, and optional line number.
_Avoid_: File ID, permalink

**Reference Symbol**:
Optional display text naming the class, function, asset, or concept a code reference is about.
_Avoid_: Symbol resolver, index key

**Source Export**:
A manual local process that copies project source into a committed source snapshot.
_Avoid_: Build step, deployment step

**Snapshot Replacement**:
The export behavior that clears one project snapshot folder before writing the current exported files.
_Avoid_: Merge export, incremental sync

**Snapshot Layout**:
The committed folder shape with metadata at the project snapshot root and source files under a `files/` child folder.
_Avoid_: Flat snapshot, mixed metadata

**Source Notice**:
A human-readable attribution and caveat note stored with a source snapshot.
_Avoid_: License file, ownership label

**Code Viewer**:
The source-browser panel that displays one text file with line numbers and optional lightweight syntax highlighting.
_Avoid_: Editor, IDE

**Source Manifest**:
The machine-readable snapshot metadata file used to list available files without embedding full source text.
_Avoid_: Source bundle, index

**File Metric**:
Manifest metadata such as byte size and line count for a source file.
_Avoid_: Source text, quality score

**Export Timestamp**:
Informational manifest metadata recording when a source snapshot was generated.
_Avoid_: Version, cache key

**Inspection-Only Browser**:
A source browser intended for reading and navigating code without raw download, copy, edit, commit, branch, blame, or PR workflows.
_Avoid_: Repository UI, online editor

**Mobile Source Switcher**:
The mobile source-browser layout that switches between file navigation and code viewing.
_Avoid_: Stacked tree, desktop pane

**Selected Line**:
The line targeted by a source address and visually highlighted in the code viewer.
_Avoid_: Cursor, selection range

**Source Navigation State**:
The browser URL state for the selected source file and explicitly selected line.
_Avoid_: Scroll state, viewer state

**Source Browser Session State**:
Transient source-browser choices such as search filters, ownership filters, tree expansion, and mobile pane.
_Avoid_: Source navigation state, source address

**Linked Text**:
Portfolio copy that mixes normal text with optional inline source references.
_Avoid_: Markdown source links, separate references list

**Reference Validation**:
Test coverage that checks code references against committed source manifests.
_Avoid_: Runtime probing, manual link check

**Export Tooling**:
The Node/TypeScript script and npm command used to run manual source export.
_Avoid_: PowerShell exporter, ad hoc copy

**Export Configuration**:
Data that declares which projects, paths, source roots, and ownership overrides source export should use.
_Avoid_: Hardcoded exporter logic, command arguments

**Export Scope**:
The requested set of configured projects to export in one source export run.
_Avoid_: Deployment scope, browser scope

**Source Project Registry**:
The export configuration list of projects that have source snapshots available or planned.
_Avoid_: Hardcoded v1 list, portfolio order

**Generic Source Browser**:
A reusable source-browser UI that reads project-specific metadata instead of hardcoding project behavior.
_Avoid_: Hospital browser, AGASS browser

**Portfolio Content Metadata**:
Project display data owned by portfolio content rather than source manifests.
_Avoid_: Manifest title, duplicated copy

**Project Source Route**:
The nested hash route `#/projects/<project-slug>/source` for a project's source browser.
_Avoid_: Top-level source route, standalone source route

**Source Modal**:
The source-browser presentation opened over the owning project detail page by a project source route.
_Avoid_: Standalone source page, private modal state

**Source Modal Background**:
The owning project detail page rendered behind an open source modal.
_Avoid_: Separate source backdrop, hidden source page

**Source Modal Shell**:
The modal-specific frame that contains source loading, unavailable, error, and browser states.
_Avoid_: Source detail page, page shell

**Modal Source Workspace**:
The near-full-viewport source modal layout sized for reading code and navigating files.
_Avoid_: Small dialog, source card

**Source Close Link**:
The source-browser control that returns from the source modal to the owning project detail route.
_Avoid_: Back to project link, browser back, Back to project

**Source Action**:
The project action that opens the internal source browser when a project is source-enabled.
_Avoid_: GitHub action, raw source link

**Source Header**:
A compact source-browser header with counts, export timestamp, and ownership filters.
_Avoid_: Hero section, case-study copy

**Ownership Filter**:
A multi-select source-browser filter that limits visible files by ownership category.
_Avoid_: Ownership rewrite, single category mode

**Filtered Source Tree**:
The file tree after source search and ownership filters are applied.
_Avoid_: Full tree, empty folder tree

**Selected File**:
The source file currently open in the code viewer.
_Avoid_: Active folder, hovered file

**Tree Expansion State**:
The user's folder open/closed choices in the source tree.
_Avoid_: Route state, filter state

**Code Pane Error**:
An inline source-browser error shown when a selected file cannot be fetched.
_Avoid_: Route not found, project not found

**Entry File**:
The configured default file opened when a source-enabled project is visited without a file query.
_Avoid_: First file, landing page

**Source Sort Order**:
The source-browser ordering that lists folders before files and sorts each group alphabetically.
_Avoid_: Export order, filesystem order

**Manifest Language**:
The machine-readable source language value stored for each manifest file.
_Avoid_: Display label, syntax theme

**Large Source File**:
A `.h` or `.cpp` file larger than 1 MB that is skipped during export with a warning.
_Avoid_: Binary file, generated file

**Export Summary**:
The terminal output after source export showing copied files, skipped files, ownership counts, and output paths.
_Avoid_: Build report, deploy log

**Derived Source Action**:
The ready source action generated from source snapshot metadata instead of manually duplicated action data.
_Avoid_: Manual source action, external source link

**Runtime Source Metadata**:
Minimal portfolio content data that points the app at a committed source manifest.
_Avoid_: Export configuration, project copy

**Manifest Schema Version**:
The manifest version number that describes the source manifest shape.
_Avoid_: App version, export timestamp

**Logical Source Path**:
The forward-slash project-relative path stored in manifests and source URLs.
_Avoid_: Windows path, filesystem separator path

**Encoded Source Path**:
The query-string file value created by encoding a logical source path.
_Avoid_: Raw path, partially encoded path

**Ownership Value**:
The machine-readable ownership category stored in the source manifest.
_Avoid_: Ownership label, color token

**No Results State**:
The source-browser empty state shown when search and filters hide every file.
_Avoid_: Empty browser, route error

**Line Number Selection**:
The explicit action of clicking a code viewer line number to select that line.
_Avoid_: Text selection, scroll tracking

**Source Test Fixture**:
A small committed manifest/file fixture used by route and component tests.
_Avoid_: Mock-only manifest, production snapshot

**Exporter Temp Tree**:
A temporary source tree created by exporter tests.
_Avoid_: Real project path, committed snapshot

**Source Implementation Order**:
The implementation sequence of exporter and manifest first, content and tests second, router and UI third.
_Avoid_: UI-first build, guessed manifest

**PRD Gate**:
The decision to turn the agreed source-browser context into a PRD before implementation starts.
_Avoid_: Immediate implementation, informal handoff

**Source Search**:
The source-browser filter that matches manifest metadata such as path, filename, language, ownership category, and optional display symbols.
_Avoid_: Full-text search, code search

**Source Route Recovery**:
The source-browser behavior that ignores invalid file or line query values and keeps the project source browser usable.
_Avoid_: Source 404, hard failure

**Source-Enabled Project**:
A portfolio project whose content explicitly declares a committed source snapshot for the source browser.
_Avoid_: Manifest-detected project, auto-enabled source

**Source-Unavailable Project**:
A portfolio project that exists but does not declare a committed source snapshot.
_Avoid_: Broken source project, empty browser

**Unreal Source Metadata**:
Text descriptors and build files that explain Unreal project, plugin, target, and module boundaries.
_Avoid_: Build guarantee, generated metadata

**Code File**:
A human-authored C++ source or header file with a `.cpp` or `.h` extension.
_Avoid_: Descriptor, config, documentation

**Project-Local Code**:
Code files located inside the local project root, including project plugins and third-party plugins copied into the project.
_Avoid_: Unreal Engine source, engine code

**Source Root**:
A project folder intended to contain source code, such as `Source/` or `Plugins/<plugin>/Source/`.
_Avoid_: Output folder, cache folder

**Configured Project Path**:
The local filesystem root used by source export for a portfolio project.
_Avoid_: Snapshot path, deployed path

**Ownership Configuration**:
Per-project export settings that classify source roots or plugins before default path rules are applied.
_Avoid_: Automatic ownership, inferred authorship

**Generated Unreal Output**:
Unreal-created intermediate, generated, cache, IDE, build, package, or log files that are not source evidence.
_Avoid_: Source metadata, project source

## Relationships

- A **Source Snapshot** belongs to exactly one portfolio project.
- A portfolio project becomes a **Source-Enabled Project** only through explicit content metadata.
- A **Source-Enabled Project** exposes a ready **Source Action** labeled `Browse source`.
- A **Source-Unavailable Project** does not expose a ready **Source Action**.
- A **Project Source Route** for a **Source-Unavailable Project** renders a source-unavailable state in the **Source Modal**.
- A **Source Export** reads from a **Configured Project Path**.
- A **Source Export** is implemented as **Export Tooling** in the app's Node/TypeScript toolchain.
- A **Source Export** reads **Export Configuration** instead of hardcoding project paths and ownership rules.
- **Export Configuration** contains a **Source Project Registry** that starts with Hospital and AGASS but can add future personal projects.
- A **Source Export** applies **Ownership Configuration** before default path-based ownership rules.
- A **Source Export** supports **Export Scope** for all configured projects or one requested project.
- A **Source Export** produces a **Source Snapshot**.
- A **Source Export** uses **Snapshot Replacement** for one project at a time.
- A **Source Export** preserves source file contents as-is while computing line counts for CRLF and LF files.
- A **Source Export** skips **Large Source Files** with warnings.
- A **Source Export** writes an **Export Summary** for each exported project.
- A **Source Snapshot** uses the **Snapshot Layout** `public/source/<project-slug>/manifest.json` and `public/source/<project-slug>/files/<project-relative-path>`.
- A **Source Snapshot** includes **Project-Local Code** from **Source Roots** only as browsable source payload.
- A **Source Snapshot** excludes non-code **Unreal Source Metadata** such as `.uproject` and `.uplugin` descriptors.
- A **Source Snapshot** excludes `.Build.cs`, `.Target.cs`, and other non-`.h`/`.cpp` files in v1.
- A **Source Snapshot** excludes Unreal Engine installation source files.
- A **Source Snapshot** excludes generated headers such as `*.generated.h` even though they end in `.h`.
- A **Source Snapshot** excludes `.h` and `.cpp` files found under output, cache, generated, IDE, build, package, or release folders.
- A **Source Snapshot** excludes **Generated Unreal Output** even when the snapshot is broad.
- The `manifest.json` file is the **Source Manifest**.
- A **Broad Source Snapshot** is a temporary review form of a **Source Snapshot**.
- Each file in a **Source Snapshot** has exactly one **Ownership Category**.
- Each **Ownership Category** is assigned from **Ownership Configuration** first, then from path defaults.
- Unknown plugin source defaults to the third-party **Ownership Category** unless **Ownership Configuration** says otherwise.
- Each **Ownership Category** appears as an **Authorship Label** in the source browser.
- A **Source Manifest** stores fixed **Ownership Values** `project-code`, `project-plugin`, and `third-party-plugin`.
- **Ownership Values** do not include config or docs categories in v1.
- **Ownership Filters** default to all ownership categories visible.
- **Ownership Filters** filter visible source-browser results without changing the **Source Manifest**.
- **Source Search** and **Ownership Filters** combine by intersection.
- A **Filtered Source Tree** hides folders that contain no visible files.
- A **Filtered Source Tree** expands ancestor folders for the **Selected File** without expanding every folder by default.
- A **Filtered Source Tree** preserves **Tree Expansion State** where possible when filters change.
- A **Filtered Source Tree** clears visible selection and shows an empty state when the **Selected File** is filtered out, without rewriting the source address.
- A **Code Reference** may target any file, class, symbol, or line in the **Source Snapshot**, including third-party or plugin code, as long as the target keeps its **Authorship Label** visible.
- **Linked Text** can contain **Code References** while plain portfolio text remains valid.
- **Reference Validation** checks that each **Code Reference** targets an existing manifest file and valid line when a line is provided.
- A **Code Viewer** opens one file from a **Source Snapshot** and keeps line navigation stable even when syntax highlighting is absent.
- A **Code Viewer** loads source text lazily from `files/` after a file is selected.
- A **Code Viewer** shows a **Code Pane Error** if a selected manifest file cannot be fetched.
- A **Code Viewer** centers the **Selected Line** when opened from a source address.
- A **Code Viewer** uses **Line Number Selection** to update source navigation state.
- Clicking code text does not update source navigation state.
- A **Code Viewer** is part of an **Inspection-Only Browser**.
- An **Inspection-Only Browser** is implemented as a **Generic Source Browser**.
- A **Generic Source Browser** starts with a compact **Source Header** before the file navigation and code viewer.
- A **Generic Source Browser** uses a desktop two-pane layout and a **Mobile Source Switcher** on small screens.
- A **Project Source Route** renders the owning project detail page with a **Source Modal** open.
- A **Project Source Route** keeps the document title focused on the source browser while the **Source Modal** is open.
- A **Source Modal Background** is visually present but inactive while the **Source Modal** is open.
- A **Source Modal** uses a **Source Modal Shell** instead of the project detail page shell.
- A **Source Modal** contains the **Generic Source Browser**.
- A **Source Modal** uses a **Modal Source Workspace**.
- A **Source Modal Shell** contains source loading, unavailable, error, and ready browser states.
- A **Modal Source Workspace** locks background page scrolling while preserving internal source-browser scrolling.
- A **Modal Source Workspace** lets the file tree and code pane scroll independently on desktop.
- A **Modal Source Workspace** is effectively full-screen on mobile.
- A **Generic Source Browser** in a **Source Modal** includes a **Source Close Link**.
- A **Source Close Link** always navigates to the owning project detail route.
- A **Source Close Link** removes **Source Navigation State** by leaving the **Project Source Route**.
- Pressing Escape in a **Source Modal** follows the same route change as the **Source Close Link**.
- A **Source Modal** backdrop does not close the source browser.
- Project previous/next navigation stays on project detail pages and does not include source routes.
- A **Source Manifest** lists file metadata but does not include full source text.
- A **Source Manifest** includes **Manifest Schema Version** `1`.
- A **Source Manifest** stores **File Metrics** for each file.
- A **Source Manifest** includes an **Export Timestamp**.
- A **Source Manifest** stays source-focused and does not own **Portfolio Content Metadata**.
- A **Source Manifest** stores **Manifest Language** values such as `cpp` and `cpp-header`; the UI owns display labels such as `C++` and `C++ Header`.
- A **Source Manifest** stores **Logical Source Paths** with `/` separators.
- The file system uses normal nested folders, while manifests and URLs use **Logical Source Paths**.
- **Portfolio Content Metadata** owns project titles, summaries, actions, and source availability.
- **Portfolio Content Metadata** stores **Runtime Source Metadata** for source-enabled projects.
- **Runtime Source Metadata** contains the manifest path and does not contain **Export Configuration**.
- A **Source-Enabled Project** uses a **Derived Source Action** labeled `Browse source`.
- Content tests require each **Source-Enabled Project** to expose a ready **Derived Source Action**.
- **Source Search** reads the **Source Manifest** and does not scan full file contents in v1.
- **Source Search** is case-insensitive.
- **Source Search** and **Ownership Filters** show a **No Results State** with a reset-filters action when no files remain visible.
- Each file in a **Source Snapshot** keeps its **Project-Relative Path**.
- A **Code Reference** resolves to a **Source Address**.
- A **Source Address** uses a **Project Source Route**.
- A **Source Address** uses the **Project-Relative Path** instead of a generated file identifier.
- A **Source Address** stores its file query as an **Encoded Source Path** created from `encodeURIComponent(path)` and decoded once.
- **Source Navigation State** changes when a file or line is explicitly selected, not when the code viewer scrolls.
- **Source Navigation State** supports browser back and forward for explicit file and line selections.
- **Source Navigation State** remains route state while the source browser is presented as a **Source Modal**.
- Browser back and forward can move between source addresses while the **Source Modal** remains open.
- **Source Browser Session State** can persist across source address changes while the same **Source Modal** stays open.
- **Source Browser Session State** resets when the **Source Modal** closes or opens for a different project.
- Selecting a different file from the tree clears the selected line unless the source address includes a line for that file.
- A **Reference Symbol** describes a **Code Reference** but does not determine navigation in v1.
- Portfolio builds consume committed **Source Snapshots** and do not run **Source Export**.
- A **Source Export** stops before **Snapshot Replacement** if the requested project's **Configured Project Path** is missing.
- **Source Route Recovery** applies when a **Source Address** has an invalid file or line, but not when the project slug is unknown.
- A source route without a file query opens the configured **Entry File** when present, otherwise the first manifest file by **Source Sort Order**.
- The source tree uses **Source Sort Order**.
- The source browser shows line count in file rows and keeps byte size out of the main row unless a details area needs it.
- Route and component tests use **Source Test Fixtures**.
- Content tests validate real committed source manifests after export exists.
- Exporter tests use **Exporter Temp Trees**.
- Exporter tests cover extension filtering, generated header exclusion, ownership configuration, missing path failure, snapshot replacement, and manifest metrics.
- Source work waits for the **PRD Gate** before implementation.
- After the **PRD Gate**, source work follows **Source Implementation Order**.
- The first implementation slice is exporter code and tests before real Hospital or AGASS export.
- App runtime enables source only from real **Runtime Source Metadata** pointing to committed manifests, not fixture data.

## Example dialogue

> **Dev:** "Should the AGASS source browser include docs, config, descriptors, and sample text?"
> **Domain expert:** "No, create a **Broad Source Snapshot** of **Code Files** only, then prune unnecessary code after review while keeping **Ownership Category** and **Authorship Label** labels clear."

## Flagged ambiguities

- "Source snapshot" could mean either cleaned recruiter-facing code or a broad project-source export; resolved: v1 starts as a **Broad Source Snapshot** of **Code Files** only and is pruned after review.
- "Ownership badge" and "ownership category" both refer to **Authorship Label** in the UI backed by **Ownership Category** in the manifest.
- "Inline code link" means **Code Reference** and is allowed to target any source-browser class or file, not only authored files.
- "Path" in source-browser routes and manifests means **Project-Relative Path**, not a flattened display path.
- "File target" means a **Source Address** based on the encoded **Project-Relative Path**, not a generated file ID.
- "Symbol" in content means **Reference Symbol** display metadata, not automatic source parsing.
- "Export script" means **Source Export**, a manual local step rather than an automatic build or deploy step.
- "Regenerate snapshot" means **Snapshot Replacement** inside the target project's `public/source/<project-slug>/` folder, not a merge with previous output.
- "Snapshot folder" follows the **Snapshot Layout** with manifest metadata separate from exported code files.
- "Notice" was previously accepted as a source artifact, but the code-only decision supersedes that: no `NOTICES.md` in v1 unless it is later re-approved as metadata outside the browsable source payload.
- "Syntax highlighting" is optional decoration in the **Code Viewer**; line numbers, selected-line highlighting, and stable source addresses are required.
- "Manifest" means **Source Manifest** metadata only, not an embedded full-text source bundle.
- "Size" and "line count" are **File Metrics** in the manifest, not fetched from the code viewer on demand.
- "Export timestamp" is informational and should not be used as a stable version or cache key.
- "Source browser" means **Inspection-Only Browser** in v1.
- "Mobile source browser" means **Mobile Source Switcher**, not a full file tree stacked above the code by default.
- "Line highlight" means **Selected Line** highlighting with initial centered scroll behavior.
- "URL update" means **Source Navigation State** changes for explicit file or line selection only.
- "Inline source link" appears through **Linked Text**, not by replacing all project copy with a new rich text model.
- "Broken code reference" is caught by **Reference Validation** before deployment rather than discovered through runtime fetch probing.
- "Export script" is **Export Tooling** run through npm, not a PowerShell workflow.
- "Export config" means **Export Configuration**, a data file for project paths, source roots, and ownership overrides.
- "Export one project" and "export all" are both supported **Export Scope** modes.
- "V1 source projects" means Hospital and AGASS in the **Source Project Registry**, while the exporter remains open to future personal projects.
- "Project-specific source page" means metadata/content only, not a custom browser implementation per project.
- "Project title" belongs to **Portfolio Content Metadata**, not the **Source Manifest**.
- "Source route" means **Project Source Route**, nested under `#/projects/<project-slug>/source`.
- "Source route as modal" means the **Project Source Route** stays canonical and deep-linkable while rendering a **Source Modal** over the owning project detail page.
- "Project behind source" means the **Source Modal Background**, not a second route or duplicated project page.
- "Close source" means following the **Source Close Link** to the owning project detail route, not replaying browser history.
- "Close source with selected file or line" discards **Source Navigation State** by returning to the project detail route.
- "`Close source`" is the source modal close-control label; `Back to project` implies the wrong browser-history behavior.
- "Escape closes source" means the same deterministic route change as the **Source Close Link**.
- "Backdrop click" does not close the **Source Modal**.
- "Reopen source" through the **Source Action** opens the default **Project Source Route**, not the last file or line from a previous modal session.
- "Source browser state" means **Source Browser Session State**, not **Source Navigation State**.
- "`Browse source`" is the ready **Source Action** label; unavailable source has no visible action.
- "`Browse source`" should be a **Derived Source Action**, not manually duplicated action state.
- "Source page header" means **Source Header**, not marketing or explanatory hero content.
- "Source modal intro" omits page-style explanatory copy; recruiter inspection context belongs in compact source-browser chrome.
- "Ownership chip" means **Ownership Filter** when interactive and **Authorship Label** when attached to a file.
- "Combined filters" means metadata search results intersected with selected ownership categories.
- "Filtered tree" means **Filtered Source Tree**, not the full project tree with empty folders left visible.
- "Auto-expand" means revealing the **Selected File** ancestors, not opening the whole tree.
- "Filtered-out selected file" means the UI shows no visible selection until another file is chosen, while the URL remains unchanged.
- "Missing source file fetch" means **Code Pane Error**, not a route-level not found.
- "Default source file" means **Entry File** first, then first file by **Source Sort Order**.
- "Source sorting" means folders first, then files, alphabetically.
- "Source language" means **Manifest Language** machine values in metadata and UI-owned display labels.
- "Large file skip" means a **Large Source File** is omitted with a warning during export.
- "Export result" means **Export Summary** printed by the exporter.
- "Runtime source metadata" means manifest-path-only **Runtime Source Metadata**, separate from **Export Configuration**.
- "Manifest schema" means **Manifest Schema Version** `1` in v1.
- "Manifest path" and "URL path" mean **Logical Source Path** with `/` separators.
- "Encoded file query" means **Encoded Source Path** created with `encodeURIComponent(path)` and decoded once.
- "Ownership value" means one of the fixed **Ownership Values** `project-code`, `project-plugin`, or `third-party-plugin`.
- "No source results" means **No Results State** with a reset-filters action, not route failure.
- "Line click" means **Line Number Selection**; selecting code text does not update the URL.
- "Back/forward" follows **Source Navigation State** entries created by explicit file and line selections.
- "New file selection" clears the selected line unless the current source address supplies a line for that file.
- "Component/router source tests" use **Source Test Fixtures**; exporter tests use **Exporter Temp Trees**.
- "Implementation start" waits for the **PRD Gate**.
- "Implementation order" means **Source Implementation Order** after the **PRD Gate**: exporter/manifest, content/tests, then router/UI.
- "First implementation slice" means exporter code and tests before real source snapshots.
- "Runtime source enablement" means real **Runtime Source Metadata**, never fixture data.
- "Search" means **Source Search** over metadata, not full-text code search.
- "Invalid source link" means **Source Route Recovery** should keep the browser open unless the project slug is unknown.
- "Invalid source link in a modal" keeps the **Source Modal** open and follows **Source Route Recovery**.
- "Source ready" means the project is a **Source-Enabled Project** in content metadata, not merely that a manifest URL might exist.
- "Source route unavailable" means the project exists but is a **Source-Unavailable Project**, not that the whole project route is unknown.
- "Descriptor" means non-code **Unreal Source Metadata** and is excluded from v1 source payload.
- "Build file" means `.Build.cs` or `.Target.cs`; these are excluded from v1 source payload because they are not `.h` or `.cpp` **Code Files**.
- "Whole project source" means all relevant **Code Files**, not descriptors, config, docs, binaries, assets, or **Generated Unreal Output**.
- "Code-only" means only `.h` and `.cpp` files in v1.
- "All project code" means **Project-Local Code**, not source files from the Unreal Engine installation.
- "`*.generated.h`" files are **Generated Unreal Output**, not recruiter-facing **Code Files**.
- "Source folder" means **Source Root**, not every folder containing `.h` or `.cpp` files.
- "Missing project path" means the requested **Configured Project Path** is absent, so source export fails before deleting snapshot output.
- "Ownership inference" means **Ownership Configuration** first and path conventions second.
- "Unknown plugin" means plugin source without an explicit ownership override, so it is treated as third-party by default.
