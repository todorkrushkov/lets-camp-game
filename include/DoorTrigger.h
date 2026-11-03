#pragma once
#include "ModelAsset.h"
class DoorTrigger {
    struct Door {
        glm::vec3 minimum, maximum;
    };
    std::vector<Door> doors;

  public:
    void build(const ModelAsset& asset, const glm::mat4& placement);
    bool near(const glm::vec3& playerCenter) const;
    size_t count() const {
        return doors.size();
    }
};
