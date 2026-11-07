// ---------------------------------------------------------------------------
// ESC Snowglobe — pedestal reader settings
// Edit this file, then upload. Everything else in the sketch is generic.
// ---------------------------------------------------------------------------
#pragma once

// --- Network ---------------------------------------------------------------
// 2.4 GHz only. The UNO R4 WiFi has no 5 GHz radio; a 5 GHz-only network will
// not connect no matter what the password is.
#define WIFI_SSID     "<redacted>"
#define WIFI_PASSWORD ""

// Room code. MUST match the code shown on the computer page (it comes from the
// page's ?room= link or the box on screen). Copy/paste it, case-sensitive.
#define ROOM_CODE "esc-snowglobe-7f3a91c2"

// Free public broker the computer page also uses. Port 1883 = plain TCP.
#define BROKER_HOST "broker.emqx.io"
#define MQTT_PORT   1883

// --- Timing (same feel as the phone version of this project) ---------------
#define TAG_START_MS  500    // a tag must sit there this long before a song starts
#define TAG_LOST_MS   500    // and be gone this long before the song stops
#define HEARTBEAT_MS  1000   // repeat "this tag is playing" while it stays on the pedestal
#define POLL_MS       60     // how often the reader looks for a tag
#define WIFI_RETRY_MS 5000
// How long to leave one association attempt alone before restarting it. The R4 can
// take ~20 s to join; retrying sooner aborts the attempt that was about to succeed.
#define WIFI_JOIN_TIMEOUT_MS 25000
#define MQTT_RETRY_MS 3000

// --- Pins (ELEGOO UNO R4 WiFi kit, RC522 on SPI) ---------------------------
// SS->D10  SCK->D13  MOSI->D11  MISO->D12  RST->D9  3.3V->3.3V  GND->GND
// IRQ is not connected. The RC522 is a 3.3 V part: never feed it 5 V.
#define RC522_SS_PIN  10
#define RC522_RST_PIN 9

// Optional beep on a passive buzzer / piezo. 0 = off.
// D13 is the SPI clock, so the built-in LED cannot be used as a status light here.
#define ENABLE_BUZZER 0
#define BUZZER_PIN    6

// Print reader events on USB serial (115200). Useful on the bench.
#define SERIAL_DEBUG 1
