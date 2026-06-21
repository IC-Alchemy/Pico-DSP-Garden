#!/usr/bin/env bash
# Compile every example sketch in Examples/ against the local rpdsp and
# pico_audio_i2s libraries. Does NOT flash (CI has no hardware).
#
# Usage: scripts/build_all_examples.sh
# Exit: non-zero if any example fails to compile.
#
# Requires: arduino-cli on PATH, rp2040:rp2040 core installed.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FQBN="$(cat "$SCRIPT_DIR/fqbn.txt")"

EXAMPLES_DIR="$REPO_ROOT/Examples"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "ERROR: arduino-cli not found on PATH." >&2
  echo "Install: https://arduino.github.io/arduino-cli/latest/" >&2
  exit 1
fi

# Discover examples: every subdirectory of Examples/ containing a .ino.
shopt -s nullglob
examples=()
for d in "$EXAMPLES_DIR"/*/; do
  ino=( "$d"*.ino )
  if [ ${#ino[@]} -gt 0 ]; then
    examples+=( "$d" )
  fi
done
shopt -u nullglob

if [ ${#examples[@]} -eq 0 ]; then
  echo "No examples found in $EXAMPLES_DIR" >&2
  exit 1
fi

echo "Building ${#examples[@]} examples with FQBN:"
echo "  $FQBN"
echo

pass=0
fail=0
failed_examples=()

for ex in "${examples[@]}"; do
  name="$(basename "$ex")"
  printf '%-32s ... ' "$name"
  if arduino-cli compile \
        --fqbn "$FQBN" \
        --library "$REPO_ROOT/libraries/rpdsp" \
        --library "$REPO_ROOT/libraries/pico_audio_i2s" \
        "$ex" >/tmp/build_"$name".log 2>&1; then
    echo "PASS"
    pass=$((pass + 1))
  else
    echo "FAIL (see /tmp/build_${name}.log)"
    fail=$((fail + 1))
    failed_examples+=( "$name" )
  fi
done

echo
echo "Results: $pass passed, $fail failed (of $((pass + fail)))"

if [ "$fail" -gt 0 ]; then
  echo "Failed: ${failed_examples[*]}" >&2
  exit 1
fi
