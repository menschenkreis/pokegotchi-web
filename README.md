# PokeGotchi

A small life in your hands. Public project website and playable browser workshop for an original virtual companion made for a round ESP32-S3 handheld.

**[Play online](https://menschenkreis.github.io/pokegotchi-web/)** · [Report an issue](https://github.com/menschenkreis/pokegotchi-web/issues)

The online emulator compiles the game's shared C11 core and 466 × 466 RGB565 renderer to WebAssembly. It includes mystery-egg care, three original species, day/night Lantern Grove, memories, procedural chip voices and six mini-games: Tic-tac-toe, Star Tilt, Rhythm Garden, Little Echoes, Leaf Balance and Bubble Bath.

The trained offline companion brain is still in development and is **not included** in this browser build. This is a gameplay preview, not a hardware release or completed AI demonstration.

## Run locally

Install and activate [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) 4.0.18 and Node.js 22, then:

```sh
bash scripts/build.sh
python3 -m http.server 8080 --directory dist
```

Open http://localhost:8080. `scripts/build.sh` builds the site and runs real WebAssembly smoke tests. GitHub Actions also runs nine shared C test suites and deploys `dist/` to GitHub Pages on pushes to `main`. Pull requests build and test without publishing.

## Controls

- Tap, hold and drag for navigation, egg warmth, petting and washing.
- Arrow keys or the on-page tilt pad simulate handheld tilt.
- `1`, `2`, `3`: Rhythm Garden. `S`: Little Echoes shake. `C`: clap.
- `Esc`: back. `M`: sound. `Space`: PWR standby/wake. Focus the game first.
- The lamp controls visible day/night; PWR controls standby.
- Workshop shortcuts can hatch your browser egg and open each game.

Saves stay in this browser's local storage; they are separate from desktop and hardware saves. Hidden tabs pause time. Startup is muted. Clearing browser data clears the companion. The site has no account, analytics or backend; GitHub hosts it and Google Fonts serves the fonts.

## Source and scope

`core/` is a snapshot of the shared game code; `web/bridge.c` exposes a small browser API; `site/` contains the public landing page and Web Audio/Canvas adapter. The two landing-page images are actual shared-renderer captures, not concept art. Source artwork and creatures are original. See [THIRD_PARTY.md](THIRD_PARTY.md) for retained upstream notices.

The private development studio, device credentials, recovery images, life saves, training corpus and model weights are not part of this repository. No serial access is needed to build or play.

## 27 October 2026 evening update

Current shared-core snapshot includes the picture-led tutorial and parachute arrival,
seven-minute incubation, post-hatch naming and profile rename, fixed-point care
and recovery, deliberate memorial reset, larger lantern, swipe menus and saved
night poses. Browser records atomically store a checked life/scene checkpoint and
settings in one localStorage value; previous life-only records migrate without
restarting the tutorial. This is browser persistence, not the hardware NVS protocol.
Unfinished lessons resume and interrupted parachute descents replay. Hidden-tab
time stays paused. Startup stays muted. The progress journal uses labeled actual
simulator captures, with synthetic care states.

The existing three creatures remain; the full art/UI redesign is in design review.
The release brain, hardware performance acceptance and 24-hour offline soak remain
open. No physical device access is needed for this update.

## Forest and Classic editions

The workshop defaults to **Forest**, the 27 October forest-world renderer snapshot.
Visitors can switch above the game or use these direct links:

- [Forest](https://menschenkreis.github.io/pokegotchi-web/?version=forest#play):
  illustrated woodland, redesigned leaf/mushroom/bird creatures, frame animation,
  wooden menus and woodland mini-game art. Animation refinement remains ongoing.
- [Classic](https://menschenkreis.github.io/pokegotchi-web/?version=classic#play):
  the previous published game from commit `8b8ff3c`, preserved in
  `versions/classic/`.

Each edition keeps separate browser progress. Classic uses the existing
`pokegotchi-browser-v1` record, so earlier visitors retain their companion there.
Forest starts its own life under `pokegotchi-browser-forest-v1`. Switching saves
the current session and loads the other edition; restarting affects only the
selected edition. Both versions start muted and pause while hidden.

`scripts/build.sh` compiles and verifies **both** WebAssembly modules. Tests also
check that their rendered frames differ and their save keys are independent.
The Forest input hashes are recorded in `versions/forest-snapshot.json`; future
Forest refreshes must leave the frozen Classic source unchanged. The generated
Forest asset header is included; source reference sheets/converter remain in the
full desktop project. Screenshots on the site are actual simulator captures,
separate from generated source art. No release brain or physical sensor support
has been added.
