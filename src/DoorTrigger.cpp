#include "DoorTrigger.h"
#include "PlayerDimensions.h"
#include <cctype>
#include <limits>
void DoorTrigger::build(const ModelAsset& asset, const glm::mat4& placement) {
    doors.clear();
    for (auto& instance : asset.instances) {
        if (!instance.portal)
            continue;
        auto& mesh = asset.meshes[instance.mesh];
        if (mesh.material < 0)
            continue;
        auto name = asset.materials[mesh.material].name;
        for (auto& c : name)
            c = char(std::tolower(static_cast<unsigned char>(c)));
        if (name != "door" && name.rfind("door.", 0) != 0 &&
            name.find("door_") == std::string::npos)
            continue;
        glm::vec3 lo(std::numeric_limits<float>::max()), hi = -lo;
        auto transform = placement * instance.transform;
        for (auto& v : mesh.vertices) {
            auto point = glm::vec3(transform * glm::vec4(v.position, 1));
            lo = glm::min(lo, point);
            hi = glm::max(hi, point);
        }
        doors.push_back({lo, hi});
    }
}
bool DoorTrigger::near(const glm::vec3& center) const {
    float feet = center.y - PlayerDimensions::CenterAboveFeet;
    for (auto& door : doors) {
        if (feet < door.minimum.y - .7f || feet > door.maximum.y)
            continue;
        float dx = center.x - glm::clamp(center.x, door.minimum.x, door.maximum.x);
        float dz = center.z - glm::clamp(center.z, door.minimum.z, door.maximum.z);
        if (dx * dx + dz * dz < .85f * .85f)
            return true;
    }
    return false;
}
