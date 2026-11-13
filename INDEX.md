# ESC Snowglobe — project index / agent handoff

## Product and source

Selected: battery-powered **BLE** pedestal + minimal static website. Arduino reads
the globe; the computer plays audio through its OS-selected output. No Wi-Fi,
room code or USB during normal use. Speaker pairing belongs to the computer.

- Public fork: https://github.com/sidatpho/ESC-Snowglobe
- Original: https://github.com/Stonewu777DW/ESC-Snowglobe
- Owner authorized the fork after upstream returned push:false and HTTP 403.
- Local origin remains upstream; personal remote is the owner fork.
- Live player: https://esc-snowglobe.pages.dev/ (Cloudflare Pages, esc-snowglobe).
- Verified 2026-11-12: workflow 43192739228 passed tests and production deployment;
  served commit 9dac0a1a484aac7e9264c9d1f1f690841012f6b4 and all nine source assets matched.
- CI/auto-deploy definition: .github/workflows/ci.yml. Credential secrets configured;
  CLOUDFLARE_DEPLOY_ENABLED=true. Pushes/merges to main deploy after tests.
- Hosted headless playback/removal/guide/mobile checks passed with simulated GATT;
  this is not actual-radio or audible speaker acceptance.

## Read next

README.md: use, setup and code map.
docs/ARCHITECTURE.md: protocol, playback, security and hardware contracts.
docs/DEVELOPMENT.md: tests and isolated browser verification.
docs/DEPLOYMENT.md: deployment credentials, pipeline and recovery.
docs/WIRELESS-PEDESTAL.md: BLE firmware install, power and enclosure.
docs/WIRING.md + docs/TAGS.md: physical wiring and NFC tag programming.
CONTRIBUTING.md + LICENSE-NOTICE.md + songs/README.md: collaboration/attribution.

Existing .hermes.md remains; its numeric test count predates later additions.
Current commands/state here supersede historical status paragraphs. Do not use
Computer Use or operate the user's desktop for this project. Use source edits
and isolated headless tests. Do not change other Hermes profiles or user systems.

## Files that matter

- wireless.html + wireless.css: circular Connect, song/progress, volume/live stats.
- instructions.html: separate setup page.
- ble.js: filtered picker, full snapshot reads, polling and generation guards.
- player.js: DOM-free state machine, shared with legacy page.
- firmware/ESC_NFC_Pedestal: build with -DESC_BLE_MODE=1 for this product.
- firmware/ESC_Tag_Programmer + firmware/libraries/ESCShared: tags and codecs.
- hardware/pedestal-v9: editable model/generator, four STLs, reference, provenance.
- tools/build_site.py: whitelisted dist export; BLE page becomes index.html.
- tools/test_browser.py: headless real-page/media test with simulated GATT.
- tools/ensure_pages_project.py: narrowly scoped bootstrap, offline unit-tested.
- index.html / tools/setup.py / lib/: legacy Wi-Fi/MQTT, not the deployed homepage.

## Commands

```sh
npm ci
npm test
npm run build
python3 -m http.server 8000 --directory dist
```

The complete test command covers four native executables, 38 JS/DOM/build tests,
five Python bootstrap/dry-run tests, legacy setup dry run and three board builds. Install
the Arduino dependencies from README first. Tests do not flash hardware.
Optional headless integration uses a repository-root server; see DEVELOPMENT.
Never upload the repository root as a website; deploy only dist/.

## Real hardware evidence versus software fixtures

BLE firmware was uploaded; serial reported reader version 0x18. Battery-powered
E069 completed a 90-second GATT capture: no tag → rudolph → no tag, without a
connection error. The browser connected too. This proves delivery/removal, not
that a person heard music. Headless tests use simulated GATT and real MP3 decode.

A browser read-event feedback loop was reproduced/fixed: readValue can emit
characteristicvaluechanged. Ignore hints during an in-flight read; polling catches
missed updates. The final fix still needs actual-radio reconnect verification.

## Remaining acceptance

1. Audible selected-speaker playback with final UI and real BLE after reconnect.
2. Closed-enclosure sustained playback, swaps/reconnect, practical battery runtime.
3. Keep deployments verified against served build-info.json and asset bytes; initial
   Cloudflare publication passed. CI deploys main automatically after tests.
4. Preserve attribution. Public visibility is not a license to third-party recordings.

Safety/contracts: RC522 is 3.3 V only. Presence uses WUPA, not REQA-only logic.
Tags are on globes; reader is under the top plate. Top plate is seated, not latched:
support/secure it when inverting; secure the battery/board/wires.

Historical reader/network findings: docs/HARDWARE-HISTORY.md. Earlier audio repair
and the Sinatra 28.30-second trim: docs/PLAYBACK-INVESTIGATION.md. Wi-Fi setup is
retained in docs/LEGACY-WIFI.md, not a teammate prerequisite.
