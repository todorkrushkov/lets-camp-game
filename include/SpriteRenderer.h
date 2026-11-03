
#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "Shader.h"
#include "Texture.h"

class SpriteRenderer {
  public:
    SpriteRenderer();
    ~SpriteRenderer();

    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    void draw(const Shader& shader, const Texture& texture, const glm::vec3& position,
              const glm::mat4& view, const glm::mat4& projection, int column, int row,
              int frameWidth, int frameHeight, float worldWidth, float worldHeight) const;

  private:
    GLuint vao = 0;
    GLuint vbo = 0;
};
