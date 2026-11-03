#include "InteriorRoom.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>
namespace {
struct Vertex {
    glm::vec3 p, n, c;
};
} // namespace
InteriorRoom::InteriorRoom()
    : shader("assets/shaders/interior.vert", "assets/shaders/interior.frag"),
      handShader("assets/shaders/lighter.vert", "assets/shaders/lighter.frag"),
      lighter("assets/textures/holding_lighter_seq.png") {
    if (lighter.getWidth() % 4 || lighter.getHeight() <= 0)
        throw std::runtime_error("Lighter sheet must contain four equal horizontal frames");
    std::vector<Vertex> data;
    auto quad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 color) {
        auto n = glm::normalize(glm::cross(b - a, c - a));
        for (auto p : {a, b, c, a, c, d})
            data.push_back({p, n, color});
    };
    auto box = [&](glm::vec3 lo, glm::vec3 hi, glm::vec3 color) {
        quad({lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z}, color);
        quad({hi.x, lo.y, hi.z}, {lo.x, lo.y, hi.z}, {lo.x, hi.y, hi.z}, {hi.x, hi.y, hi.z}, color);
        quad({lo.x, lo.y, hi.z}, {lo.x, lo.y, lo.z}, {lo.x, hi.y, lo.z}, {lo.x, hi.y, hi.z}, color);
        quad({hi.x, lo.y, lo.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {hi.x, hi.y, lo.z}, color);
        quad({lo.x, hi.y, lo.z}, {hi.x, hi.y, lo.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z}, color);
        quad({lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, lo.y, lo.z}, {lo.x, lo.y, lo.z}, color);
    };
    quad({-4, 0, 5}, {4, 0, 5}, {4, 0, -5}, {-4, 0, -5}, {.22f, .14f, .08f});
    quad({-4, 3.5f, -5}, {4, 3.5f, -5}, {4, 3.5f, 5}, {-4, 3.5f, 5}, {.09f, .08f, .07f});
    quad({-4, 0, -5}, {4, 0, -5}, {4, 3.5f, -5}, {-4, 3.5f, -5}, {.28f, .25f, .21f});
    quad({-4, 0, 5}, {-4, 0, -5}, {-4, 3.5f, -5}, {-4, 3.5f, 5}, {.25f, .22f, .18f});
    quad({4, 0, -5}, {4, 0, 5}, {4, 3.5f, 5}, {4, 3.5f, -5}, {.25f, .22f, .18f});
    quad({4, 0, 5}, {-4, 0, 5}, {-4, 3.5f, 5}, {4, 3.5f, 5}, {.25f, .22f, .18f});
    // Visible exit door on the south wall, reached before the boundary.
    quad({.8f, 0, 4.98f}, {-.8f, 0, 4.98f}, {-.8f, 2.3f, 4.98f}, {.8f, 2.3f, 4.98f},
         {.16f, .075f, .025f});
    box({.5f, 1, 4.91f}, {.58f, 1.08f, 4.98f}, {.65f, .38f, .08f});
    box({-3.6f, 0, -3.7f}, {-2.2f, 1.1f, -2.3f}, {.19f, .10f, .04f});
    box({2.1f, 0, -2.4f}, {3.4f, .8f, -1.1f}, {.16f, .085f, .035f});
    roomCount = GLsizei(data.size());
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Vertex), data.data(), GL_STATIC_DRAW);
    for (int i = 0; i < 3; ++i) {
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              reinterpret_cast<void*>(i * sizeof(glm::vec3)));
        glEnableVertexAttribArray(i);
    }
    glBindVertexArray(0);
}
InteriorRoom::~InteriorRoom() {
    if (vbo)
        glDeleteBuffers(1, &vbo);
    if (vao)
        glDeleteVertexArrays(1, &vao);
}
glm::vec3 InteriorRoom::move(const glm::vec3& from, const glm::vec3& to) {
    auto valid = [](glm::vec3 p) {
        float r = PlayerDimensions::Radius;
        if (p.x < -4 + r || p.x > 4 - r || p.z < -5 + r || p.z > 5 - r)
            return false;
        auto box = [&](float x0, float z0, float x1, float z1) {
            auto q = glm::vec2(glm::clamp(p.x, x0, x1), glm::clamp(p.z, z0, z1));
            auto d = glm::vec2(p.x, p.z) - q;
            return glm::dot(d, d) < r * r;
        };
        return !box(-3.6f, -3.7f, -2.2f, -2.3f) && !box(2.1f, -2.4f, 3.4f, -1.1f);
    };
    glm::vec3 result = from;
    glm::vec2 delta(to.x - from.x, to.z - from.z);
    int count = std::max(1, int(std::ceil(glm::length(delta) / .08f)));
    delta /= float(count);
    for (int i = 0; i < count; ++i) {
        auto p = result + glm::vec3(delta.x, 0, delta.y);
        if (valid(p))
            result = p;
        else {
            p = result + glm::vec3(delta.x, 0, 0);
            if (valid(p))
                result = p;
            p = result + glm::vec3(0, 0, delta.y);
            if (valid(p))
                result = p;
        }
    }
    result.y = PlayerDimensions::CenterAboveFeet + .02f;
    return result;
}
void InteriorRoom::update(float deltaTime) {
    if (std::isfinite(deltaTime) && deltaTime > 0)
        animationTime = std::fmod(animationTime + deltaTime, 100.0f);
}
void InteriorRoom::draw(const glm::mat4& view, const glm::mat4& projection) {
    GLint oldProgram, oldVao, oldActive, oldTexture, viewport[4];
    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActive);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
    glGetIntegerv(GL_VIEWPORT, viewport);
    bool cull = glIsEnabled(GL_CULL_FACE), blend = glIsEnabled(GL_BLEND),
         depth = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setMat4("model", glm::mat4(1));
    int frame = int(animationTime * 10.0f) % 4;
    float width = float(viewport[2]), height = float(viewport[3]);
    float handHeight = height * .42f,
          handWidth = handHeight * (lighter.getWidth() / 4.0f) / lighter.getHeight();
    glm::vec3 handPosition(width * .80f, -height * .04f, 0);
    // Flame centers in each authored frame; match the light to the displayed flame.
    const glm::vec2 flamePixels[] = {{4.5f, 7.5f}, {3.5f, 5.5f}, {5.0f, 7.5f}, {4.5f, 6.5f}};
    auto flameUV =
        flamePixels[frame] / glm::vec2(lighter.getWidth() / 4.0f, float(lighter.getHeight()));
    glm::vec2 flameScreen(handPosition.x - handWidth * .5f + flameUV.x * handWidth,
                          handPosition.y + (1.0f - flameUV.y) * handHeight);
    glm::vec2 ndc = flameScreen / glm::vec2(width, height) * 2.0f - 1.0f;
    const float reach = .55f;
    auto camera = glm::inverse(view);
    auto light = glm::vec3(camera * glm::vec4(ndc.x * reach / projection[0][0],
                                              ndc.y * reach / projection[1][1], -reach, 1));
    // Keep the flame inside the room when looking directly against a wall.
    light = glm::clamp(light, glm::vec3(-3.8f, .12f, -4.8f), glm::vec3(3.8f, 3.3f, 4.8f));
    const float frameStrength[] = {1.0f, .97f, 1.05f, 1.01f};
    float flicker = frameStrength[frame] * (1.0f + .025f * std::sin(animationTime * 13.0f) +
                                            .015f * std::sin(animationTime * 21.0f));
    glUniform3fv(glGetUniformLocation(shader.ID, "lightPosition"), 1, glm::value_ptr(light));
    glUniform1f(glGetUniformLocation(shader.ID, "lightStrength"), 3.4f * flicker);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, roomCount);
    // Screen-space hand is independent of camera pitch and zoom and never clips into walls.
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    auto screen = glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);
    handRenderer.draw(handShader, lighter, handPosition, glm::mat4(1), screen, frame, 0,
                      lighter.getWidth() / 4, lighter.getHeight(), handWidth, handHeight);
    if (depth)
        glEnable(GL_DEPTH_TEST);
    if (cull)
        glEnable(GL_CULL_FACE);
    if (!blend)
        glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    glActiveTexture(oldActive);
    glBindVertexArray(oldVao);
    glUseProgram(oldProgram);
}
