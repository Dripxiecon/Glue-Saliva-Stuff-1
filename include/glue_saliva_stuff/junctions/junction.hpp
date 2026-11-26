#pragma once

#include <cstddef>
#include <vector>
#include "glue_saliva_stuff/geometry/mesh.hpp"
#include "glue_saliva_stuff/math/vec3.hpp"
#include "glue_saliva_stuff/strands/strand.hpp"

namespace gss::junctions {

// Rounded hub used as the first building block for gooey strand connections.
// Branch tubes should begin inside the hub so their open ends are concealed.
struct Junction {
    math::Vec3 center{};
    float radius{0.1f};
};

struct JunctionMeshSettings {
    std::size_t radial_segments{12};
    std::size_t latitude_segments{8};
};

geometry::Mesh build_junction_mesh(
    const Junction& junction,
    const JunctionMeshSettings& settings = {});

// Produces one draw-ready mesh containing a hub and overlapping uncapped branch
// tubes. This reduces separate draw objects, but is an overlap assembly—not a
// watertight Boolean union. Branch control points should start inside the hub.
geometry::Mesh build_junction_assembly(
    const Junction& junction,
    const std::vector<strands::Strand>& branches,
    const JunctionMeshSettings& hub_settings = {},
    const strands::StrandMeshSettings& strand_settings = {});

} // namespace gss::junctions
