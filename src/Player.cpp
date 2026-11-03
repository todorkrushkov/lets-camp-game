
#include "Player.h"

#include <glm/geometric.hpp>

void Player::move(const glm::vec3& direction, float deltaTime) {
    if (glm::length(direction) > 0.0f) {
        position += glm::normalize(direction) * speed * deltaTime;
    }
}

const glm::vec3& Player::getPosition() const {
    return position;
}
