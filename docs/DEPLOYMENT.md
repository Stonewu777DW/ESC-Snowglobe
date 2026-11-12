# Deployment and operations

## Current state

- Source: https://github.com/sidatpho/ESC-Snowglobe (public fork).
- Original: https://github.com/Stonewu777DW/ESC-Snowglobe (push denied, 403).
- Target: Cloudflare Pages in the owner's account, project `esc-snowglobe`.
- **Not live yet:** the saved Cloudflare token reports expired. No production URL
  or successful Cloudflare deployment is claimed.
- Auto-deploy is defined in .github/workflows/ci.yml but disabled pending credentials.
  This is CI-driven Direct Upload, not the Cloudflare GitHub App integration.
  Do not configure both simultaneously.

## Build

```sh
npm ci
npm run build
python3 -m http.server 8000 --directory dist
```

Deploy only dist/. Its index is the new BLE player; the source root index.html is
retained legacy MQTT code. The export includes wireless.html as an alias, guide,
CSS, JS, four selected songs, headers and build-info.json. The manifest records
the Git commit and asset digest for verification.

## One-time credentials and project setup

1. Renew the owner's Cloudflare credential, or create a token restricted to the
   intended account with **Account → Cloudflare Pages → Edit**. No Workers,
   Tunnel, Global API Key or token-administration permission is needed.
2. Keep it outside Git. The maintainer's private Mac location is
   ~/.config/cloudflare/api-token. Do not print it or put it in issues/chat.
3. Set Actions secrets CLOUDFLARE_API_TOKEN and CLOUDFLARE_ACCOUNT_ID. Supply
   values to gh secret set on stdin, not command-line literals.
4. Create Pages project esc-snowglobe with production branch main, or use the
   idempotent tools/ensure_pages_project.py with those environment variables.
5. Set repository variable CLOUDFLARE_DEPLOY_ENABLED=true only after verifying
   the account/project. Dispatch the workflow on main.
6. Verify the deploy job, actual Pages hostname, served player/guide/assets and
   build-info.json commit. Do not guess a pages.dev URL; name collisions can suffix it.

## Automatic updates

PRs run tests only. Pushes/merges to main run the complete tests, then upload dist
with pinned Wrangler when the deploy gate is enabled. Failed tests block deploy.
The workflow has read-only GitHub contents permission, fixed production branch,
and serialized deployment concurrency. External PRs have no Cloudflare secrets.
Maintainers may need to approve Actions runs for first-time contributors.

## Manual deployment and recovery

```sh
npm run build
# Securely provide CLOUDFLARE_API_TOKEN and CLOUDFLARE_ACCOUNT_ID in the environment.
python3 tools/ensure_pages_project.py
npx --yes wrangler@4.160.1 pages deploy dist --project-name esc-snowglobe --branch main
```

Disable CLOUDFLARE_DEPLOY_ENABLED to pause. Revert a bad main commit and push to
roll forward to the previous source; wait for tests/deploy and verify the served
build-info.json. Do not modify unrelated Pages projects, portfolio or DNS.
No custom domain is provisioned by this project.

## Primary documentation checked

- https://developers.cloudflare.com/pages/how-to/use-direct-upload-with-continuous-integration/
- https://developers.cloudflare.com/pages/get-started/git-integration/

CI Direct Upload is chosen to gate publication on tests without desktop operations.
