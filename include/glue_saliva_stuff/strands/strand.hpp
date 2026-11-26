#pragma once

#include <cstddef>
#include <vector>
#include "glue_saliva_stuff/math/vec3.hpp"
#include "glue_saliva_stuff/geometry/mesh.hpp"

namespace gss::strands {

struct StrandPoint {
    math::Vec3 position{};
    float radius{0.05f};
};

struct Strand {
    std::vector<StrandPoint> points;
};

struct StrandMeshSettings {
    std::size_t radial_segments{8};
    bool cap_ends{true};
};

// Builds a tube along a strand's points. Radius is interpolated linearly
// between points; a later implementation can add taper profiles and RMF frames.
geometry::Mesh build_strand_mesh(const Strand& strand, const StrandMeshSettings& settings = {});

} // namespace gss::strands
