#include "ModelRenderer.h"
#include "ForestWorld.h"
#include "stb_image.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <unordered_set>
namespace {
void removeLeafBackground(unsigned char* pixels, int w, int h) {
    glm::vec3 background(0);
    for (auto index : {0, w - 1, (h - 1) * w, h * w - 1})
        background += glm::vec3(pixels[index * 4], pixels[index * 4 + 1], pixels[index * 4 + 2]);
    background *= .25f;
    std::vector<unsigned char> seen(size_t(w) * h, 0);
    std::vector<int> queue;
    auto visit = [&](int x, int y) {
        int index = y * w + x;
        if (seen[index])
            return;
        seen[index] = 1;
        glm::vec3 color(pixels[index * 4], pixels[index * 4 + 1], pixels[index * 4 + 2]);
        auto d = color - background;
        if (glm::dot(d, d) < 22 * 22) {
            pixels[index * 4 + 3] = 0;
            queue.push_back(index);
        }
    };
    for (int x = 0; x < w; ++x) {
        visit(x, 0);
        visit(x, h - 1);
    }
    for (int y = 0; y < h; ++y) {
        visit(0, y);
        visit(w - 1, y);
    }
    for (size_t i = 0; i < queue.size(); ++i) {
        int x = queue[i] % w, y = queue[i] / w;
        if (x > 0)
            visit(x - 1, y);
        if (x + 1 < w)
            visit(x + 1, y);
        if (y > 0)
            visit(x, y - 1);
        if (y + 1 < h)
            visit(x, y + 1);
    }
}
} // namespace
void ModelRenderer::release() {
    for (auto& m : meshes) {
        if (m.ebo)
            glDeleteBuffers(1, &m.ebo);
        if (m.vbo)
            glDeleteBuffers(1, &m.vbo);
        if (m.vao)
            glDeleteVertexArrays(1, &m.vao);
    }
    std::unordered_set<GLuint> uniqueTextures(textures.begin(), textures.end());
    for (auto t : uniqueTextures)
        if (t)
            glDeleteTextures(1, &t);
    shader.reset();
    meshes.clear();
    textures.clear();
}
ModelRenderer::~ModelRenderer() {
    release();
}
ModelRenderer::ModelRenderer(const std::string& path) {
    try {
        auto asset = ModelAsset::load(path);
        ForestWorld::arrange(asset);
        placement = glm::mat4(1);
        ground.build(asset, placement);
        door.build(asset, placement);
        start = ground.spawn(ForestWorld::spawn());
        shader = std::make_unique<Shader>("assets/shaders/model.vert", "assets/shaders/model.frag");
        for (const char* name : {"model", "view", "projection", "worldTime", "cameraPosition",
                                 "baseTexture", "emissiveTexture", "baseColor", "emissiveFactor",
                                 "baseUV", "emissiveUV", "hasTexture", "hasEmissive", "unlit",
                                 "proceduralLeaf", "grassBlade", "alphaMode", "alphaCutoff"})
            uniforms[name] = glGetUniformLocation(shader->ID, name);
        materials = asset.materials;
        instances = asset.instances;
        textures.resize(asset.textures.size(), 0);
        // Only decode color/emissive textures used by materials, not every normal/roughness image.
        std::vector<bool> used(textures.size(), false), leafBackground(textures.size(), false);
        for (auto& instance : asset.instances) {
            auto& mesh = asset.meshes[instance.mesh];
            if (!mesh.visible || mesh.material < 0)
                continue;
            auto& m = materials[mesh.material];
            if (m.texture >= 0) {
                used.at(m.texture) = true;
                if (m.name == "Material.003")
                    leafBackground.at(m.texture) = true;
            }
            if (m.emissiveTexture >= 0)
                used.at(m.emissiveTexture) = true;
        }
        std::map<std::array<int, 5>, GLuint> textureCache;
        GLint oldTexture, alignment;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        stbi_set_flip_vertically_on_load(false);
        for (size_t i = 0; i < textures.size(); ++i) {
            if (!used[i])
                continue;
            auto& t = asset.textures[i];
            auto& image = asset.images.at(t.image);
            std::array<int, 5> key{t.image, t.wrapS, t.wrapT, t.minFilter, t.magFilter};
            if (auto cached = textureCache.find(key); cached != textureCache.end()) {
                textures[i] = cached->second;
                continue;
            }
            if (image.encoded.size() > size_t(std::numeric_limits<int>::max()))
                throw std::runtime_error("Embedded image too large");
            int w = image.width, h = image.height, channels = 4;
            std::unique_ptr<unsigned char, decltype(&stbi_image_free)> decoded(nullptr,
                                                                               stbi_image_free);
            const unsigned char* pixels = image.rgba.data();
            if (image.rgba.empty()) {
                decoded.reset(stbi_load_from_memory(image.encoded.data(), int(image.encoded.size()),
                                                    &w, &h, &channels, 4));
                pixels = decoded.get();
                if (!pixels)
                    throw std::runtime_error("Cannot decode embedded model texture");
            }
            if (leafBackground[i] && decoded)
                removeLeafBackground(decoded.get(), w, h);
            // A 1024px cap saves GPU memory while retaining the original aspect ratio.
            std::vector<unsigned char> reduced;
            if (w > 1024 || h > 1024) {
                float factor = 1024.0f / std::max(w, h);
                int rw = std::max(1, int(w * factor)), rh = std::max(1, int(h * factor));
                reduced.resize(size_t(rw) * rh * 4);
                for (int y = 0; y < rh; ++y)
                    for (int x = 0; x < rw; ++x) {
                        float sx = glm::clamp((x + .5f) * w / rw - .5f, 0.0f, float(w - 1)),
                              sy = glm::clamp((y + .5f) * h / rh - .5f, 0.0f, float(h - 1));
                        int x0 = int(sx), y0 = int(sy), x1 = std::min(x0 + 1, w - 1),
                            y1 = std::min(y0 + 1, h - 1);
                        float fx = sx - x0, fy = sy - y0;
                        for (int c = 0; c < 4; ++c) {
                            float a = pixels[(y0 * w + x0) * 4 + c] * (1 - fx) +
                                      pixels[(y0 * w + x1) * 4 + c] * fx;
                            float b = pixels[(y1 * w + x0) * 4 + c] * (1 - fx) +
                                      pixels[(y1 * w + x1) * 4 + c] * fx;
                            reduced[(y * rw + x) * 4 + c] =
                                static_cast<unsigned char>(a * (1 - fy) + b * fy + .5f);
                        }
                    }
                w = rw;
                h = rh;
                pixels = reduced.data();
            }
            glGenTextures(1, &textures[i]);
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         pixels);

            textureCache[key] = textures[i];
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, t.wrapS);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t.wrapT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, t.minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, t.magFilter);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glBindTexture(GL_TEXTURE_2D, oldTexture);
        GLint oldVao, oldBuffer;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBuffer);
        meshes.reserve(asset.meshes.size());
        for (auto& source : asset.meshes) {
            meshes.push_back({});
            auto& m = meshes.back();
            m.material = source.material;
            if (!source.visible)
                continue;
            m.minimum = glm::vec3(std::numeric_limits<float>::max());
            m.maximum = -m.minimum;
            for (auto& v : source.vertices) {
                m.minimum = glm::min(m.minimum, v.position);
                m.maximum = glm::max(m.maximum, v.position);
            }
            if (source.indices.size() > size_t(std::numeric_limits<GLsizei>::max()))
                throw std::runtime_error("Model mesh too large");
            m.count = GLsizei(source.indices.size());
            glGenVertexArrays(1, &m.vao);
            glGenBuffers(1, &m.vbo);
            glGenBuffers(1, &m.ebo);
            glBindVertexArray(m.vao);
            glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
            glBufferData(GL_ARRAY_BUFFER, source.vertices.size() * sizeof(ModelVertex),
                         source.vertices.data(), GL_STATIC_DRAW);
            std::vector<uint32_t> indices = source.indices;
            m.mediumCount = GLsizei(source.mediumIndices.size());
            m.farCount = GLsizei(source.farIndices.size());
            m.mediumOffset = indices.size() * sizeof(uint32_t);
            indices.insert(indices.end(), source.mediumIndices.begin(), source.mediumIndices.end());
            m.farOffset = indices.size() * sizeof(uint32_t);
            indices.insert(indices.end(), source.farIndices.begin(), source.farIndices.end());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(),
                         GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                                  (void*)offsetof(ModelVertex, position));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                                  (void*)offsetof(ModelVertex, normal));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                                  (void*)offsetof(ModelVertex, uv));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                                  (void*)offsetof(ModelVertex, uv1));
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                                  (void*)offsetof(ModelVertex, color));
            glEnableVertexAttribArray(3);
        }
        glBindVertexArray(oldVao);
        glBindBuffer(GL_ARRAY_BUFFER, oldBuffer);
        std::cout << "Loaded forest assets: " << asset.meshes.size() << " meshes, "
                  << instances.size() << " instances.\n";
    } catch (...) {
        release();
        throw;
    }
}
void ModelRenderer::update(float deltaTime) {
    if (std::isfinite(deltaTime) && deltaTime > 0)
        vegetationTime = std::fmod(vegetationTime + deltaTime, 1000.0f);
}
void ModelRenderer::draw(const glm::mat4& view, const glm::mat4& projection) {
    GLint previousProgram, previousVao, active, texture0, texture1, frontFace;
    GLboolean blending = glIsEnabled(GL_BLEND), culling = glIsEnabled(GL_CULL_FACE), mask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glGetIntegerv(GL_FRONT_FACE, &frontFace);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0);
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture1);
    shader->use();
    auto location = [&](const char* name) { return uniforms.at(name); };
    glUniformMatrix4fv(location("view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(location("projection"), 1, GL_FALSE, glm::value_ptr(projection));
    auto eye = glm::vec3(glm::inverse(view)[3]);
    glUniform3fv(location("cameraPosition"), 1, glm::value_ptr(eye));
    glUniform1f(location("worldTime"), vegetationTime);
    glUniform1i(location("baseTexture"), 0);
    glUniform1i(location("emissiveTexture"), 1);
    // Draw opaque/masked meshes first, then back-to-front alpha-blended instances.
    std::vector<size_t> order(instances.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = i;
    auto blended = [&](size_t i) {
        int m = meshes[instances[i].mesh].material;
        return m >= 0 && materials[m].alphaMode == 2;
    };
    auto depth = [&](size_t i) {
        return (view * placement * instances[i].transform * glm::vec4(0, 0, 0, 1)).z;
    };
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        if (blended(a) != blended(b))
            return !blended(a);
        return blended(a) ? depth(a) < depth(b) : false;
    });
    for (auto i : order) {
        auto& instance = instances[i];
        auto& mesh = meshes[instance.mesh];
        if (!mesh.count)
            continue;
        GLsizei drawCount = mesh.count;
        size_t drawOffset = 0;
        if (instance.group.rfind("tree/", 0) == 0) {
            float distance =
                glm::length(glm::vec2(instance.lodOrigin.x - eye.x, instance.lodOrigin.z - eye.z));
            auto detail = ForestWorld::detail(distance);
            if (detail == ForestWorld::Detail::Hidden)
                continue;
            if (detail == ForestWorld::Detail::Far && mesh.farCount) {
                drawCount = mesh.farCount;
                drawOffset = mesh.farOffset;
            } else if (detail == ForestWorld::Detail::Medium && mesh.mediumCount) {
                drawCount = mesh.mediumCount;
                drawOffset = mesh.mediumOffset;
            }
        }
        if (instance.group.rfind("grass/", 0) == 0) {
            auto center = glm::vec3(placement * instance.transform *
                                    glm::vec4((mesh.minimum + mesh.maximum) * .5f, 1));
            float radius = glm::length(glm::vec2(mesh.maximum.x - mesh.minimum.x,
                                                 mesh.maximum.z - mesh.minimum.z)) *
                           .5f;
            if (glm::length(glm::vec2(center.x - eye.x, center.z - eye.z)) - radius > 28)
                continue;
        }
        // Skip offscreen mesh instances, particularly the forest behind the camera.
        auto clip = projection * view * placement * instance.transform;
        glm::vec4 corners[8];
        for (int c = 0; c < 8; ++c)
            corners[c] = clip * glm::vec4((c & 1) ? mesh.maximum.x : mesh.minimum.x,
                                          (c & 2) ? mesh.maximum.y : mesh.minimum.y,
                                          (c & 4) ? mesh.maximum.z : mesh.minimum.z, 1);
        bool visible = true;
        for (int plane = 0; plane < 6; ++plane) {
            bool outside = true;
            for (auto p : corners) {
                float v = plane == 0   ? p.x + p.w
                          : plane == 1 ? p.w - p.x
                          : plane == 2 ? p.y + p.w
                          : plane == 3 ? p.w - p.y
                          : plane == 4 ? p.z + p.w
                                       : p.w - p.z;
                if (v >= 0) {
                    outside = false;
                    break;
                }
            }
            if (outside) {
                visible = false;
                break;
            }
        }
        if (!visible)
            continue;
        ModelMaterial fallback;
        auto& m = mesh.material < 0 ? fallback : materials[mesh.material];
        bool blend = m.alphaMode == 2;
        if (blend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        glDepthMask(blend ? GL_FALSE : GL_TRUE);
        if (m.doubleSided)
            glDisable(GL_CULL_FACE);
        else
            glEnable(GL_CULL_FACE);
        auto model = placement * instance.transform;
        glFrontFace(glm::determinant(glm::mat3(model)) < 0 ? GL_CW : GL_CCW);
        glUniformMatrix4fv(location("model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4fv(location("baseColor"), 1, glm::value_ptr(m.color));
        glUniform3fv(location("emissiveFactor"), 1, glm::value_ptr(m.emissive));
        glUniformMatrix3fv(location("baseUV"), 1, GL_FALSE, glm::value_ptr(m.uv));
        glUniformMatrix3fv(location("emissiveUV"), 1, GL_FALSE, glm::value_ptr(m.emissiveUV));
        glUniform1i(location("hasTexture"), m.texture >= 0);
        glUniform1i(location("hasEmissive"), m.emissiveTexture >= 0);
        glUniform1i(location("unlit"), m.unlit);
        glUniform1i(location("proceduralLeaf"), m.proceduralLeaf);
        glUniform1i(location("grassBlade"), m.grassBlade);
        glUniform1i(location("alphaMode"), m.alphaMode);
        glUniform1f(location("alphaCutoff"), m.cutoff);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m.texture < 0 ? 0 : textures[m.texture]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m.emissiveTexture < 0 ? 0 : textures[m.emissiveTexture]);
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, drawCount, GL_UNSIGNED_INT,
                       reinterpret_cast<const void*>(drawOffset));
    }
    glFrontFace(frontFace);
    glDepthMask(mask);
    if (blending)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (culling)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(active);
    glBindVertexArray(previousVao);
    glUseProgram(previousProgram);
}
