export const languages = ["en", "fr"] as const;

export type Language = (typeof languages)[number];

export const defaultLanguage: Language = "en";

export const roleVariants = [
  "unreal-engine-programmer",
  "qa-tester",
  "ai-game-designer",
] as const;

export type RoleVariant = (typeof roleVariants)[number];

export const defaultRoleVariant: RoleVariant = "unreal-engine-programmer";

export type ProjectSlug =
  | "hospital-flow"
  | "agass"
  | "tlm"
  | "moba-gas"
  | "commonui-frontend"
  | "gas-crash-course"
  | "multiplayer-crash-course"
  | "unreal-2d"
  | "slash-cpp"
  | "cpp-game-development"
  | "blueprint-prototypes";

export type ProjectCategory = "original" | "learning";

export type MediaSlot = {
  id: string;
  type: "image" | "video";
  aspectRatio: "16 / 9" | "4 / 3";
  status: "placeholder" | "ready";
  caption: string;
  source?: string;
  poster?: string;
};

export type ProjectAction = {
  id: string;
  label: string;
  kind: "internal" | "external" | "source" | "build" | "course" | "contact" | "download";
  status: "ready" | "pending";
  href?: string;
  download?: string;
  opensInNewTab?: boolean;
  metaLabel?: string;
  pendingReason?: string;
};

export type RuntimeSourceMetadata = {
  status: "available";
  manifestPath: `source/${ProjectSlug}/manifest.json`;
};

export type CodeReference = {
  kind: "code-reference";
  label: string;
  file: string;
  line?: number;
};

export type RichTextContent = string | Array<string | CodeReference>;

export type ContentSection = {
  title: string;
  body: RichTextContent;
  mediaSlotId?: string;
  points?: RichTextContent[];
  sourceReferences?: CodeReference[];
};

export type ProjectPageContent = {
  template: "original" | "learning";
  overview: RichTextContent;
  roleScope: RichTextContent;
  sections: ContentSection[];
  proof: RichTextContent[];
  improvements: RichTextContent[];
  course?: {
    title: string;
    provider: string;
    instructor: string;
    url: string;
  };
  courseContext?: RichTextContent;
  whatBuilt?: RichTextContent[];
  whatLearned?: RichTextContent[];
  scopeNotes?: RichTextContent[];
};

export type PortfolioProject = {
  slug: ProjectSlug;
  visibility: "public" | "archived";
  category: ProjectCategory;
  title: string;
  shortTitle: string;
  route: `#/projects/${ProjectSlug}` | `#/projects/${ProjectSlug}?lang=fr`;
  summary: string;
  technologies: string[];
  mediaSlots: MediaSlot[];
  actions: ProjectAction[];
  source?: RuntimeSourceMetadata;
  page: ProjectPageContent;
  includeInPreviousNext: boolean;
};

export type SiteProfile = {
  name: string;
  role: string;
  location: string;
  availability: string;
  contact: {
    email: string;
    phone: string;
  };
  positioning: string;
  actions: ProjectAction[];
};

export type PortfolioUiLabels = {
  profileActions: string;
  projectActions: string;
  profileFacts: string;
  contact: string;
  location: string;
  availability: string;
  personalProjects: string;
  learningProjects: string;
  learningProjectsIntro: string;
  viewProject: string;
  technologies: (projectTitle: string) => string;
  mediaSlots: (projectTitle: string) => string;
  placeholder: (type: MediaSlot["type"], caption: string) => string;
  backToProjects: string;
  courseContext: string;
  whatBuilt: string;
  whatLearned: string;
  attributionAndScope: string;
  technicalProof: string;
  improveNext: string;
  overview: string;
  sourceReferences: (sectionTitle: string) => string;
  sourceUnavailable: string;
  sourceUnavailableBody: string;
  loadingSourceSnapshot: string;
  loadingSourceManifest: string;
  sourceManifestUnavailable: string;
  sourceUnknownManifestError: string;
  sourceBrowser: string;
  sourceFiles: string;
  sourceFilesEmpty: string;
  codeReader: string;
  loadingSourceFile: string;
  sourceUnknownFileError: string;
  closeSource: string;
  sourceTitle: (projectShortTitle: string) => string;
  line: (lineNumber: number) => string;
  toggleFolder: (path: string) => string;
  lines: (count: number) => string;
  ownershipLabels: Record<string, string>;
  notFoundPage: string;
  projectRouteUnavailable: string;
  noPublicProject: (slug: string) => string;
  unmatchedRoute: string;
  returnToProjects: string;
  documentTitle: {
    home: (profile: SiteProfile) => string;
    project: (project: PortfolioProject, profile: SiteProfile) => string;
    source: (project: PortfolioProject, profile: SiteProfile) => string;
    notFound: (profile: SiteProfile) => string;
  };
  languageToggle: string;
};

export type PortfolioContent = {
  language: Language;
  roleVariant: RoleVariant;
  siteProfile: SiteProfile;
  projectRegistry: Record<ProjectSlug, PortfolioProject>;
  projects: PortfolioProject[];
  publicProjects: PortfolioProject[];
  featuredOriginalProjects: PortfolioProject[];
  learningProjects: PortfolioProject[];
  getProjectBySlug: (slug: string) => PortfolioProject | undefined;
  getAdjacentProjects: (slug: ProjectSlug) => {
    previous?: PortfolioProject;
    next?: PortfolioProject;
  };
  ui: PortfolioUiLabels;
};
