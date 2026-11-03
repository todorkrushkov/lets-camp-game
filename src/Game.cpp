#include "Game.h"
#include "GameConfig.h"
#include <algorithm>
#include <glad/gl.h>
#include <iostream>
#include <stdexcept>

namespace {
void initializeGLFW() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
}

GLFWwindow* createWindow() {
    GLFWwindow* window = glfwCreateWindow(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                                          "LetsCamp", nullptr, nullptr);

    if (!window) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);

    return window;
}

void initializeOpenGL(GLFWwindow* window) {
    if (!gladLoadGL(glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSwapInterval(1);

    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);

    std::cout << "OpenGL: " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << '\n';
}
} // namespace

void Game::initialize() {
    spriteShader =
        std::make_unique<Shader>("assets/shaders/sprite.vert", "assets/shaders/sprite.frag");

    playerTexture = std::make_unique<Texture>(GameConfig::PLAYER_TEXTURE);

    spriteRenderer = std::make_unique<SpriteRenderer>();

    animator.play(ToshkoAnimations::idle(facing));

    model = std::make_unique<ModelRenderer>("assets/models/game_asset.glb");
    player.setPosition(model->suggestedStart());
    camera.setMode(CameraMode::ThreeD);
    camera.setPerspective(Perspective::ThirdPerson);
    room = std::make_unique<InteriorRoom>();

    pauseMenu = std::make_unique<PauseMenu>();

    camera.setTarget(player.getPosition());

    lastFrameTime = static_cast<float>(glfwGetTime());
}

void Game::updateDeltaTime() {
    float currentTime = static_cast<float>(glfwGetTime());

    deltaTime = std::clamp(currentTime - lastFrameTime, 0.0f, GameConfig::MAX_DELTA_TIME);

    lastFrameTime = currentTime;
}

void Game::updateAnimation() {
    Animation animation = moving ? ToshkoAnimations::walk(facing) : ToshkoAnimations::idle(facing);

    animator.play(animation);
    animator.update(deltaTime);
}

void Game::update() {
    updateDeltaTime();
    processInput();

    if (paused) {
        return;
    }

    updateAnimation();
    if (indoors && room)
        room->update(deltaTime);
    if (!indoors && model)
        model->update(deltaTime);

    camera.setTarget(player.getPosition());
}

void Game::loop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        update();
        render();

        glfwSwapBuffers(window);
    }
}

void Game::release() {
    room.reset();
    model.reset();
    pauseMenu.reset();

    spriteRenderer.reset();
    playerTexture.reset();
    spriteShader.reset();
}

int Game::run() {

    try {
        initializeGLFW();

        window = createWindow();

        initializeOpenGL(window);
        registerCallbacks();

        initialize();
        loop();

        release();

        glfwSetWindowUserPointer(window, nullptr);

        glfwDestroyWindow(window);
        window = nullptr;
        glfwTerminate();

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "LetsCamp error: " << error.what() << '\n';

        if (window) {
            release();

            glfwSetWindowUserPointer(window, nullptr);

            glfwDestroyWindow(window);
            window = nullptr;
        }

        glfwTerminate();

        return -1;
    }
}
