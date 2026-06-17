# Recruiter Portfolio Redesign Brief

Issue: #12  
Parent PRD: #11  
Reference inspected: https://atradr.github.io/index.html and https://atradr.github.io/ancient-outskirts.html on desktop `1440x1000` and mobile `390x844`.

## Goal

Redesign Baptiste Giquet's portfolio into a restrained, recruiter-focused static site for a Junior Unreal Engine Gameplay Programmer in Caen, France, targeting France-based or remote junior Unreal gameplay roles.

The reference portfolio should guide structure, rhythm, density, and media hierarchy. It must not be copied for identity, copy, color branding, project framing, assets, or personal voice.

## Reference Takeaways

- Simple top navigation: identity on the left, project/navigation actions on the right, compact mobile header.
- Mostly white page with dark text, restrained accents, and generous margins.
- Hero is short and typographic, not a marketing splash screen.
- Homepage is project-first: strongest project appears first, followed by additional projects and a short about/contact area.
- Desktop project rows use a media column plus a text/evidence column.
- Mobile collapses into a single readable column, keeping media before project text.
- Project detail pages open with title, media, overview, then process/evidence sections.
- Media proportions are stable and practical, usually landscape gameplay/map captures.
- Copy is direct and specific: role, work performed, project objective, tools, and supporting details.
- The page avoids decorative UI chrome and does not frame the portfolio as a dashboard.

## Visual Direction

Use a quiet editorial portfolio style rather than an app dashboard. The page should feel hand-authored, readable, and evidence-led.

- Background: plain off-white or white.
- Text: dark neutral body text with high contrast.
- Accent: one restrained, non-purple accent for links/focus/action states.
- Surfaces: avoid stacked cards; use open page sections and simple dividers.
- Corners: keep any required cards or controls at small radii.
- Motion: hover/focus states only.
- Imagery: use plain gray placeholders until real gameplay media exists.

## Typography Density

Use a compact type scale with enough hierarchy for scanning.

- Hero H1: role-first, concise, not oversized.
- Section headings: clear and compact.
- Project titles: prominent but not hero-scale inside cards/rows.
- Body copy: normal reading size, moderate line height, no uppercase label-heavy treatment.
- Lists: use only where they improve scan speed, especially proof bullets and technology/scope details.

Do not scale font sizes with viewport width. Keep letter spacing at `0`.

## Palette Direction

Use a neutral technical portfolio palette.

- Base: white/off-white.
- Text: near-black and mid-gray.
- Rules/borders/placeholders: light to medium neutral gray.
- Accent: muted blue, green, or amber may be used sparingly for links and focus states.

Avoid heavy dark gradients, purple/purple-blue gradient themes, beige/brown one-note palettes, decorative orbs, and game-HUD styling.

## Homepage Structure

1. Header with Baptiste's name, role/contact affordances, and simple project navigation.
2. Concise hero:
   - "Junior Unreal Engine Gameplay Programmer" must be immediately visible.
   - Include Caen, France and France-based/remote junior Unreal gameplay role targeting.
   - Mention networked gameplay systems, GAS interactions, controller-friendly UI, and debugging tools in compact supporting copy.
3. Featured original systems work:
   - Hospital project first.
   - AGASS under the public title "Co-op Physics Tower Builder" second.
   - Each entry has one homepage action: view project.
4. Learning projects:
   - Individual entries, not one grouped catch-all card.
   - Order: MOBA GAS, Unreal 2D, GAS crash course, multiplayer crash course, Slash/C++, Blueprint prototypes.
   - Clearly label course-based work.
5. Compact skills/evidence section:
   - Use grouped text, not a badge cloud.
6. Short about/timeline section:
   - Focus on Unreal learning progression and original systems work.
7. Footer/contact:
   - Use a future contact space instead of GitHub or Email buttons.
   - CV remains pending until a valid artifact exists.

## Detail Page Templates

### Original Project Pages

Order:

1. Header/actions.
2. Media placeholders.
3. Overview.
4. Role / Scope.
5. Systems sections.
6. Proof.
7. What I would improve.
8. Previous/next and contact.

Hospital owns networked hospital flow, patient behavior/gameplay pressure, runtime debug/validation, and controller-friendly UI sections. Patient behavior and debug/validation must not become standalone routes.

AGASS owns co-op tower gameplay, pickup/placement, physics/collision reliability, support validation, economy, hazards, online/session flow, and modding pipeline sections. Modding copy must stay scoped to content-plugin/map/event support with controlled Blueprint extension points.

### Learning Project Pages

Order:

1. Header/actions/course attribution.
2. Media placeholders.
3. Course context.
4. What I built.
5. What I learned.
6. Custom extensions/refactors where supported.
7. Scope notes.
8. What I would improve.
9. Previous/next and contact.

Course pages must distinguish course curriculum from Baptiste's local implementation. Course context should be verified, attributed, and paraphrased.

## Placeholder Treatment

Use content-driven media slots that can later become images or videos without component rewrites.

For v1, render media slots as plain gray landscape boxes.

Rules:

- No text inside placeholders.
- No fake screenshots.
- No fake diagrams.
- No fake charts.
- No fake browser frames.
- No fake play controls.
- No simulated UI proof.
- No labels inside the media box.

Captions or pending-media notes may sit outside the placeholder only when they help clarify asset status.

## Navigation And Actions

- Use hash routes for GitHub Pages: `#/` and `#/projects/:slug`.
- Keep unknown routes visible with a small not-found page.
- Header and footer should provide a consistent contact path.
- Homepage project cards should only expose "View project".
- Detail pages may expose ready source snapshots, course links, curated downloads, and CV actions.
- Active internal/external actions render as anchors.
- External links opening new tabs must use safe `rel` attributes.
- Missing source, build, GitHub, email, or contact artifacts should not render placeholder buttons.

## Explicit Bans

- Do not clone the reference identity, copy, logo, assets, color branding, or personal voice.
- Do not add fake screenshots, fake diagrams, fake charts, fake browser frames, fake play controls, fake metrics, or simulated proof.
- Do not render global readiness, internal QA, or media-capture checklist sections as public portfolio content.
- Do not use heavy gradients, decorative blobs/orbs, parallax, counters, scroll reveals, or decorative motion.
- Do not use nested framed panels, dashboard cards, mock terminal/UI frames, or oversized product-dashboard sections.
- Do not add source/build/download links that look active without valid URLs.
- Do not imply shipped commercial work, professional studio experience, professional QA, production infrastructure, dedicated-server packaging, original art ownership, or native-code modding unless supported by evidence.
- Do not split small course exercises into thin pages when the PRD says to group them.

## Replacement Direction For Current Site

The current single-page, dashboard-like implementation should be replaced by a small static portfolio system:

- One canonical project registry.
- Hash-routed homepage and project detail pages.
- Reusable page primitives for shell, navigation, media slots, action groups, project summaries, project headers, content sections, and previous/next navigation.
- No public planning/checklist sections.
- No mock media components.
- No flow-diagram or test-matrix public sections for v1.
- Honest pending action states for source, build, CV, email, and media.

## Approval Checklist

- [x] Live reference inspected on desktop.
- [x] Live reference inspected on mobile.
- [x] Visual direction defined.
- [x] Section order defined.
- [x] Typography density defined.
- [x] Palette/theme direction defined.
- [x] Placeholder treatment defined.
- [x] Navigation and page templates defined.
- [x] Explicit implementation bans documented.
- [x] Baptiste's safe positioning preserved.
