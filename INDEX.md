# INDEX — ESC Snowglobe (NFC pedestal)

**Status:** v2.0.0, NFC rebuild of the camera version. Software verified on 2026-11-06;
**not yet run on hardware** (no Arduino was on USB during the rebuild). Read this file
before exploring the tree.

## What this repo is

The game computer page (`index.html`) plays an mp3 when an NFC tag is placed on a
pedestal. The pedestal is an Arduino UNO R4 WiFi + RC522 that publishes the song slug
over MQTT. The old phone/camera/AprilTag path is gone — do not reintroduce it.

## Verified facts

| Thing | Value | How it was checked |
|---|---|---|
| Sketches compile for `arduino:renesas_uno:unor4wifi` | pedestal 28% flash / 25% RAM, programmer 24% / 21% | `arduino-cli compile --libraries firmware/libraries` |
| NFC payload round-trip | 15/15 assertions pass | `tests/ndef_roundtrip.cpp` |
| Start/stop timing rules | 10/10 scenarios pass | `tests/presence_test.cpp` (fake clock) |
| Page logic + real DOM | 16/16 pass | `tests/player.test.mjs`, `tests/page.test.mjs` (jsdom) |
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

**Known constraint:** guest/university wifi with a captive portal cannot work — the board
has no browser to complete the login. The script detects the failure and says so; the
answer is a phone hotspot.

## Open

- Flash and confirm on real hardware: continuous read while the globe sits still,
  MQTT reconnection after WiFi drop, and the 2.2 mm gap in the printed pedestal.
