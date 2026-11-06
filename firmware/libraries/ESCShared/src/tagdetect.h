// Continuous tag detection for a tag that *stays* on the reader.
//
// The obvious call, PICC_IsNewCardPresent(), sends REQA. REQA only wakes cards in
// the IDLE state. A card that has been selected and then halted — which is what
// happens after every read — ignores REQA until it leaves the field. On a pedestal
// the globe sits still, so REQA is useless: the first read works, every read after
// that reports "no tag" and the music would cut out.
//
// WUPA (0x52) wakes cards in both IDLE and HALT, so the correct cycle is:
//     WUPA -> SELECT -> read pages -> HALT -> (60 ms later) WUPA -> ...
// That is what escTagPresent() + the caller's PICC_HaltA() give you.
#pragma once
#include <MFRC522.h>

inline bool escTagPresent(MFRC522 &rfid) {
  uint8_t atqa[2];
  uint8_t atqaSize = sizeof(atqa);
  MFRC522::StatusCode status = rfid.PICC_WakeupA(atqa, &atqaSize);
  if (status != MFRC522::STATUS_OK && status != MFRC522::STATUS_COLLISION) return false;
  return rfid.PICC_ReadCardSerial();  // SELECT, fills rfid.uid
}
