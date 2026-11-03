
#pragma once

#include "Camera.h"
#include "PauseMenuLayout.h"
#include "Shader.h"
#include <glad/gl.h>

class PauseMenu {
  public:
    PauseMenu();
    ~PauseMenu();

    PauseMenu(const PauseMenu&) = delete;
    PauseMenu& operator=(const PauseMenu&) = delete;

    void draw(int width, int height, const Camera& camera, bool interior = false);
    MenuAction actionAt(double x, double y, int width, int height) const {
        return PauseMenuLayout(width, height).hit(x, y);
    }

  private:
    Shader shader;
    GLuint vao = 0;
    GLuint vbo = 0;

    GLint screenLocation = -1;
    GLint colorLocation = -1;

    void drawRectangle(float x, float y, float width, float height, int screenWidth,
                       int screenHeight, float r, float g, float b, float a);

    void drawText(const char* text, float x, float y, float pixelSize, int screenWidth,
                  int screenHeight, float r, float g, float b);
};
