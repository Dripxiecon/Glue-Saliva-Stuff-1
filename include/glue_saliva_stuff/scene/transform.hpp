#pragma once
#include "glue_saliva_stuff/math/mat4.hpp"
namespace gss::scene {
struct Transform {
    math::Vec3 position{};
    math::Vec3 rotation_radians{}; // Euler XYZ, radians.
    math::Vec3 scale{1.0f,1.0f,1.0f};
    math::Mat4 matrix() const noexcept {
        const float cx=std::cos(rotation_radians.x), sx=std::sin(rotation_radians.x);
        const float cy=std::cos(rotation_radians.y), sy=std::sin(rotation_radians.y);
        const float cz=std::cos(rotation_radians.z), sz=std::sin(rotation_radians.z);
        math::Mat4 t=math::Mat4::identity(), rx=t, ry=t, rz=t, s=t;
        t.at(0,3)=position.x; t.at(1,3)=position.y; t.at(2,3)=position.z;
        rx.at(1,1)=cx; rx.at(1,2)=-sx; rx.at(2,1)=sx; rx.at(2,2)=cx;
        ry.at(0,0)=cy; ry.at(0,2)=sy; ry.at(2,0)=-sy; ry.at(2,2)=cy;
        rz.at(0,0)=cz; rz.at(0,1)=-sz; rz.at(1,0)=sz; rz.at(1,1)=cz;
        s.at(0,0)=scale.x; s.at(1,1)=scale.y; s.at(2,2)=scale.z;
        return t*rz*ry*rx*s;
    }
};
}
