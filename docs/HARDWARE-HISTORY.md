# Historical project index (before public consolidation)

Retained evidence only. Current source/hosting state is in the root INDEX.md.

# INDEX — ESC Snowglobe (NFC pedestal)

**Status:** v2.0.0 NFC rebuild, with a new BLE deployment candidate prepared on
2026-11-11. Earlier Wi-Fi firmware was uploaded and reader version 0x18 plus broker
heartbeats were observed. The BLE build was subsequently uploaded successfully to
UNO R4 WiFi serial 64E8335FE068; post-upload serial confirms reader version 0x18.
Battery-powered BLE was subsequently observed live as E069: a 90-second GATT
capture recorded no tag → rudolph → no tag and completed without errors. The browser
connected too. Audible speaker output and the final closed enclosure remain unverified.
Read this file before exploring the tree.

**Selected deployment:** battery-powered, direct Bluetooth LE to a computer website.
Implementation and organizer instructions: [WIRELESS-PEDESTAL.md](docs/WIRELESS-PEDESTAL.md).
`wireless.html` + `ble.js` are the new no-Wi-Fi/no-room-code player. Build the existing
pedestal sketch with `-DESC_BLE_MODE=1`; the default build remains legacy Wi-Fi.
The simplified player uses `wireless.html`, `wireless.css`, `instructions.html`,
`player.js`, `ble.js`, and four referenced MP3s. Retain `wireless.html` as an alias if
publishing it as index.html, because the guide links back to it. Main UI: circular
Connect/Disconnect, real playback/reader telemetry, progress and volume; separate guide.
Verified: 37 JS tests, four native test executables, three UNO R4 builds, and real
Chromium media playback with a simulated GATT device. Headless DOM checks cover
320/390/700/1280 widths, navigation, picker invocation, volume, removal and disconnect.
A readValue/characteristicvaluechanged feedback loop was reproduced and fixed with a
regression test; actual radio use of that browser fix still needs a reconnect. Public
hosting is also pending: both available GitHub accounts lack write access to the
original repo; a push dry run returned 403. Existing GitHub Pages uses main and is
not this feature branch. No new public repo or replacement homepage was created.

**Current playback repair:** [investigation and audio source](docs/PLAYBACK-INVESTIGATION.md).
Browser fixes cover stale fades/unlock callbacks, fade bounds, and manual-test timeout.

## What this repo is

The game computer page plays an MP3 when an NFC tag is placed on a pedestal.
The UNO R4 WiFi + RC522 can publish the song slug directly over BLE (`wireless.html`,
selected battery-powered direction) or over legacy Wi-Fi/MQTT (`index.html`). The old phone/camera/AprilTag path is gone — do not reintroduce it.

## Verified facts

| Thing | Value | How it was checked |
|---|---|---|
| Sketches compile for `arduino:renesas_uno:unor4wifi` | pedestal 28% flash / 25% RAM, programmer 24% / 21% | `arduino-cli compile --libraries firmware/libraries` |
| NFC payload round-trip | 15/15 assertions pass | `tests/ndef_roundtrip.cpp` |
| Start/stop timing rules | 10/10 scenarios pass | `tests/presence_test.cpp` (fake clock) |
| Page/connection logic + real DOM | 37/37 pass | `tests/*.test.mjs` (node + jsdom) |
| BLE firmware compiles | 94,912 B flash / 9,936 B RAM | same pedestal sketch with `-DESC_BLE_MODE=1` |
| Broker topics/payloads | round-trip on `broker.emqx.io:1883` | live pub/sub check |
| Tag → antenna gap (physical) | 2.2 mm | `~/Developer/EscapeSC-NFC-Pedestal` model |

## Commands

```bash
./run_tests.sh                       # everything above except the broker check
npm install                          # once, for jsdom
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --libraries firmware/libraries firmware/ESC_NFC_Pedestal
python3 -m http.server 8000          # page: http://localhost:8000/?room=<ROOM_CODE>
```

## Layout

