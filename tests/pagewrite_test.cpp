// The write path that actually works on this chip: 4-byte pages.
//
// The bug this guards: a 16-byte block write stored only its first page on the
// module in this kit, so a 26-byte NDEF blob came back as
//     03 17 D1 01 00 00 00 00 ... | 67 6C 65 2D 00 ...
// The payload is now pushed one page at a time, which makes this splitting the
// thing that has to be right.
#include <cstdio>
#include <cstring>
#include <cstdint>

#include "../firmware/libraries/ESCShared/src/pages.h"
#include "../firmware/libraries/ESCShared/src/ndef.h"
#include "../firmware/libraries/ESCShared/src/presence.h"  // SLUG_MAX

static int failures = 0;

static void check(bool ok, const char *what) {
  if (!ok) { std::printf("  FAIL %s\n", what); failures++; }
}

int main() {
  // A real record, exactly as ndef.h builds it for the bell globe.
  uint8_t blob[64] = {0};
  size_t n = ndefBuildSlug("jingle-bells", blob, sizeof(blob));
  check(n == 26, "jingle-bells record is 26 bytes");

  // 26 bytes -> 7 pages, and the last page must be zero-padded, not garbage.
  check(escPageCount((uint16_t)n) == 7, "26 bytes need 7 pages");

  uint8_t rebuilt[64] = {0};
  for (uint16_t p = 0; p < escPageCount((uint16_t)n); p++) {
    uint8_t pageBytes[4];
    escPageSlice(blob, (uint16_t)n, p, pageBytes);
    // Every page is exactly 4 bytes, so anything past the payload must be 0.
    for (int i = 0; i < 4; i++) {
      uint16_t off = (uint16_t)(p * 4 + i);
      if (off >= n) check(pageBytes[i] == 0, "tail byte is zero-padded");
    }
    std::memcpy(rebuilt + p * 4, pageBytes, 4);
  }

  // Page-by-page then reassembled must be byte-identical to the original.
  check(std::memcmp(rebuilt, blob, n) == 0, "page round-trip is byte-identical");

  // And it must still parse back to the same slug.
  char text[64] = {0}, slug[SLUG_MAX] = {0};
  check(ndefParseText(rebuilt, sizeof(rebuilt), text, sizeof(text)), "reparses as NDEF text");
  check(std::strcmp(text, "esc:jingle-bells") == 0, "text is esc:jingle-bells");
  check(ndefTextToSlug(text, slug, sizeof(slug)) && std::strcmp(slug, "jingle-bells") == 0,
        "slug round-trips");

  // Edge cases.
  check(escPageCount(1) == 1 && escPageCount(4) == 1 && escPageCount(5) == 2,
        "page count boundaries");

  // A page past the end of a short buffer must read as zeros, never as stack junk.
  uint8_t tiny[2] = {0xAA, 0xBB};
  uint8_t page3[4];
  escPageSlice(tiny, 2, 3, page3);
  check(page3[0] == 0 && page3[3] == 0, "buffer shorter than one page is zero-padded");

  std::printf(failures == 0 ? "ALL PASS (0 failures)\n" : "FAILURES: %d\n", failures);
  return failures == 0 ? 0 : 1;
}
