import {
  useEffect,
  useMemo,
  useRef,
  useState,
  type ReactNode,
  type RefObject,
} from "react";
import {
  getPortfolioContent,
  projects,
  type CodeReference,
  type ContentSection,
  type Language,
  type MediaSlot,
  type PortfolioContent,
  type PortfolioProject,
  type ProjectAction,
  type RichTextContent,
} from "./content/portfolio";
import {
  parseHashRoute,
  projectPath,
  projectSourcePath,
  roleHomePath,
  type AppRoute,
  type SourceAddress,
} from "./router";
import {
  highlightSource,
  splitSourceLines,
  type SourceHighlightLine,
} from "./sourceHighlighting";

function useHashRoute() {
  const [route, setRoute] = useState<AppRoute>(() =>
    parseHashRoute(window.location.hash),
  );

  useEffect(() => {
    const handleHashChange = () => setRoute(parseHashRoute(window.location.hash));

    window.addEventListener("hashchange", handleHashChange);
    return () => window.removeEventListener("hashchange", handleHashChange);
  }, []);

  return route;
}

function useRouteEffects(route: AppRoute, content: PortfolioContent) {
  useEffect(() => {
    if (route.name === "home") {
      document.title = content.ui.documentTitle.home(content.siteProfile);
    } else if (route.name === "project") {
      document.title = content.ui.documentTitle.project(
        route.project,
        content.siteProfile,
      );
    } else if (route.name === "source") {
      document.title = content.ui.documentTitle.source(
        route.project,
        content.siteProfile,
      );
    } else {
      document.title = content.ui.documentTitle.notFound(content.siteProfile);
    }

    document.documentElement.lang = content.language;
    window.scrollTo({ top: 0, left: 0 });
  }, [content, route]);
}

function languageHref(language: Language) {
  const route = parseHashRoute(window.location.hash);

  if (route.name === "project") {
    return projectPath(route.project.slug, language, route.roleVariant);
  }

  if (route.name === "source") {
    return projectSourcePath(
      route.project.slug,
      route.sourceAddress,
      language,
      route.roleVariant,
    );
  }

  return roleHomePath(route.roleVariant, language);
}

function LanguageToggle({ content }: { content: PortfolioContent }) {
  return (
    <nav
      aria-label={content.ui.languageToggle}
      className="language-toggle"
    >
      {(["en", "fr"] as const).map((language) => (
        <a
          aria-pressed={content.language === language}
          className={content.language === language ? "active" : undefined}
          href={languageHref(language)}
          key={language}
          role="button"
        >
          {language.toUpperCase()}
        </a>
      ))}
    </nav>
  );
}

function SiteHeader({ content }: { content: PortfolioContent }) {
  return (
    <header className="site-header">
      <a
        className="identity-link"
        href={roleHomePath(content.roleVariant, content.language)}
      >
        <span>{content.siteProfile.name}</span>
        <span>{content.siteProfile.role}</span>
      </a>
      <LanguageToggle content={content} />
    </header>
  );
}

function HomeLanguageToggle({ content }: { content: PortfolioContent }) {
  return (
    <div className="home-language-toggle">
      <LanguageToggle content={content} />
    </div>
  );
}

function ActionControl({
  action,
  className,
  content,
}: {
  action: ProjectAction;
  className: string;
  content?: PortfolioContent;
}) {
  const href = resolveActionHref(action, content);

  if (action.status === "ready" && href) {
    return (
      <a
        className={className}
        download={action.download}
        href={href}
        rel={action.opensInNewTab ? "noreferrer noopener" : undefined}
        target={action.opensInNewTab ? "_blank" : undefined}
      >
        <span>{action.label}</span>
        {action.metaLabel ? (
          <span className="action-meta">{action.metaLabel}</span>
        ) : null}
      </a>
    );
  }

  return (
    <button
      aria-disabled="true"
      className={`${className} pending`}
      disabled
      title={action.pendingReason}
      type="button"
    >
      <span>{action.label}</span>
      {action.metaLabel ? (
        <span className="action-meta">{action.metaLabel}</span>
      ) : null}
    </button>
  );
}

