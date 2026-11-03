#include "TerrainRenderer.h"
#include "TileAtlas.h"
#include <stdexcept>
#include <vector>
TerrainRenderer::TerrainRenderer() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
}
TerrainRenderer::~TerrainRenderer() {
    if (vbo)
        glDeleteBuffers(1, &vbo);
    if (vao)
        glDeleteVertexArrays(1, &vao);
}
void TerrainRenderer::build(const TerrainMap& map, const Texture& texture) {
    std::vector<float> vertices;
    for (size_t l = 0; l < map.layers.size(); ++l) {
        const auto& tiles = map.layers[l].tiles;
        if (tiles.size() != size_t(map.columns) * map.rows)
            throw std::runtime_error("Terrain layer size mismatch");
        for (int r = 0; r < map.rows; ++r)
            for (int c = 0; c < map.columns; ++c) {
                int id = tiles[r * map.columns + c];
                if (id == -1)
                    continue; // zero is grass!
                auto uv = TileAtlas::uv(id, texture);
                float x = (c - map.columns * 0.5f) * map.cellSize,
                      z = (r - map.rows * 0.5f) * map.cellSize;
                float s = map.cellSize, y = float(l) * 0.002f;
                auto add = [&](float px, float pz, float u, float v) {
                    vertices.insert(vertices.end(), {px, y, pz, u, v});
                };
                // Winding faces +Y; source row zero points toward minimum Z.
                add(x, z, uv.x, uv.y);
                add(x, z + s, uv.x, uv.w);
                add(x + s, z, uv.z, uv.y);
                add(x + s, z, uv.z, uv.y);
                add(x, z + s, uv.x, uv.w);
                add(x + s, z + s, uv.z, uv.w);
            }
    }
    GLint oldVao, oldBuffer;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBuffer);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    vertexCount = static_cast<GLsizei>(vertices.size() / 5);
    glBindVertexArray(oldVao);
    glBindBuffer(GL_ARRAY_BUFFER, oldBuffer);
}
void TerrainRenderer::draw(const TerrainMap&, const Shader& shader, const Texture& texture,
                           const glm::mat4& view, const glm::mat4& projection) const {
    shader.use();
    shader.setMat4("model", glm::mat4(1.0f));
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    glUniform1i(glGetUniformLocation(shader.ID, "terrainTexture"), 0);
    texture.bind(0);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}
