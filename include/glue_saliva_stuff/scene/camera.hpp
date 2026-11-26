#pragma once
#include "glue_saliva_stuff/math/mat4.hpp"
namespace gss::scene {
struct Camera {
    math::Vec3 position{0.0f,0.0f,4.0f};
    math::Vec3 target{0.0f,0.0f,0.0f};
    math::Vec3 up{0.0f,1.0f,0.0f};
    float vertical_fov_radians{1.04719755f};
    float near_plane{0.05f};
    float far_plane{250.0f};
    math::Mat4 view() const noexcept { return math::Mat4::look_at(position,target,up); }
    math::Mat4 projection(float aspect) const noexcept { return math::Mat4::perspective(vertical_fov_radians,aspect,near_plane,far_plane); }
    math::Mat4 view_projection(float aspect) const noexcept { return projection(aspect)*view(); }
};
}
