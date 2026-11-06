#!/usr/bin/env bash
# Everything that can be checked without an Arduino on the desk.
set -euo pipefail
cd "$(dirname "$0")"

echo "== NFC payload codec (C++)"
g++ -std=c++17 -Wall -Wextra -Werror -o /tmp/esc_ndef_test tests/ndef_roundtrip.cpp
/tmp/esc_ndef_test | tail -1

echo
echo "== pedestal timing rules (C++)"
g++ -std=c++17 -Wall -Wextra -Werror -o /tmp/esc_presence_test tests/presence_test.cpp
/tmp/esc_presence_test | tail -1

echo
echo "== page logic + real page in a DOM (node)"
if [ ! -d node_modules/jsdom ]; then
  echo "jsdom is not installed. Run: npm install"
  exit 1
fi
node --test --test-timeout=20000 tests/player.test.mjs tests/page.test.mjs | tail -9

echo
echo "== setup script plans the right thing (dry run, no board touched)"
out=$(python3 tools/setup.py --dry-run 2>&1)
for slug in jingle-bells frosty rudolph white-christmas; do
  grep -q "would write .$slug." <<<"$out" || { echo "MISSING sticker $slug"; exit 1; }
done
grep -q "would run: arduino-cli upload" <<<"$out" || { echo "MISSING upload step"; exit 1; }
grep -q "would save both to config.h" <<<"$out" || { echo "MISSING wifi save step"; exit 1; }
echo "PASS — 4 stickers, uploads, and the WiFi save are all planned"

echo
echo "== firmware compiles for the UNO R4 WiFi"
for sketch in ESC_NFC_Pedestal ESC_Tag_Programmer; do
  printf '%-22s ' "$sketch"
  arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi --libraries firmware/libraries "firmware/$sketch" 2>&1 \
    | grep -E 'Sketch uses|error' | head -1
done
