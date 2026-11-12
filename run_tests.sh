#!/usr/bin/env bash
# Exercise the production codecs, browser pages, and all supported board builds.
set -euo pipefail
cd "$(dirname "$0")"
test_tmp=$(mktemp -d "${TMPDIR:-${RUNNER_TEMP:-.}}/esc-tests.XXXXXX")
trap 'rm -rf "$test_tmp"' EXIT
for name in ndef_roundtrip pagewrite_test presence_test bleprotocol_test; do
  printf '\n== %s (C++)\n' "$name"
  g++ -std=c++17 -Wall -Wextra -Werror -o "$test_tmp/$name" "tests/$name.cpp"
  "$test_tmp/$name"
done
printf '\n== player, connection and real page DOM tests (node)\n'
if [ ! -d node_modules/jsdom ]; then
  printf 'jsdom is not installed. Run: npm install\n'
  exit 1
fi
node --test --test-timeout=20000 tests/*.test.mjs
printf '\n== legacy setup script plans the right thing (dry run, no board touched)\n'
out=$(python3 tools/setup.py --dry-run 2>&1)
for slug in jingle-bells frosty rudolph white-christmas; do
  [[ "$out" == *"would write “${slug}”"* ]] || { printf 'MISSING sticker %s\n' "$slug"; exit 1; }
done
[[ "$out" == *'would run: arduino-cli upload'* ]] || { printf 'MISSING upload step\n'; exit 1; }
[[ "$out" == *'would save both to config.h'* ]] || { printf 'MISSING wifi save step\n'; exit 1; }
printf 'PASS — 4 stickers, uploads, and the WiFi save are planned\n'
printf '\n== Pages project bootstrap (offline unit tests)\n'
python3 -m unittest discover -s tests -p test_pages_project.py
printf '\n== firmware compiles for the UNO R4 WiFi\n'
for mode in wifi ble programmer; do
  sketch=ESC_NFC_Pedestal
  args=(--fqbn arduino:renesas_uno:unor4wifi --libraries firmware/libraries)
  if [[ "$mode" == ble ]]; then args+=(--build-property 'compiler.cpp.extra_flags=-DESC_BLE_MODE=1'); fi
  if [[ "$mode" == programmer ]]; then sketch=ESC_Tag_Programmer; fi
  printf '%s (%s)\n' "$sketch" "$mode"
  if ! arduino-cli compile "${args[@]}" "firmware/$sketch" >"$test_tmp/build.log" 2>&1; then
    python3 -c 'import pathlib,sys; print(pathlib.Path(sys.argv[1]).read_text())' "$test_tmp/build.log"
    exit 1
  fi
  python3 -c 'import pathlib,sys; print("\n".join(x for x in pathlib.Path(sys.argv[1]).read_text().splitlines() if x.startswith(("Sketch uses", "Global variables"))))' "$test_tmp/build.log"
done