function resolveActionHref(
  action: ProjectAction,
  content: PortfolioContent | undefined,
) {
  if (!action.href || !content) {
    return action.href;
  }

  const sourceMatch = action.href.match(/^#\/projects\/([^/?#]+)\/source(?:\?([^#]*))?$/);

  if (!sourceMatch) {
    return action.href;
  }

  const slug = sourceMatch[1] as PortfolioProject["slug"];

  return projectSourcePath(slug, {}, content.language, content.roleVariant);
}

function ActionList({
  actions,
  content,
  label,
}: {
  actions: ProjectAction[];
  content: PortfolioContent;
  label: string;
}) {
  return (
    <div className="action-list" aria-label={label}>
      {actions.map((action) => (
        <ActionControl
          action={action}
          className="action-link"
          content={content}
          key={action.id}
        />
      ))}
    </div>
  );
}

function MediaSlotFigure({
  content,
  slot,
}: {
  content: PortfolioContent;
  slot: MediaSlot;
}) {
  const style = { aspectRatio: slot.aspectRatio };
  const hasReadySource = slot.status === "ready" && Boolean(slot.source);

  return (
    <figure className="media-slot">
      {hasReadySource && slot.type === "image" ? (
        <img
          alt={slot.caption}
          className="media-asset"
          src={slot.source}
          style={style}
        />
      ) : null}
      {hasReadySource && slot.type === "video" ? (
        <video
          aria-label={slot.caption}
          className="media-asset"
          controls
          poster={slot.poster}
          preload="metadata"
          style={style}
        >
          <source src={slot.source} />
        </video>
      ) : null}
      {!hasReadySource ? (
        <div
          aria-label={content.ui.placeholder(slot.type, slot.caption)}
          className="media-placeholder"
          role="img"
          style={style}
        />
      ) : null}
    </figure>
  );
}

function ProjectSummary({
  content,
  project,
}: {
  content: PortfolioContent;
  project: PortfolioProject;
}) {
  return (
    <article className="project-row">
      <MediaSlotFigure content={content} slot={project.mediaSlots[0]} />
      <div className="project-row-copy">
        <h3>{project.title}</h3>
        <p>{project.summary}</p>
        <ul
          className="technology-list"
          aria-label={content.ui.technologies(project.title)}
        >
          {project.technologies.slice(0, 6).map((technology) => (
            <li key={technology}>{technology}</li>
          ))}
        </ul>
        <a
          className="project-action"
          href={projectPath(project.slug, content.language, content.roleVariant)}
        >
          {content.ui.viewProject}
        </a>
      </div>
    </article>
  );
}

function ProfileActions({ content }: { content: PortfolioContent }) {
  return (
    <div className="profile-actions" aria-label={content.ui.profileActions}>
      {content.siteProfile.actions.map((action) => (
        <ActionControl
          action={action}
          className="profile-action"
          content={content}
          key={action.id}
        />
      ))}
    </div>
  );
}

function HomePage({ content }: { content: PortfolioContent }) {
  return (
    <>
      <section className="hero-section">
        <div>
          <h1>{content.siteProfile.role}</h1>
          <div className="hero-profile-row">
            <p className="hero-profile-name">{content.siteProfile.name}</p>
            <ProfileActions content={content} />
          </div>
        </div>
        <dl className="profile-facts" aria-label={content.ui.profileFacts}>
          <div>
            <dt>{content.ui.contact}</dt>
            <dd>
              <a href={`mailto:${content.siteProfile.contact.email}`}>
                {content.siteProfile.contact.email}
              </a>
              <a href={`tel:${content.siteProfile.contact.phone.replaceAll(".", "")}`}>
                {content.siteProfile.contact.phone}
              </a>
            </dd>
          </div>
          <div>
            <dt>{content.ui.location}</dt>
            <dd>{content.siteProfile.location}</dd>
          </div>
          <div>
            <dt>{content.ui.availability}</dt>
            <dd>{content.siteProfile.availability}</dd>
          </div>
        </dl>
      </section>

      <section className="project-section" id="featured-work">
        <div className="section-heading">
          <h1>{content.ui.personalProjects}</h1>
        </div>
        <div className="project-list">
          {content.featuredOriginalProjects.map((project) => (
            <ProjectSummary content={content} key={project.slug} project={project} />
          ))}
        </div>
      </section>

      <section className="project-section" id="learning-projects">
        <div className="section-heading">
          <h1>{content.ui.learningProjects}</h1>
          <p>{content.ui.learningProjectsIntro}</p>
        </div>
        <div className="project-list">
          {content.learningProjects.map((project) => (
            <ProjectSummary content={content} key={project.slug} project={project} />
          ))}
        </div>
      </section>
    </>
  );
}

function ProjectDetailPage({
  content,
  project,
}: {
  content: PortfolioContent;
  project: PortfolioProject;
}) {
  if (project.page.template === "learning") {
    return (
      <article className="detail-page learning-detail-page">
        <ProjectHeader content={content} project={project} />

        <ProjectMedia content={content} project={project} />

        {project.page.courseContext ? (
          <section className="content-section">
            <h2>{content.ui.courseContext}</h2>
            <p>
              <RichText
                content={project.page.courseContext}
                contentContext={content}
                project={project}
              />
            </p>
          </section>
        ) : null}

        {project.page.whatBuilt ? (
          <ContentList
            content={content}
            items={project.page.whatBuilt}
            project={project}
            title={content.ui.whatBuilt}
          />
        ) : null}

        {project.page.whatLearned ? (
          <ContentList
            content={content}
            items={project.page.whatLearned}
            project={project}
            title={content.ui.whatLearned}
          />
        ) : null}

        {project.page.sections.map((section) => (
          <ProjectContentSection
            key={section.title}
            content={content}
            project={project}
            section={section}
          />
        ))}

        {project.page.scopeNotes ? (
          <ContentList
            content={content}
            items={project.page.scopeNotes}
            project={project}
            title={content.ui.attributionAndScope}
          />
        ) : null}

        {project.page.proof.length > 0 ? (
          <ContentList
            content={content}
            items={project.page.proof}
            project={project}
            title={content.ui.technicalProof}
          />
        ) : null}
        {project.page.improvements.length > 0 ? (
          <ContentList
            content={content}
            items={project.page.improvements}
            project={project}
            title={content.ui.improveNext}
          />
        ) : null}
      </article>
    );
  }

  return (
    <article className="detail-page">
      <ProjectHeader content={content} project={project} />

      <ProjectMedia content={content} project={project} />

      <section className="content-section">
        <h2>{content.ui.overview}</h2>
        <p>
          <RichText
            content={project.page.overview}
            contentContext={content}
            project={project}
          />
        </p>
      </section>

      {project.page.sections.map((section) => (
        <ProjectContentSection
          key={section.title}
          content={content}
          project={project}
          section={section}
        />
      ))}

      {project.page.proof.length > 0 ? (
        <ContentList
          content={content}
          items={project.page.proof}
          project={project}
          title={content.ui.technicalProof}
        />
      ) : null}
      {project.page.improvements.length > 0 ? (
        <ContentList
          content={content}
          items={project.page.improvements}
          project={project}
          title={content.ui.improveNext}
        />
      ) : null}
    </article>
  );
}

function ProjectHeader({
  content,
  project,
}: {
  content: PortfolioContent;
  project: PortfolioProject;
}) {
  return (
    <header className="project-header">
      <a
        className="back-link"
        href={roleHomePath(content.roleVariant, content.language)}
      >
        {content.ui.backToProjects}
      </a>
      <h1 tabIndex={-1}>{project.title}</h1>
      <p>{project.summary}</p>
      <ActionList
        actions={project.actions}
        content={content}
        label={content.ui.projectActions}
      />
    </header>
  );
}

function ProjectMedia({
  content,
  project,
}: {
  content: PortfolioContent;
  project: PortfolioProject;
}) {
  const linkedMediaSlotIds = new Set(
    project.page.sections
      .map((section) => section.mediaSlotId)
      .filter((slotId): slotId is string => Boolean(slotId)),
  );
  const standaloneSlots = project.mediaSlots.filter(
    (slot) => !linkedMediaSlotIds.has(slot.id),
  );

  if (standaloneSlots.length === 0) {
    return null;
  }

  return (
    <div className="media-grid" aria-label={content.ui.mediaSlots(project.title)}>
      {standaloneSlots.map((slot) => (
        <MediaSlotFigure content={content} key={slot.id} slot={slot} />
      ))}
    </div>
  );
}

function getSectionMediaSlot(
  project: PortfolioProject,
  mediaSlotId: string | undefined,
) {
  return mediaSlotId
    ? project.mediaSlots.find((slot) => slot.id === mediaSlotId)
    : undefined;
}

function ProjectContentSection({
  content,
  project,
  section,
}: {
  content: PortfolioContent;
  project: PortfolioProject;
  section: ContentSection;
}) {
  const mediaSlot = getSectionMediaSlot(project, section.mediaSlotId);

  return (
    <section
      className={mediaSlot ? "content-section system-section" : "content-section"}
    >
      <div className="content-section-copy">
        <h2>{section.title}</h2>
        <p>
          <RichText
            content={section.body}
            contentContext={content}
            project={project}
          />
        </p>
        {section.points ? (
          <PlainList
            content={content}
            items={section.points}
            project={project}
          />
        ) : null}
        <SourceReferenceList
          content={content}
          project={project}
          references={section.sourceReferences}
          sectionTitle={section.title}
        />
      </div>
      {mediaSlot ? <MediaSlotFigure content={content} slot={mediaSlot} /> : null}
    </section>
  );
}

function ContentList({
  content,
  title,
  items,
  project,
}: {
  content: PortfolioContent;
  title: string;
  items: RichTextContent[];
  project: PortfolioProject;
}) {
  return (
    <section className="content-section">
      <h2>{title}</h2>
      <PlainList content={content} items={items} project={project} />
    </section>
  );
}

function PlainList({
  content,
  items,
  project,
}: {
  content: PortfolioContent;
  items: RichTextContent[];
  project?: PortfolioProject;
}) {
  return (
    <ul className="plain-list">
      {items.map((item, index) => (
        <li key={richTextKey(item, index)}>
          <RichText content={item} contentContext={content} project={project} />
        </li>
      ))}
    </ul>
  );
}

function isCodeReference(value: string | CodeReference): value is CodeReference {
  return typeof value !== "string" && value.kind === "code-reference";
}

function CodeReferenceLink({
  content,
  project,
  reference,
}: {
  content: PortfolioContent;
  project: PortfolioProject;
  reference: CodeReference;
}) {
  return (
    <a
      className="code-reference-link"
      href={projectSourcePath(project.slug, {
        file: reference.file,
        line: reference.line,
      }, content.language, content.roleVariant)}
    >
      {reference.label}
    </a>
  );
}

function SourceReferenceList({
  content,
  project,
  references,
  sectionTitle,
}: {
  content: PortfolioContent;
  project: PortfolioProject;
  references?: CodeReference[];
  sectionTitle: string;
}) {
  if (!references?.length) {
    return null;
  }

  return (
    <div
      aria-label={content.ui.sourceReferences(sectionTitle)}
      className="source-reference-strip"
    >
      {references.map((reference, index) => (
        <CodeReferenceLink
          content={content}
          key={`${reference.file}:${reference.line ?? ""}:${index}`}
          project={project}
          reference={reference}
        />
      ))}
    </div>
  );
}

function richTextKey(content: RichTextContent, index: number) {
  if (typeof content === "string") {
    return content;
  }

  return `${index}:${content
    .map((run) => (typeof run === "string" ? run : `${run.file}:${run.line ?? ""}`))
    .join("|")}`;
}

function RichText({
  content,
  contentContext,
  project,
}: {
  content: RichTextContent;
  contentContext: PortfolioContent;
  project?: PortfolioProject;
}) {
  if (typeof content === "string") {
    return <>{content}</>;
  }

  return (
    <>
      {content.map((run, index) => {
        if (!isCodeReference(run)) {
          return run;
        }

        if (!project) {
          return run.label;
        }

        return (
          <CodeReferenceLink
            content={contentContext}
            key={`${run.file}:${run.line ?? ""}:${index}`}
            project={project}
            reference={run}
          />
        );
      })}
    </>
  );
}

type SourceManifest = {
  exportedAt: string;
  entryFile?: string;
  metrics: {
    fileCount: number;
    totalBytes?: number;
    totalLines: number;
    ownershipCounts: Record<string, number>;
  };
  files: SourceManifestFile[];
};

type SourceManifestFile = {
  path: string;
  snapshotPath?: string;
  language?: string;
  ownership: string;
  lines: number;
};

type SourceManifestState =
  | { status: "unavailable" }
  | { status: "loading" }
  | { status: "ready"; manifest: SourceManifest }
  | { status: "error"; message: string };

type SourceFileState =
  | { status: "idle" }
  | { status: "loading" }
  | { status: "ready"; content: string }
  | { status: "error"; message: string };

type SourceTreeNode = {
  name: string;
  path: string;
  children: SourceTreeNode[];
  file?: SourceManifestFile;
};

function getAppBasePath() {
  return window.location.pathname.endsWith("/")
    ? window.location.pathname
    : window.location.pathname.replace(/\/[^/]*$/, "/");
}

function compareSourcePath(a: string, b: string) {
  return a.localeCompare(b, "en", { sensitivity: "base" });
}

function sortSourceFiles(files: SourceManifestFile[]) {
  return [...files].sort((a, b) => compareSourcePath(a.path, b.path));
}

function ownershipLabel(content: PortfolioContent, ownership: string) {
  return content.ui.ownershipLabels[ownership] ?? ownership;
}

function sourceAncestorPaths(path: string) {
  const parts = path.split("/");

  return parts.slice(0, -1).map((_, index) => parts.slice(0, index + 1).join("/"));
}

function selectedSourceFolderPaths(path?: string) {
  return path ? sourceAncestorPaths(path) : [];
}

function resolveSourceSelection(
  manifest: SourceManifest,
  sourceAddress: SourceAddress,
) {
  const filesByPath = new Map(manifest.files.map((file) => [file.path, file]));
  const requestedFile = sourceAddress.file
    ? filesByPath.get(sourceAddress.file)
    : undefined;
  const entryFile = manifest.entryFile
    ? filesByPath.get(manifest.entryFile)
    : undefined;
  const selectedFile = requestedFile ?? entryFile ?? sortSourceFiles(manifest.files)[0];
  const selectedLine =
    selectedFile &&
    sourceAddress.line &&
    (!sourceAddress.file || requestedFile) &&
    sourceAddress.line <= selectedFile.lines
      ? sourceAddress.line
      : undefined;

  return {
    fileRecovered: Boolean(sourceAddress.file && !requestedFile && selectedFile),
    lineRecovered: Boolean(sourceAddress.line && selectedFile && !selectedLine),
    selectedFile,
    selectedLine,
  };
}

function SourceSnapshotPage({
  content,
  project,
  sourceAddress,
  titleId,
}: {
  content: PortfolioContent;
  project: PortfolioProject;
  sourceAddress: SourceAddress;
  titleId: string;
}) {
  const [manifestState, setManifestState] = useState<SourceManifestState>(() =>
    project.source ? { status: "loading" } : { status: "unavailable" },
  );

  useEffect(() => {
    if (!project.source) {
      setManifestState({ status: "unavailable" });
      return;
    }

    const controller = new AbortController();
    const manifestUrl = `${getAppBasePath()}${project.source.manifestPath}`;

    setManifestState({ status: "loading" });

    fetch(manifestUrl, { signal: controller.signal })
      .then((response) => {
        if (!response.ok) {
          throw new Error(`Manifest request failed with ${response.status}.`);
        }

        return response.json() as Promise<SourceManifest>;
      })
      .then((manifest) => setManifestState({ status: "ready", manifest }))
      .catch((error: unknown) => {
        if (controller.signal.aborted) {
          return;
        }

        setManifestState({
          status: "error",
          message:
            error instanceof Error
              ? error.message
              : content.ui.sourceUnknownManifestError,
        });
      });

    return () => controller.abort();
  }, [project]);

  return (
    <article className="source-detail-page">
      <a
        className="source-close-link"
        href={projectPath(project.slug, content.language, content.roleVariant)}
      >
        {content.ui.closeSource}
      </a>
      <h1 className="visually-hidden" id={titleId}>
        {content.ui.sourceTitle(project.shortTitle)}
      </h1>

      {manifestState.status === "unavailable" ? (
        <section className="content-section">
          <h2>{content.ui.sourceUnavailable}</h2>
          <p>{content.ui.sourceUnavailableBody}</p>
        </section>
      ) : null}

      {manifestState.status === "loading" ? (
        <section className="content-section" aria-live="polite">
          <h2>{content.ui.loadingSourceSnapshot}</h2>
          <p>{content.ui.loadingSourceManifest}</p>
        </section>
      ) : null}

      {manifestState.status === "error" ? (
        <section className="content-section" role="alert">
          <h2>{content.ui.sourceManifestUnavailable}</h2>
          <p>{manifestState.message}</p>
        </section>
      ) : null}

      {manifestState.status === "ready" ? (
        <SourceBrowser
          content={content}
          manifest={manifestState.manifest}
          project={project}
          sourceAddress={sourceAddress}
        />
      ) : null}
    </article>
  );
}

function SourceModalShell({
  children,
  content,
  project,
}: {
  children: (titleId: string) => ReactNode;
  content: PortfolioContent;
  project: PortfolioProject;
}) {
  const titleId = `source-dialog-title-${project.slug}`;
  const shellRef = useRef<HTMLElement | null>(null);

  useEffect(() => {
    const shell = shellRef.current;

    if (!shell) {
      return;
    }

    const focusableElements = getFocusableElements(shell);
    const initialFocusTarget = focusableElements[0] ?? shell;

    initialFocusTarget.focus();

    const restoreBackground = setSourceModalBackgroundInert(shell);
    const restoreBodyScroll = lockSourceModalBodyScroll();
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        event.preventDefault();
        navigateToHashRoute(
          projectPath(project.slug, content.language, content.roleVariant),
        );
        return;
      }

      if (event.key !== "Tab") {
        return;
      }

      const tabbableElements = getFocusableElements(shell);

      if (tabbableElements.length === 0) {
        shell.focus();
        event.preventDefault();
        return;
      }

      const firstElement = tabbableElements[0];
      const lastElement = tabbableElements[tabbableElements.length - 1];
      const activeElement = document.activeElement;

      if (!shell.contains(activeElement)) {
        firstElement.focus();
        event.preventDefault();
        return;
      }

      if (event.shiftKey && activeElement === firstElement) {
        lastElement.focus();
        event.preventDefault();
        return;
      }

      if (!event.shiftKey && activeElement === lastElement) {
        firstElement.focus();
        event.preventDefault();
      }
    };

    document.addEventListener("keydown", handleKeyDown);

    return () => {
      document.removeEventListener("keydown", handleKeyDown);
      restoreBodyScroll();
      restoreBackground();
    };
  }, [content.language, project.slug]);

  return (
    <div className="source-modal-overlay">
      <div className="source-modal-backdrop" />
      <section
        aria-labelledby={titleId}
        aria-modal="true"
        className="source-modal-shell"
        ref={shellRef}
        role="dialog"
        tabIndex={-1}
      >
        {children(titleId)}
      </section>
    </div>
  );
}

