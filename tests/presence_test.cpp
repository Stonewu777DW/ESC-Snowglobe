// Host test for firmware/common/presence.h — the pedestal's timing rules,
// driven by a fake clock. No hardware required.
//   g++ -std=c++17 -Wall -Wextra -o /tmp/presence_test tests/presence_test.cpp && /tmp/presence_test
#include "../firmware/libraries/ESCShared/src/presence.h"

#include <cstdio>
#include <string>

static int failures = 0;

static void check(bool ok, const char *what) {
  printf("%-62s %s\n", what, ok ? "PASS" : "FAIL");
  if (!ok) failures++;
}

// Feed the tracker `reads` of `slug`, one every `stepMs`, advancing the clock.
struct Sim {
  PresenceTracker t{500, 500};
  uint32_t now = 1000;
  std::string started, stopped;
  int events = 0;

  void hold(const char *slug, uint32_t forMs, uint32_t stepMs = 20) {
    for (uint32_t d = 0; d < forMs; d += stepMs) {
      now += stepMs;
      int e = t.tagSeen(slug, now);
      if (e == 1) { started = t.active(); events++; }
      if (e == -1) { stopped = t.active(); events++; }
    }
  }
  void idle(uint32_t forMs, uint32_t stepMs = 20) {
    for (uint32_t d = 0; d < forMs; d += stepMs) {
      now += stepMs;
      if (t.tick(now) == -1) stopped = "(none)";
    }
  }
};

int main() {
  {  // A tag flashed quickly never starts a song.
    Sim s;
    s.hold("jingle-bells", 200);
    check(s.started.empty() && !s.t.playing(), "tap-and-go tag (200 ms) does not start music");
  }
  {  // Held steady for half a second: starts exactly once.
    Sim s;
    s.hold("jingle-bells", 700);
    check(s.started == "jingle-bells" && s.t.playing(), "tag held 700 ms starts its song");
    check(s.events == 1, "song starts exactly once, not once per poll");
  }
  {  // Brief loss is tolerated (blurred read, bumped globe).
    Sim s;
    s.hold("rudolph", 700);
    s.idle(300);
    s.hold("rudolph", 300);
    check(s.t.playing(), "300 ms dropout keeps the music playing");
  }
  {  // Long loss stops it.
    Sim s;
    s.hold("rudolph", 700);
    s.idle(900);
    check(!s.t.playing(), "tag gone 900 ms stops the music");
  }
  {  // Swapping globes stops the first and starts the second.
    Sim s;
    s.hold("frosty", 700);
    s.hold("white-christmas", 700);
    check(s.stopped == "frosty", "swap: the old song is stopped first");
    check(s.t.playing() && std::string(s.t.active()) == "white-christmas",
          "swapping globes plays the new globe's song");
    check(s.started == "white-christmas", "the new globe is the one that started last");
  }
  {  // Jitter around the threshold does not machine-gun the song.
    Sim s;
    for (int i = 0; i < 6; i++) {
      s.hold("jingle-bells", 240);
      s.idle(240);
    }
    check(s.events == 0, "tag dithering at the edge never starts a song");
  }
  {  // A song that already started survives a short tag-off blip and stays one song.
    Sim s;
    s.hold("frosty", 700);
    int before = s.events;
    s.idle(200);
    s.hold("frosty", 400);
    check(s.events == before && s.t.playing(), "no re-trigger after a blip: music keeps rolling");
  }

  printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL PASS", failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
