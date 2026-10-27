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

Open http://localhost:8080. `scripts/build.sh` builds the site and runs real WebAssembly smoke tests. GitHub Actions also runs seven shared C test suites and deploys `dist/` to GitHub Pages on pushes to `main`. Pull requests build and test without publishing.

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