function navigateToHashRoute(hash: string) {
  window.location.hash = hash;
  window.dispatchEvent(new Event("hashchange"));
}

const sourceModalFocusableSelector = [
  "a[href]",
  "button:not([disabled])",
  "input:not([disabled])",
  "select:not([disabled])",
  "textarea:not([disabled])",
  "[tabindex]:not([tabindex='-1'])",
].join(",");

type InertableHTMLElement = HTMLElement & {
  inert?: boolean;
};

function getFocusableElements(container: HTMLElement) {
  return Array.from(
    container.querySelectorAll<HTMLElement>(sourceModalFocusableSelector),
  ).filter((element) => {
    if (element.getAttribute("aria-disabled") === "true") {
      return false;
    }

    const tabIndex = element.getAttribute("tabindex");
    return tabIndex === null || Number(tabIndex) >= 0;
  });
}

function lockSourceModalBodyScroll() {
  const previousOverflow = document.body.style.overflow;

  document.body.style.overflow = "hidden";

  return () => {
    document.body.style.overflow = previousOverflow;
  };
}

function setSourceModalBackgroundInert(shell: HTMLElement) {
  const overlay = shell.closest<HTMLElement>(".source-modal-overlay");
  const backgroundElements = new Set<HTMLElement>();

  for (const selector of [".site-header"]) {
    const element = document.querySelector<HTMLElement>(selector);

    if (element) {
      backgroundElements.add(element);
    }
  }

  if (overlay?.parentElement) {
    for (const sibling of Array.from(overlay.parentElement.children)) {
      if (sibling !== overlay && sibling instanceof HTMLElement) {
        backgroundElements.add(sibling);
      }
    }
  }

  const previousStates = Array.from(backgroundElements).map((element) => {
    const inertElement = element as InertableHTMLElement;
    const state = {
      ariaHidden: element.getAttribute("aria-hidden"),
      element,
      hadInertAttribute: element.hasAttribute("inert"),
      inert: Boolean(inertElement.inert),
    };

    inertElement.inert = true;
    element.setAttribute("inert", "");
    element.setAttribute("aria-hidden", "true");

    return state;
  });

  return () => {
    for (const state of previousStates) {
      const inertElement = state.element as InertableHTMLElement;

      inertElement.inert = state.inert;

      if (state.hadInertAttribute) {
        state.element.setAttribute("inert", "");
      } else {
        state.element.removeAttribute("inert");
      }

      if (state.ariaHidden === null) {
        state.element.removeAttribute("aria-hidden");
      } else {
        state.element.setAttribute("aria-hidden", state.ariaHidden);
      }
    }
  };
}

