#pragma once

#include <cstddef>
#include "glue_saliva_stuff/geometry/mesh.hpp"
#include "glue_saliva_stuff/math/vec3.hpp"

namespace gss::junctions {

// A bounded, rounded hub used as the first building block for gooey strand
// connections. Strand ends can overlap this hub for a lightweight joined look.
struct Junction {
    math::Vec3 center{};
    float radius{0.1f};
};

struct JunctionMeshSettings {
    std::size_t radial_segments{12};
    std::size_t latitude_segments{8};
};

// Builds a UV-sphere hub. Invalid values and excessive tessellation return an
// empty mesh; callers may combine it with strand meshes using the same material.
geometry::Mesh build_junction_mesh(
    const Junction& junction,
    const JunctionMeshSettings& settings = {});

} // namespace gss::junctions
