#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p dist
cp -R site/. dist/
emcc web/bridge.c core/*.c third_party/pocket_tank/{q4_model,word_tok}.c -Icore -Ithird_party/pocket_tank -O2 -lm --no-entry \
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS='["_web_init","_web_reset","_web_tick","_web_frame","_web_pointer","_web_tilt","_web_key","_web_hatch","_web_open","_web_save_buffer","_web_save","_web_load","_web_state","_web_sound","_web_pcm","_web_audio_done","_web_audio_ready"]' \
  -sEXPORTED_RUNTIME_METHODS='["HEAPU8","HEAPU16","HEAP16"]' -o dist/game.mjs
node tests/wasm.mjs
