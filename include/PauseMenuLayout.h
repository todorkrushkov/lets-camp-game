#pragma once
#include <algorithm>
enum class MenuAction { None = -1, Mode = 0, Perspective = 1, Resume = 2 };
struct PauseMenuLayout {
    float scale, x, y, width, height;
    PauseMenuLayout(int w, int h) {
        scale = std::min(1.0f, std::min(w / 520.0f, h / 400.0f));
        width = 460 * scale;
        height = 336 * scale;
        x = (w - width) * 0.5f;
        y = (h - height) * 0.5f;
    }
    float rowY(int row) const {
        return y + (106 + row * 62) * scale;
    }
    MenuAction hit(double px, double py) const {
        if (px < x + 30 * scale || px > x + width - 30 * scale)
            return MenuAction::None;
        for (int i = 0; i < 3; ++i)
            if (py >= rowY(i) && py <= rowY(i) + 48 * scale)
                return static_cast<MenuAction>(i);
        return MenuAction::None;
    }
};
