// ---------------------------------------------------------------------------
// Song table and UID fallback.
//
// Normal path: the tag carries "esc:<slug>" and the slug goes straight to the
// computer page, which owns the list of mp3 files. Nothing here needs editing
// to add a song — see README.
//
// Fallback path: a tag with no NDEF payload (a blank sticker, or one a phone
// wiped) can still work by UID. Put its UID here. UID is printed by the
// programmer sketch and by this sketch's serial log when it sees an unknown tag.
// ---------------------------------------------------------------------------
#pragma once
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

struct UidSong {
  const char *uid;   // 14 hex chars, uppercase, as printed by the programmer
  const char *slug;  // slug registered in index.html
};

static const UidSong UID_SONGS[] = {
  // The four globes as programmed on 2026-11-06 (UIDs read off the reader itself).
  // These make each globe work even if its NDEF payload read comes back garbled.
  {"04EE67B5D32A81", "jingle-bells"},     // bell globe
  {"04E767B5D32A81", "rudolph"},          // reindeer
  {"04E867B5D32A81", "frosty"},           // snowman
  {"04E967B5D32A81", "white-christmas"},  // bunny
};
static const size_t UID_SONG_COUNT = sizeof(UID_SONGS) / sizeof(UID_SONGS[0]);

inline const char *songForUid(const char *uid) {
  for (size_t i = 0; i < UID_SONG_COUNT; i++) {
    const char *a = UID_SONGS[i].uid;
    const char *b = uid;
    bool same = true;
    while (*a && *b) {
      char ca = (*a >= 'a' && *a <= 'f') ? (char)(*a - 32) : *a;
      char cb = (*b >= 'a' && *b <= 'f') ? (char)(*b - 32) : *b;
      if (ca != cb) { same = false; break; }
      a++; b++;
    }
    if (same && *a == '\0' && *b == '\0') return UID_SONGS[i].slug;
  }
  return nullptr;
}
