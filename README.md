# ESC Snowglobe — NFC pedestal

Put a snow globe on the pedestal. It reads the NFC tag on the bottom of the globe
and the game computer plays that globe's song. Lift the globe and the music stops.

This is the rebuild of the original camera version of this project. The tags are now
NFC stickers on the globes and the reader is an Arduino under the pedestal top —
there is no phone, no front camera, no QR code and nothing to hold up.

| | Before | Now |
|---|---|---|
| Tag | printed AprilTag 36h11, held up to a phone | NFC sticker on the globe's base |
| Reader | phone front camera + js-aruco | Arduino UNO R4 WiFi + RC522 |
| Link to the computer | phone → public MQTT broker | Arduino → public MQTT broker |
| Start / stop | tag in view 0.5 s / gone 0.5 s | tag on the pedestal 0.5 s / gone 0.5 s |
| Sound | the computer | the computer (unchanged) |

```
   globe with NFC sticker on its base
        │  sits in the dish on the pedestal
   ┌────▼─────────────────────────────┐
   │ 0.6 mm PLA roof  (2.2 mm to tag) │   ← tag reads through this
   │ RC522 reader, antenna face up    │
   │ Arduino UNO R4 WiFi   ──── WiFi ─┼──► broker.emqx.io ──► index.html plays the mp3
   └──────────────────────────────────┘
```

The 3D-printed pedestal, cradle ring and the 2.2 mm tag-to-antenna gap live in the
model project: `~/Developer/EscapeSC-NFC-Pedestal`. This repo owns the electronics
and the software.

## Your kit — everything is already in it

| Part | Where it comes from |
|---|---|
| Arduino UNO R4 WiFi | the board you have (USB descriptor `UNO WiFi R4 CMSIS-DAP`) |
| RC522 RFID module, 13.56 MHz | **in the kit** — ELEGOO EL-KIT-0025 lesson 2.20, pins SS **D10**, RST **D9** |
| 7 jumper leads | **in the kit** — its 20 female-to-male Dupont wires; the lesson specifies F-M |
| MIFARE fobs/cards | come with the RC522 module — usable only via the UID table, see `docs/TAGS.md` |
| NFC stickers | **NTAG213** — ordered separately, one per globe. 213 is plenty for one short string |
| Printed pedestal | `~/Developer/EscapeSC-NFC-Pedestal` v9: Body, TopPlate, BaseLid, CradleRing ×4 |
| Computer | any laptop with a browser and speakers |

Nothing else to buy. The leads are **female-to-male**: socket end over the RC522's pins,
pin end into the R4's female headers. (Female-to-female, which most RC522 tutorials say,
only works on an UNO R3 or a Pi — both of those have male header pins.)

## Setup

### The short way

Do the seven jumper wires (docs/WIRING.md), plug the Arduino into the Mac, then:

```bash
python3 tools/setup.py
```

It does not ask about WiFi — it reads the network this Mac is joined to, takes the
password from Keychain if there is one, and writes both into the sketch. Then it flashes
the pedestal, makes the board prove it can reach the song server, and tells you when to
hold each sticker on the reader while it writes all four. It finishes by putting the
pedestal firmware back, so there is nothing left to upload.

Campus guest wifi was measured on 2026-11-06 and works: it is open (no password) and
lets outbound `broker.emqx.io:1883` through. The script would stop and say so only if the
network needed a login page or blocked that port.

`python3 tools/setup.py --dry-run` shows the whole plan without touching anything.

### The manual way

**1. Wire the reader** — see [docs/WIRING.md](docs/WIRING.md). Seven wires, RC522 runs
on **3.3 V**, never 5 V. It is the kit's own lesson 2.20 wiring: SS→D10, RST→D9.

**2. Flash the pedestal**

```bash
arduino-cli lib install MFRC522 ArduinoMqttClient
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --libraries firmware/libraries firmware/ESC_NFC_Pedestal
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi --libraries firmware/libraries -p /dev/cu.usbmodemXXXX firmware/ESC_NFC_Pedestal
```

