# Domain Docs

How the engineering skills should consume this repo's domain documentation when exploring the codebase.

## Before Exploring, Read These

- `CONTEXT.md` at the repo root, if it exists.
- `CONTEXT-MAP.md` at the repo root, if it exists.
- `docs/adr/`, if it exists.

If any of these files do not exist, proceed silently. The producer skill (`grill-with-docs`) creates them lazily when terms or decisions actually get resolved.

## Layout

This is a single-context repository.

Expected structure if domain docs are later created:

```text
/
├── CONTEXT.md
├── docs/
│   └── adr/
└── src/
```

## Vocabulary

When output names a domain concept in an issue title, PRD, implementation plan, refactor proposal, hypothesis, or test name, use the term as defined in `CONTEXT.md`.

If the concept is not in the glossary yet, either reconsider whether the term belongs here or note the gap for a future `grill-with-docs` pass.

## ADR Conflicts

If output contradicts an existing ADR, surface that conflict explicitly instead of silently overriding it.
