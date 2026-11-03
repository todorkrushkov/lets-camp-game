#pragma once
#define GLFW_INCLUDE_NONE
#include "Animation.h"
#include "Camera.h"
#include "InteriorRoom.h"
#include "ModelRenderer.h"
#include "PauseMenu.h"
#include "Player.h"
#include "Shader.h"
#include "SpriteRenderer.h"
#include "Texture.h"
#include <GLFW/glfw3.h>
#include <memory>

struct MouseState {
    bool firstMovement = true;
    double lastX = 0.0;
    double lastY = 0.0;
};

// Owns one window/session. All GPU resources are released before its context.
class Game {
  public:
    int run();

  private:
    GLFWwindow* window = nullptr;

    Camera camera;
    Player player;
    MouseState mouse;

    Direction facing = Direction::Down;
    bool moving = false;
    Animator animator;

    std::unique_ptr<Shader> spriteShader;
    std::unique_ptr<Texture> playerTexture;
    std::unique_ptr<SpriteRenderer> spriteRenderer;

    std::unique_ptr<ModelRenderer> model;
    std::unique_ptr<InteriorRoom> room;
    bool indoors = false, doorArmed = true;
    Camera outdoorCamera;
    glm::vec3 outdoorPosition{0};

    std::unique_ptr<PauseMenu> pauseMenu;

    bool paused = false;
    bool pauseKeyWasPressed = false;

    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
    void initialize();
    void loop();
    void release();
    void update();
    void updateDeltaTime();
    void updateAnimation();
    void processInput();
    void movePlayer(const glm::vec3& movement, float movementDelta);
    void enterInterior();
    void leaveInterior();
    void updatePause();
    void setPaused(bool value);
    void registerCallbacks();
    void render();
    void renderPlayer(const glm::mat4& view, const glm::mat4& projection);
    static Game* fromWindow(GLFWwindow* window);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void cursorPositionCallback(GLFWwindow* window, double x, double y);
    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};
