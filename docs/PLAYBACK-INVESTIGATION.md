# Playback investigation — November 11, 2026

## Result

The user confirmed the original Sinatra recording was correct and requested only
removal of its opening spelling-out-the-title section. The player now uses
`songs/Jingle Bells - Sinatra (Intro Trimmed).mp3`, cut at 28.30 seconds from
`songs/Frank Sinatra - Jingle Bells 4.mp3` with a 10 ms anti-click fade-in.
Local word-timestamp transcription places the first regular chorus at approximately
28.48 seconds. The original asset remains untouched; the Dream English replacement
is no longer used. Full ffmpeg decode of the trimmed MP3 succeeded.

## Confirmed software defects

Regression tests were run red before each fix:

1. An old removal fade could execute after another globe started, pausing the new
   song. Background tabs make delayed animation callbacks especially relevant.
2. The initial sound-unlock play promise could resolve after a scan and pause that
   real song. Audio operations now carry a generation guard.
3. The keyboard speaker test was subject to the reader timeout, despite having no
   reader. Manual playback now stays on; real scans restore removal/timeout rules.
4. A requestAnimationFrame timestamp could predate fade initialization slightly.
   This produced volume > 1 and an actual browser exception; progress is now clamped
   to [0, 1].

The page now displays tag-removal/read-loss versus heartbeat-timeout stop reasons.
Firmware timing and intentional lift-to-stop behavior are unchanged.

## What this does not prove

The earlier user-observed stop was not captured in a retained serial/MQTT trace.
These are reproduced defects, not proof of which one caused that exact event.
During a new 100-second diagnostic capture the board sent reader heartbeats and
reported reader version 0x18, but no tag scan occurred. A physical placement/lift
retest is still required to close hardware acceptance.

## Verification

Run `./run_tests.sh` for the complete offline suite and both real UNO R4 builds.
Real Chromium media tests separately check decoded audio time advancing, rapid
remove/re-scan not pausing the new song, and actual removal pausing playback.
No board reflash is needed for these browser-only changes. Localhost remains the
existing background process on port 8000; reload the player and enable sound.
