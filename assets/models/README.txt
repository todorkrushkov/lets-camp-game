FOREST VILLAGE

Active asset library: game_asset.glb. The source GLB is kept unchanged.
ForestWorld.cpp uses its six named root groups as reusable prototypes.

Layout:
- Smallest house: Medieval housee, one copy at the north side of the clearing.
- Medium house: Medieval house 2, two copies.
- Large house: Medieval house, two copies.
- Houses face the clearing. Only the small house door enters the interior.
- 120 trees: 40 A tree, 40 Monterey Pine Tree, 40 Tree, with seeded placement,
  varied heights and rotation in the surrounding forest ring.
- Flat 128 x 128 ground with a muted olive grass texture and 6,000 nonblocking
  grass tufts. Door approaches and the spawn area are kept clear.
- Toshko starts in the clearing at (0, 0.52, 5), facing toward the small house.

Camera and interior:
- Starts outside in free 3D third person. 2D code and menu mode are retained.
- Approaching the small house door enters the dark first-person room.
- Animated right-hand lighter and warm local lighting remain active inside.
- The room exit restores the outdoor position and camera; mode changes are
  locked indoors. Other house doors do not trigger the room.

Collision/rendering:
- House collision uses solid mesh triangles. Tree trunks use invisible capsule-
  compatible cylinder colliders; foliage and the source tree-ground patches
  do not obstruct walking. Grass supplies the player ground height.
- Original tree geometry is reduced with shape-bounded meshoptimizer edge
  collapses for wood and whole-component sampling for leaves. Original leaf
  positions, UVs, materials and native pine geometry are retained. No generated
  support branches or replacement needle canopy is used. The RGB broad-leaf
  scan gets a border-background alpha mask during texture upload.
- Tree detail changes at 18 and 36 units. Trees beyond 65 units and offscreen
  meshes are culled. Collision is independent of visual detail.
- 3D grass shrinks out between 18 and 28 units and distant chunks are skipped.
- GPU textures are capped at 1024 pixels on their longest side and shared when
  image/sampler settings match. Shader uniform locations are cached.
- Lighting remains simplified directional shading, not a full PBR pipeline.


CPU checks:
cmake -S . -B build -DLETSCAMP_BUILD_CHECKS=ON
cmake --build build
ctest --test-dir build --output-on-failure

Architecture: docs/architecture-bg.md and docs/architecture-bg.pdf.
