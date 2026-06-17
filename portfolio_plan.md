# Game Dev Portfolio Plan

## Summary

Build an English custom portfolio website for Baptiste Giquet, positioned as a Junior Unreal Engine Gameplay Programmer and hosted on GitHub Pages.

The site should use a dark Unreal-focused technical portfolio style and prioritize proof over broad claims: short videos, screenshots, system diagrams, contribution notes, debugging/testing examples, downloadable builds, and clean source-only repositories.

## Positioning

- Name: Baptiste Giquet.
- Role: Junior Unreal Engine Gameplay Programmer.
- Location: Caen, France.
- Availability: France-based or remote junior Unreal Engine gameplay roles.
- Hero line: Junior Unreal Engine Gameplay Programmer focused on networked gameplay systems, GAS-driven interactions, CommonUI workflows, and debugging tools.

Avoid claiming professional studio experience, shipped commercial game experience, expert AI/game AI experience, professional QA experience, native-level spoken English, US work authorization, or dedicated-server experience.

## Homepage Structure

1. Hero with name, role, location/availability, short positioning line, featured work CTA, contact CTA, and CV download link.
2. Featured work, led by the main hospital/ER project.
3. Dedicated testing/debugging supporting case study.
4. Additional Unreal Projects section for course-based projects, prototypes, and smaller experiments.
5. Evidence-linked skills section.
6. Compact visual timeline in About.
7. Contact/footer.

## Main Featured Project

Title: Networked Co-op Hospital Flow System.

Subtitle: UE5 listen-server co-op project with server-authoritative patient routing, replicated queues, GAS-driven interactions, and controller-friendly CommonUI workflows.

Primary action: View case study.

Secondary action: Download build.

Source wording: Source code available for review on request, subject to asset-license restrictions. A public source-only repo may be linked after cleanup.

The main case study should be presented as a technical systems case study first, with the game pitch included briefly.

### Case Study Sections

1. Networked Co-op Flow
   - Listen-server co-op.
   - Server-authoritative interactions.
   - Replicated patient, queue, and machine state.
   - Reception validation with client feedback.

2. Patient Routing & Treatment Logic
   - Pathology-driven exam requirements.
   - Reception to exam to treatment to payment/exit.
   - Waiting-room, exam, and treatment queues.
   - AI navigation intents.
   - Pacing through spawn rate and patience pressure.

3. Controller-Friendly Tools & UI
   - CommonUI/CommonInput foundation.
   - Controller-first frontend/lobby flow.
   - Triage/machine UI focus rules.
   - World-space widget interaction.
   - Developer/debug tools and automation proof.

## Testing & Debugging Case Study

Title: Runtime Debug Tools & Automated Gameplay Validation.

Subtitle: In-game developer tools, telemetry, replicated debug snapshots, and deterministic automation used to reproduce and validate patient-flow and run-progression bugs.

Frame this as gameplay reliability work, not professional QA experience.

## Additional Unreal Projects

Course projects, small prototypes, and experiments should appear as compact cards ordered by relevance/strength.

Each course-based card should visibly state:

> Course-based project from [Course Name](link), extended with custom work in [specific systems/features].

Course projects should not compete with the main case study unless they were substantially extended beyond the course scope.

## Source-Only Repository Policy

Every showcased project should provide a clean public source-only repository when possible.

Source-only repositories include:

- Original gameplay/source code.
- Selected config when useful.
- README and technical notes.
- Screenshots or media links when useful.

Source-only repositories exclude:

- Third-party assets.
- Paid plugin content.
- Marketplace/Fab content.
- MetaHuman assets.
- Private project content.
- Generated folders.
- Binaries.
- Packaged builds.
- Raw development history.

Before publishing any source-only repository, run a cleanup pass to remove development noise and make the code suitable for external review. This includes removing AI-tool artifacts, scratch comments, obsolete experiments, misleading TODOs, generated clutter, dead code, unused files, and private workflow notes.

Publish each source-only repo as a fresh clean repository. Preserve readability and maintainability over raw history.

README requirements:

- Solo projects can state "Solo project" and focus on architecture/source review.
- Course-based or collaborative projects must include a "My contribution" section.
- Course-based projects must include course name and link.
- Include a "What is not included" section.
- State clearly if the repo is intended for source review and may not compile standalone.

## Media Requirements

For every featured or additional project:

- 1 short video/GIF or 2-3 screenshots minimum.
- Source-only repo link when available.
- Playable build link when available.
- 2-4 technical proof bullets.
- Clear status: solo, course-based, or collaborative.

For the main hospital/ER project:

- 60-90 second systems walkthrough with light captions.
- 3-5 short clips for networking, patient flow, UI/controller, and debug tools.
- 6-10 screenshots.
- Downloadable build if ready.
- Source-only repo link after cleanup.
- One clean conceptual diagram: reception to exam queue to machine exam to treatment queue to treatment bed to payment/exit, with server validation and replicated state notes.

## Timeline

Use a compact visual timeline in About:

- June-November 2025: structured Unreal Engine course projects.
- November 2025-March 2026: main UE5 hospital/ER co-op project.
- Since March 2026: smaller Unreal projects, portfolio polish, debugging/tooling practice, and gameplay systems experiments.

## Skills Section

Use evidence-linked skill groups:

- Unreal Gameplay: C++, Blueprints, GAS, Enhanced Input, CommonUI.
- Networking: listen-server co-op, RPCs, replication, replicated UI/game state.
- Systems: patient flow, data-driven gameplay, AI routing, state/tags.
- Debugging & Validation: developer tools, telemetry, automation tests, bug reproduction.

## Release Strategy

Ship v1 once the main project proof is ready. Do not block v1 on every smaller project source-only repo.

V1 should include:

- Homepage.
- Main hospital/ER case study.
- Testing/debugging case study.
- Additional projects section with compact proof.
- Contact/CV.
- Main downloadable build if ready.
- Main hospital/ER source-only repo after cleanup, or no source link until ready.

## Do Not Publish Checklist

Do not publish source repos or builds if they include:

- Third-party assets or paid plugin content.
- Secrets, tokens, personal paths, or private notes.
- AI-tool artifacts or scratch prompts.
- Generated folders or binaries.
- Misleading TODOs or dead code.
- Broken README links.
- Unsupported claims about role level, studio experience, dedicated servers, or commercial shipping.
