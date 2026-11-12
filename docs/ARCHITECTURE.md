# Architecture and contracts

## Selected path

`NFC sticker → RC522 (SPI) → UNO R4 WiFi → BLE → ble.js → player.js → Audio → OS output`

The UNO R4 advertises; the computer scans and requests permission. BLE carries
state, not audio. MP3s come from the static site. The browser cannot pair a speaker
or silently connect to a nearby pedestal without user permission.

## Firmware

Build firmware/ESC_NFC_Pedestal with `-DESC_BLE_MODE=1`. The BLE branch avoids
Wi-Fi/MQTT and does not wait for a USB serial host. The default build retains
legacy Wi-Fi mode. The tag programmer writes `esc:<slug>` to NTAG stickers;
songs.h retains UID fallbacks for known tags.

- Pins: SS D10, RST D9, SCK D13, MOSI D11, MISO D12, GND, 3.3 V.
- Use WUPA, not PICC_IsNewCardPresent: resting tags can be HALTed.
- Start/lost debounce 500 ms each; poll 60 ms; heartbeat 1,000 ms.
- Slugs are `[a-z0-9-]{1,24}`. Tags cannot select arbitrary URLs.

## BLE snapshot protocol v1

Service: `5b8c0001-8f28-4a8d-a1f8-667363736e6f`.
Read/notify state: `5b8c0002-8f28-4a8d-a1f8-667363736e6f`.

```text
1|E069|ok|rudolph    globe present
1|E069|ok|-          no globe
1|E069|bad|-         reader fault
```

Reader is four uppercase hex characters; ok/bad reports reader health.
Notifications are hints: read the complete value to avoid ATT truncation of long
slugs. A read can itself emit a change event; ignore hints during a read rather
than recursively reading forever. A one-second poll recovers missed changes.
Connection generations invalidate old callbacks. Initial reads recover a globe
already placed. Repeated snapshots do not restart songs. Removal, reader fault,
disconnect or a 3,500 ms browser liveness timeout stops playback.

## Player and UI

player.js is DOM-free and shared with the legacy page. wireless.html owns the
playlist and Audio element. Guard asynchronous audio work with a generation so
old sound-unlock callbacks cannot stop a newer song. Progress/duration is actual
media state, not a simulated clock. Do not invent speaker identity, battery
percentage or signal strength: none is provided.

The connection circle opens the browser picker. instructions.html holds setup.
tools/build_site.py publishes wireless.html as index.html using an asset whitelist,
never firmware/config, Git internals or private files.

## Security and mechanics

BLE is an unauthenticated local demo peripheral; a nearby client can occupy it.
Only one computer should connect at a time. The site has no backend or accounts.
PRs do not receive deployment secrets. Only trusted main pushes may deploy after CI.
Public visibility does not grant anonymous push access.

See hardware/pedestal-v9/README.md. Secure/support the unlatched top plate when
inverting the body. Mount the reader below the top plate; tags belong on globes.
