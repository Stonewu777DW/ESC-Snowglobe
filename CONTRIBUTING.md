# Contributing

Anyone can inspect this public repo, fork it, report issues and propose changes
through pull requests. Anonymous direct writes to main are not enabled.

## Shared main site

The owner has invited Stone (`Stonewu777DW`) with **write** access to this repo;
Stone must accept the GitHub invitation before pushing. Every collaborator with
write access can push to `main`. Once the pipeline is published and Cloudflare
credentials are configured, any such push or merged PR runs tests and automatically
updates the same main site—no separate deployment approval or personal Cloudflare
account is required. Failed tests prevent deployment. Changes only on a contributor's
own fork do not replace the main site; open a PR here first.

1. Read INDEX.md and docs/ARCHITECTURE.md. Keep the main UI minimal; setup belongs on the guide.
2. Branch from main. Preserve firmware, tag and transport contracts.
3. Run npm test and npm run build. Add regression tests for behavioral changes.
4. Open a PR describing the change, automated checks and actual hardware evidence.
5. A maintainer reviews/merges. Main auto-deploys after passing CI once Cloudflare
   credentials have been enabled.

Do not commit API keys, .env files, Wi-Fi passwords, private logs or personal
account data. Do not add third-party music without documenting its source and
permission. Existing recordings are not covered by a code license.
