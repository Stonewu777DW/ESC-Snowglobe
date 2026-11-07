// ---------------------------------------------------------------------------
// ESC Snowglobe — pedestal reader
//
// One snow globe is set down on the pedestal. Its underside carries an NFC tag.
// The RC522 under the thin top reads that tag, and this sketch tells the game
// computer which song to play. The computer page (index.html) plays the audio on
// whatever speaker that computer is connected to — the pedestal itself is silent.
//
// This replaces the old phone + AprilTag + camera flow: no phone, no QR code,
// no camera, nothing to hold up. The tag lives on the globe.
//
// Behaviour
//   * tag sits on the reader for TAG_START_MS  -> song starts from the beginning
//   * tag gone for TAG_LOST_MS                 -> song stops (fade handled by page)
//   * "no tag" is only reported after the grace period, so a nudge is not a stop
//
// Messages (same broker as the computer page, room-scoped)
//   esc-snowglobe/<ROOM>/tag     {"reader":"..","song":"<slug>","present":true}
//   esc-snowglobe/<ROOM>/reader  {"reader":"..","fw":"1.0.0","state":"online"}
//
// Wiring is in docs/WIRING.md. Settings are in config.h.
// ---------------------------------------------------------------------------
#include <SPI.h>
#include <MFRC522.h>
#include <WiFiS3.h>
#include <ArduinoMqttClient.h>

#include "config.h"
#include <ESCShared.h>
#include "songs.h"

MFRC522 rfid(RC522_SS_PIN, RC522_RST_PIN);
WiFiClient wifiClient;
MqttClient mqtt(wifiClient);
PresenceTracker tracker(TAG_START_MS, TAG_LOST_MS);

char readerId[16] = "reader";
char topicTag[64];
char topicReader[64];
uint32_t lastPoll = 0, lastHeartbeat = 0, lastReaderStatus = 0;
uint32_t lastWifiTry = 0, lastMqttTry = 0;
char playingSlug[SLUG_MAX] = {0};
bool publishedPlaying = false;

// --- little helpers --------------------------------------------------------
void logLine(const char *fmt, ...) {
#if SERIAL_DEBUG
  char buf[160];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.println(buf);
#endif
}

void beep(uint16_t ms) {
#if ENABLE_BUZZER
  tone(BUZZER_PIN, 2400, ms);
#endif
}