function useSourceModalFocusRestore(route: AppRoute) {
  const previousRouteRef = useRef<AppRoute>(route);

  useEffect(() => {
    const previousRoute = previousRouteRef.current;

    if (
      previousRoute.name === "source" &&
      route.name === "project" &&
      previousRoute.project.slug === route.project.slug
    ) {
      window.setTimeout(() => {
        const sourceAction = document.querySelector<HTMLElement>(
          `a[href="${projectSourcePath(
            route.project.slug,
            {},
            route.language,
            route.roleVariant,
          )}"]`,
        );
        const projectHeading = document.querySelector<HTMLElement>(
          ".project-header h1",
        );

        (sourceAction ?? projectHeading)?.focus();
      }, 0);
    }

    previousRouteRef.current = route;
  }, [route]);
}

function SourceBrowser({
  content,
  manifest,
  project,
  sourceAddress,
}: {
  content: PortfolioContent;
  manifest: SourceManifest;
  project: PortfolioProject;
  sourceAddress: SourceAddress;
}) {
  const selection = resolveSourceSelection(manifest, sourceAddress);
  const [fileState, setFileState] = useState<SourceFileState>({ status: "idle" });
  const tree = useMemo(() => buildSourceTree(manifest.files), [manifest.files]);
  const selectedFilePath = selection.selectedFile?.path;
  const selectedTreeFileRef = useRef<HTMLAnchorElement | null>(null);
  const previousManifestFilesRef = useRef(manifest.files);
  const [expandedPaths, setExpandedPaths] = useState<Set<string>>(
    () => new Set(selectedSourceFolderPaths(selectedFilePath)),
  );

  useEffect(() => {
    if (previousManifestFilesRef.current === manifest.files) {
      return;
    }

    previousManifestFilesRef.current = manifest.files;
    setExpandedPaths(new Set(selectedSourceFolderPaths(selectedFilePath)));
  }, [manifest.files, selectedFilePath]);

  useEffect(() => {
    if (!selectedFilePath) {
      return;
    }

    setExpandedPaths((currentPaths) => {
      const nextPaths = new Set(currentPaths);

      for (const path of sourceAncestorPaths(selectedFilePath)) {
        nextPaths.add(path);
      }

      return nextPaths;
    });
  }, [selectedFilePath]);

  useEffect(() => {
    selectedTreeFileRef.current?.scrollIntoView({
      block: "center",
      inline: "nearest",
    });
  }, [expandedPaths, selectedFilePath]);

  useEffect(() => {
    if (!project.source || !selection.selectedFile) {
      setFileState({ status: "idle" });
      return;
    }

    const controller = new AbortController();
    const snapshotPath =
      selection.selectedFile.snapshotPath ?? `files/${selection.selectedFile.path}`;
    const sourceRoot = project.source.manifestPath.replace(/manifest\.json$/, "");
    const fileUrl = encodeURI(`${getAppBasePath()}${sourceRoot}${snapshotPath}`);

    setFileState({ status: "loading" });

    fetch(fileUrl, { signal: controller.signal })
      .then((response) => {
        if (!response.ok) {
          throw new Error(`Source file request failed with ${response.status}.`);
        }

        return response.text();
      })
      .then((content) => setFileState({ status: "ready", content }))
      .catch((error: unknown) => {
        if (controller.signal.aborted) {
          return;
        }

        setFileState({
          status: "error",
          message:
            error instanceof Error
              ? error.message
              : content.ui.sourceUnknownFileError,
        });
      });

    return () => controller.abort();
  }, [content.ui.sourceUnknownFileError, project, selection.selectedFile]);

  return (
    <>
      <section
        aria-label={content.ui.sourceBrowser}
        className="source-browser"
      >
        <aside
          aria-label={content.ui.sourceFiles}
          className="source-tree-pane"
          id="source-files-pane"
        >
          <SourceTree
            content={content}
            expandedPaths={expandedPaths}
            nodes={tree}
            onToggleFolder={(path) =>
              setExpandedPaths((currentPaths) => {
                const nextPaths = new Set(currentPaths);

                if (nextPaths.has(path)) {
                  nextPaths.delete(path);
                } else {
                  nextPaths.add(path);
                }

                return nextPaths;
              })
            }
            project={project}
            selectedFileRef={selectedTreeFileRef}
            selectedPath={selection.selectedFile?.path}
          />
        </aside>
        <SourceCodePane
          content={content}
          file={selection.selectedFile}
          fileState={fileState}
          project={project}
          selectedLine={selection.selectedLine}
        />
      </section>
    </>
  );
}

