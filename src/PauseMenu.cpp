
#include "PauseMenu.h"

#include <array>

namespace {

using Glyph = std::array<unsigned char, 7>;

Glyph getGlyph(char c) {
    switch (c) {
    case '2':
        return {14, 17, 1, 2, 4, 8, 31};
    case '3':
        return {30, 1, 1, 14, 1, 1, 30};
    case ':':
        return {0, 4, 4, 0, 4, 4, 0};
    case 'A':
        return {14, 17, 17, 31, 17, 17, 17};
    case 'B':
        return {30, 17, 17, 30, 17, 17, 30};
    case 'C':
        return {14, 17, 16, 16, 16, 17, 14};
    case 'D':
        return {30, 17, 17, 17, 17, 17, 30};
    case 'E':
        return {31, 16, 16, 30, 16, 16, 31};
    case 'F':
        return {31, 16, 16, 30, 16, 16, 16};
    case 'G':
        return {14, 17, 16, 23, 17, 17, 14};
    case 'H':
        return {17, 17, 17, 31, 17, 17, 17};
    case 'I':
        return {31, 4, 4, 4, 4, 4, 31};
    case 'J':
        return {7, 2, 2, 2, 18, 18, 12};
    case 'K':
        return {17, 18, 20, 24, 20, 18, 17};
    case 'L':
        return {16, 16, 16, 16, 16, 16, 31};
    case 'M':
        return {17, 27, 21, 21, 17, 17, 17};
    case 'N':
        return {17, 25, 21, 19, 17, 17, 17};
    case 'O':
        return {14, 17, 17, 17, 17, 17, 14};
    case 'P':
        return {30, 17, 17, 30, 16, 16, 16};
    case 'Q':
        return {14, 17, 17, 17, 21, 18, 13};
    case 'R':
        return {30, 17, 17, 30, 20, 18, 17};
    case 'S':
        return {15, 16, 16, 14, 1, 1, 30};
    case 'T':
        return {31, 4, 4, 4, 4, 4, 4};
    case 'U':
        return {17, 17, 17, 17, 17, 17, 14};
    case 'V':
        return {17, 17, 17, 17, 17, 10, 4};
    case 'W':
        return {17, 17, 17, 21, 21, 21, 10};
    case 'X':
        return {17, 17, 10, 4, 10, 17, 17};
    case 'Y':
        return {17, 17, 10, 4, 4, 4, 4};
    case 'Z':
        return {31, 1, 2, 4, 8, 16, 31};
    default:
        return {0, 0, 0, 0, 0, 0, 0};
    }
}

} // namespace

PauseMenu::PauseMenu() : shader("assets/shaders/pause.vert", "assets/shaders/pause.frag") {
    screenLocation = glGetUniformLocation(shader.ID, "uScreen");

    colorLocation = glGetUniformLocation(shader.ID, "uColor");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

PauseMenu::~PauseMenu() {
    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }

    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }
}

void PauseMenu::drawRectangle(float x, float y, float width, float height, int screenWidth,
                              int screenHeight, float r, float g, float b, float a) {
    float vertices[] = {x, y, x + width, y,          x + width, y + height,

                        x, y, x + width, y + height, x,         y + height};

    glUseProgram(shader.ID);

    glUniform2f(screenLocation, static_cast<float>(screenWidth), static_cast<float>(screenHeight));

    glUniform4f(colorLocation, r, g, b, a);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void PauseMenu::drawText(const char* text, float x, float y, float pixelSize, int screenWidth,
                         int screenHeight, float r, float g, float b) {
    float cursorX = x;

    for (const char* p = text; *p; ++p) {
        Glyph glyph = getGlyph(*p);

        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph[row] & (1 << (4 - col))) {
                    drawRectangle(cursorX + col * pixelSize, y + row * pixelSize, pixelSize,
                                  pixelSize, screenWidth, screenHeight, r, g, b, 1.0f);
                }
            }
        }

        cursorX += pixelSize * 6.0f;
    }
}

void PauseMenu::draw(int width, int height, const Camera& camera, bool interior) {
    GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);

    GLint previousProgram = 0;
    GLint previousVAO = 0;
    GLint previousVBO = 0;
    GLint previousBlendSrc = 0;
    GLint previousBlendDst = 0;

    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);

    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousVBO);

    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSrc);

    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDst);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Dark overlay
    drawRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), width, height,
                  0.02f, 0.04f, 0.07f, 0.78f);

    PauseMenuLayout layout(width, height);
    drawRectangle(layout.x, layout.y, layout.width, layout.height, width, height, 0.10f, 0.14f,
                  0.18f, 0.96f);
    drawText("PAUSED", layout.x + layout.width * 0.5f - 108 * layout.scale,
             layout.y + 30 * layout.scale, 6 * layout.scale, width, height, 1, 1, 1);
    const bool twoD = camera.getMode() == CameraMode::TwoD;
    const char* rows[] = {
        interior ? "MODE: 3D LOCKED" : (twoD ? "MODE: 2D" : "MODE: 3D"),
        twoD ? "VIEW: LOCKED"
             : (camera.isFirstPerson() ? "VIEW: FIRST PERSON" : "VIEW: THIRD PERSON"),
        "RESUME GAME"};
    for (int i = 0; i < 3; ++i) {
        const bool disabled = (i < 2 && interior) || (i == 1 && twoD), resume = (i == 2);
        drawRectangle(layout.x + 30 * layout.scale, layout.rowY(i),
                      layout.width - 60 * layout.scale, 48 * layout.scale, width, height,
                      resume ? 0.87f : 0.16f, resume ? 0.68f : 0.21f, resume ? 0.40f : 0.28f, 1);
        float color = disabled ? 0.45f : (resume ? 0.12f : 1.0f);
        drawText(rows[i], layout.x + 46 * layout.scale, layout.rowY(i) + 16 * layout.scale,
                 2.5f * layout.scale, width, height, color, color, color);
    }

    // Restore OpenGL state
    glUseProgram(static_cast<GLuint>(previousProgram));

    glBindVertexArray(static_cast<GLuint>(previousVAO));

    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousVBO));

    glBlendFunc(static_cast<GLenum>(previousBlendSrc), static_cast<GLenum>(previousBlendDst));

    if (depthEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (blendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}
