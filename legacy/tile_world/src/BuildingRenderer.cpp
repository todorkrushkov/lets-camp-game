#include "BuildingRenderer.h"
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
BuildingRenderer::BuildingRenderer(const TerrainMap& map, const std::string& path)
    : shader("assets/shaders/building.vert", "assets/shaders/building.frag") {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open building layout: " + path);
    std::map<std::string, std::shared_ptr<Texture>> textures;
    std::string line;
    int number = 0;
    while (std::getline(file, line)) {
        ++number;
        if (line.find_first_not_of(" \t\r") == std::string::npos ||
            line[line.find_first_not_of(" \t")] == '#')
            continue;
        std::istringstream in(line);
        std::string name, extra;
        float column, row, scale, depth;
        if (!(in >> name >> column >> row >> scale >> depth) || (in >> extra) ||
            name.find('/') != std::string::npos || name.find('\\') != std::string::npos ||
            !std::isfinite(column) || !std::isfinite(row) || !std::isfinite(scale) ||
            !std::isfinite(depth) || scale <= 0 || scale > 10 || depth <= 0 || column < 0 ||
            column >= map.columns || row < 0 || row >= map.rows)
            throw std::runtime_error("Invalid building layout line " + std::to_string(number));
        auto& texture = textures[name];
        if (!texture)
            texture = std::make_shared<Texture>("assets/textures/" + name);
        float width = float(texture->getWidth()) / map.tileWidth * map.cellSize * scale;
        float height = float(texture->getHeight()) / map.tileHeight * map.cellSize * scale;
        // Front edge at the path. Footprint extends north, behind the facade.
        glm::vec3 position((column + 0.5f - map.columns * 0.5f) * map.cellSize, 0.01f,
                           (row + 0.5f - map.rows * 0.5f) * map.cellSize);
        if (position.x - width * 0.5f < -map.width * 0.5f ||
            position.x + width * 0.5f > map.width * 0.5f ||
            position.z - depth * map.cellSize < -map.depth * 0.5f)
            throw std::runtime_error("Building footprint outside map on line " +
                                     std::to_string(number));
        buildings.push_back(
            {texture, position, width, height, width * 0.42f, depth * map.cellSize});
    }
}
bool BuildingRenderer::blocked(const glm::vec3& p) const {
    constexpr float radius = 0.22f;
    for (const auto& b : buildings) {
        float x = glm::clamp(p.x, b.position.x - b.halfFootprintWidth,
                             b.position.x + b.halfFootprintWidth);
        float z = glm::clamp(p.z, b.position.z - b.depth, b.position.z);
        float dx = p.x - x, dz = p.z - z;
        if (dx * dx + dz * dz < radius * radius)
            return true;
    }
    return false;
}
glm::vec3 BuildingRenderer::resolveMovement(const glm::vec3& from, const glm::vec3& to) const {
    // Player step is capped at 0.36 units; footprints are several tiles deep.
    glm::vec3 result = from;
    glm::vec3 x = from;
    x.x = to.x;
    if (!blocked(x))
        result = x;
    glm::vec3 z = result;
    z.z = to.z;
    if (!blocked(z))
        result = z;
    result.y = to.y;
    return result;
}
void BuildingRenderer::draw(const glm::mat4& view, const glm::mat4& projection) {
    GLboolean blend = glIsEnabled(GL_BLEND), cull = glIsEnabled(GL_CULL_FACE);
    GLboolean mask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    // Alpha cutout plus depth writes makes draw order independent for opaque pixels.
    for (const auto& b : buildings)
        renderer.draw(shader, *b.texture, b.position, view, projection, 0, 0, b.texture->getWidth(),
                      b.texture->getHeight(), b.width, b.height);
    if (blend)
        glEnable(GL_BLEND);
    if (cull)
        glEnable(GL_CULL_FACE);
    glDepthMask(mask);
}
