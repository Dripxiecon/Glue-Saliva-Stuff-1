#pragma once
#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>
#include "glue_saliva_stuff/render/render_backend.hpp"
#include "glue_saliva_stuff/scene/camera.hpp"
#include "glue_saliva_stuff/scene/transform.hpp"
#include "glue_saliva_stuff/strands/strand.hpp"

namespace gss::scene {
using ObjectHandle = std::uint64_t;

struct Object {
    ObjectHandle handle{0};
    geometry::Mesh mesh; // Shared mesh type consumed by every RenderBackend.
    Transform transform{};
    render::Material material{};
    bool visible{true};
};

class Scene {
public:
    Camera camera{};
    std::vector<Object> objects;

    // Direct strand-generator -> scene object connection.
    // The returned object's handle remains valid if objects vector reallocates.
    Object& add_strand(const strands::Strand& strand,
                       const strands::StrandMeshSettings& settings = {},
                       const Transform& transform = {},
                       const render::Material& material = {}) {
        Object object;
        object.handle = next_handle_++;
        object.mesh = strands::build_strand_mesh(strand, settings);
        object.transform = transform;
        object.material = material;
        objects.push_back(std::move(object));
        return objects.back();
    }

    // Regenerate the strand mesh in its existing object. Transform, material,
    // visibility, and mesh cache identity are preserved; mesh revision changes
    // so render backends refresh cached GPU data.
    bool update_strand(ObjectHandle handle,
                       const strands::Strand& strand,
                       const strands::StrandMeshSettings& settings = {}) {
        const auto it = std::find_if(objects.begin(), objects.end(),
            [handle](const Object& object) { return object.handle == handle; });
        if (it == objects.end()) return false;
        auto updated = strands::build_strand_mesh(strand, settings);
        it->mesh = std::move(updated);
        return true;
    }

    Object* find(ObjectHandle handle) noexcept {
        const auto it = std::find_if(objects.begin(), objects.end(),
            [handle](const Object& object) { return object.handle == handle; });
        return it == objects.end() ? nullptr : &*it;
    }

    const Object* find(ObjectHandle handle) const noexcept {
        const auto it = std::find_if(objects.begin(), objects.end(),
            [handle](const Object& object) { return object.handle == handle; });
        return it == objects.end() ? nullptr : &*it;
    }

    bool render(render::RenderBackend& backend, int width, int height) const {
        if (width <= 0 || height <= 0) return false;
        if (!backend.begin_frame(width, height,
                camera.view_projection(static_cast<float>(width) / static_cast<float>(height))))
            return false;
        for (const auto& object : objects)
            if (object.visible && !object.mesh.empty())
                backend.draw_mesh(object.mesh, object.transform.matrix(), object.material);
        backend.end_frame();
        return true;
    }

private:
    ObjectHandle next_handle_{1};
};
}
