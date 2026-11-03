#pragma once
#include "PlayerDimensions.h"
#include "Shader.h"
#include "SpriteRenderer.h"
#include "Texture.h"
#include <cmath>
class InteriorRoom {
    Shader shader;
    Shader handShader;
    Texture lighter;
    SpriteRenderer handRenderer;
    float animationTime = 0.0f;
    GLuint vao = 0, vbo = 0;
    GLsizei roomCount = 0;

  public:
    InteriorRoom();
    ~InteriorRoom();
    InteriorRoom(const InteriorRoom&) = delete;
    InteriorRoom& operator=(const InteriorRoom&) = delete;
    void update(float deltaTime);
    void draw(const glm::mat4& view, const glm::mat4& projection);
    static glm::vec3 start() {
        return {0, PlayerDimensions::CenterAboveFeet + .02f, 2};
    }
    static glm::vec3 move(const glm::vec3& from, const glm::vec3& to);
    static bool atExit(const glm::vec3& p) {
        return p.z > 4.1f && std::abs(p.x) < .8f;
    }
};
