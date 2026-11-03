#pragma once
#include "PlayerDimensions.h"

namespace GameConfig {
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

constexpr float MAX_DELTA_TIME = 0.05f;

constexpr int FRAME_WIDTH = 64;
constexpr int FRAME_HEIGHT = 64;

constexpr float SPRITE_WIDTH = PlayerDimensions::Width;
constexpr float SPRITE_HEIGHT = PlayerDimensions::Height;

constexpr float ZOOM_SENSITIVITY = 0.05f;
constexpr float SPRINT_MULTIPLIER = 1.8f;

constexpr const char* PLAYER_TEXTURE = "assets/textures/Toshko_v1_titles.png";

} // namespace GameConfig
