#include "ModelAsset.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include <cmath>
#include <cstring>
#include <functional>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <memory>
#include <stdexcept>
namespace {
glm::mat3 uvMatrix(const cgltf_texture_view& view) {
    int uv =
        view.has_transform && view.transform.has_texcoord ? view.transform.texcoord : view.texcoord;
    if (view.texture && uv != 0)
        throw std::runtime_error("Only UV set zero is supported");
    glm::mat3 m(1);
    if (view.has_transform) {
        auto& t = view.transform;
        float c = std::cos(t.rotation), s = std::sin(t.rotation);
        m[0] = {c * t.scale[0], s * t.scale[0], 0};
        m[1] = {-s * t.scale[1], c * t.scale[1], 0};
        m[2] = {t.offset[0], t.offset[1], 1};
    }
    return m;
}
} // namespace
ModelAsset ModelAsset::load(const std::string& path) {
    cgltf_options options{};
    cgltf_data* raw = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &raw) != cgltf_result_success)
        throw std::runtime_error("Cannot parse GLB: " + path);
    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(raw, cgltf_free);
    if (cgltf_load_buffers(&options, raw, path.c_str()) != cgltf_result_success ||
        cgltf_validate(raw) != cgltf_result_success)
        throw std::runtime_error("Invalid GLB buffers: " + path);
    if (raw->skins_count || raw->animations_count)
        throw std::runtime_error(
            "This renderer supports static models; animated GLB not yet supported");
    for (size_t i = 0; i < raw->extensions_required_count; ++i)
        if (std::strcmp(raw->extensions_required[i], "KHR_texture_transform") != 0)
            throw std::runtime_error(std::string("Unsupported required GLB extension: ") +
                                     raw->extensions_required[i]);
    ModelAsset result;
    for (size_t i = 0; i < raw->images_count; ++i) {
        auto& image = raw->images[i];
        if (!image.buffer_view)
            throw std::runtime_error("Model requires embedded GLB images");
        auto bytes = cgltf_buffer_view_data(image.buffer_view);
        if (!bytes)
            throw std::runtime_error("Missing embedded image");
        ModelImage embedded;
        embedded.encoded.assign(bytes, bytes + image.buffer_view->size);
        result.images.push_back(std::move(embedded));
    }
    for (size_t i = 0; i < raw->textures_count; ++i) {
        auto& t = raw->textures[i];
        ModelTexture out;
        if (!t.image)
            throw std::runtime_error("Unsupported compressed model texture");
        out.image = int(t.image - raw->images);
        if (t.sampler) {
            out.wrapS = t.sampler->wrap_s;
            out.wrapT = t.sampler->wrap_t;
            if (t.sampler->min_filter)
                out.minFilter = t.sampler->min_filter;
            if (t.sampler->mag_filter)
                out.magFilter = t.sampler->mag_filter;
        }
        result.textures.push_back(out);
    }
    for (size_t i = 0; i < raw->materials_count; ++i) {
        auto& m = raw->materials[i];
        ModelMaterial out;
        out.name = m.name ? m.name : "";
        if (m.has_pbr_metallic_roughness) {
            auto& base = m.pbr_metallic_roughness;
            out.color = glm::make_vec4(base.base_color_factor);
            out.uv = uvMatrix(base.base_color_texture);
            if (base.base_color_texture.texture)
                out.texture = int(base.base_color_texture.texture - raw->textures);
        }
        out.emissive = glm::make_vec3(m.emissive_factor) *
                       (m.has_emissive_strength ? m.emissive_strength.emissive_strength : 1.0f);
        out.emissiveUV = uvMatrix(m.emissive_texture);
        if (m.emissive_texture.texture)
            out.emissiveTexture = int(m.emissive_texture.texture - raw->textures);
        out.alphaMode = int(m.alpha_mode);
        out.cutoff = m.alpha_cutoff;
        out.doubleSided = m.double_sided;
        out.unlit = m.unlit;
        // This Blender procedural material exports neither texture nor leaf shape.
        // Only substitute its missing appearance; authored textured materials stay intact.
        out.proceduralLeaf = out.name == "SimpleProceduralLeaf" && out.texture < 0 &&
                             glm::length(glm::vec3(out.color) - glm::vec3(1)) < 0.001f;
        result.materials.push_back(out);
    }
    std::vector<std::vector<size_t>> meshIndices(raw->meshes_count);
    for (size_t mi = 0; mi < raw->meshes_count; ++mi) {
        auto& mesh = raw->meshes[mi];
        for (size_t pi = 0; pi < mesh.primitives_count; ++pi) {
            auto& primitive = mesh.primitives[pi];
            if (primitive.type != cgltf_primitive_type_triangles ||
                primitive.has_draco_mesh_compression || primitive.targets_count)
                throw std::runtime_error(
                    "Unsupported model primitive, compression or morph target");
            const cgltf_accessor *position = nullptr, *normal = nullptr, *uv = nullptr,
                                 *uv1 = nullptr, *color = nullptr;
            for (size_t ai = 0; ai < primitive.attributes_count; ++ai) {
                auto& a = primitive.attributes[ai];
                if (a.type == cgltf_attribute_type_position)
                    position = a.data;
                if (a.type == cgltf_attribute_type_normal)
                    normal = a.data;
                if (a.type == cgltf_attribute_type_texcoord && a.index == 0)
                    uv = a.data;
                if (a.type == cgltf_attribute_type_texcoord && a.index == 1)
                    uv1 = a.data;
                if (a.type == cgltf_attribute_type_color && a.index == 0)
                    color = a.data;
            }
            if (!position || !position->count)
                throw std::runtime_error("Missing model positions");
            ModelMesh out;
            out.material = primitive.material ? int(primitive.material - raw->materials) : -1;
            out.vertices.resize(position->count);
            for (size_t i = 0; i < position->count; ++i) {
                auto& v = out.vertices[i];
                if (!cgltf_accessor_read_float(position, i, &v.position.x, 3))
                    throw std::runtime_error("Invalid model vertex");
                v.normal = {0, 1, 0};
                v.uv = {0, 0};
                if (normal && !cgltf_accessor_read_float(normal, i, &v.normal.x, 3))
                    throw std::runtime_error("Invalid normals");
                if (uv && !cgltf_accessor_read_float(uv, i, &v.uv.x, 2))
                    throw std::runtime_error("Invalid texture coordinates");
                v.uv1 = v.uv;
                if (uv1 && !cgltf_accessor_read_float(uv1, i, &v.uv1.x, 2))
                    throw std::runtime_error("Invalid secondary texture coordinates");
                if (color && !cgltf_accessor_read_float(color, i, &v.color.x, 4))
                    throw std::runtime_error("Invalid vertex colors");
            }
            size_t count = primitive.indices ? primitive.indices->count : position->count;
            if (count % 3)
                throw std::runtime_error("Invalid triangle index count");
            out.indices.resize(count);
            for (size_t i = 0; i < count; ++i) {
                size_t index =
                    primitive.indices ? cgltf_accessor_read_index(primitive.indices, i) : i;
                if (index >= out.vertices.size())
                    throw std::runtime_error("Model index outside vertex buffer");
                out.indices[i] = uint32_t(index);
            }
            if (!normal) {
                for (auto& v : out.vertices)
                    v.normal = {0, 0, 0};
                for (size_t i = 0; i < count; i += 3) {
                    auto& a = out.vertices[out.indices[i]];
                    auto& b = out.vertices[out.indices[i + 1]];
                    auto& c = out.vertices[out.indices[i + 2]];
                    auto n = glm::cross(b.position - a.position, c.position - a.position);
                    a.normal += n;
                    b.normal += n;
                    c.normal += n;
                }
                for (auto& v : out.vertices)
                    v.normal =
                        glm::length(v.normal) > 0 ? glm::normalize(v.normal) : glm::vec3(0, 1, 0);
            }
            meshIndices[mi].push_back(result.meshes.size());
            result.meshes.push_back(std::move(out));
        }
    }
    result.minimum = glm::vec3(std::numeric_limits<float>::max());
    result.maximum = -result.minimum;
    std::function<void(cgltf_node*, const std::string&)> visit = [&](cgltf_node* node,
                                                                     const std::string& group) {
        if (node->mesh) {
            float matrix[16];
            cgltf_node_transform_world(node, matrix);
            auto transform = glm::make_mat4(matrix);
            if (std::abs(glm::determinant(glm::mat3(transform))) < 1e-12f)
                throw std::runtime_error("Singular model node transform");
            for (auto index : meshIndices[size_t(node->mesh - raw->meshes)]) {
                result.instances.push_back({index, transform, group});
                for (auto& vertex : result.meshes[index].vertices) {
                    auto world = glm::vec3(transform * glm::vec4(vertex.position, 1));
                    for (int axis = 0; axis < 3; ++axis)
                        if (!std::isfinite(world[axis]))
                            throw std::runtime_error("Non-finite model geometry");
                    result.minimum = glm::min(result.minimum, world);
                    result.maximum = glm::max(result.maximum, world);
                }
            }
        }
        for (size_t i = 0; i < node->children_count; ++i)
            visit(node->children[i], group);
    };
    auto scene = raw->scene ? raw->scene : (raw->scenes_count ? raw->scenes : nullptr);
    if (!scene)
        throw std::runtime_error("GLB has no scene");
    for (size_t i = 0; i < scene->nodes_count; ++i)
        visit(scene->nodes[i], scene->nodes[i]->name ? scene->nodes[i]->name : "");
    if (result.instances.empty())
        throw std::runtime_error("GLB scene is empty");
    return result;
}
