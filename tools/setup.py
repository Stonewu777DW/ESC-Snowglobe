#!/usr/bin/env python3
"""
ESC Snowglobe — one-command setup.

    python3 tools/setup.py

It asks two questions (WiFi name and password), then does everything else:
installs the libraries, flashes the pedestal, walks you through writing the four
globe stickers one at a time, and puts the pedestal firmware back on the board.

    python3 tools/setup.py --dry-run     # show what it would do, touch nothing
"""
import getpass
import glob
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FQBN = "arduino:renesas_uno:unor4wifi"
LIBS = "--libraries"
SKETCHES = {
    "pedestal": ROOT / "firmware" / "ESC_NFC_Pedestal",
    "tags": ROOT / "firmware" / "ESC_Tag_Programmer",
}
CONFIG = ROOT / "firmware" / "ESC_NFC_Pedestal" / "config.h"
SONGS = [("jingle-bells", "Jingle Bells"), ("frosty", "Frosty the Snowman"),
         ("rudolph", "Rudolph"), ("white-christmas", "White Christmas")]
BAUD = 115200

dry = "--dry-run" in sys.argv


def say(text=""):
    print(text, flush=True)


def step(n, total, text):
    say()
    say(f"[{n}/{total}] {text}")
    say("-" * 62)


def run(cmd, **kw):
    if dry:
        say("   would run: " + " ".join(str(c) for c in cmd))
        return subprocess.CompletedProcess(cmd, 0, stdout="", stderr="")
    return subprocess.run([str(c) for c in cmd], capture_output=True, text=True, **kw)


def die(text, hint=""):
    say()
    say("STOPPED: " + text)
    if hint:
        say(hint)
    sys.exit(1)


# --- 1. board --------------------------------------------------------------
def find_port(wait_seconds=60):
    ports = sorted(glob.glob("/dev/cu.usbmodem*"))
    if ports:
        return ports[0]
    say("   waiting for the Arduino — plug its USB-C cable into this Mac…")
    deadline = time.time() + wait_seconds
    while time.time() < deadline:
        time.sleep(2)
        ports = sorted(glob.glob("/dev/cu.usbmodem*"))
        if ports:
            return ports[0]
    return None


# --- 2. WiFi ---------------------------------------------------------------
def read_config():
    return CONFIG.read_text()


def current_wifi(text):
    ssid = re.search(r'#define WIFI_SSID\s+"([^"]*)"', text)
    password = re.search(r'#define WIFI_PASSWORD\s+"([^"]*)"', text)
    return (ssid.group(1) if ssid else ""), (password.group(1) if password else "")


def set_wifi(ssid, password):
    text = read_config()
    text = re.sub(r'#define WIFI_SSID\s+"[^"]*"', f'#define WIFI_SSID     "{ssid}"', text)
    text = re.sub(r'#define WIFI_PASSWORD\s+"[^"]*"', f'#define WIFI_PASSWORD "{password}"', text)
    CONFIG.write_text(text)


def room_code():
    match = re.search(r'#define ROOM_CODE\s+"([^"]*)"', read_config())
    return match.group(1) if match else ""


# --- 3. serial -------------------------------------------------------------
def open_serial(port):
    import serial  # pyserial
    time.sleep(1.5)
    return serial.Serial(port, BAUD, timeout=0.3)


def drain(ser, seconds):
    """Collect whatever the board prints for a while."""
    lines, deadline = [], time.time() + seconds
    while time.time() < deadline:
        raw = ser.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", "replace").strip()
        if line:
            lines.append(line)
            say("   board: " + line)
    return lines


def wait_for(ser, needle, seconds):
    lines, deadline = [], time.time() + seconds
    while time.time() < deadline:
        raw = ser.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", "replace").strip()
        if line:
            lines.append(line)
            say("   board: " + line)
            if needle in line:
                return True, lines
    return False, lines


# --- 4. flashing -----------------------------------------------------------
def flash(sketch, port):
    name = SKETCHES[sketch]
    say(f"   building {name.name}…")
    built = run(["arduino-cli", "compile", "--fqbn", FQBN, "--libraries",
                 ROOT / "firmware" / "libraries", name])
    if built.returncode != 0:
        say(built.stdout[-1500:] + built.stderr[-1500:])
        die(f"{name.name} does not compile.")
    uploaded = run(["arduino-cli", "upload", "-p", port, "--fqbn", FQBN,
                    "--libraries", ROOT / "firmware" / "libraries", name])
    if uploaded.returncode != 0:
        say(uploaded.stdout[-1500:] + uploaded.stderr[-1500:])
        die(f"Could not upload {name.name}.", "Is another program using the port? Close the Serial Monitor.")
    say(f"   uploaded {name.name}")


