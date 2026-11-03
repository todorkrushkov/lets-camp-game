#pragma once
#include "Shader.h"
#include "SpriteRenderer.h"
#include "TerrainMap.h"
#include "Texture.h"
#include <memory>
#include <string>
#include <vector>
class BuildingRenderer {
    struct Building {
        std::shared_ptr<Texture> texture;
        glm::vec3 position;
        float width, height, halfFootprintWidth, depth;
    };
    std::vector<Building> buildings;
    Shader shader;
    SpriteRenderer renderer;

  public:
    BuildingRenderer(const TerrainMap& map, const std::string& layoutPath);
    void draw(const glm::mat4& view, const glm::mat4& projection);
    bool blocked(const glm::vec3& position) const;
    glm::vec3 resolveMovement(const glm::vec3& from, const glm::vec3& to) const;
};