function buildSourceTree(files: SourceManifestFile[]): SourceTreeNode[] {
  const root: SourceTreeNode = { name: "", path: "", children: [] };

  for (const file of sortSourceFiles(files)) {
    const parts = file.path.split("/");
    let node = root;

    parts.forEach((part, index) => {
      const path = parts.slice(0, index + 1).join("/");
      const isFile = index === parts.length - 1;
      let child = node.children.find((candidate) => candidate.name === part);

      if (!child) {
        child = { name: part, path, children: [] };
        node.children.push(child);
      }

      if (isFile) {
        child.file = file;
      }

      node = child;
    });
  }

  sortSourceTree(root);
  return root.children;
}

function sortSourceTree(node: SourceTreeNode) {
  node.children.sort((a, b) => {
    if (Boolean(a.file) !== Boolean(b.file)) {
      return a.file ? 1 : -1;
    }

    return compareSourcePath(a.name, b.name);
  });

  for (const child of node.children) {
    sortSourceTree(child);
  }
}

function SourceTree({
  content,
  expandedPaths,
  nodes,
  onToggleFolder,
  project,
  selectedFileRef,
  selectedPath,
}: {
  content: PortfolioContent;
  expandedPaths: Set<string>;
  nodes: SourceTreeNode[];
  onToggleFolder: (path: string) => void;
  project: PortfolioProject;
  selectedFileRef: RefObject<HTMLAnchorElement | null>;
  selectedPath?: string;
}) {
  if (nodes.length === 0) {
    return <p>{content.ui.sourceFilesEmpty}</p>;
  }

  return (
    <ul className="source-tree-list">
      {nodes.map((node) => (
        <SourceTreeItem
          content={content}
          expandedPaths={expandedPaths}
          key={node.path}
          node={node}
          onToggleFolder={onToggleFolder}
          project={project}
          selectedFileRef={selectedFileRef}
          selectedPath={selectedPath}
        />
      ))}
    </ul>
  );
}

