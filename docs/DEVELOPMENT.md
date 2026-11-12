# Development

## Complete verification gate

Node 24+, Python 3.10+, g++ and Arduino CLI are required. Install the
arduino:renesas_uno@1.6.0 board core and libraries ArduinoBLE 2.1.0, MFRC522 1.4.12,
ArduinoMqttClient 0.1.8. The CI workflow documents setup.

```sh
npm ci
npm test
npm run build
```

Tests never flash a board. They exercise four native executables, Node/DOM tests,
legacy setup dry run and three board builds (Wi-Fi, BLE, programmer). Site-build
checks verify the export whitelist, guide and four selected songs.

## Isolated browser integration (no desktop)

Optional Python dependencies: pip install playwright, then
python3 -m playwright install chromium. Run tools/test_browser.py after starting
python3 -m http.server 8000 from the repo root.

The test launches headless Chromium, injects a fake GATT device at the hardware
boundary, runs the real ble.js/player.js, and decodes a real MP3. It checks picker
invocation, read-event recursion, playback/removal/disconnect, volume, elapsed and
duration, guide navigation and responsive widths. It never operates the user's
desktop and does not prove actual Bluetooth radio or audible speaker output.

## Remaining acceptance

Real battery BLE presence/removal was observed. Closed-enclosure sustained music,
practical battery runtime and audible selected-speaker output with the redesigned
page remain manual gates. Media telemetry is not proof somebody heard sound.
The v9 generator is retained and syntax-checked, not freshly geometry-accepted.
Included STLs are the original generated assets, not a new mechanical revision.
