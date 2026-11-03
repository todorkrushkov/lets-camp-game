#include "DoorTrigger.h"
#include "ForestWorld.h"
#include "GroundSurface.h"
#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_set>
#undef assert
#define assert(condition)                                                                          \
    do {                                                                                           \
        if (!(condition))                                                                          \
            throw std::runtime_error("Check failed: " #condition);                                 \
    } while (false)
bool foliage(const std::string& name) {
    return name == "1-1" || name == "Material.003" || name.find("leaf") != std::string::npos;
}
uint64_t triangleHash(const ModelMesh& mesh, size_t i) {
    uint64_t h = 1469598103934665603ull;
    for (int j = 0; j < 3; ++j) {
        auto& v = mesh.vertices[mesh.indices[i + j]];
        for (float f : {v.position.x, v.position.y, v.position.z, v.uv.x, v.uv.y}) {
            uint32_t bits;
            std::memcpy(&bits, &f, 4);
            h = (h ^ bits) * 1099511628211ull;
        }
    }
    return (h ^ uint64_t(mesh.material)) * 1099511628211ull;
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "Expected path to game_asset.glb\n";
            return 2;
        }
        auto begin = std::chrono::steady_clock::now();
        auto a = ModelAsset::load(argv[1]);
        std::unordered_set<uint64_t> authoredTriangles;
        std::map<int, std::pair<size_t, size_t>> authoredCounts;
        auto authoredMaterials = a.materials;
        for (auto& m : a.meshes)
            if (m.material >= 0 && foliage(a.materials[m.material].name)) {
                authoredCounts[m.material].first += m.vertices.size();
                authoredCounts[m.material].second += m.indices.size();
            }
        for (auto& m : a.meshes)
            if (m.material >= 0 && foliage(a.materials[m.material].name))
                for (size_t i = 0; i < m.indices.size(); i += 3)
                    authoredTriangles.insert(triangleHash(m, i));
        ForestWorld::arrange(a);
        std::map<int, std::pair<size_t, size_t>> restoredCounts;
        for (auto& m : a.meshes)
            if (m.material >= 0 && foliage(a.materials[m.material].name)) {
                restoredCounts[m.material].first += m.vertices.size();
                restoredCounts[m.material].second += m.indices.size();
            }
        size_t originalVertices = 0, reducedVertices = 0;
        for (auto& [id, counts] : authoredCounts) {
            originalVertices += counts.first;
            if (!restoredCounts.count(id)) {
                assert(counts.first < 1000);
                continue;
            }
            auto reduced = restoredCounts.at(id);
            reducedVertices += reduced.first;
            assert(reduced.first <= counts.first && reduced.second <= counts.second);
        }
        assert(reducedVertices < originalVertices / 10);
        assert(ForestWorld::detail(0) == ForestWorld::Detail::Near &&
               ForestWorld::detail(18) == ForestWorld::Detail::Medium);
        assert(ForestWorld::detail(36) == ForestWorld::Detail::Far &&
               ForestWorld::detail(65) == ForestWorld::Detail::Hidden);
        for (auto& mesh : a.meshes) {
            assert(mesh.farIndices.size() <= mesh.mediumIndices.size());
            assert(mesh.mediumIndices.size() <= mesh.indices.size());
            for (auto& level : {mesh.mediumIndices, mesh.farIndices}) {
                assert(level.size() % 3 == 0);
                for (auto index : level)
                    assert(index < mesh.vertices.size());
                if (mesh.material >= 0 && foliage(a.materials[mesh.material].name)) {
                    ModelMesh check;
                    check.material = mesh.material;
                    check.vertices = mesh.vertices;
                    check.indices = level;
                    for (size_t i = 0; i < level.size(); i += 3)
                        assert(authoredTriangles.count(triangleHash(check, i)));
                }
            }
        }
        std::cout << "Foliage vertices: " << originalVertices << " -> " << reducedVertices << " ("
                  << 100.0 * (1.0 - double(reducedVertices) / originalVertices)
                  << "% reduction); medium/far levels validated.\n";
        for (auto& [id, counts] : authoredCounts) {
            auto& before = authoredMaterials[id];
            auto& after = a.materials[id];
            if (before.name == "Material.003")
                assert(after.alphaMode == 1 && after.cutoff == .3f);
            else
                assert(before.alphaMode == after.alphaMode && before.cutoff == after.cutoff);
            assert(before.doubleSided == after.doubleSided && before.texture == after.texture);
        }
        // Every original foliage material survives; no generated replacement tree.
        for (auto& [id, counts] : authoredCounts)
            assert(restoredCounts.count(id));
        for (auto& material : a.materials)
            assert(material.name != "PineNeedles");
        std::cout << "PASS: all original tree foliage materials retained, no generated replacement "
                     "canopies.\n";
        int preservedTriangles = 0, grassChunks = 0;
        for (auto& m : a.meshes)
            if (m.material >= 0 && foliage(a.materials[m.material].name))
                for (size_t i = 0; i < m.indices.size(); i += 3) {
                    assert(authoredTriangles.count(triangleHash(m, i)));
                    ++preservedTriangles;
                }
        assert(preservedTriangles > 1000);
        for (auto& i : a.instances)
            if (i.group.rfind("grass/", 0) == 0) {
                ++grassChunks;
                assert(!i.collision && !i.portal);
                auto& mesh = a.meshes[i.mesh];
                assert(a.materials[mesh.material].grassBlade);
                for (auto& v : mesh.vertices) {
                    assert(v.position.y >= .005f && v.position.y < .5f);
                    assert(
                        !(std::abs(v.position.x) < 1.2f && v.position.z > -7 && v.position.z < 8));
                }
            }
        assert(grassChunks > 10);
        double red = 0, green = 0, blue = 0;
        auto& tile = a.images.back();
        for (size_t i = 0; i < tile.rgba.size(); i += 4) {
            red += tile.rgba[i];
            green += tile.rgba[i + 1];
            blue += tile.rgba[i + 2];
        }
        double pixels = tile.rgba.size() / 4;
        assert((green - red) / pixels < 20 && (green - blue) / pixels < 30);
        std::cout << "PASS: " << preservedTriangles
                  << " foliage triangles retain original positions and UVs; " << grassChunks
                  << " nonblocking grass chunks; muted ground color.\n";
        std::set<std::string> houses;
        std::map<int, std::set<std::string>> trees;
        size_t vertices = 0, triangles = 0;
        int cylinders = 0, grass = 0;
        for (auto& m : a.meshes) {
            vertices += m.vertices.size();
            triangles += m.indices.size() / 3;
            for (auto i : m.indices)
                assert(i < m.vertices.size());
        }
        for (auto& i : a.instances) {
            if (i.group.rfind("house/", 0) == 0)
                houses.insert(i.group);
            if (i.group.rfind("tree/", 0) == 0) {
                int type = i.group[5] - '0';
                trees[type].insert(i.group);
                assert(!i.collision && !i.portal);
            }
            if (i.group == "collision/tree")
                ++cylinders;
            if (i.group == "ground")
                ++grass;
        }
        assert(houses.size() == 5 && houses.count("house/small"));
        assert(cylinders == 120 && grass == 1 && trees.size() == 3);
        for (auto& [type, items] : trees)
            assert(items.size() == 40);
        GroundSurface ground;
        ground.build(a, glm::mat4(1));
        DoorTrigger door;
        door.build(a, glm::mat4(1));
        assert(door.count() == 1);
        auto spawn = ground.spawn(ForestWorld::spawn());
        assert(!ground.blocked(spawn));
        assert(!door.near(spawn));
        auto approach = spawn;
        for (int i = 0; i < 200 && !door.near(approach); ++i)
            approach = ground.move(approach, approach + glm::vec3(0, 0, -.08f));
        std::cout << "Straight approach: " << approach.x << "," << approach.y << "," << approach.z
                  << " nearDoor=" << door.near(approach) << "\n";
        assert(door.near(approach));
        int blocked = 0, reachable = 0;
        for (float x = -1.5; x < 1.5; x += .1f)
            for (float z = -10; z < -6; z += .1f) {
                auto h = ground.height(x, z);
                if (!h)
                    continue;
                glm::vec3 p(x, *h + .52f, z);
                if (door.near(p)) {
                    if (ground.blocked(p))
                        ++blocked;
                    else
                        ++reachable;
                }
            }
        assert(reachable > 0);
        assert(a.images.back().rgba.size() == 128 * 128 * 4);
        assert(a.materials.back().name == "GroundGrass");
        std::cout
            << "PASS: 5 houses (small x1, medium x2, large x2), 3 tree types x40, grass ground, "
               "120 trunk colliders, one portal, clear spawn and reachable small-house door.\n";
        std::cout << "Unique meshes " << a.meshes.size() << ", vertices " << vertices
                  << ", triangles " << triangles << ", instances " << a.instances.size()
                  << ", clear door samples " << reachable << ", processing seconds "
                  << std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count()
                  << "\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
