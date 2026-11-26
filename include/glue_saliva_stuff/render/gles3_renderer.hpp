#pragma once
#include "glue_saliva_stuff/render/render_backend.hpp"
#ifdef GSS_ENABLE_GLES3
#include <GLES3/gl3.h>
#include <cstdint>
#include <unordered_map>
namespace gss::render {
class Gles3Renderer final:public RenderBackend{
public:
 Gles3Renderer()=default;~Gles3Renderer()override;bool initialize();void shutdown()noexcept;
 bool begin_frame(int,int,const math::Mat4&)override;
 void draw_mesh(const geometry::Mesh&,const math::Mat4&,const Material&)override;void end_frame()override;
private:
 struct CachedMesh{GLuint vao{0},vbo{0},ebo{0};GLsizei count{0};std::uint64_t revision{0},last_seen{0};};
 void destroy(CachedMesh&)noexcept;
 GLuint program_{0};GLint mvp_{-1},model_{-1},color_{-1},opacity_{-1};
 math::Mat4 vp_{math::Mat4::identity()};std::unordered_map<std::uint64_t,CachedMesh> cache_;std::uint64_t frame_{0};
};}
#endif