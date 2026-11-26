#pragma once
#include "glue_saliva_stuff/geometry/mesh.hpp"
#include "glue_saliva_stuff/math/mat4.hpp"
namespace gss::render {
struct Material {
    math::Vec3 base_color{0.72f,0.82f,0.95f};
    float roughness{0.35f};
    float opacity{1.0f};
};
class RenderBackend {
public:
    virtual ~RenderBackend()=default;
    // Host owns and activates its graphics context before rendering.
    virtual bool begin_frame(int width,int height,const math::Mat4& view_projection)=0;
    virtual void draw_mesh(const geometry::Mesh& mesh,const math::Mat4& model,const Material& material)=0;
    virtual void end_frame()=0;
};
}
