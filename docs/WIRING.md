# Wiring

Arduino UNO R4 WiFi + RC522 (13.56 MHz). Seven wires, and the module is a **3.3 V
part** — this is the one thing that kills them.

| RC522 pin | Arduino UNO R4 WiFi | Notes |
|---|---|---|
| SDA / SS | **D10** | chip select |
| SCK | **D13** | SPI clock — this is why the built-in LED is unusable as a status light |
| MOSI | **D11** | |
| MISO | **D12** | |
| RST | **D9** | |
| GND | **GND** | |
| 3.3V | **3.3V** | never 5 V |
| IRQ | — | unused, leave it |

These are the defaults in `firmware/libraries/ESCShared/src/pins.h`. If you move a pin,
define it in the sketch's `config.h` and change it there too.

Optional piezo/buzzer for a short confirmation beep: **D6 → buzzer +**, **buzzer − → GND**,
then set `ENABLE_BUZZER 1` in `config.h`. D6 is free; D9–D13 are taken by the reader.

## What kind of jumper leads

**Female-to-male**, seven of them — one socket end and one pin end.

| End | Goes into |
|---|---|
| female socket | the **male** pin on the RC522 |
| male pin | the **female** header hole on the UNO R4 WiFi |

The R4 ships with **female** headers (Tom's Hardware and Arduino's own R4 sales brief
both list "female pin headers"), and an RC522 has **male** pins once its header is
soldered on. So the classic "female-to-female" advice you'll find online is for a
Raspberry Pi or an UNO R3, both of which have male pins — it will not seat on an R4.

Both ends of this are already in the kit: the ELEGOO EL-KIT-0025 ships the RC522 with
its header fitted and 20 female-to-male Dupont wires, and lesson 2.20 asks for exactly
those 7 leads. Nothing to solder and nothing to buy.

You only need seven of the eight RC522 pins — `IRQ` stays empty.

### Voltage on the signal pins

The RC522's VCC is 3.3 V, and the R4's digital pins are 5 V. Driving the RC522's SPI
inputs from 5 V pins is outside the MFRC522's ratings, and it is what the kit's own
lesson does — it works on practically every build. Leave it as drawn. **Only** if reads
are unreliable after the gap is right, put a 4-channel level shifter (or a 1 kΩ/2 kΩ
divider on MOSI, SCK and SS) between the board and the reader — never move VCC to 5 V.

## Power

- The RC522 draws about 26 mA at 3.3 V. The R4's 3.3 V pin supplies it.
- **Do not** power the RC522 from 5 V, and do not power it from the 5 V rail "just to
  test". 5 V destroys it.
- Powering the Arduino from USB is fine for the whole build. If you run it from a wall
  adapter, use the USB-C port; the onboard regulator feeds 3.3 V to the reader.

## Mounting it in the pedestal

The reader sits **under** the thin printed top, antenna face **up**, directly below the
tag on the globe's base. From the 3D model project:

- tag to antenna gap: **~2.2 mm** (that is the RC522's own PCB plus the printed roof)
- nothing metal between them: no screws, no foil, no Arduino body, no wire bundle
- the RC522's antenna coil and its MFRC522 chip are on the **same face**, so the PCB is
  the hard floor on that gap — you cannot mount it closer than its own thickness
- hot-glue the board to the roof underside once the read works, not before
- the printed stack: TopPlate is 3.5 mm, but only **0.6 mm of PLA** sits above the
  antenna (2.5 mm dish + 0.4 mm tag pocket are cut away above it)

Range is roughly "touching" for a 25–35 mm sticker through 2 mm of PLA. That is why the
globe's base sits in a dish rather than hovering: alignment is the whole game.

## Getting power and USB in

The v9 BaseLid has a **26 × 10 × 14 mm notch at its front edge** — the USB-C cable leaves
through that, and it is also where you reach the board. The body has no wall opening on
purpose (any hole in the sloped wall fails the slicer's bridging check). So: build and
flash with the lid off, tape the RC522 down, check the two-minute sitting test, then bond
the lid with the cable already in the notch.

## If the reader is not detected

The sketch prints `RC522 ok, version 0x92` on boot, or `RC522 not answering`.

1. Check 3.3 V and GND with a multimeter at the module, not at the Arduino.
2. SDA must be D10 — not D9 or A4.
3. Re-seat the jumpers; the RC522 header pins are thin and bend.
4. Nothing else may use D9–D13 (a servo on D9 will silently break the reader).
