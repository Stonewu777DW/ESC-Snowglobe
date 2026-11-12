// Host test for firmware/common/ndef.h — no Arduino, no hardware.
//   g++ -std=c++17 -Wall -Wextra -o ndef_test tests/ndef_roundtrip.cpp && ./ndef_test
#include "../firmware/libraries/ESCShared/src/ndef.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

static int failures = 0;

static void check(bool ok, const char *what) {
  printf("%-58s %s\n", what, ok ? "PASS" : "FAIL");
  if (!ok) failures++;
}

// Mirror of what the firmware does on the tag: encode into a fresh 4-page-aligned
// memory image starting at page 4, then read it back the way MIFARE_Read returns it.
static std::vector<uint8_t> writeAndDump(const char *slug, int reads) {
  std::vector<uint8_t> mem(180, 0x00);            // NTAG213 user memory, blank
  uint8_t buf[64];
  size_t n = ndefBuildSlug(slug, buf, sizeof(buf));
  if (n == 0) return {};
  memcpy(mem.data() + 16, buf, n);                // page 4 == offset 16
  std::vector<uint8_t> dump;
  for (int r = 0; r < reads; r++) dump.insert(dump.end(), mem.begin() + 16 + r * 16, mem.begin() + 32 + r * 16);
  return dump;
}

static std::string readBack(const std::vector<uint8_t> &dump) {
  char text[64] = {0}, slug[32] = {0};
  if (!ndefParseText(dump.data(), dump.size(), text, sizeof(text))) return "<parse failed>";
  if (!ndefTextToSlug(text, slug, sizeof(slug))) return std::string("<not ours: ") + text + ">";
  return slug;
}

int main() {
  // 1. Every slug that matters round-trips.
  const char *slugs[] = {"jingle-bells", "frosty", "rudolph", "white-christmas", "a", "abcdefghijklmnopqrstuvwx"};
  for (const char *s : slugs) {
    std::string got = readBack(writeAndDump(s, 4));
    char label[80];
    snprintf(label, sizeof(label), "round-trip '%s'", s);
    check(got == s, label);
  }

  // 2. The real page-4 dump the programmer writes, byte for byte.
  uint8_t buf[64];
  size_t n = ndefBuildSlug("jingle-bells", buf, sizeof(buf));
  const uint8_t expected[] = {0x03, 0x17, 0xD1, 0x01, 0x13, 0x54, 0x02, 0x65,
                              0x6E, 'e',  's',  'c',  ':',  'j',  'i',  'n',
                              'g',  'l',  'e',  '-',  'b',  'e',  'l',  'l',
                              's',  0xFE};
  check(n == sizeof(expected) && memcmp(buf, expected, n) == 0, "encoded bytes match the documented layout");

  // 3. A shorter slug still parses after a longer one left residue behind.
  {
    std::vector<uint8_t> mem(180, 0x00);
    uint8_t a[64];
    size_t na = ndefBuildSlug("white-christmas", a, sizeof(a));
    memcpy(mem.data() + 16, a, na);
    uint8_t b[64];
    size_t nb = ndefBuildSlug("rudolph", b, sizeof(b));
    memset(mem.data() + 16, 0, 64);
    memcpy(mem.data() + 16, b, nb);
    std::vector<uint8_t> dump(mem.begin() + 16, mem.begin() + 80);
    check(readBack(dump) == "rudolph", "short slug after a longer one parses cleanly");
  }

  // 4. Junk / blank tags are rejected instead of producing a bogus song.
  {
    char output[64] = {0};
    std::vector<uint8_t> blank(64, 0x00);
    check(!ndefParseText(blank.data(), blank.size(), output, sizeof(output)), "blank tag rejected");
    std::vector<uint8_t> random = {0xE1, 0x10, 0x12, 0x00, 0x01, 0x03, 0xA0, 0x0C};
    check(!ndefParseText(random.data(), random.size(), output, sizeof(output)), "manufacturer-only tag rejected");
  }

  // 5. A phone-written NDEF text record (whatever the phone app puts there) is not mistaken for ours.
  {
    const char *url = "https://example.com/x";
    uint8_t rec[64] = {0x03, (uint8_t)(4 + 3 + strlen(url)), 0xD1, 0x01, (uint8_t)(3 + strlen(url)), 'T', 0x02, 'e', 'n'};
    memcpy(rec + 9, url, strlen(url));
    rec[9 + strlen(url)] = 0xFE;
    char text[64] = {0}, slug[32] = {0};
    bool parsed = ndefParseText(rec, 10 + strlen(url), text, sizeof(text));
    check(parsed && !ndefTextToSlug(text, slug, sizeof(slug)), "phone-written text tag is read but ignored");
  }

  // 6. Bad input does not overflow or crash.
  {
    char out[8];
    check(!ndefParseText(nullptr, 0, out, sizeof(out)), "empty buffer rejected");
    check(ndefBuildSlug("", buf, sizeof(buf)) == 0, "empty slug refused");
    check(ndefBuildSlug("HAS CAPS", buf, sizeof(buf)) == 0, "uppercase slug refused");
    check(ndefBuildSlug("way-too-long-slug-for-a-tag-here", buf, sizeof(buf)) == 0, "over-long slug refused");
  }

  printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASS", failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
