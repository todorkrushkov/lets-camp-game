#include "GroundSurface.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace {
constexpr float CellSize = 2.0f, FeetClearance = 0.02f;
bool terrain(const std::string& name) {
    std::string lower = name;
    for (auto& c : lower)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return lower.find("ground") != std::string::npos ||
           lower.find("terrain") != std::string::npos ||
           lower.find("landscape") != std::string::npos;
}
bool foliage(const std::string& name) {
    std::string lower = name;
    for (auto& c : lower)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return lower.find("leaf") != std::string::npos || lower.find("leaves") != std::string::npos ||
           lower.find("foliage") != std::string::npos;
}
float pointTriangleDistance2(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    auto ab = b - a, ac = c - a, ap = p - a;
    float d1 = glm::dot(ab, ap), d2 = glm::dot(ac, ap);
    if (d1 <= 0 && d2 <= 0)
        return glm::dot(ap, ap);
    auto bp = p - b;
    float d3 = glm::dot(ab, bp), d4 = glm::dot(ac, bp);
    if (d3 >= 0 && d4 <= d3)
        return glm::dot(bp, bp);
    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) {
        auto delta = p - (a + ab * (d1 / (d1 - d3)));
        return glm::dot(delta, delta);
    }
    auto cp = p - c;
    float d5 = glm::dot(ab, cp), d6 = glm::dot(ac, cp);
    if (d6 >= 0 && d5 <= d6)
        return glm::dot(cp, cp);
    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        auto delta = p - (a + ac * (d2 / (d2 - d6)));
        return glm::dot(delta, delta);
    }
    float va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        auto delta = p - (b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6))));
        return glm::dot(delta, delta);
    }
    float inverse = 1 / (va + vb + vc);
    auto delta = p - (a + ab * (vb * inverse) + ac * (vc * inverse));
    return glm::dot(delta, delta);
}
float segmentDistance2(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b) {
    auto u = q - p, v = b - a, w = p - a;
    float uu = glm::dot(u, u), vv = glm::dot(v, v), uv = glm::dot(u, v), uw = glm::dot(u, w),
          vw = glm::dot(v, w);
    float s = 0, t = 0;
    if (uu < 1e-12f && vv < 1e-12f)
        return glm::dot(w, w);
    if (uu < 1e-12f)
        t = glm::clamp(vw / vv, 0.0f, 1.0f);
    else if (vv < 1e-12f)
        s = glm::clamp(-uw / uu, 0.0f, 1.0f);
    else {
        float determinant = uu * vv - uv * uv;
        if (determinant > 1e-12f)
            s = glm::clamp((uv * vw - vv * uw) / determinant, 0.0f, 1.0f);
        t = (uv * s + vw) / vv;
        if (t < 0) {
            t = 0;
            s = glm::clamp(-uw / uu, 0.0f, 1.0f);
        } else if (t > 1) {
            t = 1;
            s = glm::clamp((uv - uw) / uu, 0.0f, 1.0f);
        }
    }
    auto delta = (p + s * u) - (a + t * v);
    return glm::dot(delta, delta);
}
float capsuleTriangleDistance2(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    auto n = glm::cross(b - a, c - a);
    float denominator = glm::dot(n, q - p);
    if (std::abs(denominator) > 1e-10f) {
        float t = glm::dot(n, a - p) / denominator;
        if (t >= 0 && t <= 1 && pointTriangleDistance2(p + t * (q - p), a, b, c) < 1e-8f)
            return 0;
    }
    return std::min({pointTriangleDistance2(p, a, b, c), pointTriangleDistance2(q, a, b, c),
                     segmentDistance2(p, q, a, b), segmentDistance2(p, q, b, c),
                     segmentDistance2(p, q, c, a)});
}
float cross(glm::vec2 a, glm::vec2 b) {
    return a.x * b.y - a.y * b.x;
}
glm::vec2 xz(glm::vec3 p) {
    return {p.x, p.z};
}
} // namespace
int64_t GroundSurface::key(int x, int z) {
    return int64_t((uint64_t(uint32_t(x)) << 32) | uint32_t(z));
}
void GroundSurface::build(const ModelAsset& asset, const glm::mat4& placement) {
    triangles.clear();
    cells.clear();
    obstacles.clear();
    obstacleCells.clear();
    for (const auto& instance : asset.instances) {
        if (!instance.collision)
            continue;
        const auto& mesh = asset.meshes.at(instance.mesh);
        std::string name = mesh.material < 0 ? "" : asset.materials.at(mesh.material).name;
        bool floor = terrain(name);
        if (!floor && foliage(name))
            continue;
        auto matrix = placement * instance.transform;
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            auto world = [&](size_t index) {
                return glm::vec3(matrix *
                                 glm::vec4(mesh.vertices.at(mesh.indices.at(index)).position, 1));
            };
            Triangle t{world(i), world(i + 1), world(i + 2)};
            auto normal = glm::cross(t.b - t.a, t.c - t.a);
            float length = glm::length(normal);
            if (length < 1e-6f)
                continue;
            bool walkable = floor && std::abs(normal.y) / length >= 0.55f;
            auto& collection = walkable ? triangles : obstacles;
            auto& buckets = walkable ? cells : obstacleCells;
            size_t index = collection.size();
            collection.push_back(t);
            auto lo = glm::min(t.a, glm::min(t.b, t.c)), hi = glm::max(t.a, glm::max(t.b, t.c));
            for (int x = int(std::floor(lo.x / CellSize)); x <= int(std::floor(hi.x / CellSize));
                 ++x)
                for (int z = int(std::floor(lo.z / CellSize));
                     z <= int(std::floor(hi.z / CellSize)); ++z)
                    buckets[key(x, z)].push_back(index);
        }
    }
    if (triangles.empty())
        throw std::runtime_error(
            "No walkable model terrain: assign a Ground or Terrain material to the floor");
}
bool GroundSurface::blocked(const glm::vec3& center) const {
    constexpr float radius = PlayerDimensions::Radius, bodyHeight = PlayerDimensions::Height;
    float feet = center.y - PlayerCenterAboveFeet;
    glm::vec3 lower(center.x, feet + radius, center.z),
        upper(center.x, feet + bodyHeight - radius, center.z);
    for (int x = int(std::floor((center.x - radius) / CellSize));
         x <= int(std::floor((center.x + radius) / CellSize)); ++x)
        for (int z = int(std::floor((center.z - radius) / CellSize));
             z <= int(std::floor((center.z + radius) / CellSize)); ++z) {
            auto bucket = obstacleCells.find(key(x, z));
            if (bucket == obstacleCells.end())
                continue;
            for (auto index : bucket->second) {
                auto& t = obstacles[index];
                auto lo = glm::min(t.a, glm::min(t.b, t.c)), hi = glm::max(t.a, glm::max(t.b, t.c));
                if (hi.y < feet || lo.y > feet + bodyHeight || hi.x < center.x - radius ||
                    lo.x > center.x + radius || hi.z < center.z - radius ||
                    lo.z > center.z + radius)
                    continue;
                if (capsuleTriangleDistance2(lower, upper, t.a, t.b, t.c) <
                    (radius - .001f) * (radius - .001f))
                    return true;
            }
        }
    return false;
}
std::optional<float> GroundSurface::height(float x, float z) const {
    auto bucket = cells.find(key(int(std::floor(x / CellSize)), int(std::floor(z / CellSize))));
    if (bucket == cells.end())
        return std::nullopt;
    std::optional<float> best;
    for (size_t index : bucket->second) {
        auto& t = triangles[index];
        auto a = xz(t.a), b = xz(t.b), c = xz(t.c), p = glm::vec2(x, z);
        float determinant = cross(b - a, c - a);
        if (std::abs(determinant) < 1e-8f)
            continue;
        float u = cross(p - a, c - a) / determinant, v = cross(b - a, p - a) / determinant;
        if (u < -1e-5f || v < -1e-5f || u + v > 1.00001f)
            continue;
        float y = t.a.y + u * (t.b.y - t.a.y) + v * (t.c.y - t.a.y);
        if (!best || y > *best)
            best = y;
    }
    return best;
}
glm::vec3 GroundSurface::spawn(const glm::vec3& preferred) const {
    if (auto y = height(preferred.x, preferred.z)) {
        glm::vec3 p{preferred.x, *y + PlayerCenterAboveFeet + FeetClearance, preferred.z};
        if (!blocked(p))
            return p;
    }
    float distance = std::numeric_limits<float>::max();
    glm::vec3 best(0);
    auto consider = [&](glm::vec2 p, glm::vec2 center) {
        // Stay half a world unit inward from the mesh boundary where possible.
        auto inward = center - p;
        float length = glm::length(inward);
        if (length > 0)
            p += inward * (std::min(0.5f, length) / length);
        auto y = height(p.x, p.y);
        if (!y)
            return;
        glm::vec3 standing(p.x, *y + PlayerCenterAboveFeet + FeetClearance, p.y);
        if (blocked(standing))
            return;
        float d = glm::dot(p - xz(preferred), p - xz(preferred));
        if (d < distance) {
            distance = d;
            best = {p.x, *y + PlayerCenterAboveFeet + FeetClearance, p.y};
        }
    };
    for (auto& t : triangles) {
        glm::vec2 a = xz(t.a), b = xz(t.b), c = xz(t.c), center = (a + b + c) / 3.0f;
        consider(center, center);
        for (auto edge : {std::make_pair(a, b), std::make_pair(b, c), std::make_pair(c, a)}) {
            auto delta = edge.second - edge.first;
            float length2 = glm::dot(delta, delta);
            if (length2 < 1e-8f)
                continue;
            float f = glm::clamp(glm::dot(xz(preferred) - edge.first, delta) / length2, 0.0f, 1.0f);
            consider(edge.first + f * delta, center);
        }
    }
    if (!std::isfinite(distance))
        throw std::runtime_error("Cannot find a model-ground spawn");
    return best;
}
glm::vec3 GroundSurface::move(const glm::vec3& from, const glm::vec3& candidate) const {
    glm::vec3 result = from;
    glm::vec2 delta(candidate.x - from.x, candidate.z - from.z);
    // Substeps prevent sprinting through thin walls, branches or tree trunks.
    int steps = std::max(1, int(std::ceil(glm::length(delta) / 0.08f)));
    delta /= float(steps);
    for (int i = 0; i < steps; ++i) {
        auto fit = [&](glm::vec3 p) -> std::optional<glm::vec3> {
            auto y = height(p.x, p.z);
            if (!y)
                return std::nullopt;
            p.y = *y + PlayerCenterAboveFeet + FeetClearance;
            if (std::abs(p.y - result.y) > 0.5f || blocked(p))
                return std::nullopt;
            return p;
        };
        if (auto p = fit(result + glm::vec3(delta.x, 0, delta.y))) {
            result = *p;
            continue;
        }
        // Slide along walls instead of stopping all movement on contact.
        if (auto p = fit(result + glm::vec3(delta.x, 0, 0)))
            result = *p;
        if (auto p = fit(result + glm::vec3(0, 0, delta.y)))
            result = *p;
    }
    return result;
}
