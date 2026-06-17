# Portfolio Agent Notes

This repository contains the planning and implementation workspace for Baptiste Giquet's game development portfolio.

## Agent skills

### Issue tracker

Issues and PRDs are tracked in this repository's GitHub Issues. See `docs/agents/issue-tracker.md`.

### Triage labels

The default five-label vocabulary is used: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, and `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

This is a single-context repo. Domain terms live in `CONTEXT.md` if one is created, and architectural decisions live in `docs/adr/`. See `docs/agents/domain.md`.

### Portfolio project context

Use `docs/project-context.md` as the source summary of Baptiste's different game projects, their technical evidence, portfolio-safe positioning, caveats, and media needs.

### Local npm recovery

If `npm run build` fails before reaching this project with a missing `npm-cli.js` under `C:\Users\Home\AppData\Roaming\npm`, do not run `npm install -g npm@latest`. That recreates a user-level npm package that the Windows npm shim may prefer over the Node.js bundled npm.

Instead, remove or rename the user-level npm package, then verify the bundled npm is used:

```powershell
Rename-Item "$env:APPDATA\npm\node_modules\npm" "npm.user-level-backup-$(Get-Date -Format yyyyMMdd-HHmmss)" -ErrorAction SilentlyContinue
where.exe npm
npm -v
npm run build
```

If that still fails, verify which shims are being used with `where.exe node`, `where.exe npm`, and `Get-Command npm -All`, then repair or reinstall the Windows Node.js LTS installer so `npm -v` works in a fresh PowerShell session before retrying Codex.
