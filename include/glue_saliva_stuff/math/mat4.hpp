#pragma once

#include "glue_saliva_stuff/math/vec3.hpp"
#include <array>
#include <cmath>

namespace gss::math {
struct Mat4 {
    std::array<float,16> v{}; // Column-major: OpenGL / OpenGL ES layout.
    constexpr float& at(int row,int col) noexcept { return v[col*4+row]; }
    constexpr float at(int row,int col) const noexcept { return v[col*4+row]; }
    static constexpr Mat4 identity() noexcept { Mat4 m{}; m.at(0,0)=m.at(1,1)=m.at(2,2)=m.at(3,3)=1; return m; }
    static Mat4 perspective(float fov_y,float aspect,float near_z,float far_z) noexcept {
        Mat4 m{}; if(aspect<=0 || near_z<=0 || far_z<=near_z) return m;
        const float f=1.0f/std::tan(fov_y*0.5f); m.at(0,0)=f/aspect; m.at(1,1)=f;
        m.at(2,2)=(far_z+near_z)/(near_z-far_z); m.at(2,3)=(2*far_z*near_z)/(near_z-far_z);
        m.at(3,2)=-1; return m;
    }
    static Mat4 look_at(const Vec3& eye,const Vec3& target,const Vec3& up) noexcept {
        const Vec3 f=(target-eye).normalized(), s=f.cross(up).normalized(), u=s.cross(f);
        Mat4 m=identity();
        m.at(0,0)=s.x; m.at(0,1)=s.y; m.at(0,2)=s.z; m.at(0,3)=-s.dot(eye);
        m.at(1,0)=u.x; m.at(1,1)=u.y; m.at(1,2)=u.z; m.at(1,3)=-u.dot(eye);
        m.at(2,0)=-f.x; m.at(2,1)=-f.y; m.at(2,2)=-f.z; m.at(2,3)=f.dot(eye); return m;
    }
};
inline Mat4 operator*(const Mat4& a,const Mat4& b) noexcept {
    Mat4 o{}; for(int r=0;r<4;++r) for(int c=0;c<4;++c) for(int k=0;k<4;++k) o.at(r,c)+=a.at(r,k)*b.at(k,c); return o;
}
} // namespace gss::math