function SourceTreeItem({
  content,
  expandedPaths,
  node,
  onToggleFolder,
  project,
  selectedFileRef,
  selectedPath,
}: {
  content: PortfolioContent;
  expandedPaths: Set<string>;
  node: SourceTreeNode;
  onToggleFolder: (path: string) => void;
  project: PortfolioProject;
  selectedFileRef: RefObject<HTMLAnchorElement | null>;
  selectedPath?: string;
}) {
  if (node.file) {
    const isSelected = node.file.path === selectedPath;

    return (
      <li className="source-tree-file">
        <a
          aria-label={node.file.path}
          aria-current={isSelected ? "page" : undefined}
          className={isSelected ? "selected" : undefined}
          href={projectSourcePath(
            project.slug,
            { file: node.file.path },
            content.language,
            content.roleVariant,
          )}
          ref={isSelected ? selectedFileRef : undefined}
        >
          <span>{node.name}</span>
          <small>
            {ownershipLabel(content, node.file.ownership)} -{" "}
            {content.ui.lines(node.file.lines)}
          </small>
        </a>
      </li>
    );
  }

  const isExpanded = expandedPaths.has(node.path);

  return (
    <li className="source-tree-folder">
      <button
        aria-expanded={isExpanded}
        aria-label={content.ui.toggleFolder(node.path)}
        className="source-folder-label"
        onClick={() => onToggleFolder(node.path)}
        type="button"
      >
        {node.name}
      </button>
      {isExpanded ? (
        <SourceTree
          content={content}
          expandedPaths={expandedPaths}
          nodes={node.children}
          onToggleFolder={onToggleFolder}
          project={project}
          selectedFileRef={selectedFileRef}
          selectedPath={selectedPath}
        />
      ) : null}
    </li>
  );
}