`tools/setup.py` fills in `WIFI_SSID` and `WIFI_PASSWORD` for you. `ROOM_CODE` is
already set (make up your own if you like, e.g. `esc-snowglobe-7f3a91c2`) — it only has
to match what the page uses.

**3. Program the NFC stickers** — see [docs/TAGS.md](docs/TAGS.md). Flash
`firmware/ESC_Tag_Programmer`, open the serial monitor at 115200, hold a sticker on
the reader and type `write rudolph`. Label the sticker. Repeat per globe.

**4. Open the page** — `index.html`, either on GitHub Pages or from a local web server:

```bash
python3 -m http.server 8000    # then open http://localhost:8000/?room=YOUR-ROOM-CODE
```

The room code has to match `config.h`. The page can also be told the code by pasting
it into the room box and pressing Enter; it remembers it.

Click the sound button, wait for **Pedestal connected**, then set a globe down.

## Songs

`SONGS` at the top of `index.html` is the whole map:

```js
const SONGS = [
  { slug: 'jingle-bells',    title: 'Jingle Bells',      file: 'songs/Frank Sinatra - Jingle Bells 4.mp3' },
  ...
];
```

- The **slug** is what gets written to the tag: lowercase letters, digits, hyphens.
- The **title** is what the page shows while it plays.
- To add a song: drop the mp3 in `songs/`, add a line, write the slug to a tag. The
  pedestal does **not** need reflashing.

Four slugs ship with the project: `jingle-bells`, `frosty`, `rudolph`,
`white-christmas`.

If a sticker cannot be programmed (locked, wrong chip, or a tag a phone wiped), its
UID can be mapped instead — `firmware/ESC_NFC_Pedestal/songs.h`. The serial log prints
the UID of any unknown tag, so nothing is ever a dead end.

## Testing without the hardware

Open the page and press <kbd>1</kbd>–<kbd>4</kbd> to play each song, <kbd>Esc</kbd> to stop.
That checks the speakers, the files and the page before the Arduino is even wired.

Run the test suite:

```bash
npm install          # once, for jsdom
./run_tests.sh
```

It covers the NFC payload encoder/decoder (C++), the pedestal's timing rules against a
simulated clock (C++), the page's message handling (node), and the real `index.html` in a
DOM (jsdom). The broker round-trip was checked separately against `broker.emqx.io`.

## Troubleshooting

| Symptom | Cause |
|---|---|
| Serial prints `RC522 not answering` | wiring, or the module is getting 5 V / no GND |
| Page says **Pedestal offline** | room code mismatch, or the R4 is on a 5 GHz-only network (it is 2.4 GHz only) |
| Serial shows the UID then nothing | the sticker has no payload — program it, or add the UID to `songs.h` |
| Music plays but never stops | tag still on the reader: that is correct, lift the globe |
| Music cuts out while the globe sits still | antenna too far from the tag — the gap must be ~2 mm plastic, no metal, no screws in the middle |
| Wrong song for a globe | two stickers swapped; `scan` each one and relabel |

## Files

```
index.html                     the computer page (music)
player.js                      its logic, isolated so it can be tested
songs/                         the mp3 files
firmware/ESC_NFC_Pedestal/     the pedestal sketch + config.h + songs.h
firmware/ESC_Tag_Programmer/   writes song payloads onto NFC stickers
tools/setup.py                 does steps 2-3 for you, end to end
firmware/libraries/ESCShared/  NFC payload format, timing rules, tag detection
tests/                         C++ and node tests
docs/WIRING.md                 pin-by-pin wiring
docs/TAGS.md                   which stickers, how to program them
lib/mqtt.min.js                browser MQTT client (MIT)
```

No server of ours is involved: the page is static files, and the only link between
pedestal and computer is the free public EMQX broker.

## License

The firmware and page are part of this project. `lib/mqtt.min.js` is MQTT.js, MIT
(see `lib/LICENSE-mqtt.md`).
