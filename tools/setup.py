#!/usr/bin/env python3
"""
ESC Snowglobe — one-command setup. No questions about WiFi: it reads the network
this Mac is on, takes the password from Keychain if there is one, and writes both
into the sketch.

    python3 tools/setup.py                 # WiFi, flash, write the four stickers
    python3 tools/setup.py --no-tags       # WiFi + flash only
    python3 tools/setup.py --dry-run       # print the plan, touch nothing

If this network has a login page (university/guest wifi), the board cannot use it —
the script says so plainly and tells you to switch to a phone hotspot and re-run.
"""
import glob
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FQBN = "arduino:renesas_uno:unor4wifi"
SKETCHES = {
    "pedestal": ROOT / "firmware" / "ESC_NFC_Pedestal",
    "tags": ROOT / "firmware" / "ESC_Tag_Programmer",
}
CONFIG = ROOT / "firmware" / "ESC_NFC_Pedestal" / "config.h"
SONGS = [("jingle-bells", "Jingle Bells"), ("frosty", "Frosty the Snowman"),
         ("rudolph", "Rudolph"), ("white-christmas", "White Christmas")]
BAUD = 115200

dry = "--dry-run" in sys.argv
skip_tags = "--no-tags" in sys.argv


def say(text=""):
    print(text, flush=True)


def step(n, total, text):
    say()
    say(f"[{n}/{total}] {text}")
    say("-" * 64)


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


# --- what network is this Mac on? -----------------------------------------
def detect_ssid():
    """The Wi-Fi network this Mac is joined to, or None."""
    try:
        out = subprocess.run(["ipconfig", "getsummary", "en0"],
                             capture_output=True, text=True).stdout
        match = re.search(r"^\s*SSID\s*:\s*(.+)$", out, re.MULTILINE)
        if match:
            return match.group(1).strip()
    except OSError:
        pass
    try:
        out = subprocess.run(["networksetup", "-getairportnetwork", "en0"],
                             capture_output=True, text=True).stdout
        match = re.search(r"Current Wi-Fi Network:\s*(.+)", out)
        if match:
            return match.group(1).strip()
    except OSError:
        pass
    return None


def detect_password(ssid):
    """The saved Wi-Fi password for that network, or '' for an open network.
    Never printed. A Keychain prompt would appear here, which is why stdin is
    closed and the lookup is time-limited."""
    try:
        found = subprocess.run(
            ["security", "find-generic-password", "-D", "AirPort network password",
             "-a", ssid, "-w"],
            capture_output=True, text=True, timeout=6, stdin=subprocess.DEVNULL,
        )
        if found.returncode == 0:
            return found.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        pass
    return ""


# --- config.h --------------------------------------------------------------
def read_config():
    return CONFIG.read_text()


def room_code():
    match = re.search(r'#define ROOM_CODE\s+"([^"]*)"', read_config())
    return match.group(1) if match else ""


def set_wifi(ssid, password):
    text = read_config()
    text = re.sub(r'#define WIFI_SSID\s+"[^"]*"', f'#define WIFI_SSID     "{ssid}"', text)
    text = re.sub(r'#define WIFI_PASSWORD\s+"[^"]*"', f'#define WIFI_PASSWORD "{password}"', text)
    CONFIG.write_text(text)


# --- board -----------------------------------------------------------------
def find_port(wait_seconds=90):
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


def ensure_libraries():
    if dry:
        say("   would check MFRC522, ArduinoMqttClient, and the ESCShared symlink")
        return
    # ESCShared is ours, not a registry library: link it into the sketchbook so a
    # plain compile/upload (no --libraries flag) can find it.
    sketchbook = Path.home() / "Documents" / "Arduino" / "libraries"
    link = sketchbook / "ESCShared"
    if not link.exists():
        sketchbook.mkdir(parents=True, exist_ok=True)
        link.symlink_to(ROOT / "firmware" / "libraries" / "ESCShared")
        say(f"   linked {link.name} into the sketchbook")
    listed = run(["arduino-cli", "lib", "list"]).stdout
    for lib in ("MFRC522", "ArduinoMqttClient"):
        if lib not in listed:
            say(f"   installing {lib}…")
            run(["arduino-cli", "lib", "install", lib])


def flash(sketch, port):
    name = SKETCHES[sketch]
    say(f"   building {name.name}…")
    built = run(["arduino-cli", "compile", "--fqbn", FQBN, "--libraries",
                 ROOT / "firmware" / "libraries", name])
    if built.returncode != 0:
        say(built.stdout[-1500:] + built.stderr[-1500:])
        die(f"{name.name} does not compile.")
    uploaded = run(["arduino-cli", "upload", "-p", port, "--fqbn", FQBN, name])
    if uploaded.returncode != 0:
        say(uploaded.stdout[-1500:] + uploaded.stderr[-1500:])
        die(f"Could not upload {name.name}.",
            "Close any Serial Monitor or other program using the port, then run this again.")
    say(f"   uploaded {name.name}")


# --- serial ----------------------------------------------------------------
def open_serial(port):
    import serial
    time.sleep(1.5)
    return serial.Serial(port, BAUD, timeout=0.3)


