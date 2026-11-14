# ESC Snowglobe

Battery-powered NFC pedestal + a very small Christmas music player.

**Put a globe down → its song plays. Lift it → the music stops.**
The Arduino identifies the globe; the computer plays the audio through its selected
speaker. There is no Wi-Fi setup, room code, account or USB cable during normal use.

## Use it

1. Connect the prepared 9 V battery lead and close the pedestal. Remove the globe
   and support the removable top plate before turning it upside down.
2. Open the hosted player in **desktop Chrome or Edge**, click **Connect**, and
   select **ESC Pedestal XXXX** in the browser's pop-up.
3. Place a globe. Adjust the volume; lift the globe to stop.

Pair a Bluetooth speaker with the **computer**, not the Arduino. Keep the tab open
and computer awake. The **How it works** page has the detailed instructions.

Anyone else who opens the player sees what the connected computer is doing (song,
progress, puzzle box) live, with no sound and the controls locked. Only the computer
connected to the pedestal can change anything. Add `?room=NAME` to the URL to keep two
rooms apart. The live view goes through the free public broker `broker.emqx.io`.

**Live player:** https://esc-snowglobe.pages.dev/

Pushes/merges to `main` automatically run tests and deploy to the same site.
The verified deployment and recovery steps are in [DEPLOYMENT.md](docs/DEPLOYMENT.md).

## What is in this repo?

| Path | Purpose |
|---|---|
| `wireless.html`, `wireless.css` | Minimal player: connection circle, song, progress, volume and live status |
| `instructions.html` | Separate teammate setup/troubleshooting page |
| `ble.js` | Web Bluetooth transport, validation, polling and reconnect handling |
| `player.js` | Shared playback state machine |
| `firmware/` | UNO R4 WiFi pedestal and NFC tag programmer sketches + shared library |
| `hardware/pedestal-v9/` | Editable Blender file, generator, four STLs and reference image |
| `songs/` | Four-song playlist + unmodified original Jingle Bells recording |
| `tests/` | Native C++ and JavaScript/DOM regression tests |
| `tools/build_site.py` | Whitelist-only static site export to `dist/` |
| `.github/workflows/ci.yml` | Tests on PRs; gated Cloudflare deployment on main |
| `index.html`, `lib/`, `tools/setup.py` | Retained legacy Wi-Fi/MQTT implementation, **not** the deployed main player |

## Run locally

Use Node 24+, Python 3.10+, npm and a desktop browser with Web Bluetooth support.

```sh
npm ci
npm run build
python3 -m http.server 8000 --directory dist
# Open http://localhost:8000/ in desktop Chrome or Edge.
```

The build exports the new Bluetooth page as **index.html**, keeps a wireless.html
alias for guide links, and copies only its CSS/JS/guide and four selected MP3s.
Never upload the repository root as the website.

## Prepare an Arduino once (organizer only)

Hardware: **Arduino UNO R4 WiFi**, RC522 reader, NTAG213/215/216 stickers and the
prepared center-positive 9 V barrel-jack lead. RC522 power is **3.3 V, never 5 V**.
See [wiring](docs/WIRING.md), [tags](docs/TAGS.md) and
[BLE build/install](docs/WIRELESS-PEDESTAL.md#organizer-build-and-install).
Normal use needs no Arduino IDE or firmware upload.

```sh
arduino-cli core update-index
arduino-cli core install arduino:renesas_uno@1.6.0
arduino-cli lib install 'ArduinoBLE@2.1.0' 'MFRC522@1.4.12' 'ArduinoMqttClient@0.1.8'
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi \
  --libraries firmware/libraries \
  --build-property compiler.cpp.extra_flags=-DESC_BLE_MODE=1 \
  --output-dir build/ble firmware/ESC_NFC_Pedestal
arduino-cli board list
# Replace <PORT> with the actual discovered UNO R4 port:
arduino-cli upload --fqbn arduino:renesas_uno:unor4wifi --port <PORT> --input-dir build/ble
```

`ESC_BLE_MODE=1` is required: the default sketch build is the retained Wi-Fi mode.
Do not use the legacy `tools/setup.py` for the battery/BLE flow.

## Test and contribute

```sh
npm ci
npm test       # g++, Python, Arduino CLI/core/libraries required; no board touched
npm run build
```

CI runs the complete suite, including all three board builds. Headless integration
tests can be run separately; see [DEVELOPMENT.md](docs/DEVELOPMENT.md).
Fork the public repo and open a pull request. Maintainers review/merge into main;
that triggers a deployment once Cloudflare credentials are configured. PRs do not
receive deployment credentials. See [CONTRIBUTING.md](CONTRIBUTING.md).

## Verified versus still unverified

- BLE firmware uploaded; reader responds.
- Battery-only BLE/GATT capture completed for 90 seconds: no globe → Rudolph → no globe.
- Software tests cover reader faults, disconnect, repeated scans and playback races.
- Isolated headless Chromium decoded real MP3 audio and checked responsive layouts.
- **Still unverified:** audible output through the chosen speaker with the final UI,
  closed-enclosure sustained playback and practical battery runtime.

## Agent handoff and references

Start at [INDEX.md](INDEX.md), then [.hermes.md](.hermes.md) /
[architecture](docs/ARCHITECTURE.md). [Architecture](docs/ARCHITECTURE.md) documents state, wire
format and safety contracts. Historical Wi-Fi instructions are in
[LEGACY-WIFI.md](docs/LEGACY-WIFI.md).

## Attribution and media

This is a public fork of [Stonewu777DW/ESC-Snowglobe](https://github.com/Stonewu777DW/ESC-Snowglobe);
its Git history and attribution are preserved. The inherited project did not include
a top-level license grant. See [LICENSE-NOTICE.md](LICENSE-NOTICE.md) and
[song/media notes](songs/README.md). Public repository access is not a blanket
license to the third-party recordings. MQTT.js retains its own MIT notice.
