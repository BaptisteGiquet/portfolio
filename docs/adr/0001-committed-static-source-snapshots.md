# Committed static source snapshots

The portfolio will expose project source through committed static source snapshots under `public/source/<project-slug>/` instead of linking directly to local Unreal project paths or depending on GitHub repository pages. This keeps the GitHub Pages deployment self-contained, lets project detail copy link to stable source-browser routes, and preserves attribution through manifest ownership metadata while accepting the trade-off that snapshots must be manually regenerated after local source changes.
