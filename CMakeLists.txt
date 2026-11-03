cmake_minimum_required(VERSION 3.20)
project(LetsCamp LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LETSCAMP_BUILD_CHECKS "Build CPU gameplay and forest checks" OFF)
option(LETSCAMP_BUILD_LEGACY "Compile-check the preserved tile world" OFF)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

add_library(meshoptimizer STATIC
    external/meshoptimizer/simplifier.cpp
    external/meshoptimizer/allocator.cpp
)
target_include_directories(meshoptimizer PUBLIC external/meshoptimizer)
target_compile_options(meshoptimizer PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-O2>
)

# CPU data, scene generation, camera and gameplay math have no OpenGL dependency.
add_library(letscamp_core STATIC
    src/ModelAsset.cpp
    src/ForestWorld.cpp
    src/GroundSurface.cpp
    src/DoorTrigger.cpp
    src/Camera.cpp
    src/Player.cpp
)
target_include_directories(letscamp_core PUBLIC include PRIVATE external/cgltf)
target_link_libraries(letscamp_core PUBLIC glm::glm PRIVATE meshoptimizer)

add_executable(LetsCamp
    src/main.cpp
    src/Game.cpp
    src/GameInput.cpp
    src/GameWorld.cpp
    src/GameRender.cpp
    src/ModelRenderer.cpp
    src/InteriorRoom.cpp
    src/Shader.cpp
    src/Texture.cpp
    src/PauseMenu.cpp
    src/SpriteRenderer.cpp
)
target_include_directories(LetsCamp PRIVATE external/stb)
target_link_libraries(LetsCamp PRIVATE letscamp_core glad glfw OpenGL::GL)

foreach(target letscamp_core LetsCamp)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall;-Wextra;-Wpedantic>
    )
endforeach()

# Large GLB processing must stay optimized even during a Debug build.
set_source_files_properties(src/ForestWorld.cpp src/ModelAsset.cpp src/ModelRenderer.cpp
    PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-O2>"
)

# Asset-only edits must reach the build directory without relinking the game.
add_custom_target(letscamp_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:LetsCamp>/assets
    COMMENT "Synchronizing game assets"
    VERBATIM
)
add_dependencies(LetsCamp letscamp_assets)

if(LETSCAMP_BUILD_CHECKS)
    enable_testing()
    add_executable(ForestWorldCheck tests/ForestWorldCheck.cpp)
    target_link_libraries(ForestWorldCheck PRIVATE letscamp_core)
    add_test(NAME ForestWorldLayout COMMAND ForestWorldCheck
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/game_asset.glb)
    add_executable(CoreBehaviorCheck tests/CoreBehaviorCheck.cpp)
    target_link_libraries(CoreBehaviorCheck PRIVATE letscamp_core)
    add_test(NAME CoreBehavior COMMAND CoreBehaviorCheck)
endif()

if(LETSCAMP_BUILD_LEGACY)
    add_library(letscamp_legacy STATIC
        legacy/tile_world/src/BuildingRenderer.cpp
        legacy/tile_world/src/Ground.cpp
        legacy/tile_world/src/TerrainMap.cpp
        legacy/tile_world/src/TerrainRenderer.cpp
    )
    target_include_directories(letscamp_legacy PRIVATE legacy/tile_world/include include)
    target_link_libraries(letscamp_legacy PRIVATE glad glm::glm)
endif()
