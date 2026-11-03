
#pragma once

#include <glm/glm.hpp>

class Player {
  private:
    glm::vec3 position{0.0f, 0.5f, 0.0f};
    float speed = 4.0f;

  public:
    void move(const glm::vec3& direction, float deltaTime);

    const glm::vec3& getPosition() const;
    void setPosition(const glm::vec3& value) {
        position = value;
    }
};
