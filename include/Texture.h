
#pragma once

#include <glad/gl.h>
#include <string>

class Texture {
  public:
    explicit Texture(const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(GLuint unit = 0) const;

    int getWidth() const;
    int getHeight() const;

  private:
    GLuint id = 0;
    int width = 0;
    int height = 0;
};
