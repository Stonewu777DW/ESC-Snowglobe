# Battery-powered wireless pedestal

## Accepted teammate experience

This is the selected product direction; USB/Wi-Fi/MQTT is now a legacy fallback,
not a teammate setup requirement. An organizer installs the BLE firmware once.

1. Remove any globe. Support the top plate while opening the bottom cover.
2. Connect the prepared 9 V battery lead and close the pedestal.
3. On the computer, open the hosted wireless player in desktop Chrome or Edge.
4. Click **Connect**, select **ESC Pedestal XXXX**, and place a globe.
5. To use a Bluetooth speaker, pair it with the computer and select it as the
   computer's output. Use the website's **Test speaker** button.

There are two hardware handling steps (connect battery, close pedestal), followed
by a browser permission/device selection. A browser cannot silently connect to a
new nearby device without that user action. No Arduino IDE, USB computer cable,
Wi-Fi credentials, room code or public message relay is needed during normal use.

## Architecture implemented

- Existing UNO R4 WiFi and RC522 wiring are preserved.
- `ESC_BLE_MODE=1` builds the BLE-only transport. It does not join Wi-Fi.
- Battery boot has no `while (!Serial)` or computer dependency.
- The board advertises `ESC Pedestal XXXX` using its BLE address suffix.
- The **computer scans**; the Arduino advertises and reads NFC tags.
- One custom read/notify GATT characteristic holds a versioned state:
  `1|E068|ok|jingle-bells`, `1|E068|ok|-`, or `1|E068|bad|-`.
- Service UUID: `5b8c0001-8f28-4a8d-a1f8-667363736e6f`.
- State UUID: `5b8c0002-8f28-4a8d-a1f8-667363736e6f`.
- `-` is reserved for no tag. Actual song slugs are validated against the local
  playlist. Notifications trigger a full read, so a short ATT packet does not
  silently truncate a long slug. State is refreshed every second; the page also
  polls once per second to recover missed notifications.
- Disconnect/removal stops audio. Reconnection reads the current globe without
  needing the user to lift and replace it. No commands can select arbitrary URLs.
- Only one computer should control a pedestal at a time. Disconnect there before
  connecting a different computer. A nearby device can occupy this unauthenticated
  demo peripheral; device selection is not a production authentication scheme.

## Actual enclosure checked

Sources: `hardware/pedestal-v9/build_escape_sc_pedestal_v9.py`, the
v9 body/top/base STL assets, and `EscapeSC_v9_hero.png`. No model edits were made.

- The top plate and base lid are separate removable pieces.
- The top plate seats into a rebate. The inspected geometry does **not** show a
  positive top-plate latch; retain/support it before turning the body upside down.
- The bottom lid has an insertion lip and a cable notch; there is no dedicated
  battery holder or board fastener in the inspected generation script.
- Secure the Arduino, battery and wiring so they do not fall or tug the reader when
  inverted. Insulate the battery terminals and use strain relief on the power lead.
- Mount the **reader antenna** underneath the center of the **top** plate, close to
  the globe's tag. Mounting it flat on the bottom lid would put it too far away.
- Keep the **tag on each globe**, not permanently on the pedestal. A reader on the
  top plate may be connected by flexible wires to electronics carried by the base.
- The model's specified antenna/tag separation is 2.2 mm; actual print, adhesive,
  alignment and sustained reads must still be tested before fixing the mount.

## Power

Use the prepared center-positive 9 V barrel-jack lead. Do not feed 9 V into the
5 V or 3.3 V pins. Arduino's UNO R4 WiFi datasheet specifies VIN/DC jack input of
6–24 V. A nominal 9 V supply is within that range, but does not establish practical
battery runtime. Measure the complete unit on the exact intended battery and keep
spares for the event. Disconnect it when finished. No battery gauge is claimed.

## Organizer build and install

Dependencies: `ArduinoBLE` (compiled with 2.1.0), `MFRC522`, and the existing UNO R4
board core. The legacy transport retains `ArduinoMqttClient` as a dependency.

```bash
arduino-cli lib install ArduinoBLE
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi \
  --libraries firmware/libraries \
  --build-property compiler.cpp.extra_flags=-DESC_BLE_MODE=1 \
  --output-dir "$TMPDIR/esc-ble-build" firmware/ESC_NFC_Pedestal
arduino-cli board list --format json
# Use the discovered UNO R4 port; do not copy an old remembered port:
arduino-cli upload --fqbn arduino:renesas_uno:unor4wifi \
  --port <DISCOVERED_PORT> --input-dir "$TMPDIR/esc-ble-build"
```

The BLE build observed here used 94,912 bytes flash and 9,936 bytes RAM. Compile
success is not radio initialization proof. If `[ble] radio initialization failed`
appears, investigate the board's connectivity firmware before shipping it.

## Website delivery

Run `npm run build` to export the BLE player as dist/index.html, with a
wireless.html alias, instructions.html, wireless.css, player.js, ble.js and four
selected MP3s. Serve dist/ from HTTPS (localhost works for development).
No server/backend is needed. Never deploy the repository root.
Use only approved audio assets for public deployment. Keep the tab open and the
computer awake. Safari/Firefox are not the target for the direct-BLE website.

The existing project's GitHub Pages source is **main /** and still hosts the old
camera version. Do not claim that feature-branch files are already live there.
Upstream push was denied (403). The owner explicitly authorized a personal fork:
https://github.com/sidatpho/ESC-Snowglobe. Cloudflare Pages is the selected host;
the saved Cloudflare token reports expired, so deployment remains blocked.
See DEPLOYMENT.md for current CI/credential setup. No live URL is claimed.

## Evidence and remaining acceptance gates

- Native snapshot formatter tested with no-tag, reader failure, longest slug,
  invalid fields and undersized buffer.
- Browser connection tests cover selection, notifications/full reads, pending-read
  invalidation, cancelled setup, read errors and duplicate Connect clicks.
- DOM tests cover speaker output requests, no-room-code flow, reader faults,
  stop-on-disconnect and unsupported-browser guidance.
- Real Chromium with an explicitly simulated Bluetooth transport decoded/played
  the trimmed MP3, sustained playback across repeats, stopped on removal and
  disconnect, recovered a present globe on reconnect, and stopped for a bad reader.
- Follow-up installation: BLE firmware uploaded successfully to UNO R4 WiFi
  serial `64E8335FE068`; post-upload serial reports `[reader] ok, version 0x18`.
  Bluetooth was subsequently enabled. A battery-powered E069 GATT session ran for
  90 seconds with no tag → rudolph → no tag and no connection error; the browser
  also connected. The redesigned browser transport fix still needs a real reconnect.
- Final day is **not yet hardware accepted**. Still required: two minutes of
  stationary-globe playback through the closed enclosure, lift/swap/reconnect,
  chosen Bluetooth speaker output, and battery runtime over the required session.

Primary references checked: Arduino UNO R4 WiFi datasheet
`https://docs.arduino.cc/resources/datasheets/ABX00087-datasheet.pdf`; Chrome Web
Bluetooth documentation `https://developer.chrome.com/docs/capabilities/bluetooth`;
installed ArduinoBLE 2.1.0 peripheral examples. The firmware and website above are
our implementation, not manufacturer-certified complete-system behavior.
