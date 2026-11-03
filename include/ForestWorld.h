#pragma once
#include "ModelAsset.h"
// Rebuilds the asset-library scene as a deterministic forest village.
class ForestWorld {
  public:
    enum class Detail { Near, Medium, Far, Hidden };
    static Detail detail(float distance) {
        return distance >= 65   ? Detail::Hidden
               : distance >= 36 ? Detail::Far
               : distance >= 18 ? Detail::Medium
                                : Detail::Near;
    }
    static void arrange(ModelAsset& asset);
    static glm::vec3 spawn() {
        return {0, .52f, 5};
    }
};