def read_until(ser, needles, seconds):
    """Read lines until one contains any needle. Returns (line or None, all lines)."""
    lines, deadline = [], time.time() + seconds
    while time.time() < deadline:
        raw = ser.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", "replace").strip()
        if not line:
            continue
        lines.append(line)
        say("   board: " + line)
        if any(n in line for n in needles):
            return line, lines
    return None, lines


def check_radio(port):
    """The flash is on the board; make it prove the reader and the network work."""
    # The reader is checked from the boot banner, the network from the MQTT log.
    if dry:
        say("   would confirm the RC522 answers, then that WiFi reaches the song server")
        return
    ser = open_serial(port)
    try:
        line, lines = read_until(
            ser, ["[mqtt] connected", "[wifi] cannot join", "[mqtt] failed"], 50)
        if any("not answering" in l for l in lines):
            die("The reader is not responding.",
                "Check the seven wires. Usual ones: SDA must be D10, and 3.3 V — never 5 V.\n"
                "Then run this again.")
        if line and "[mqtt] connected" in line:
            say("   reader is talking and the board is on WiFi ✓")
            return
        if line and "[wifi] cannot join" in line:
            ssid = detect_ssid()
            diagnoses = [
                "  · a 5 GHz-only network — the Arduino radio is 2.4 GHz only",
                "  · the board is too far from the access point to associate",
            ]
            if detect_password(ssid):
                diagnoses.append("  · the saved password does not match the network")
            else:
                diagnoses.append("  · an open network that still needs a login page before "
                                 "it works")
            die("The board cannot join this WiFi.",
                f"Network: {ssid}\nUsual causes, in order:\n" + "\n".join(diagnoses) +
                "\nFastest fix: phone hotspot — join the Mac to it, run this again.")
        die("The board joined WiFi but cannot reach the song server.",
            "The network let the board on but dropped its connection outward. Causes:\n"
            "  · a login/captive page this Mac cleared and the board cannot\n"
            "  · the network blocks outbound port 1883 (MQTT)\n"
            "\nTurn on your phone's Personal Hotspot, connect this Mac to it, run this again.\n"
            "The hotspot avoids both problems and is the safest thing on the night.")
    finally:
        ser.close()


# --- tags ------------------------------------------------------------------
def write_tags(port):
    flash("tags", port)
    if dry:
        for slug, _ in SONGS:
            say(f"   would write “{slug}” to its sticker")
        return []
    ser = open_serial(port)
    time.sleep(1.5)
    failed = []
    say()
    say("  For each sticker: hold it flat against the reader, then press Enter.")
    say("  Write the song name on the sticker with a marker straight after.")
    try:
        for slug, title in SONGS:
            while True:
                input(f"\n   “{title}” sticker ready  →  press Enter ")
                ser.reset_input_buffer()
                ser.write(f"write {slug}\n".encode())
                start = time.time()
                ok = False
                while time.time() - start < 12:
                    raw = ser.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", "replace").strip()
                    if line.startswith("Found") or line.startswith("OK —"):
                        say("   board: " + line)
                    if line.startswith("OK —"):
                        ok = True
                        break
                if ok:
                    say(f"   ✓ {title} sticker done")
                    break
                say("   ✗ no sticker seen, or the write failed")
                if input("   try again? [Y/n] ").strip().lower() == "n":
                    failed.append(title)
                    break
    finally:
        ser.close()
    return failed


# --- main ------------------------------------------------------------------
def main():
    say()
    say("  ESC Snowglobe — setup")
    say("  A snow globe on the pedestal plays its song on this computer.")
    if dry:
        say("  (dry run: nothing will be flashed or written)")

    step(1, 5, "Finding the Arduino")
    port = "/dev/cu.usbmodemDRYRUN" if dry else find_port()
    if port is None:
        die("No Arduino found on USB.",
            "Plug the board into this Mac with a USB-C cable, then run this again.\n"
            "If it is already plugged in, try another cable — some are charge-only.")
    say(f"   found it on {port}")

    step(2, 5, "WiFi (read from this Mac, not typed)")
    ssid = detect_ssid()
    if not ssid:
        die("This Mac is not on WiFi.",
            "Join a WiFi network first — your phone's Personal Hotspot is the reliable "
            "choice — then run this again.")
    password = detect_password(ssid)
    say(f"   network:  {ssid}")
    say("   password: " + ("open network, none needed" if not password
                           else f"found in Keychain ({len(password)} characters)"))
    if dry:
        say("   would save both to config.h")
    else:
        set_wifi(ssid, password)
        say("   saved to config.h")
    say(f"   room code: {room_code()}")

    step(3, 5, "Flashing the pedestal")
    ensure_libraries()
    flash("pedestal", port)

    step(4, 5, "Checking the reader and the network")
    check_radio(port)

    step(5, 5, "Writing the four globe stickers")
    if skip_tags:
        say("   skipped (--no-tags)")
    else:
        failed = write_tags(port)
        say()
        say("   putting the pedestal firmware back on the board")
        flash("pedestal", port)
        if failed:
            say()
            say("  Stickers that failed: " + ", ".join(failed))
            say("  Re-run to retry them; those globes stay silent until they are written.")

    say()
    say("=" * 64)
    say("  DONE.")
    say("=" * 64)
    say()
    say("  To play music:")
    say("    1. in this folder:  python3 -m http.server 8000")
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
