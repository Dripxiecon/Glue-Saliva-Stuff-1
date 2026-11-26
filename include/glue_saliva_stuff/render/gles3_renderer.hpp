#pragma once

#include "glue_saliva_stuff/render/render_backend.hpp"

#ifdef GSS_ENABLE_GLES3
#include <GLES3/gl3.h>
namespace gss::render {
class Gles3Renderer final : public RenderBackend {
public:
    Gles3Renderer() = default;
    ~Gles3Renderer() override;
    bool initialize();
    void shutdown() noexcept;
    bool begin_frame(int width,int height,const math::Mat4& view_projection) override;
    void draw_mesh(const geometry::Mesh& mesh,const math::Mat4& model,const Material& material) override;
    void end_frame() override;
private:
    GLuint program_{0};
    GLint mvp_location_{-1};
    GLint model_location_{-1};
    GLint color_location_{-1};
    math::Mat4 view_projection_{math::Mat4::identity()};
};
}
#endif
