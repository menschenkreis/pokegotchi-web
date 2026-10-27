> Public browser snapshot: `core/forest_assets.h` is included. The reference PNGs and asset converter described below belong to the full desktop project and are not included in this repository. The following records their provenance.

# Forest art assets

This emulator-first visual pass uses generated reference art converted into bounded C arrays for the existing shared renderer. The background source is Higgsfield job `ca4510c2-61d2-4e32-97bc-79085ecfce8c`; the three creature sources are jobs `3e8de121-a388-4658-bebc-ab49608aa068`, `c405581d-ef2a-41ed-b72b-2dc8bb4b693f` and `0779ab0e-850a-4f20-bc3d-e1e2e58a2990`. Source PNGs under `reference/` are production inputs, not simulator captures.

The animation-sheet sources are Higgsfield jobs `abc48c38-1850-43e0-860a-1a1eb4c87b84`, `64988e3e-64e3-4466-aad8-716d926a6b97` and `133ad0f2-2806-46d6-ad0b-fa318c99c8e8`.

The additional idle sheets are jobs `1a65f9a0-d2db-4af8-8ca8-255d8c993c66`, `59123c7d-df13-47a7-ae12-af35bf93d25c`, and `f03338a9-ccb0-4f74-990e-85cbb2d15a0a`; care sheets are `8aaa1e90-27d1-4263-a184-45e2b9332274`, `99349619-3018-403f-8468-84f4159433b6`, and `874215b0-6568-41ac-87ca-3ac8d4ddb3ed`. Decorative isolated sheet artifacts are discarded during conversion.

Run `python3 tools/build_forest_assets.py` to rebuild `core/forest_assets.h`. The generated 160×160 day, night and dark UI backgrounds plus 24 72×72 transparent-key frames for each of three creatures plus eight 80×112 egg and eight 112×160 parachute frames occupy 1,330,176 bytes of RGB565 constants. They use no runtime decoder or heap allocation. Frames are selected for idle, walking, blinking, hunger, tiredness, dirt, happiness, curiosity, eating, cleaning, fright and sleep, with existing small particle/food overlays. Creature frames are sampled at a fixed scale, never squashed, rotated or sheared. Compact portraits use fixed nearest-neighbour resampling. Existing saved species IDs map to the new leaf-tailed, mushroom and bird appearances without changing saved IDs.

The egg and parachute sheets are Higgsfield jobs `cce5c647-ca20-4c2d-9e81-037e6c55bd19` and `dfedd125-d605-4415-a893-a8594bf1c1a7`. Egg idle, warmth, cracks, hatching and sleep, and the fluttering delivery all use fixed RGB565 frames. The warmth glow can be cropped at the canvas edge; no frame is stretched at runtime. Menu plaques, animated pixel leaf ornaments, and the small two-frame acorn, star, note, leaf and bubble mini-game sprites are code-native; they add no image decoding or heap allocations.

The previous three creatures and original renderer are represented by historical simulator screenshots in `evidence/screens/` and the dated project record. New actual-renderer screenshots are in `evidence/forest-overhaul/`; distinguish these from `project-hub/public/media/art-overhaul/` concept illustrations. Visual acceptance, actual device frame rate and memory measurements remain open. No physical device was accessed for this work.
