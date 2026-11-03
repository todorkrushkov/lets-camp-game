#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "Shader.h"
#include "TerrainMap.h"
#include "Texture.h"

class TerrainRenderer {
  public:
    TerrainRenderer();
    ~TerrainRenderer();

    TerrainRenderer(const TerrainRenderer&) = delete;
    TerrainRenderer& operator=(const TerrainRenderer&) = delete;

    void build(const TerrainMap& map, const Texture& texture);

    void draw(const TerrainMap& map, const Shader& shader, const Texture& texture,
              const glm::mat4& view, const glm::mat4& projection) const;

  private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertexCount = 0;
};
