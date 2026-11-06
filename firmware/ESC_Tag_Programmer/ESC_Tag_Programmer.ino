// ---------------------------------------------------------------------------
// ESC Snowglobe — NFC tag programmer
//
// Writes the song payload onto the sticker that goes on the bottom of a globe.
// Uses the same Arduino + RC522 as the pedestal, so no extra writer hardware.
//
// Open Serial Monitor at 115200 (line ending: Newline) and type:
//   scan              show the tag's UID, its current payload, and its chip type
//   write <slug>      write a song payload, then read it back to prove it stuck
//   write 2           write the slug listed as #2 in SONGS (index.html order)
//   erase             remove the payload (tag keeps its UID)
//   help
//
// A slug is lowercase letters, digits and hyphens: jingle-bells, frosty, rudolph,
// white-christmas, snowman-2 ...  It must also be in SONGS in index.html.
//
// Only MIFARE Ultralight / NTAG stickers are written. Anything else is refused,
// because writing raw pages to a Classic card would destroy its data.
// ---------------------------------------------------------------------------
#include <SPI.h>
#include <MFRC522.h>

#include <ESCShared.h>

MFRC522 rfid(RC522_SS_PIN, RC522_RST_PIN);

void printUid();

// Keep this in the same order as SONGS in index.html.
static const char *SONGS[] = {"jingle-bells", "frosty", "rudolph", "white-christmas"};
static const size_t SONG_COUNT = sizeof(SONGS) / sizeof(SONGS[0]);

static const uint8_t FIRST_PAGE = 4;     // NTAG user memory
static const uint8_t WRITE_PAGES = 16;   // 64 bytes: far more than one record needs

String prompt = "> ";

bool waitForTag() {
  Serial.println("Hold a tag on the reader…");
  uint32_t start = millis();
  while (millis() - start < 30000) {
    if (escTagPresent(rfid)) {
      MFRC522::PICC_Type type = rfid.PICC_GetType(rfid.uid.sak);
      Serial.print("Found ");
      Serial.print(rfid.PICC_GetTypeName(type));
      Serial.print("  UID ");
      printUid();
      bool isNtag = (type == MFRC522::PICC_TYPE_MIFARE_UL);
      if (!isNtag) {
        Serial.println("Refusing: only MIFARE Ultralight / NTAG stickers are programmable here.");
        rfid.PICC_HaltA();
        return false;
      }
      return true;
    }
    delay(30);
  }
  Serial.println("Timed out waiting for a tag.");
  return false;
}

void printUid() {
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print('0');
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
}

bool readPages(uint8_t *out, uint8_t pages) {
  uint8_t block[18];
  for (uint8_t r = 0; r < pages / 4; r++) {
    uint8_t size = sizeof(block);
    if (rfid.MIFARE_Read(FIRST_PAGE + r * 4, block, &size) != MFRC522::STATUS_OK) {
      Serial.println("Read failed.");
      return false;
    }
    memcpy(out + r * 16, block, 16);
  }
  return true;
}

void printCurrentPayload() {
  uint8_t data[WRITE_PAGES * 4];
  if (!readPages(data, WRITE_PAGES)) return;
  char text[64] = {0}, slug[SLUG_MAX] = {0};
  if (ndefParseText(data, sizeof(data), text, sizeof(text))) {
    if (ndefTextToSlug(text, slug, sizeof(slug))) {
      Serial.print("Payload: ");
      Serial.println(slug);
    } else {
      Serial.print("Tag holds text we do not use: \"");
      Serial.print(text);
      Serial.println("\"");
    }
  } else {
    Serial.println("Payload: empty (no NDEF text record)");
  }
}

bool writeSlug(const char *slug) {
  uint8_t blob[64] = {0};
  size_t n = ndefBuildSlug(slug, blob, sizeof(blob));
  if (n == 0) {
    Serial.println("Bad slug. Lowercase letters, digits, hyphens; 24 characters max.");
    return false;
  }
  for (uint8_t r = 0; r < WRITE_PAGES / 4; r++) {
    uint8_t page = FIRST_PAGE + r * 4;
    MFRC522::StatusCode st = rfid.MIFARE_Write(page, blob + r * 16, 16);
    if (st != MFRC522::STATUS_OK) {
      Serial.print("Write failed at page ");
      Serial.print(page);
      Serial.print(": ");
      Serial.println(rfid.GetStatusCodeName(st));
      return false;
    }
  }
  // Prove it, do not assume it.
  uint8_t check[WRITE_PAGES * 4];
  if (!readPages(check, WRITE_PAGES)) return false;
  char text[64] = {0}, got[SLUG_MAX] = {0};
  if (!ndefParseText(check, sizeof(check), text, sizeof(text)) ||
      !ndefTextToSlug(text, got, sizeof(got)) || strcmp(got, slug) != 0) {
    Serial.println("Wrote the tag but the read-back does not match. Try again.");
    return false;
  }
  Serial.print("OK — tag now reads back as \"");
  Serial.print(got);
  Serial.println("\"");
  Serial.println("Label the sticker before it goes anywhere; blank tags are identical.");
  return true;
}

bool eraseTag() {
  uint8_t blank[64] = {0};
  blank[0] = 0xFE;  // NDEF terminator: "this tag has no message"
  for (uint8_t r = 0; r < WRITE_PAGES / 4; r++) {
    MFRC522::StatusCode st = rfid.MIFARE_Write(FIRST_PAGE + r * 4, blank + r * 16, 16);
    if (st != MFRC522::STATUS_OK) {
      Serial.print("Erase failed: ");
      Serial.println(rfid.GetStatusCodeName(st));
      return false;
    }
  }
  Serial.println("Erased. The tag still has its UID, so it can still be used from songs.h.");
  return true;
}

void help() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  scan            identify the tag and show its payload");
  Serial.println("  write <slug>    write a song payload");
  Serial.println("  erase           remove the payload");
  Serial.println();
  Serial.print("Songs on the computer page, in order:");
  for (size_t i = 0; i < SONG_COUNT; i++) {
    Serial.print("  ");
    Serial.print(i);
    Serial.print("=");
    Serial.print(SONGS[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println();
  Serial.println("ESC Snowglobe — NFC tag programmer");

  SPI.begin();
  rfid.PCD_Init();
  delay(50);
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("RC522 not answering. Check 3.3V, GND, and D9/D10/D11/D12/D13 wiring.");
  }
  help();
  Serial.print(prompt);
}

void loop() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) { Serial.print(prompt); return; }

  if (line.equalsIgnoreCase("help")) {
    help();
  } else if (line.equalsIgnoreCase("scan")) {
    if (waitForTag()) {
      printUid();
      printCurrentPayload();
      rfid.PICC_HaltA();
    }
  } else if (line.equalsIgnoreCase("erase")) {
    if (waitForTag()) {
      eraseTag();
      rfid.PICC_HaltA();
    }
  } else if (line.startsWith("write ")) {
    String arg = line.substring(6);
    arg.trim();
    const char *slug = arg.c_str();
    // "write 2" means "the third slug in SONGS" (0-based, matching the page's list).
    if (arg.length() > 0 && arg.toInt() >= 0 && arg.toInt() < (int)SONG_COUNT && String(arg.toInt()) == arg) {
      slug = SONGS[arg.toInt()];
    }
    if (!ndefValidSlug(slug)) {
      Serial.println("Bad slug. Lowercase letters, digits and hyphens only.");
    } else if (waitForTag()) {
      writeSlug(slug);
      rfid.PICC_HaltA();
    }
  } else {
    Serial.println("Unknown command. Type help.");
  }

  delay(200);
  Serial.print(prompt);
}
