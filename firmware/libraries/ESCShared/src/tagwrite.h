// Writing MIFARE Ultralight / NTAG memory, one 4-byte page at a time.
//
// MFRC522::MIFARE_Write() sends 16 bytes in one go, which is the documented
// Ultralight block write. On the module in this kit that silently stores only the
// first page of each block: a 26-byte NDEF blob came back as
//     03 17 D1 01 00 00 00 00 ... 00 | 67 6C 65 2D 00 ...
// i.e. 4 good bytes per 16. MIFARE_Ultralight_Write() (0xA2, page, 4 bytes) works.
#pragma once
#include <Arduino.h>
#include <MFRC522.h>
#include "pages.h"

// Write `len` bytes starting at `firstPage`, 4 bytes per page, zero-padding the tail.
// Returns true only if every page took the write.
inline bool escWritePages(MFRC522 &rfid, uint8_t firstPage, const uint8_t *data, uint16_t len,
                          uint8_t *failedPage = nullptr) {
  uint16_t pages = escPageCount(len);
  for (uint16_t i = 0; i < pages; i++) {
    uint8_t p4[ESC_PAGE_SIZE];
    escPageSlice(data, len, i, p4);
    MFRC522::StatusCode st = rfid.MIFARE_Ultralight_Write((uint8_t)(firstPage + i), p4, ESC_PAGE_SIZE);
    if (st != MFRC522::STATUS_OK) {
      if (failedPage) *failedPage = (uint8_t)(firstPage + i);
      return false;
    }
    delay(4);  // the chip needs a few ms to finish burning each page
  }
  return true;
}
