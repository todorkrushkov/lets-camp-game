# LetsCamp - Project Architecture

**Documentation date:** November 3, 2026  
**Author:** Тодор Крушков (Todor Krushkov)  
**Year of study:** 4th year  
**Subject:** Computer Graphics  
**Faculty number:** 121223145

This Markdown document is in English. The accompanying architecture PDF remains in Bulgarian. The existing Markdown filename is retained so that project references continue to work.

## LetsCamp Architecture

LetsCamp is a desktop game written in C++17 using OpenGL 4.1 Core. Its active scene is a forest village with five houses, three tree types, a grass surface, and the character Toshko. The outdoor world supports a 3D camera and the retained orthographic mode. The smallest house leads to a separate dark room viewed in first person.

This document describes the actual implementation after the code cleanup. It covers module responsibilities, data structures, the game loop, rendering passes, collision handling, optimizations, and extension points.

### Contents

The page numbers refer to the corresponding sections of the Bulgarian PDF.

| Section | PDF page |
| --- | --- |
| [Layers and Dependencies](#layers-and-dependencies) | 2 |
| [Project Structure](#project-structure) | 3 |
| [Game Loop and Resource Ownership](#game-loop-and-resource-ownership) | 4 |
| [Building the Forest Village](#building-the-forest-village) | 5 |
| [GLB Loading and Data Structures](#glb-loading-and-data-structures) | 6 |
| [Outdoor Rendering](#outdoor-rendering) | 7 |
| [Input, Cameras, and Character](#input-cameras-and-character) | 8 |
| [Ground, Collisions, and Entrances](#ground-collisions-and-entrances) | 9 |
| [Scene Transitions and Interior](#scene-transitions-and-interior) | 10 |
| [Optimization](#optimization) | 11 |
| [Building and Validation](#building-and-validation) | 12 |
| [Active Module Reference](#active-module-reference) | 13 |
| [Preserved Code and Future Development](#preserved-code-and-future-development) | 14 |

**Project location:** `~/Desktop/Projects/University/ComputerGraphics/LetsCamp`  
**Main asset:** `assets/models/game_asset.glb`

The original GLB remains unchanged. Scene preparation takes place in memory.

## Layers and Dependencies

*01 / From input data to the rendered image*

Data flow: `Game` coordinates the CPU core and graphics layer; CPU scene data is converted into GPU resources.

### The game session coordinates the subsystems

`Game` owns the window, camera, player, animator, and graphics objects. `main.cpp` only creates a `Game` and calls `run()`. GLFW callbacks retrieve the appropriate session through the window's user pointer.

### CPU core without OpenGL

`letscamp_core` contains `ModelAsset`, `ForestWorld`, `GroundSurface`, `DoorTrigger`, `Camera`, and `Player`. These modules handle geometry, transformations, and gameplay mathematics. They do not create a window or call OpenGL, allowing the logic to be tested without a graphics context.

### Graphics layer

`ModelRenderer` converts the prepared scene into GPU buffers and textures and exposes access to ground movement and the entrance trigger. `InteriorRoom` renders the separate room and hand. `SpriteRenderer` draws Toshko, while `PauseMenu` draws the menu. `Shader` and `Texture` own the basic graphics resources.

The project uses a compact modular architecture with direct calls between objects. It does not have an entity-component system, a general event bus, a separate loading thread, or a general-purpose scene manager.

## Project Structure

*02 / Source code and external dependencies*

| Directory | Contents |
| --- | --- |
| `include/` | Interfaces, data structures, `GameConfig`, `PlayerDimensions`, and header-only animation code. |
| `src/` | Active implementations of the game session, world, camera, and rendering. |
| `assets/` | One GLB library, sprite images, and active GLSL shaders. |
| `external/` | Bundled GLAD, cgltf, stb_image, and the required portion of meshoptimizer. |
| `tests/` | `ForestWorldCheck` and `CoreBehaviorCheck`. |
| `legacy/tile_world/` | The previous tile terrain, 2D buildings, and associated shaders. |
| `docs/` | The Bulgarian architecture PDF and this English Markdown documentation. |
| `build/` | Generated executables, libraries, copied assets, and local backups. This is not source code. |

### CMake targets

`LetsCamp` is the game executable. `letscamp_core` is the static CPU library. `letscamp_assets` synchronizes resources even when only an image or shader changes. `glad` loads OpenGL functions, and `meshoptimizer` simplifies the original tree geometry.

`ForestWorldCheck` and `CoreBehaviorCheck` are created when `LETSCAMP_BUILD_CHECKS=ON`. `letscamp_legacy` is created when `LETSCAMP_BUILD_LEGACY=ON` and checks that the preserved implementation still compiles.

### Dependencies and language standards

The project requires CMake 3.20 or newer, C11/C++17, GLFW, GLM, and OpenGL 4.1. GLFW handles the window and input; GLM provides vectors and matrices. cgltf reads GLB data, while stb_image decodes images. The bundled libraries retain their licenses in their respective directories.

Project-owned C++ code follows a shared `.clang-format` configuration. External libraries are not reformatted. The main project targets enable `-Wall`, `-Wextra`, and `-Wpedantic` when built with Clang or GCC.

## Game Loop and Resource Ownership

*03 / Game.cpp and object lifetimes*

Frame loop: GLFW events -> `update()` -> `render()` -> `glfwSwapBuffers()`.

### Initialization

`Game::run()` initializes GLFW, requests a forward-compatible OpenGL 4.1 Core context, creates a 1280 x 720 window, makes the context current, and loads GLAD. It then enables depth testing and alpha blending, configures VSync, and registers the callbacks.

`Game::initialize()` creates Toshko's shader and texture, `SpriteRenderer`, `ModelRenderer`, `InteriorRoom`, and `PauseMenu`. The player receives a safe initial position from the world. The initial camera is 3D in third person.

### One frame

`loop()` calls `glfwPollEvents()`, `update()`, `render()`, and `glfwSwapBuffers()` in that order. `update()` calculates `deltaTime`, clamps it to the range from 0 to 0.05 seconds, and processes input. While the game is running, it also updates animation, the active environment's time, and the camera target.

Pausing stops simulation updates, but rendering and menu input continue.

### Cleanup and error handling

`Game` owns its graphics objects through `std::unique_ptr`. `release()` destroys them before `glfwDestroyWindow()` and `glfwTerminate()`. `Shader` automatically deletes its program; `Texture` and the renderer classes release their own buffers and textures. Copying these resource-owning objects is disabled.

Constructors can throw exceptions for missing resources, invalid models, or failed shaders. `run()` reports the error and releases already-created objects while the graphics context is still available.

## Building the Forest Village

*04 / ForestWorld::arrange()*

The GLB acts as a prototype library. `ModelAsset` preserves the names of root groups. `ForestWorld` expects exactly three house groups and three tree groups. This is a contract with the current asset, rather than a general arrangement algorithm for arbitrary models.

| Element | Placement and rule |
| --- | --- |
| Small house | `Medieval housee`; one instance at XZ `(0, -11)`; the only active entrance. |
| Medium houses | `Medieval house 2`; two instances at `(-13, 7)` and `(13, 7)`. |
| Large houses | `Medieval house`; two instances at `(-13, -9)` and `(13, -9)`. |
| Trees | `A tree`, `Monterey Pine Tree`, and `Tree`; 40 instances of each type. |
| Forest | A ring with radius 26-54; at least 4 units between trunks; tree heights of 10-13.5 units. |
| Ground | A 128 x 128 square at `Y=0`, using a repeating 128 x 128-pixel texture. |
| Spawn | Preferred player-center position: `(0, 0.52, 5)`. |

### Transformations and reuse

Houses are ranked by bounding-box volume, scaled by 0.9, and rotated toward the clearing. Each placed object receives a `ModelInstance` with its own transformation. Copies share the same mesh data. Tree geometry is simplified once per prototype, rather than separately for all 120 trees.

### Deterministic generation

Placement and variation use the fixed seed `20261102`. Grass tufts use seed `6102`, while the ground texture uses seed `81`. The world contains 6,000 tufts divided into 64 spatial chunks. The entrance path, spawn area, and areas around houses remain clear.

Visible tree meshes do not directly participate in collision detection. Instead, 120 invisible trunk cylinders are added. Ground patches from the source tree models are excluded from the new scene. Finally, unused meshes are removed from the prepared `ModelAsset`.

## GLB Loading and Data Structures

*05 / ModelAsset and asset requirements*

| Type | Purpose |
| --- | --- |
| `ModelVertex` | Position, normal, UV0, UV1, and vertex color. |
| `ModelMesh` | Vertices, near/medium/far index lists, material index, and visibility. |
| `ModelMaterial` | Base color, emission, textures, UV transforms, transparency settings, and flags. |
| `ModelImage` / `ModelTexture` | Encoded bytes or an RGBA image; an image reference and sampler settings. |
| `ModelInstance` | Shared mesh index, world transform, group, collision/portal flags, and LOD origin. |
| `ModelAsset` | Container for the above data and the scene's overall bounds. |

### Loading sequence

`ModelAsset::load()` uses cgltf to parse the file, load its buffers, and validate them. Images are retained as encoded bytes and decoded later only when needed for rendering. Materials are converted to a simpler internal format. Each triangle primitive becomes a separate `ModelMesh`.

Vertices and indices are validated. Missing normals are calculated from triangles. Traversing the scene hierarchy preserves accumulated node transforms and the root group name. Singular transforms, non-finite coordinates, and indices outside the vertex buffer are rejected.

### Supported features and limitations

The loader supports static triangle models, embedded images, material alpha modes, double-sided and unlit materials, emission, and `KHR_texture_transform` on UV0. UV1 is read as an additional vertex attribute, but the texture loader rejects materials that require a different UV set.

Skins, GLB animations, morph targets, Draco primitives, and arbitrary required extensions are unsupported. Reading base-color data from a PBR material does not provide full PBR lighting: normal, roughness, and metallic maps are not used by the current renderer.

## Outdoor Rendering

*06 / ModelRenderer and GLSL 4.10*

Outdoor frame order: world -> character and silhouette -> menu when paused.

### GPU preparation

The `ModelRenderer` constructor loads the model, builds the village, and prepares `GroundSurface` and `DoorTrigger`. It creates a VAO, VBO, and EBO for each visible mesh. One EBO stores the three detail levels consecutively. Rendering selects a level by its index count and byte offset.

Textures are shared when their source image and sampler settings match. Only used base-color and emissive maps are decoded. Color textures are uploaded as `GL_SRGB8_ALPHA8` with mipmaps. Their longest dimension is limited to 1,024 pixels while preserving the aspect ratio.

For the RGB leaf scan named `Material.003`, border-connected background pixels close to the corner color are converted into an alpha mask.

### Draw ordering

`draw()` obtains the camera position from the inverse view matrix, selects LOD, skips distant trees and grass chunks, and tests bounding boxes against the six clip planes. Opaque and masked meshes are drawn before `BLEND` instances. Blended instances are sorted back to front using their origins' view-space depth and do not write to the depth buffer.

`model.vert` applies the model, view, and projection matrices, transforms normals using the inverse transpose, and adds wind motion to grass. `model.frag` combines material color, vertex color, and texture; performs alpha cutout; and applies simplified directional lighting. It adds emission and converts the result to display-space color.

Materials control culling, winding, and blending. The renderer restores the GL state it changes after drawing. Transparency sorting operates per instance, rather than per individual leaf or triangle.

## Input, Cameras, and Character

*07 / GameInput, Camera, Player, and Animator*

| Control | Action |
| --- | --- |
| WASD / Shift | Move relative to the camera's horizontal direction; base speed is 4 units per second, with a sprint multiplier of 1.8. |
| Mouse | Fixed X/Y directions. The first mouse event after resuming only establishes the initial cursor position. |
| Mouse wheel / V + vertical mouse movement | Zoom; changes the field of view in first person. |
| P / left mouse button | Pause; select camera mode, perspective, or resume through the menu. |
| Esc | Request window closure. |

### Three camera behaviors

`TwoD` uses an orthographic projection, fixed yaw, pitch between 30 and 40 degrees, and zoom distance between 5 and 18 units.

`ThreeD` / `ThirdPerson` uses a perspective projection with a 55-degree field of view, pitch between 5 and 25 degrees, and orbit distance between 2.5 and 15 units. Its focus is raised 0.6 units above the player center.

`ThreeD` / `FirstPerson` places the camera at eye height, limits pitch to +/-85 degrees, and limits the field of view to 55-80 degrees.

`Player` stores position and normalizes movement direction so diagonal movement is not faster. `GameWorld` then corrects the candidate position against collisions. `PlayerDimensions` provides shared dimensions: width and height of 1.275 units, radius of 0.187, and a center 0.5 units above the feet.

### Sprite rendering and animation

`Animation` describes a row, starting frame, frame count, and frame duration. `Animator` selects idle or walk animation without restarting an unchanged sequence. Toshko uses 64 x 64 cells; walking has 8 frames, each lasting 0.12 seconds. `SpriteRenderer` constructs a billboard from the inverse view rotation.

`GameRender` first draws the character normally using `GL_LESS`. It then draws only occluded pixels as a translucent blue silhouette using `GL_GREATER`, with depth writes disabled. The sprite is hidden in first person.

The menu uses a shared `PauseMenuLayout` for positioning and hit testing, including when the window is small.

## Ground, Collisions, and Entrances

*08 / GroundSurface and DoorTrigger*

### Geometry classification

`GroundSurface::build()` examines only instances with `collision=true`. Materials whose names contain `ground`, `terrain`, or `landscape` can provide walkable ground. Their triangles are walkable when the normal has a sufficient vertical component: `abs(normal.y) / length >= 0.55`. Remaining triangles become obstacles. Foliage materials are skipped. Geometry is transformed into world coordinates.

### Spatial grid

Walkable triangles and obstacles are stored in separate XZ hash grids with 2-unit cells. A key combines two 32-bit coordinates into one 64-bit value. Each triangle is inserted into every cell overlapped by its bounding box. This provides the broad phase; actual contact is determined geometrically.

`height(x, z)` finds triangles in the cell, checks barycentric coordinates, and returns the highest valid Y value. `blocked()` checks a vertical player capsule against obstacles, first using bounding boxes and then capsule-to-triangle distance.

### Safe movement

`move()` splits horizontal movement into substeps of at most 0.08 units. Each substep requires ground, a height change no greater than 0.5, and enough room for the capsule. On contact, it tries X and Z movement separately to allow wall sliding. The feet remain 0.02 units above the ground.

`spawn()` prefers the requested position and searches for a nearby safe location if necessary.

### Room entrance

`DoorTrigger` uses only instances with `portal=true` and material names equal to `door`, beginning with `door.`, or containing `door_`. It calculates a world-space bounding box. `near()` checks foot height and an XZ distance of less than 0.85 units from that box. Only the smallest house receives `portal=true`.

Movement is kinematic and follows the ground surface. There is no gravity, jumping, rigid-body simulation, dynamic obstacle system, or camera collision.

## Scene Transitions and Interior

*09 / GameWorld.cpp and InteriorRoom*

Transition flow: outdoors -> enter when moving near the armed doorway -> indoors -> exit -> restore the outdoor state.

### Entering and returning

`movePlayer()` checks the entrance after resolving movement. When the player is close enough, `doorArmed` is enabled, and movement input is present, `enterInterior()` saves the outdoor position and complete `Camera` state. It selects the interior starting position and a new 3D first-person camera. Indoors, the menu cannot change camera mode or perspective.

Exiting is triggered at `Z > 4.1` and `abs(X) < 0.8`. `leaveInterior()` restores the outdoor position and camera, resets initial mouse tracking, and disables `doorArmed`. Entry is rearmed only after the player leaves the trigger area, preventing immediate re-entry.

### Room and local obstacles

`InteriorRoom` generates a room measuring 8 x 10 units with a height of 3.5, a visible exit door, and two rectangular objects. Movement checks the walls and objects separately, using the player's radius, substeps of at most 0.08 units, and wall sliding.

The interior is a separate scene; it is not the GLB interior of the outdoor house.

### Animated hand and warm light

`holding_lighter_seq.png` contains four horizontal frames played at 10 frames per second. The hand is drawn in screen space in the lower-right area, using an orthographic projection with depth testing disabled. Its height is 42% of the viewport, and its proportions come from the image.

The flame position is specified for each frame, transformed from screen coordinates into camera and world coordinates, and clamped inside the room. `interior.frag` applies a warm point light with distance attenuation, diffuse shading, and subtle flicker. This lighting does not produce actual dynamic shadows.

## Optimization

*10 / Geometry, textures, and visibility*

| XZ distance from camera | Tree detail |
| --- | --- |
| Below 18 | Near |
| 18 to below 36 | Medium |
| 36 to below 65 | Far |
| 65 or more | Hidden; not drawn |

### Reduced detail with original tree geometry

Wood geometry is reduced using meshoptimizer edge collapses with UV attributes. The near level targets 3.5% of the original index count, but stops at the configured relative geometric error of 0.001. Medium and far levels are derived from it using larger error thresholds. The percentage is a target, not a guaranteed final size.

Leaves are selected as complete connected components in a deterministic hash order. The near budget is up to 48,000 vertices per foliage mesh; the medium budget is one third of that, and the far budget is one twelfth. Retained leaves preserve their original positions and UVs. There are no generated replacement branches or procedural replacements for the pine canopy.

### CPU check measurements

The tree prototypes' total vertex count decreases from 4,906,355 to 495,354. Foliage vertices decrease from 1,776,731 to 96,615, a reduction of approximately 94.6%.

These figures describe unique prototype data. They are neither the number of vertices visible in a particular frame nor an FPS measurement.

### Additional limits

Meshes are shared between instances, but there are currently no GPU-instanced draw calls: each visible instance is drawn separately. Frustum culling skips offscreen bounding boxes. Grass shrinks between 18 and 28 units, and distant chunks are skipped. Textures are capped at 1,024 pixels and cached. Model shader uniform locations are also cached.

The original GLB is approximately 316 MiB and is loaded in full before simplification. Startup time and CPU memory remain costs. LOD transitions are discrete and may be noticeable. Further optimization would include offline asset preparation and frame profiling on the target laptop.

## Building and Validation

*11 / Reproducible checks and current limitations*

### Main commands

Run these commands from the project root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLETSCAMP_BUILD_CHECKS=ON
cmake --build build -j1
ctest --test-dir build --output-on-failure
cd build
./LetsCamp
```

CMake synchronizes `assets/` beside the executable. Resource paths are relative, so the working directory must be the project root or `build/`. Add `-DLETSCAMP_BUILD_LEGACY=ON` when configuring to compile-check the preserved implementation.

| Check | What it verifies |
| --- | --- |
| `ForestWorldLayout` | Five houses, 120 trees, colliders, grass, safe spawning, a reachable entrance, original foliage triangles/UVs, and valid LOD indices. |
| `CoreBehavior` | Camera modes and limits, eye height, player movement, animation continuity, and scaled menu hit testing. |
| Legacy compilation | The preserved modules still compile as a separate static library. |
| Formatting | Project-owned C++ code follows `.clang-format`. |

### Recorded validation result

Both CTest checks passed after restructuring. The game and legacy library compiled. No compiler warnings were reported in the active project-owned code with the enabled warning flags.

The linker reported that the installed Homebrew GLFW/GLM libraries target a newer macOS version than the selected deployment target. This reflects the local dependency setup.

The automated tests are CPU checks. They do not establish visual quality, GLSL compilation on a real driver, or rendering performance. A graphics review was not possible during that validation session because desktop access was denied. The game still requires a real visual run to review trees, transparency, lighting, and the menu.

## Active Module Reference

*12 / Where to change specific behavior*

| Module / file | Responsibility |
| --- | --- |
| `main.cpp` | Minimal entry point; creates `Game` and returns the result of `run()`. |
| `Game.h` / `Game.cpp` | Session state and ownership, window, initialization, loop, timing, and cleanup. |
| `GameInput.cpp` | GLFW callbacks, WASD, sprint, zoom, pause, and menu actions. |
| `GameWorld.cpp` | Movement correction and `enterInterior()` / `leaveInterior()` transitions. |
| `GameRender.cpp` | Outdoor/interior rendering selection, frame matrices, character/silhouette, and menu. |
| `GameConfig.h` | Window settings, delta-time limit, sprite dimensions, zoom sensitivity, sprint, and character texture. |
| `PlayerDimensions.h` | Shared sprite, capsule, and eye dimensions. |
| `ModelAsset.h/.cpp` | CPU data structures and validated static GLB loading. |
| `ForestWorld.h/.cpp` | Prototypes, layout, tree simplification, grass, ground, and trunk colliders. |
| `GroundSurface.h/.cpp` | Spatial grids, ground height, capsule collision, spawning, and sliding movement. |
| `DoorTrigger.h/.cpp` | Extracting entrance zones and checking proximity. |
| `ModelRenderer.h/.cpp` | GPU preparation, textures, LOD, culling, materials, and village rendering. |
| `Camera.h/.cpp` | Position and basis vectors, projections, modes, pitch, and zoom. |
| `Player.h/.cpp` | Position and normalized movement at the configured speed. |
| `Animation.h` | `Direction`, `Animation`, `Animator`, and `ToshkoAnimations`. |
| `SpriteRenderer.h/.cpp` | Billboard quad and sprite-sheet frame selection through UVs. |
| `InteriorRoom.h/.cpp` | Room geometry and collisions, hand rendering, and local lighting. |
| `PauseMenu.h/.cpp` | Pause overlay and text rendered as bitmap-glyph geometry. |
| `PauseMenuLayout.h` | Shared scaling and row hit testing. |
| `Shader.h/.cpp` | Loading, compiling, linking, using, and deleting a GL program. |
| `Texture.h/.cpp` | stb_image loading, sprite RGBA textures, nearest filtering, and texture ownership. |

The active shader pairs are `model`, `sprite`, `interior`, `lighter`, and `pause` in `assets/shaders/`. The first four serve the world, character, and interior; `pause` serves only the screen-space menu.

## Preserved Code and Future Development

*13 / Tile world, extension points, and limitations*

### Previous tile-based world

`legacy/tile_world/` contains `TerrainMap`, `TileAtlas`, `TerrainRenderer`, `BuildingRenderer`, and `Ground`. `TerrainMap` reads a validated text format with layers, dimensions, and tile indices from -1 to 89.

`TileAtlas` defines a 10 x 9 grid and an effective height of 1,840 pixels for an original 2048 x 1840 or 2048 x 1843 image. The final three rows of the taller image are never sampled.

`TerrainRenderer` builds layer geometry. `BuildingRenderer` loads house images from a text layout, draws their facades, and checks their footprints. `Ground` is an earlier simple ground mesh. The associated `basic`, `building`, and `terrain` shaders and `buildings.txt` are preserved alongside these modules.

The archived tile world is not part of the active scene. Its original map, atlas, and house images are absent from the active assets. Compiling the library does not make it a ready-to-run scene. The orthographic `Camera` continues to work independently with the active 3D world.

### Adding functionality

To change the world layout, modify `ForestWorld` and update `ForestWorldCheck`. A replacement GLB must preserve the three-house/three-tree group contract and the meaning of material names, or the relevant code must be adapted.

A new material type requires coordinated changes in `ModelMaterial`, `ModelAsset`, `ModelRenderer`, and the model shaders.

Camera behavior changes belong in `Camera` and `CoreBehaviorCheck`. A new menu action must be added consistently to `MenuAction`, `PauseMenuLayout`, `PauseMenu::draw()`, and `GameInput`.

A new interior scene requires a state or transition in `GameWorld`, along with its own rendering and movement rules.

### Future architectural steps

As the project grows, village settings could become external data, resources could be prepared offline, and transitions could be generalized into a scene manager. GPU instancing, profiling, more visual checks, and dedicated collision meshes could reduce costs. These are proposed improvements, not implemented subsystems.

This documentation was checked against `include/`, `src/`, `CMakeLists.txt`, the active GLSL files, `tests/`, and the preserved legacy code. The editable English text remains at `docs/architecture-bg.md` for compatibility with existing project references. The Bulgarian PDF is maintained separately.
