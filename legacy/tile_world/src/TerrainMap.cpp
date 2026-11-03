#include "TerrainMap.h"
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
TerrainMap TerrainMap::load(const std::string& path, float size) {
    if (!std::isfinite(size) || size <= 0)
        throw std::runtime_error("Invalid terrain cell size");
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open map: " + path);
    TerrainMap m;
    m.cellSize = size;
    std::string line;
    int row = 0;
    std::set<std::string> headers;
    std::set<int> ids;
    auto fail = [&]() { throw std::runtime_error("Malformed terrain map: " + path); };
    while (std::getline(file, line)) {
        if (line.find_first_not_of(" \t\r") == std::string::npos)
            continue;
        if (line.find(',') != std::string::npos) {
            if (m.layers.empty() || row >= m.rows)
                fail();
            auto end = line.find_last_not_of(" \t\r");
            if (line[end] == ',')
                line.erase(end, 1);
            std::istringstream csv(line);
            std::string token;
            int count = 0;
            while (std::getline(csv, token, ',')) {
                std::istringstream cell(token);
                int id;
                std::string extra;
                if (!(cell >> id) || (cell >> extra) || id < -1 || id >= 90)
                    fail();
                m.layers.back().tiles.push_back(id);
                ++count;
            }
            if (count != m.columns)
                fail();
            ++row;
        } else {
            std::istringstream in(line);
            std::string key, extra;
            int v;
            if (!(in >> key >> v) || (in >> extra))
                fail();
            if (key == "layer") {
                if (headers.size() != 4 || (!m.layers.empty() && row != m.rows) ||
                    !ids.insert(v).second)
                    fail();
                m.layers.push_back({v, {}});
                row = 0;
            } else {
                if (!m.layers.empty() || v <= 0 || v > 10000 || !headers.insert(key).second)
                    fail();
                if (key == "tileswide")
                    m.columns = v;
                else if (key == "tileshigh")
                    m.rows = v;
                else if (key == "tilewidth")
                    m.tileWidth = v;
                else if (key == "tileheight")
                    m.tileHeight = v;
                else
                    fail();
            }
        }
    }
    if (m.layers.empty() || row != m.rows)
        fail();
    m.width = m.columns * size;
    m.depth = m.rows * size;
    return m;
}
