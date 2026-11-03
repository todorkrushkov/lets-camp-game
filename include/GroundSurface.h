#pragma once
#include "ModelAsset.h"
#include "PlayerDimensions.h"
#include <optional>
#include <unordered_map>
class GroundSurface {
    struct Triangle {
        glm::vec3 a, b, c;
    };
    std::vector<Triangle> triangles, obstacles;
    std::unordered_map<int64_t, std::vector<size_t>> cells;
    std::unordered_map<int64_t, std::vector<size_t>> obstacleCells;
    static int64_t key(int x, int z);

  public:
    static constexpr float PlayerCenterAboveFeet = PlayerDimensions::CenterAboveFeet;
    void build(const ModelAsset& asset, const glm::mat4& placement);
    std::optional<float> height(float x, float z) const;
    bool blocked(const glm::vec3& playerCenter) const;
    glm::vec3 spawn(const glm::vec3& preferred) const;
    glm::vec3 move(const glm::vec3& from, const glm::vec3& candidate) const;
};
