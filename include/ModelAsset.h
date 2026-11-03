#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>
struct ModelVertex {
    glm::vec3 position, normal;
    glm::vec2 uv, uv1;
    glm::vec4 color{1};
};
struct ModelMesh {
    bool visible = true;
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices, mediumIndices, farIndices;
    int material = -1;
};
struct ModelImage {
    std::vector<unsigned char> encoded;
    std::vector<unsigned char> rgba;
    int width = 0, height = 0;
};
struct ModelTexture {
    int image = -1, wrapS = 10497, wrapT = 10497, minFilter = 9987, magFilter = 9729;
};
struct ModelMaterial {
    std::string name;
    glm::vec4 color{1};
    glm::vec3 emissive{0};
    int texture = -1, emissiveTexture = -1;
    glm::mat3 uv{1}, emissiveUV{1};
    bool doubleSided = false, unlit = false, proceduralLeaf = false, grassBlade = false;
    int alphaMode = 0;
    float cutoff = .5f;
};
struct ModelInstance {
    size_t mesh;
    glm::mat4 transform;
    std::string group;
    bool collision = true, portal = true;
    glm::vec3 lodOrigin{0};
};
struct ModelAsset {
    std::vector<ModelMesh> meshes;
    std::vector<ModelInstance> instances;
    std::vector<ModelMaterial> materials;
    std::vector<ModelImage> images;
    std::vector<ModelTexture> textures;
    glm::vec3 minimum{0}, maximum{0};
    static ModelAsset load(const std::string& path);
};