String uidToHex() {
  String s;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) s += '0';
    s += String(rfid.uid.uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

// Read the NDEF text record the tag programmer wrote and turn it into a slug.
bool slugFromTag(char *slug, size_t cap) {
  uint8_t block[18];
  uint8_t data[NDEF_READ_PAGES * 16];
  // One page at a time. A 16-byte block read is unreliable on this clone chip (the
  // same silicon that only accepts 4-byte writes), so page reads are the safe path.
  for (uint8_t r = 0; r < NDEF_READ_PAGES; r++) {
    uint8_t page = 4 + r * 4;  // user memory of an NTAG213/215/216 starts at page 4
    for (uint8_t k = 0; k < 4; k++) {
      uint8_t size = sizeof(block);
      if (rfid.MIFARE_Read(page + k, block, &size) != MFRC522::STATUS_OK) return false;
      memcpy(data + r * 16 + k * 4, block, 4);
    }
  }
  char text[64];
  if (!ndefParseText(data, sizeof(data), text, sizeof(text))) return false;
  return ndefTextToSlug(text, slug, cap);
}

// --- network ---------------------------------------------------------------
bool wifiLinked = false, wifiWarned = false, wifiTried = false;
uint32_t wifiStartedAt = 0;

// An open network (a guest SSID with no password) must be joined with the
// one-argument begin(), otherwise the board waits for a key that will never come.
void joinWifi() {
  // Measured on this board (2026-11-06): an OPEN network joins only via the
  // one-argument begin(ssid). begin(ssid, "") sits at WL_IDLE forever and never
  // associates, which is the failure this project spent hours chasing.
  if (strlen(WIFI_PASSWORD) == 0) {
    logLine("[wifi] joining \"%s\" (open: single-arg begin)", WIFI_SSID);
    WiFi.begin(WIFI_SSID);
  } else {
    logLine("[wifi] joining \"%s\" (passphrase)", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

bool networkUp() {
  if (strlen(WIFI_SSID) == 0) return false;   // serial-only bench mode, no joining
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiLinked) {
      wifiLinked = true;
      logLine("[wifi] joined \"%s\"", WIFI_SSID);
    }
    return true;
  }
  wifiLinked = false;

  uint32_t now = millis();
  if (!wifiTried) {  // first attempt, right after boot
    wifiTried = true;
    wifiStartedAt = now;
    lastWifiTry = now;
    joinWifi();
    return false;
  }
  // Association on the R4 takes up to ~20 s. Restarting an attempt inside that window
  // is what kept this board off USC Guest Wireless: a retry loop on a 5 s timer
  // aborted every join before it could finish. So wait the full timeout before
  // touching anything, and never call WiFi.end() - the ESP32 re-begin is enough and
  // end() kills a join that is still in progress.
  if (now - wifiStartedAt >= WIFI_JOIN_TIMEOUT_MS && now - lastWifiTry >= WIFI_RETRY_MS) {
    lastWifiTry = now;
    wifiStartedAt = now;
    logLine("[wifi] retrying \"%s\" (status %d)", WIFI_SSID, WiFi.status());
    joinWifi();
  }
  if (!wifiWarned && now - wifiStartedAt >= WIFI_JOIN_TIMEOUT_MS) {
    wifiWarned = true;
    logLine("[wifi] cannot join \"%s\" after %u ms (status %d)",
            WIFI_SSID, (unsigned)WIFI_JOIN_TIMEOUT_MS, WiFi.status());
  }
  return false;
}

void mqttPublish(const char *topic, const char *payload) {
  if (!mqtt.connected()) return;
  mqtt.beginMessage(topic);
  mqtt.print(payload);
  mqtt.endMessage();
}

void publishTag(const char *slug, bool present) {
  if (!mqtt.connected()) return;
  char payload[128];
  if (present) {
    snprintf(payload, sizeof(payload), "{\"reader\":\"%s\",\"song\":\"%s\",\"present\":true}", readerId, slug);
  } else {
    snprintf(payload, sizeof(payload), "{\"reader\":\"%s\",\"song\":null,\"present\":false}", readerId);
  }
  mqttPublish(topicTag, payload);
  logLine("[mqtt] %s", payload);
}

void publishReaderStatus() {
  if (!mqtt.connected()) return;
  char payload[96];
  snprintf(payload, sizeof(payload), "{\"reader\":\"%s\",\"fw\":\"1.0.0\",\"state\":\"online\"}", readerId);
  mqttPublish(topicReader, payload);
}

void networkTick() {
  if (!networkUp()) return;

  if (!mqtt.connected()) {
    uint32_t now = millis();
    if (now - lastMqttTry < MQTT_RETRY_MS) return;
    lastMqttTry = now;
    mqtt.setId(readerId);
    mqtt.setCleanSession(true);
    logLine("[mqtt] connecting to %s:%u", BROKER_HOST, MQTT_PORT);
    if (!mqtt.connect(BROKER_HOST, MQTT_PORT)) {
      logLine("[mqtt] failed, error %d", mqtt.connectError());
      return;
    }
    logLine("[mqtt] connected, room %s", ROOM_CODE);
    publishReaderStatus();
    lastReaderStatus = millis();
    // Tell the page what is on the pedestal right now, if anything.
    if (tracker.playing()) publishTag(tracker.active(), true);
    return;
  }

  mqtt.poll();

  uint32_t now = millis();
  if (now - lastReaderStatus >= 2000) {  // liveness, so the page can show "reader offline"
    lastReaderStatus = now;
    publishReaderStatus();
  }
  if (tracker.playing() && now - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = now;
    publishTag(tracker.active(), true);  // repeat so a reloaded page recovers
  }
}

// --- reader ----------------------------------------------------------------
void handleEvent(int event) {
  if (event == 1) {
    strncpy(playingSlug, tracker.active(), sizeof(playingSlug) - 1);
    playingSlug[sizeof(playingSlug) - 1] = '\0';
    publishedPlaying = true;
    lastHeartbeat = millis();
    logLine("[play] %s", playingSlug);
    Serial.print("TAG ");  // serial protocol: useful on the bench and for a local bridge
    Serial.println(playingSlug);
    publishTag(playingSlug, true);
    beep(60);
  } else if (event == -1) {
    publishedPlaying = false;
    logLine("[stop] %s", playingSlug[0] ? playingSlug : "(none)");
    Serial.println("NONE");
    playingSlug[0] = '\0';
    publishTag(nullptr, false);
    beep(30);
  }
}

void readerTick() {
  uint32_t now = millis();
  if (now - lastPoll < POLL_MS) {
    handleEvent(tracker.tick(now));
    return;
  }
  lastPoll = now;

  bool sawTag = false;
  if (escTagPresent(rfid)) {  // WUPA, not REQA: a resting tag is HALTed between reads
    char slug[SLUG_MAX] = {0};
    if (slugFromTag(slug, sizeof(slug))) {
      sawTag = true;
      handleEvent(tracker.tagSeen(slug, now));
    } else {
      String uid = uidToHex();
      const char *mapped = songForUid(uid.c_str());
      if (mapped) {
        sawTag = true;
        logLine("[tag] %s -> uid table -> %s", uid.c_str(), mapped);
        handleEvent(tracker.tagSeen(mapped, now));
      } else {
        static String lastUnknown;
        if (lastUnknown != uid) {
          lastUnknown = uid;
          logLine("[tag] %s has no song payload, and is not in UID_SONGS (songs.h)", uid.c_str());
        }
      }
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  if (!sawTag) handleEvent(tracker.tick(now));
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("ESC Snowglobe — NFC pedestal reader");

  SPI.begin();
  rfid.PCD_Init();
  delay(50);
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("RC522 not answering. Check 3.3V, GND, and D9/D10/D11/D12/D13 wiring.");
  } else {
    Serial.print("RC522 ok, version 0x");
    Serial.println(version, HEX);
  }

#if ENABLE_BUZZER
  pinMode(BUZZER_PIN, OUTPUT);
#endif

  snprintf(topicTag, sizeof(topicTag), "esc-snowglobe/%s/tag", ROOM_CODE);
  snprintf(topicReader, sizeof(topicReader), "esc-snowglobe/%s/reader", ROOM_CODE);

  uint8_t mac[6] = {0};
  WiFi.macAddress(mac);
  if (mac[0] || mac[1] || mac[2] || mac[3] || mac[4] || mac[5]) {
    snprintf(readerId, sizeof(readerId), "%02X%02X", mac[4], mac[5]);
  } else {
    snprintf(readerId, sizeof(readerId), "pedestal");
  }
  Serial.print("room: ");
  Serial.println(ROOM_CODE);
  Serial.print("reader id: ");
  Serial.println(readerId);

  // No begin() here: networkUp() owns the join, so an open network goes through
  // joinWifi() and there is never a half-started association to fight with.
  if (strlen(WIFI_SSID) == 0) {
    Serial.println("No WIFI_SSID in config.h — running serial-only. Set it to talk to the page.");
  }
}

void loop() {
  readerTick();
  networkTick();
#if SERIAL_DEBUG
  // Repeat the reader state so it is visible whenever the port is opened, not
  // only in the 300 ms after a reset.
  static uint32_t lastStatus = 0;
  if (millis() - lastStatus >= 10000) {
    lastStatus = millis();
    byte v = rfid.PCD_ReadRegister(MFRC522::VersionReg);
    if (v == 0x00 || v == 0xFF) Serial.println("[reader] NOT answering (0x00/0xFF)");
    else Serial.print("[reader] ok, version 0x") , Serial.println(v, HEX);
  }
#endif
}
