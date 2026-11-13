// ---------------------------------------------------------------------------
// Tag presence state machine — the pedestal's whole behaviour, with no
// hardware in it, so it can be tested on a laptop (tests/presence_test.cpp).
//
// This is the same rule the old phone code used, moved to the pedestal:
//   * a tag must sit on the reader for TAG_START_MS before its song starts,
//     so a tag waved past does nothing;
//   * it may slip off for up to TAG_LOST_MS without the song stopping,
//     so a bad read or a nudge does not cut the music;
//   * swapping one globe for another stops the first song and starts the second.
// ---------------------------------------------------------------------------
#pragma once
#include <stdint.h>
#include <string.h>

#define SLUG_MAX 32

class PresenceTracker {
 public:
  PresenceTracker(uint32_t startMs, uint32_t lostMs, uint32_t startGapMs = 200)
      : startMs_(startMs), lostMs_(lostMs), startGapMs_(startGapMs) {
    candidate_[0] = '\0';
    active_[0] = '\0';
  }

  // Call every time the reader successfully reads a tag at time `now`.
  // Returns 1 when a song starts, -1 when the previous one stops because a
  // different tag arrived, 0 when nothing changed.
  int tagSeen(const char *slug, uint32_t now) {
    if (!slug || !slug[0]) return 0;
    // A real gap in reads restarts the "held steady" clock, so a tag that keeps
    // slipping off the reader never accumulates its way to a song.
    if (lastSeen_ != 0 && (uint32_t)(now - lastSeen_) > startGapMs_) candidate_[0] = '\0';
    if (strcmp(candidate_, slug) != 0) {
      strncpy(candidate_, slug, SLUG_MAX - 1);
      candidate_[SLUG_MAX - 1] = '\0';
      candidateSince_ = now;
    }
    lastSeen_ = now;

    int event = 0;
    if (playing_ && strcmp(active_, slug) != 0) {  // a different globe was put down
      playing_ = false;
      event = -1;
    }
    if (!playing_ && (uint32_t)(now - candidateSince_) >= startMs_) {
      strncpy(active_, slug, SLUG_MAX - 1);
      active_[SLUG_MAX - 1] = '\0';
      playing_ = true;
      if (event == 0) event = 1;
    }
    return event;
  }

  // Call on every loop pass where no tag was read. Returns -1 when the song
  // stops because the tag has been gone too long, otherwise 0.
  int tick(uint32_t now) {
    uint32_t goneFor = (uint32_t)(now - lastSeen_);
    if (playing_ && goneFor >= lostMs_) {
      playing_ = false;
      return -1;
    }
    if (!playing_ && candidate_[0] && goneFor >= lostMs_) candidate_[0] = '\0';
    return 0;
  }

  bool playing() const { return playing_; }
  const char *active() const { return active_; }
  const char *candidate() const { return candidate_; }

 private:
  uint32_t startMs_, lostMs_, startGapMs_;
  char candidate_[SLUG_MAX];
  char active_[SLUG_MAX];
  uint32_t candidateSince_ = 0;
  uint32_t lastSeen_ = 0;
  bool playing_ = false;
};