function SourceCodePane({
  content,
  file,
  fileState,
  project,
  selectedLine,
}: {
  content: PortfolioContent;
  file?: SourceManifestFile;
  fileState: SourceFileState;
  project: PortfolioProject;
  selectedLine?: number;
}) {
  const [highlightedLines, setHighlightedLines] = useState<
    SourceHighlightLine[] | undefined
  >(undefined);
  const selectedLineRef = useRef<HTMLSpanElement | null>(null);

  useEffect(() => {
    if (fileState.status !== "ready") {
      setHighlightedLines(undefined);
      return;
    }

    let ignoreHighlight = false;

    setHighlightedLines(undefined);

    highlightSource(fileState.content, file?.language)
      .then((result) => {
        if (ignoreHighlight || result.kind !== "highlighted") {
          return;
        }

        setHighlightedLines(result.lines);
      })
      .catch(() => {
        if (!ignoreHighlight) {
          setHighlightedLines(undefined);
        }
      });

    return () => {
      ignoreHighlight = true;
    };
  }, [file?.language, fileState]);

  useEffect(() => {
    if (fileState.status !== "ready" || !selectedLine) {
      return;
    }

    selectedLineRef.current?.scrollIntoView({
      block: "center",
      inline: "nearest",
    });
  }, [fileState.status, highlightedLines, selectedLine]);

  return (
    <section
      aria-label={content.ui.codeReader}
      className="source-code-pane"
      id="source-code-pane"
    >
      {!file ? <p>{content.ui.sourceFilesEmpty}</p> : null}

      {file && fileState.status === "loading" ? (
        <p className="source-pane-message" aria-live="polite">
          {content.ui.loadingSourceFile}
        </p>
      ) : null}

      {file && fileState.status === "error" ? (
        <p className="source-pane-message" role="alert">
          {fileState.message}
        </p>
      ) : null}

      {file && fileState.status === "ready" ? (
        <pre className="source-code-block">
          <code>
            {splitSourceLines(fileState.content).map((line, index) => {
              const lineNumber = index + 1;
              const highlightedLine = highlightedLines?.[index];

              return (
                <span
                  className={
                    selectedLine === lineNumber
                      ? "source-code-line selected"
                      : "source-code-line"
                  }
                  data-line-number={lineNumber}
                  key={lineNumber}
                  ref={selectedLine === lineNumber ? selectedLineRef : undefined}
                >
                  <a
                    aria-label={content.ui.line(lineNumber)}
                    className="source-line-number"
                    href={projectSourcePath(
                      project.slug,
                      {
                        file: file.path,
                        line: lineNumber,
                      },
                      content.language,
                      content.roleVariant,
                    )}
                  >
                    {lineNumber}
                  </a>
                  <SourceLineText highlightedLine={highlightedLine} line={line} />
                </span>
              );
            })}
          </code>
        </pre>
      ) : null}
    </section>
  );
}

