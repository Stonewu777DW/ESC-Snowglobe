#include <cassert>
#include <cstring>
#include "../firmware/libraries/ESCShared/src/bleprotocol.h"
int main() {
  char out[40];
  assert(escBleSnapshot(out, sizeof(out), "E068", "jingle-bells", true));
  assert(std::strcmp(out, "1|E068|ok|jingle-bells") == 0);
  assert(escBleSnapshot(out, sizeof(out), "E068", nullptr, false));
  assert(std::strcmp(out, "1|E068|bad|-") == 0);
  assert(escBleSnapshot(out, sizeof(out), "AB12", "abcdefghijklmnopqrstuvwx", true));
  assert(!escBleSnapshot(out, 8, "AB12", "frosty", true));
  assert(!escBleSnapshot(out, sizeof(out), "????", "frosty", true));
  assert(!escBleSnapshot(out, sizeof(out), "AB12", "bad|slug", true));
  assert(!escBleSnapshot(out, sizeof(out), "AB12", "abcdefghijklmnopqrstuvwxyz", true));
  assert(!escBleSnapshot(out, sizeof(out), "AB12", "-", true));
  puts("ALL PASS (BLE snapshot format)");
}
