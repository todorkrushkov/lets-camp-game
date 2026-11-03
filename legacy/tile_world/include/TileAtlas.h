#pragma once
#include "Texture.h"
#include <cmath>
#include <glm/glm.hpp>
#include <stdexcept>
// The source atlas is an enlarged 10 x 9 grid, not 16px source cells.
class TileAtlas {
  public:
    static constexpr int Columns = 10, Rows = 9, Count = 90;
    static glm::vec4 uv(int id, const Texture& texture) {
        int w = texture.getWidth(), h = texture.getHeight();
        bool original = (w == 2048 && (h == 1840 || h == 1843));
        bool integral =
            (w > 0 && h > 0 && w % Columns == 0 && h % Rows == 0 && w / Columns == h / Rows);
        if (id < 0 || id >= Count || (!original && !integral))
            throw std::runtime_error("Invalid tile ID or terrain atlas dimensions");
        int c = id % Columns, r = id / Columns;
        float effectiveWidth = float(w), effectiveHeight = original ? 1840.0f : float(h);
        // Denominator is the uploaded size: the final three rows are never sampled.
        return {(std::round(c * effectiveWidth / Columns) + 0.5f) / texture.getWidth(),
                (std::round(r * effectiveHeight / Rows) + 0.5f) / texture.getHeight(),
                (std::round((c + 1) * effectiveWidth / Columns) - 0.5f) / texture.getWidth(),
                (std::round((r + 1) * effectiveHeight / Rows) - 0.5f) / texture.getHeight()};
    }
};
