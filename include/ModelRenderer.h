#pragma once
#include "DoorTrigger.h"
#include "GroundSurface.h"
#include "ModelAsset.h"
#include "Shader.h"
#include <memory>
#include <unordered_map>
class ModelRenderer {
    struct Mesh {
        GLuint vao = 0, vbo = 0, ebo = 0;
        GLsizei count = 0, mediumCount = 0, farCount = 0;
        size_t mediumOffset = 0, farOffset = 0;
        int material = -1;
        glm::vec3 minimum{0}, maximum{0};
    };
    std::vector<Mesh> meshes;
    std::vector<GLuint> textures;
    std::vector<ModelMaterial> materials;
    std::vector<ModelInstance> instances;
    std::unique_ptr<Shader> shader;
    glm::mat4 placement{1};
    glm::vec3 start{0, .5f, 0};
    float vegetationTime = 0.0f;
    std::unordered_map<std::string, GLint> uniforms;
    GroundSurface ground;
    DoorTrigger door;
    void release();

  public:
    ModelRenderer(const std::string& path);
    ~ModelRenderer();
    ModelRenderer(const ModelRenderer&) = delete;
    ModelRenderer& operator=(const ModelRenderer&) = delete;
    void update(float deltaTime);
    void draw(const glm::mat4& view, const glm::mat4& projection);
    bool nearDoor(const glm::vec3& p) const {
        return door.near(p);
    }
    glm::vec3 suggestedStart() const {
        return start;
    }
    glm::vec3 resolveMovement(const glm::vec3& from, const glm::vec3& to) const {
        return ground.move(from, to);
    }
};