function SourceLineText({
  highlightedLine,
  line,
}: {
  highlightedLine?: SourceHighlightLine;
  line: string;
}) {
  if (!highlightedLine) {
    return <span className="source-line-text">{line || " "}</span>;
  }

  return (
    <span className="source-line-text">
      {highlightedLine.tokens.map((token, index) => (
        <span
          className="source-token"
          data-token-kind={token.semanticKind}
          key={`${index}:${token.content}`}
          style={{
            color: token.color,
            fontStyle: token.fontStyle ? "italic" : undefined,
          }}
        >
          {token.content}
        </span>
      ))}
    </span>
  );
}

function NotFoundPage({
  content,
  route,
}: {
  content: PortfolioContent;
  route: AppRoute & { name: "not-found" };
}) {
  const project = route.slug ? content.getProjectBySlug(route.slug) : undefined;

  return (
    <section className="not-found-page">
      <h1>
        {project ? content.ui.projectRouteUnavailable : content.ui.notFoundPage}
      </h1>
      <p>
        {route.slug
          ? content.ui.noPublicProject(route.slug)
          : content.ui.unmatchedRoute}
      </p>
      <a
        className="text-link"
        href={roleHomePath(content.roleVariant, content.language)}
      >
        {content.ui.returnToProjects}
      </a>
    </section>
  );
}

function App() {
  const route = useHashRoute();
  const content = useMemo(
    () => getPortfolioContent(route.language, route.roleVariant),
    [route.language, route.roleVariant],
  );

  useRouteEffects(route, content);
  useSourceModalFocusRestore(route);

  return (
    <div className="app-shell">
      {route.name === "home" ? (
        <HomeLanguageToggle content={content} />
      ) : (
        <SiteHeader content={content} />
      )}
      <main>
        {route.name === "home" ? <HomePage content={content} /> : null}
        {route.name === "project" ? (
          <ProjectDetailPage content={content} project={route.project} />
        ) : null}
        {route.name === "source" ? (
          <>
            <ProjectDetailPage content={content} project={route.project} />
            <SourceModalShell content={content} project={route.project}>
              {(titleId) => (
                <SourceSnapshotPage
                  content={content}
                  key={route.project.slug}
                  project={route.project}
                  sourceAddress={route.sourceAddress}
                  titleId={titleId}
                />
              )}
            </SourceModalShell>
          </>
        ) : null}
        {route.name === "not-found" ? (
          <NotFoundPage content={content} route={route} />
        ) : null}
      </main>
    </div>
  );
}

export { projects };
export default App;
