#pragma once
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ESC_BLE_SERVICE "5b8c0001-8f28-4a8d-a1f8-667363736e6f"
#define ESC_BLE_STATE "5b8c0002-8f28-4a8d-a1f8-667363736e6f"

// Read the full characteristic after a notification: the value may exceed one ATT packet.
inline bool escBleSnapshot(char *out, size_t cap, const char *reader,
                           const char *slug, bool healthy) {
  if (!out || !cap || !reader || strlen(reader) != 4) return false;
  for (size_t i = 0; i < 4; ++i)
    if (!((reader[i] >= '0' && reader[i] <= '9') || (reader[i] >= 'A' && reader[i] <= 'F'))) return false;
  if (slug) {
    size_t n = strlen(slug);
    if (!n || n > 24 || strcmp(slug, "-") == 0) return false;
    for (size_t i = 0; i < n; ++i)
      if (!((slug[i] >= 'a' && slug[i] <= 'z') || (slug[i] >= '0' && slug[i] <= '9') || slug[i] == '-')) return false;
  }
  int n = snprintf(out, cap, "1|%s|%s|%s", reader, healthy ? "ok" : "bad", slug ? slug : "-");
  return n >= 0 && static_cast<size_t>(n) < cap;
}
