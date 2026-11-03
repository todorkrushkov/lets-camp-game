
#pragma once

#include <algorithm>
#include <cmath>

enum class Direction { Up, Down, Left, Right };

struct Animation {
    int row = 0;
    int startFrame = 0;
    int frameCount = 1;
    float frameDuration = 0.12f;
};

class Animator {
  public:
    void play(const Animation& animation) {
        if (current.row != animation.row || current.startFrame != animation.startFrame ||
            current.frameCount != animation.frameCount) {
            current = animation;
            currentFrame = 0;
            elapsed = 0.0f;
        }
    }

    void update(float deltaTime) {
        if (current.frameCount <= 1 || current.frameDuration <= 0.0f) {
            return;
        }

        elapsed += deltaTime;

        while (elapsed >= current.frameDuration) {
            elapsed -= current.frameDuration;
            currentFrame = (currentFrame + 1) % current.frameCount;
        }
    }

    int getColumn() const {
        return current.startFrame + currentFrame;
    }

    int getRow() const {
        return current.row;
    }

  private:
    Animation current;
    int currentFrame = 0;
    float elapsed = 0.0f;
};

namespace ToshkoAnimations {
inline Animation idle(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return {0, 0, 1, 0.12f};
    case Direction::Left:
        return {1, 0, 1, 0.12f};
    case Direction::Down:
        return {2, 0, 1, 0.12f};
    case Direction::Right:
        return {3, 0, 1, 0.12f};
    }

    return {2, 0, 1, 0.12f};
}

inline Animation walk(Direction direction) {
    switch (direction) {
    case Direction::Up:
        return {4, 0, 8, 0.12f};
    case Direction::Left:
        return {5, 0, 8, 0.12f};
    case Direction::Down:
        return {6, 0, 8, 0.12f};
    case Direction::Right:
        return {7, 0, 8, 0.12f};
    }

    return {6, 0, 8, 0.12f};
}
} // namespace ToshkoAnimations