```
index.html            computer page (SONGS table lives here)
player.js             page logic, no DOM, unit-tested
songs/                mp3 files (names are referenced by index.html)
firmware/ESC_NFC_Pedestal/    config.h (WiFi, room, pins, timing), songs.h (UID fallback)
firmware/ESC_Tag_Programmer/  writes "esc:<slug>" onto NTAG stickers over serial
firmware/libraries/ESCShared/ ndef.h, presence.h, tagdetect.h, pins.h
tests/                C++ and node tests
docs/                 WIRING.md, TAGS.md
```

## Contracts (breaking these breaks the product)

1. A slug is `[a-z0-9-]{1,24}`. It appears in `index.html` → `SONGS`, on the tag as
   `esc:<slug>`, and optionally in `songs.h`.
2. `ROOM_CODE` in `config.h` must equal the page's `?room=` value.
3. Topics: `esc-snowglobe/<room>/tag` and `esc-snowglobe/<room>/reader`.
4. Presence is detected with **WUPA** (`escTagPresent()`), never
   `PICC_IsNewCardPresent()` — a tag resting on the reader is HALTed and REQA will not
   wake it. See `firmware/libraries/ESCShared/src/tagdetect.h`.
5. The RC522 is 3.3 V only.

## Setup is automatic

`tools/setup.py` reads the current Wi-Fi SSID (`ipconfig getsummary en0`), pulls the
password from Keychain (empty for open networks), writes both to `config.h`, flashes,
then verifies from the serial log that the board reached the broker before touching the
tags. It never asks for the network name.

Open networks (empty password) are joined with the one-argument `WiFi.begin(ssid)` —
passing an empty password string makes the R4 wait for a key that never comes.

Measured on USC guest wifi (open, no portal), 2026-11-06: outbound `broker.emqx.io:1883`
(MQTT, the board's path) and `:8084` wss (the browser's path) both connect and carry
publish+subscribe. So this network works for the project as-is.

**Only fail-fast case:** a network that needs a login page, or one that blocks port 1883.
The script detects either and points at a phone hotspot.

The join is owned entirely by `networkUp()` — `setup()` must not call `WiFi.begin()`
itself, or an open network gets a half-started association with an empty password.

## Harness facts (from this owner's kit, not from generic tutorials)

- Board: **Arduino UNO R4 WiFi**, USB descriptor `UNO WiFi R4 CMSIS-DAP` (from
  `~/Developer/Arduino/HANDOFF.md`).
- Kit: **ELEGOO UNO R4 WiFi Super Starter Kit EL-KIT-0025** — contains the RC522 module
  (its lesson 2.20 uses SS **D10**, RST **D9**) and 20 female-to-male Dupont wires.
  Kit modules ship with headers fitted; nothing to solder.
- The R4 has **female** headers, the RC522 has **male** pins → the leads are
  **female-to-male**. Female-to-female (what most RC522 tutorials say) needs an R3 or a
  Pi, both male-pinned. See `docs/WIRING.md`.
- Tags: NTAG213/215/216 stickers. The MIFARE fobs/cards that ship with the module are
  Classic — usable only through `songs.h` UID mapping, never written as NDEF.

## Hardware findings (measured on the real board, 2026-11-06)

| Finding | Evidence |
|---|---|
| Reader answers over SPI; `VersionReg = 0x18` (clone chip, not 0x92) | serial `[reader] ok, version 0x18` |
| **Block writes store only the first page**: must write 4 bytes at a time | hex `dump` before/after, see `docs/TAGS.md` |
| WUPA + SELECT works for a resting tag; the pedestal read path needs no change | probe: 17 consecutive `WUPA: Success -> SELECT ok`, `[play] jingle-bells` |
| **An open network joins ONLY via single-arg `WiFi.begin(ssid)`** | `begin(ssid,"")` sat at `status 0` for 60 s; `begin(ssid)` reached `status 3, ip 10.25.191.152` |
| `WiFi.end()` in the retry loop prevented association | pedestal never joined while a one-shot probe did |
| ESP32-S3 firmware rewrites from this Mac via esptool; Arduino's tool fails on the port switch | `write-flash 0x0` 1,966,080 B, hash verified, scan still returns 10 networks |
| Full chain verified | broker received `{"reader":"E068","fw":"1.0.0","state":"online"}` every 2 s |

## Open

- Flash and confirm on real hardware: continuous read while the globe sits still,
  MQTT reconnection after WiFi drop, and the 2.2 mm gap in the printed pedestal.
