
#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdexcept>
#include <string>

Texture::Texture(const std::string& path) {
    int channels = 0;

    // Keep the image's top-left origin.
    stbi_set_flip_vertically_on_load(false);

    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
}

Texture::~Texture() {
    if (id != 0) {
        glDeleteTextures(1, &id);
    }
}

void Texture::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

int Texture::getWidth() const {
    return width;
}

int Texture::getHeight() const {
    return height;
}
