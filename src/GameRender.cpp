#include "Game.h"
#include "GameConfig.h"
#include <glad/gl.h>

namespace {
void clearScreen() {
    glClearColor(0.12f, 0.17f, 0.22f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
} // namespace

void Game::renderPlayer(const glm::mat4& view, const glm::mat4& projection) {
    glm::vec3 position = player.getPosition();
    position.y -= GroundSurface::PlayerCenterAboveFeet;

    auto drawPlayer = [&]() {
        spriteRenderer->draw(*spriteShader, *playerTexture, position, view, projection,
                             animator.getColumn(), animator.getRow(), GameConfig::FRAME_WIDTH,
                             GameConfig::FRAME_HEIGHT, GameConfig::SPRITE_WIDTH,
                             GameConfig::SPRITE_HEIGHT);
    };

    GLint previousDepthFunction;
    GLboolean previousDepthMask;
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    const GLboolean blending = glIsEnabled(GL_BLEND);
    const GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    spriteShader->use();
    const GLint occludedPass = glGetUniformLocation(spriteShader->ID, "occludedPass");

    // Normal pixels retain full color and normal scene occlusion.
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glUniform1i(occludedPass, GL_FALSE);
    drawPlayer();

    // Only pixels hidden behind scene depth get a translucent silhouette.
    // Do not write depth: buildings and subsequent UI keep their existing depth.
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_FALSE);
    glUniform1i(occludedPass, GL_TRUE);
    drawPlayer();

    glUniform1i(occludedPass, GL_FALSE);
    glDepthFunc(previousDepthFunction);
    glDepthMask(previousDepthMask);
    if (!blending)
        glDisable(GL_BLEND);
    if (!depthTest)
        glDisable(GL_DEPTH_TEST);
}

void Game::render() {
    int framebufferWidth = 0;
    int framebufferHeight = 0;

    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return;
    }

    glViewport(0, 0, framebufferWidth, framebufferHeight);

    clearScreen();

    glm::mat4 projection = camera.getProjectionMatrix(static_cast<float>(framebufferWidth) /
                                                      static_cast<float>(framebufferHeight));

    glm::mat4 view = camera.getViewMatrix();

    if (indoors) {
        glClearColor(.002f, .002f, .003f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        room->draw(view, projection);
    } else {
        if (model)
            model->draw(view, projection);
        if (!camera.isFirstPerson())
            renderPlayer(view, projection);
    }

    if (paused && pauseMenu) {
        int windowWidth = 0;
        int windowHeight = 0;

        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        if (windowWidth > 0 && windowHeight > 0) {
            // OpenGL renders at framebuffer resolution.
            // Menu positions use GLFW window coordinates.
            pauseMenu->draw(windowWidth, windowHeight, camera, indoors);
        }
    }
}