def ensure_libraries():
    if dry:
        say("   would check that MFRC522 and ArduinoMqttClient are installed")
        return
    listed = run(["arduino-cli", "lib", "list"]).stdout
    for lib in ("MFRC522", "ArduinoMqttClient"):
        if lib not in listed:
            say(f"   installing {lib}…")
            run(["arduino-cli", "lib", "install", lib])


# --- 5. main ---------------------------------------------------------------
def main():
    say()
    say("  ESC Snowglobe — setup")
    say("  A snow globe on the pedestal plays its song on this computer.")
    say("  This script wires up the software. It cannot do the seven jumper wires.")
    if dry:
        say("  (dry run: nothing will be flashed or written)")

    step(1, 5, "Finding the Arduino")
    port = "/dev/cu.usbmodemDRYRUN" if dry else find_port()
    if port is None:
        die("No Arduino found on USB.",
            "Plug the board into this Mac with a USB-C cable, then run this again.\n"
            "If it is plugged in: try a different cable (some are charge-only).")
    say(f"   found it on {port}")

    step(2, 5, "WiFi")
    ssid, password = current_wifi(read_config())
    if ssid:
        say(f'   config.h already has WiFi "{ssid}"')
        if input("   use it? [Y/n] ").strip().lower() == "n":
            ssid = ""
    if not ssid:
        ssid = input("   WiFi network name (2.4 GHz only): ").strip()
        password = getpass.getpass("   WiFi password (not shown, not stored in chat): ")
        if not ssid:
            die("No WiFi name given.")
        if dry:
            say("   would save it to config.h")
        else:
            set_wifi(ssid, password)
            say("   saved to config.h")
    say(f"   room code: {room_code()}")

    step(3, 5, "Flashing the pedestal reader")
    ensure_libraries()
    flash("pedestal", port)
    if not dry:
        ser = open_serial(port)
        lines = drain(ser, 4)
        ser.close()
        if any("RC522 ok" in line for line in lines):
            say("   reader is talking. ✓")
        elif any("not answering" in line for line in lines):
            die("The reader is not responding.",
                "Check the seven wires. The usual ones: SDA must be D10, and 3.3 V — not 5 V.\n"
                "Then run this again.")

    step(4, 5, "Writing the four globe stickers")
    flash("tags", port)
    if dry:
        for slug, title in SONGS:
            say(f"   would write “{slug}” to the {title} sticker")
        step(5, 5, "Putting the pedestal firmware back")
        say("   would flash ESC_NFC_Pedestal again")
        say()
        say("  Dry run finished. Everything above is what the real run does.")
        return

    ser = open_serial(port)
    drain(ser, 2)
    say()
    say("  For each sticker: hold it flat against the RC522, then press Enter.")
    say("  Write the song name on the sticker with a marker straight after.")
    failed = []
    for slug, title in SONGS:
        while True:
            input(f"\n   holder ready for “{title}”  →  press Enter ")
            ser.reset_input_buffer()
            ser.write(f"write {slug}\n".encode())
            ok, lines = wait_for(ser, "OK —", 12)
            if ok:
                say(f"   ✓ {title} sticker done")
                break
            say(f"   ✗ that did not work ({'no tag' if not any('Found' in l for l in lines) else 'write failed'})")
            if input("   try again? [Y/n] ").strip().lower() == "n":
                failed.append(title)
                break
    ser.close()

    step(5, 5, "Putting the pedestal firmware back")
    flash("pedestal", port)

    say()
    say("=" * 62)
    say("  DONE. The pedestal is ready.")
    say("=" * 62)
    if failed:
        say("  Stickers that failed: " + ", ".join(failed))
        say("  Those globes will do nothing until they are written. Run this again to retry.")
    say()
    say("  To play music:")
    say(f"    1. in this folder:  python3 -m http.server 8000")
    say(f"    2. open:            http://localhost:8000/?room={room_code()}")
    say("    3. click the yellow sound button")
    say("    4. set a globe on the pedestal")
    say()
    say("  Press 1-4 on that page any time to test the speakers without a globe.")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        say("\nStopped.")
        sys.exit(130)
