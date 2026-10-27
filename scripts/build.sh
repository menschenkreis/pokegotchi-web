#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p dist
cp -R site/. dist/
for version in forest classic; do
  mkdir -p "dist/versions/$version"
  if [ "$version" = classic ]; then
    source_dir=versions/classic
  else
    source_dir=.
  fi
  emcc "$source_dir/web/bridge.c" "$source_dir"/core/*.c third_party/pocket_tank/{q4_model,word_tok}.c -I"$source_dir/core" -Ithird_party/pocket_tank -O2 -lm --no-entry \
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node -sALLOW_MEMORY_GROWTH=1 \
    -sEXPORTED_FUNCTIONS='["_web_init","_web_reset","_web_tick","_web_frame","_web_pointer","_web_tilt","_web_key","_web_hatch","_web_open","_web_save_buffer","_web_save","_web_load","_web_state","_web_sound","_web_pcm","_web_audio_done","_web_audio_ready"]' \
    -sEXPORTED_RUNTIME_METHODS='["HEAPU8","HEAPU16","HEAP16"]' -o "dist/versions/$version/game.mjs"
  node tests/wasm.mjs "$version"
done
node tests/versions.mjs
