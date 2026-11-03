
#include "SpriteRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

SpriteRenderer::SpriteRenderer() {
    // X, Y, U, V
    constexpr float vertices[] = {
        -0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f,  1.0f, 1.0f, 0.0f,

        -0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f, 1.0f, 0.0f, -0.5f, 1.0f, 0.0f, 0.0f};

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

SpriteRenderer::~SpriteRenderer() {
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }

    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
}

void SpriteRenderer::draw(const Shader& shader, const Texture& texture, const glm::vec3& position,
                          const glm::mat4& view, const glm::mat4& projection, int column, int row,
                          int frameWidth, int frameHeight, float worldWidth,
                          float worldHeight) const {
    // Remove camera rotation from the sprite's orientation.
    glm::mat4 billboard = glm::mat4(glm::mat3(glm::inverse(view)));

    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model *= billboard;
    model = glm::scale(model, glm::vec3(worldWidth, worldHeight, 1.0f));

    float uvWidth = static_cast<float>(frameWidth) / texture.getWidth();

    float uvHeight = static_cast<float>(frameHeight) / texture.getHeight();

    float uvX = column * uvWidth;
    float uvY = row * uvHeight;

    shader.use();

    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glUniform2f(glGetUniformLocation(shader.ID, "uvOffset"), uvX, uvY);

    glUniform2f(glGetUniformLocation(shader.ID, "uvScale"), uvWidth, uvHeight);

    glUniform1i(glGetUniformLocation(shader.ID, "spriteTexture"), 0);

    texture.bind(0);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
