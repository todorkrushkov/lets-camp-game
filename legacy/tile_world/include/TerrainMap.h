#pragma once
#include <string>
#include <vector>
struct TerrainLayer {
    int id;
    std::vector<int> tiles;
};
class TerrainMap {
  public:
    int columns = 0, rows = 0, tileWidth = 0, tileHeight = 0;
    float cellSize = 1.0f;
    float width = 0, depth = 0;
    std::vector<TerrainLayer> layers;
    static TerrainMap load(const std::string& path, float cellSize = 1.0f);
};
