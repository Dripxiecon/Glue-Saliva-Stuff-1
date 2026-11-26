#pragma once

#include <cstdint>
#include <vector>
#include "glue_saliva_stuff/math/vec3.hpp"

namespace gss::geometry {

struct Triangle {
    std::uint32_t a{0};
    std::uint32_t b{0};
    std::uint32_t c{0};
};

struct Mesh {
    std::vector<math::Vec3> vertices;
    std::vector<Triangle> triangles;

    void clear() noexcept { vertices.clear(); triangles.clear(); }
    bool empty() const noexcept { return vertices.empty() || triangles.empty(); }
};

} // namespace gss::geometry
