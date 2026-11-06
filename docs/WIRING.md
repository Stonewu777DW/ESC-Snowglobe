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

Range is roughly "touching" for a 25–35 mm sticker through 2 mm of PLA. That is why the
globe's base sits in a dish rather than hovering: alignment is the whole game.

## If the reader is not detected

The sketch prints `RC522 ok, version 0x92` on boot, or `RC522 not answering`.

1. Check 3.3 V and GND with a multimeter at the module, not at the Arduino.
2. SDA must be D10 — not D9 or A4.
3. Re-seat the jumpers; the RC522 header pins are thin and bend.
4. Nothing else may use D9–D13 (a servo on D9 will silently break the reader).
