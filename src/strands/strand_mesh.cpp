#include "glue_saliva_stuff/strands/strand.hpp"

#include <algorithm>
#include <cmath>

namespace gss::strands {
namespace {
constexpr float kPi = 3.14159265358979323846f;
}

geometry::Mesh build_strand_mesh(const Strand& strand, const StrandMeshSettings& settings) {
    geometry::Mesh mesh;
    if (strand.points.size() < 2 || settings.radial_segments < 3) return mesh;

    const std::size_t ring_count = strand.points.size();
    const std::size_t sides = settings.radial_segments;
    mesh.vertices.reserve(ring_count * sides + (settings.cap_ends ? 2 : 0));
    mesh.triangles.reserve((ring_count - 1) * sides * 2 + (settings.cap_ends ? sides * 2 : 0));

    for (std::size_t i = 0; i < ring_count; ++i) {
        const auto& point = strand.points[i];
        math::Vec3 tangent;
        if (i == 0) tangent = (strand.points[1].position - point.position).normalized();
        else if (i + 1 == ring_count) tangent = (point.position - strand.points[i - 1].position).normalized();
        else tangent = (strand.points[i + 1].position - strand.points[i - 1].position).normalized();

        // Choose a stable reference axis that is not parallel to the tangent.
        const math::Vec3 reference = std::abs(tangent.y) < 0.9f ? math::Vec3{0, 1, 0} : math::Vec3{1, 0, 0};
        const math::Vec3 normal = tangent.cross(reference).normalized();
        const math::Vec3 binormal = tangent.cross(normal).normalized();
        const float radius = std::max(0.0f, point.radius);

        for (std::size_t side = 0; side < sides; ++side) {
            const float angle = 2.0f * kPi * static_cast<float>(side) / static_cast<float>(sides);
            const math::Vec3 offset = normal * std::cos(angle) + binormal * std::sin(angle);
            mesh.vertices.push_back(point.position + offset * radius);
        }
    }

    for (std::size_t ring = 0; ring + 1 < ring_count; ++ring) {
        for (std::size_t side = 0; side < sides; ++side) {
            const std::size_t next = (side + 1) % sides;
            const auto a = static_cast<std::uint32_t>(ring * sides + side);
            const auto b = static_cast<std::uint32_t>(ring * sides + next);
            const auto c = static_cast<std::uint32_t>((ring + 1) * sides + side);
            const auto d = static_cast<std::uint32_t>((ring + 1) * sides + next);
            mesh.triangles.push_back({a, c, b});
            mesh.triangles.push_back({b, c, d});
        }
    }

    if (settings.cap_ends) {
        const auto start_center = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(strand.points.front().position);
        const auto end_center = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(strand.points.back().position);
        const std::size_t last_ring = (ring_count - 1) * sides;
        for (std::size_t side = 0; side < sides; ++side) {
            const auto next = (side + 1) % sides;
            mesh.triangles.push_back({start_center, static_cast<std::uint32_t>(next), static_cast<std::uint32_t>(side)});
            mesh.triangles.push_back({end_center, static_cast<std::uint32_t>(last_ring + side), static_cast<std::uint32_t>(last_ring + next)});
        }
    }

    return mesh;
}

} // namespace gss::strands
