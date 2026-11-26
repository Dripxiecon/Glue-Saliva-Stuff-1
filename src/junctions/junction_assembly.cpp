#include "glue_saliva_stuff/junctions/junction.hpp"

#include <cstdint>
#include <limits>

namespace gss::junctions {
namespace {
constexpr std::size_t kMaxAssemblyVertices = 1'000'000;

bool append_mesh(geometry::Mesh& destination, const geometry::Mesh& source) {
    if (source.empty() || destination.vertices.size() > kMaxAssemblyVertices ||
        source.vertices.size() > kMaxAssemblyVertices - destination.vertices.size())
        return false;
    if (destination.vertices.size() >
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) -
            source.vertices.size())
        return false;

    if (source.triangles.size() >
        std::numeric_limits<std::size_t>::max() - destination.triangles.size())
        return false;

    const auto base = static_cast<std::uint32_t>(destination.vertices.size());
    destination.vertices.insert(destination.vertices.end(),
                               source.vertices.begin(), source.vertices.end());
    destination.triangles.reserve(destination.triangles.size() + source.triangles.size());
    for (const auto& triangle : source.triangles) {
        destination.triangles.push_back({
            static_cast<std::uint32_t>(base + triangle.a),
            static_cast<std::uint32_t>(base + triangle.b),
            static_cast<std::uint32_t>(base + triangle.c)
        });
    }
    return true;
}
} // namespace

geometry::Mesh build_junction_assembly(
    const Junction& junction,
    const std::vector<strands::Strand>& branches,
    const JunctionMeshSettings& hub_settings,
    const strands::StrandMeshSettings& strand_settings) {
    auto result = build_junction_mesh(junction, hub_settings);
    if (result.empty()) return {};

    auto open_settings = strand_settings;
    open_settings.cap_ends = false;
    for (const auto& branch : branches) {
        const auto branch_mesh = strands::build_strand_mesh(branch, open_settings);
        if (branch_mesh.empty() || !append_mesh(result, branch_mesh))
            return {};
    }
    return result;
}

} // namespace gss::junctions
