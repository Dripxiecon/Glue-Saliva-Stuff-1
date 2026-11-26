#pragma once
#include <utility>
#include <vector>
#include "glue_saliva_stuff/render/render_backend.hpp"
#include "glue_saliva_stuff/scene/camera.hpp"
#include "glue_saliva_stuff/scene/transform.hpp"
#include "glue_saliva_stuff/strands/strand.hpp"

namespace gss::scene {
struct Object {
    geometry::Mesh mesh; // Shared mesh type consumed by every RenderBackend.
    Transform transform{};
    render::Material material{};
    bool visible{true};
};
class Scene {
public:
    Camera camera{};
    std::vector<Object> objects;

    // Direct strand-generator -> scene object connection. The generated
    // geometry::Mesh is stored unchanged and is immediately renderable.
    Object& add_strand(const strands::Strand& strand,
                       const strands::StrandMeshSettings& settings = {},
                       const Transform& transform = {},
                       const render::Material& material = {}) {
        Object object;
        object.mesh = strands::build_strand_mesh(strand,settings);
        object.transform = transform;
        object.material = material;
        objects.push_back(std::move(object));
        return objects.back();
    }

    bool render(render::RenderBackend& backend,int width,int height) const {
        if(width<=0 || height<=0) return false;
        if(!backend.begin_frame(width,height,camera.view_projection(static_cast<float>(width)/static_cast<float>(height)))) return false;
        for(const auto& object:objects) if(object.visible && !object.mesh.empty())
            backend.draw_mesh(object.mesh,object.transform.matrix(),object.material);
        backend.end_frame(); return true;
    }
};
}
