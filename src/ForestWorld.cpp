#include "ForestWorld.h"
#include "meshoptimizer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace {
struct Bounds {
    glm::vec3 lo{std::numeric_limits<float>::max()}, hi{-std::numeric_limits<float>::max()};
    void add(glm::vec3 p) {
        lo = glm::min(lo, p);
        hi = glm::max(hi, p);
    }
    glm::vec3 center() const {
        return {(lo.x + hi.x) * .5f, lo.y, (lo.z + hi.z) * .5f};
    }
};
struct Prototype {
    std::vector<ModelInstance> parts;
    Bounds bounds;
    glm::vec2 trunk{0};
    float radius = .3f;
};
bool groundMaterial(const std::string& name) {
    return name == "ground" || name == "Ground";
}
// Edge collapses follow the original connected surface; no spatial clustering of
// unrelated twigs. Keep original vertex positions/UVs and bound geometric error.
void simplifyWood(ModelMesh& mesh) {
    if (mesh.indices.empty())
        return;
    auto reduce = [&](float ratio, float error) {
        std::vector<uint32_t> out(mesh.indices.size());
        const float weights[] = {.02f, .02f};
        size_t count = meshopt_simplifyWithAttributes(
            out.data(), mesh.indices.data(), mesh.indices.size(), &mesh.vertices[0].position.x,
            mesh.vertices.size(), sizeof(ModelVertex), &mesh.vertices[0].uv.x, sizeof(ModelVertex),
            weights, 2, nullptr, std::max(size_t(12), size_t(mesh.indices.size() * ratio) / 3 * 3),
            error, meshopt_SimplifyPermissive, nullptr);
        out.resize(count);
        return out;
    };
    auto near = reduce(.035f, .001f);
    // Compact shared vertices once; distance levels use the same authored mesh.
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> remap(mesh.vertices.size(), UINT32_MAX);
    for (auto& i : near) {
        if (remap[i] == UINT32_MAX) {
            remap[i] = uint32_t(vertices.size());
            vertices.push_back(mesh.vertices[i]);
        }
        i = remap[i];
    }
    mesh.vertices = std::move(vertices);
    mesh.indices = std::move(near);
    mesh.mediumIndices = reduce(.5f, .002f);
    mesh.farIndices = reduce(.2f, .004f);
}
// Distance levels retain complete original leaves/twigs and their authored UVs.
std::vector<uint32_t> componentIndices(const ModelMesh& mesh, size_t budget) {
    if (mesh.vertices.size() <= budget)
        return mesh.indices;
    std::vector<uint32_t> parent(mesh.vertices.size()), counts(mesh.vertices.size(), 0);
    std::iota(parent.begin(), parent.end(), 0);
    auto root = [&](uint32_t v) {
        while (parent[v] != v) {
            parent[v] = parent[parent[v]];
            v = parent[v];
        }
        return v;
    };
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        auto r = root(mesh.indices[i]);
        parent[root(mesh.indices[i + 1])] = r;
        parent[root(mesh.indices[i + 2])] = r;
    }
    for (uint32_t i = 0; i < parent.size(); ++i) {
        parent[i] = root(i);
        ++counts[parent[i]];
    }
    auto hash = [](uint32_t v) {
        v ^= v >> 16;
        v *= 0x7feb352d;
        v ^= v >> 15;
        v *= 0x846ca68b;
        v ^= v >> 16;
        return v;
    };
    std::vector<uint32_t> components;
    for (uint32_t i = 0; i < counts.size(); ++i)
        if (counts[i])
            components.push_back(i);
    std::sort(components.begin(), components.end(),
              [&](auto a, auto b) { return hash(a) < hash(b); });
    std::vector<bool> keep(parent.size(), false);
    size_t used = 0;
    for (auto r : components)
        if (used + counts[r] <= budget) {
            keep[r] = true;
            used += counts[r];
        }
    if (!used)
        return mesh.indices;
    std::vector<uint32_t> indices;
    indices.reserve(used * 3);
    for (size_t i = 0; i < mesh.indices.size(); i += 3)
        if (keep[parent[mesh.indices[i]]])
            indices.insert(indices.end(),
                           {mesh.indices[i], mesh.indices[i + 1], mesh.indices[i + 2]});
    return indices;
}
void lightweightCanopy(ModelMesh& mesh, size_t budget) {
    auto indices = componentIndices(mesh, budget);
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> remap(mesh.vertices.size(), std::numeric_limits<uint32_t>::max());
    for (auto& i : indices) {
        if (remap[i] == std::numeric_limits<uint32_t>::max()) {
            remap[i] = uint32_t(vertices.size());
            vertices.push_back(mesh.vertices[i]);
        }
        i = remap[i];
    }
    mesh.vertices = std::move(vertices);
    mesh.indices = std::move(indices);
    mesh.mediumIndices = componentIndices(mesh, std::max(size_t(32), budget / 3));
    mesh.farIndices = componentIndices(mesh, std::max(size_t(16), budget / 12));
}
void addGrassTufts(ModelAsset& asset) {
    std::map<std::string, Bounds> houseBounds;
    for (auto& part : asset.instances)
        if (part.group.rfind("house/", 0) == 0) {
            auto& bounds = houseBounds[part.group];
            for (auto& v : asset.meshes[part.mesh].vertices)
                bounds.add(glm::vec3(part.transform * glm::vec4(v.position, 1)));
        }
    ModelMaterial material;
    material.name = "GrassBlades";
    material.doubleSided = true;
    material.grassBlade = true;
    int materialIndex = int(asset.materials.size());
    asset.materials.push_back(material);
    std::array<ModelMesh, 64> chunks;
    for (auto& mesh : chunks)
        mesh.material = materialIndex;
    std::mt19937 random(6102);
    std::uniform_real_distribution<float> unit(0, 1);
    int tufts = 0;
    for (int attempt = 0; tufts < 6000 && attempt < 50000; ++attempt) {
        float x = unit(random) * 122 - 61, z = unit(random) * 122 - 61;
        if (std::abs(x) < 1.5f && z > -13 && z < 10)
            continue;
        if (glm::length(glm::vec2(x, z - 5)) < 2.5f)
            continue;
        bool excluded = false;
        for (auto& [name, b] : houseBounds)
            if (x > b.lo.x - .6f && x < b.hi.x + .6f && z > b.lo.z - .6f && z < b.hi.z + .6f) {
                excluded = true;
                break;
            }
        if (excluded)
            continue;
        // Sparse, shorter tufts in the clearing; denser growth in the forest.
        float radius = std::sqrt(x * x + z * z);
        if (radius < 22 && unit(random) > .3f)
            continue;
        int cx = glm::clamp(int((x + 64) / 16), 0, 7), cz = glm::clamp(int((z + 64) / 16), 0, 7);
        auto& mesh = chunks[cz * 8 + cx];
        for (int blade = 0; blade < 5; ++blade) {
            float angle = unit(random) * 6.2831853f;
            glm::vec3 across(std::cos(angle), 0, std::sin(angle)), lean(-across.z, 0, across.x);
            float height = (.13f + unit(random) * .26f) * (radius < 22 ? .75f : 1.0f),
                  width = .015f + unit(random) * .016f;
            glm::vec3 base(x + (unit(random) - .5f) * .18f, .006f, z + (unit(random) - .5f) * .18f);
            glm::vec3 middle = base + glm::vec3(0, height * .58f, 0) + lean * height * .12f;
            glm::vec3 tip = base + glm::vec3(0, height, 0) + lean * height * .38f;
            glm::vec3 points[] = {base - across * width, base + across * width,
                                  middle - across * width * .6f, middle + across * width * .6f,
                                  tip};
            float factors[] = {0, 0, .58f, .58f, 1};
            uint32_t index = uint32_t(mesh.vertices.size());
            auto normal = glm::normalize(glm::cross(points[1] - points[0], tip - points[0]));
            float tint = .85f + unit(random) * .3f;
            for (int i = 0; i < 5; ++i) {
                ModelVertex v{};
                v.position = points[i];
                v.normal = normal;
                v.uv = v.uv1 = {float(i % 2), factors[i]};
                v.color = glm::vec4(glm::mix(glm::vec3(.035f, .048f, .024f),
                                             glm::vec3(.105f, .135f, .072f), factors[i]) *
                                        tint,
                                    1);
                mesh.vertices.push_back(v);
            }
            mesh.indices.insert(mesh.indices.end(),
                                {index, index + 1, index + 2, index + 1, index + 3, index + 2,
                                 index + 2, index + 3, index + 4});
        }
        ++tufts;
    }
    for (size_t i = 0; i < chunks.size(); ++i)
        if (!chunks[i].indices.empty()) {
            asset.instances.push_back(
                {asset.meshes.size(), glm::mat4(1), "grass/" + std::to_string(i), false, false});
            asset.meshes.push_back(std::move(chunks[i]));
        }
    std::cout << "Added " << tufts << " nonblocking grass tufts.\n";
}
void addCylinder(ModelAsset& asset) {
    ModelMaterial m;
    m.name = "SolidTreeTrunk";
    int material = int(asset.materials.size());
    asset.materials.push_back(m);
    ModelMesh mesh;
    mesh.material = material;
    mesh.visible = false;
    for (int i = 0; i < 12; ++i) {
        float a = i * 6.2831853f / 12, b = (i + 1) * 6.2831853f / 12;
        glm::vec3 p(std::cos(a), 0, std::sin(a)), q(std::cos(b), 0, std::sin(b));
        uint32_t base = uint32_t(mesh.vertices.size());
        for (auto v : {p, q, q + glm::vec3(0, 1, 0), p + glm::vec3(0, 1, 0)}) {
            ModelVertex vertex{};
            vertex.position = v;
            vertex.normal = {v.x, 0, v.z};
            mesh.vertices.push_back(vertex);
        }
        mesh.indices.insert(mesh.indices.end(),
                            {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    asset.meshes.push_back(std::move(mesh));
}
void addGrass(ModelAsset& asset) {
    // Code-generated seamless grass tile; no dependence on the old tile atlas.
    ModelImage image;
    image.width = image.height = 128;
    image.rgba.resize(128 * 128 * 4);
    std::mt19937 rng(81);
    std::uniform_int_distribution<int> noise(-8, 8);
    for (int y = 0; y < 128; ++y)
        for (int x = 0; x < 128; ++x) {
            int n = noise(rng);
            bool blade = ((x * 13 + y * 7) % 41 < 2);
            size_t i = (y * 128 + x) * 4;
            image.rgba[i] = static_cast<unsigned char>(74 + n + (blade ? 7 : 0));
            image.rgba[i + 1] = static_cast<unsigned char>(87 + n + (blade ? 8 : 0));
            image.rgba[i + 2] = static_cast<unsigned char>(63 + n / 2 + (blade ? 5 : 0));
            image.rgba[i + 3] = 255;
        }
    ModelTexture texture;
    texture.image = int(asset.images.size());
    asset.images.push_back(std::move(image));
    ModelMaterial material;
    material.name = "GroundGrass";
    material.texture = int(asset.textures.size());
    material.doubleSided = true;
    asset.textures.push_back(texture);
    ModelMesh mesh;
    mesh.material = int(asset.materials.size());
    asset.materials.push_back(material);
    const glm::vec3 points[] = {{-64, 0, -64}, {-64, 0, 64}, {64, 0, -64}, {64, 0, 64}};
    const glm::vec2 uv[] = {{0, 0}, {0, 64}, {64, 0}, {64, 64}};
    for (int i = 0; i < 4; ++i) {
        ModelVertex v{};
        v.position = points[i];
        v.normal = {0, 1, 0};
        v.uv = v.uv1 = uv[i];
        mesh.vertices.push_back(v);
    }
    mesh.indices = {0, 1, 2, 2, 1, 3};
    asset.instances.push_back({asset.meshes.size(), glm::mat4(1), "ground", true, false});
    asset.meshes.push_back(std::move(mesh));
}
} // namespace
void ForestWorld::arrange(ModelAsset& asset) {
    std::map<std::string, Prototype> groups;
    for (auto& part : asset.instances) {
        auto& group = groups[part.group];
        group.parts.push_back(part);
        auto& mesh = asset.meshes[part.mesh];
        for (auto& vertex : mesh.vertices)
            group.bounds.add(glm::vec3(part.transform * glm::vec4(vertex.position, 1)));
    }
    std::vector<std::string> houses, trees;
    for (auto& [name, prototype] : groups) {
        if (name.find("house") != std::string::npos)
            houses.push_back(name);
        else
            trees.push_back(name);
    }
    if (houses.size() != 3 || trees.size() != 3)
        throw std::runtime_error("game_asset.glb must have three house and three tree root groups");
    std::sort(houses.begin(), houses.end(), [&](auto& a, auto& b) {
        auto ea = groups[a].bounds.hi - groups[a].bounds.lo,
             eb = groups[b].bounds.hi - groups[b].bounds.lo;
        return ea.x * ea.y * ea.z < eb.x * eb.y * eb.z;
    });
    std::sort(trees.begin(), trees.end());
    std::unordered_set<size_t> treeMeshes;
    size_t before = 0, after = 0;
    for (auto& name : trees) {
        auto& prototype = groups[name];
        glm::vec2 sum(0);
        size_t count = 0;
        std::vector<glm::vec2> basePoints;
        float low = prototype.bounds.lo.y + (prototype.bounds.hi.y - prototype.bounds.lo.y) * .08f;
        for (auto& part : prototype.parts) {
            auto& mesh = asset.meshes[part.mesh];
            std::string material = mesh.material < 0 ? "" : asset.materials[mesh.material].name;
            if (groundMaterial(material))
                continue;
            treeMeshes.insert(part.mesh);
            bool bark = material.find("Bark") != std::string::npos ||
                        material.find("bark") != std::string::npos ||
                        material.find("trunk") != std::string::npos;
            if (bark)
                for (auto& vertex : mesh.vertices) {
                    auto p = glm::vec3(part.transform * glm::vec4(vertex.position, 1));
                    if (p.y < low) {
                        glm::vec2 q(p.x, p.z);
                        sum += q;
                        ++count;
                        basePoints.push_back(q);
                    }
                }
        }
        auto center = prototype.bounds.center();
        prototype.trunk = count ? sum / float(count) : glm::vec2(center.x, center.z);
        std::vector<float> radii;
        for (auto q : basePoints)
            radii.push_back(glm::length(q - prototype.trunk));
        if (!radii.empty()) {
            std::nth_element(radii.begin(), radii.begin() + radii.size() * 3 / 4, radii.end());
            prototype.radius = radii[radii.size() * 3 / 4];
        }
    }
    for (auto id : treeMeshes) {
        auto& mesh = asset.meshes[id];
        before += mesh.vertices.size();
        std::string name = mesh.material < 0 ? "" : asset.materials[mesh.material].name;
        bool leaf =
            name.find("leaf") != std::string::npos || name == "1-1" || name == "Material.003";
        if (leaf)
            lightweightCanopy(mesh, 48000);
        else
            simplifyWood(mesh);
        after += mesh.vertices.size();
    }
    for (auto& name : trees)
        groups[name].radius = glm::clamp(groups[name].radius, .16f, .4f);
    // The source RGB leaf scan has a solid background instead of exported alpha.
    for (auto& material : asset.materials)
        if (material.name == "Material.003") {
            material.alphaMode = 1;
            material.cutoff = .3f;
        }
    asset.instances.clear();
    auto addHouse = [&](const std::string& prototypeName, glm::vec2 at, const std::string& label,
                        bool entry) {
        auto& prototype = groups[prototypeName];
        float yaw = std::atan2(-at.x, -at.y);
        auto base = glm::translate(glm::mat4(1), glm::vec3(at.x, .02f, at.y)) *
                    glm::rotate(glm::mat4(1), yaw, glm::vec3(0, 1, 0)) *
                    glm::scale(glm::mat4(1), glm::vec3(.9f)) *
                    glm::translate(glm::mat4(1), -prototype.bounds.center());
        for (auto part : prototype.parts) {
            part.transform = base * part.transform;
            part.group = label;
            part.collision = true;
            part.portal = entry;
            asset.instances.push_back(part);
        }
    };
    addHouse(houses[0], {0, -11}, "house/small", true);
    addHouse(houses[1], {-13, 7}, "house/medium/0", false);
    addHouse(houses[1], {13, 7}, "house/medium/1", false);
    addHouse(houses[2], {-13, -9}, "house/large/0", false);
    addHouse(houses[2], {13, -9}, "house/large/1", false);
    addCylinder(asset);
    size_t collider = asset.meshes.size() - 1;
    std::mt19937 random(20261102);
    std::uniform_real_distribution<float> unit(0, 1);
    std::vector<glm::vec2> placed;
    for (int i = 0; i < 120; ++i) {
        glm::vec2 at;
        bool found = false;
        for (int tries = 0; tries < 1000; ++tries) {
            float angle = unit(random) * 6.2831853f,
                  radius = std::sqrt(26 * 26 + unit(random) * (54 * 54 - 26 * 26));
            at = {std::sin(angle) * radius, std::cos(angle) * radius};
            found = true;
            for (auto old : placed)
                if (glm::length(old - at) < 4.0f) {
                    found = false;
                    break;
                }
            if (found)
                break;
        }
        if (!found)
            throw std::runtime_error("Cannot place forest without overlapping trunks");
        placed.push_back(at);
        int type = i % 3;
        auto& prototype = groups[trees[type]];
        float height = 10.0f + unit(random) * 3.5f,
              scale = height / (prototype.bounds.hi.y - prototype.bounds.lo.y),
              yaw = unit(random) * 6.2831853f;
        auto base = glm::translate(glm::mat4(1), glm::vec3(at.x, .01f, at.y)) *
                    glm::rotate(glm::mat4(1), yaw, glm::vec3(0, 1, 0)) *
                    glm::scale(glm::mat4(1), glm::vec3(scale)) *
                    glm::translate(glm::mat4(1), -prototype.bounds.center());
        for (auto part : prototype.parts) {
            auto& mesh = asset.meshes[part.mesh];
            if (mesh.material >= 0 && groundMaterial(asset.materials[mesh.material].name))
                continue;
            part.transform = base * part.transform;
            part.group = "tree/" + std::to_string(type) + "/" + std::to_string(i);
            part.lodOrigin = {at.x, .01f, at.y};
            part.collision = false;
            part.portal = false;
            asset.instances.push_back(part);
        }
        auto center = glm::vec3(
            base * glm::vec4(prototype.trunk.x, prototype.bounds.lo.y, prototype.trunk.y, 1));
        float radius = glm::clamp(prototype.radius * scale, .22f, .55f);
        auto transform = glm::translate(glm::mat4(1), center) *
                         glm::scale(glm::mat4(1), glm::vec3(radius, 3.0f, radius));
        asset.instances.push_back({collider, transform, "collision/tree", true, false});
    }
    addGrassTufts(asset);
    addGrass(asset);
    // Keep only mesh data actually used by the generated scene.
    std::vector<ModelMesh> compact;
    std::map<size_t, size_t> remap;
    for (auto& part : asset.instances) {
        auto it = remap.find(part.mesh);
        if (it == remap.end()) {
            size_t id = compact.size();
            remap[part.mesh] = id;
            compact.push_back(std::move(asset.meshes[part.mesh]));
            part.mesh = id;
        } else
            part.mesh = it->second;
    }
    asset.meshes = std::move(compact);
    asset.minimum = {-64, 0, -64};
    asset.maximum = {64, 18, 64};
    std::cout
        << "Forest village: 5 houses (one small entrance), 120 trees (40 per type); tree vertices "
        << before << " -> " << after << ".\n";
}
