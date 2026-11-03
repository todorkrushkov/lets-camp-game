#include "Game.h"
#include "GameConfig.h"
#include <cmath>
#include <glad/gl.h>

namespace {
glm::vec3 getMovementDirection(GLFWwindow* window, const Camera& camera) {
    glm::vec3 direction(0.0f);

    glm::vec3 forward = camera.getForward();
    glm::vec3 right = camera.getRight();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        direction += forward;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        direction -= forward;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        direction -= right;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        direction += right;
    }

    direction.y = 0.0f;

    if (glm::dot(direction, direction) > 0.0f) {
        direction = glm::normalize(direction);
    }

    return direction;
}
} // namespace

Game* Game::fromWindow(GLFWwindow* window) {
    return static_cast<Game*>(glfwGetWindowUserPointer(window));
}

void Game::setPaused(bool value) {
    if (paused == value) {
        return;
    }

    paused = value;

    if (paused) {
        moving = false;

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        mouse.firstMovement = true;
    }
}

void Game::framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

void Game::cursorPositionCallback(GLFWwindow* window, double x, double y) {
    Game* game = fromWindow(window);

    if (!game || game->paused) {
        return;
    }

    MouseState& mouse = game->mouse;

    if (mouse.firstMovement) {
        mouse.lastX = x;
        mouse.lastY = y;
        mouse.firstMovement = false;
        return;
    }

    float deltaX = static_cast<float>(x - mouse.lastX);
    float deltaY = static_cast<float>(y - mouse.lastY);

    mouse.lastX = x;
    mouse.lastY = y;

    bool zooming = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;

    if (zooming) {
        game->camera.zoom(-deltaY * GameConfig::ZOOM_SENSITIVITY);
    } else {
        game->camera.rotate(deltaX, deltaY);
    }
}

void Game::scrollCallback(GLFWwindow* window, double /*xOffset*/, double yOffset) {
    Game* game = fromWindow(window);
    if (game && !game->paused) {
        game->camera.zoom(static_cast<float>(yOffset) * 0.5f);
    }
}

void Game::mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    Game* game = fromWindow(window);

    if (!game || !game->paused || !game->pauseMenu) {
        return;
    }

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
        return;
    }

    double x = 0.0;
    double y = 0.0;

    glfwGetCursorPos(window, &x, &y);

    int width = 0;
    int height = 0;

    // GLFW window coordinates match cursor coordinates.
    glfwGetWindowSize(window, &width, &height);

    switch (game->pauseMenu->actionAt(x, y, width, height)) {
    case MenuAction::Mode:
        if (game->indoors)
            break;
        game->camera.setMode(game->camera.getMode() == CameraMode::TwoD ? CameraMode::ThreeD
                                                                        : CameraMode::TwoD);
        game->mouse.firstMovement = true;
        break;
    case MenuAction::Perspective:
        if (!game->indoors && game->camera.getMode() == CameraMode::ThreeD) {
            game->camera.setPerspective(game->camera.getPerspective() == Perspective::ThirdPerson
                                            ? Perspective::FirstPerson
                                            : Perspective::ThirdPerson);
            game->mouse.firstMovement = true;
        }
        break;
    case MenuAction::Resume:
        game->setPaused(false);
        break;
    default:
        break;
    }
}

void Game::registerCallbacks() {
    glfwSetWindowUserPointer(window, this);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwSetCursorPosCallback(window, cursorPositionCallback);

    glfwSetScrollCallback(window, scrollCallback);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    mouse.firstMovement = true;
}

void Game::updatePause() {
    bool pressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;

    if (pressed && !pauseKeyWasPressed) {
        setPaused(!paused);
    }

    pauseKeyWasPressed = pressed;
}

void Game::processInput() {

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    updatePause();

    if (paused) {
        return;
    }

    glm::vec3 movement = getMovementDirection(window, camera);

    moving = glm::dot(movement, movement) > 0.0f;

    float horizontal = 0.0f;
    float vertical = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        vertical += 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        vertical -= 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        horizontal += 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        horizontal -= 1.0f;
    }

    if (horizontal != 0.0f || vertical != 0.0f) {
        if (std::abs(horizontal) > std::abs(vertical)) {
            facing = horizontal > 0.0f ? Direction::Right : Direction::Left;
        } else {
            facing = vertical > 0.0f ? Direction::Up : Direction::Down;
        }
    }

    bool sprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                     glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    float speedMultiplier = sprinting ? GameConfig::SPRINT_MULTIPLIER : 1.0f;

    movePlayer(movement, deltaTime * speedMultiplier);
}
