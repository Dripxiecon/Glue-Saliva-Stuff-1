#include "glue_saliva_stuff/junctions/junction.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace gss::junctions {

geometry::Mesh build_junction_mesh(
    const Junction& junction,
    const JunctionMeshSettings& settings) {
    geometry::Mesh mesh;
    constexpr std::size_t kMaxRadialSegments = 128;
    constexpr std::size_t kMaxLatitudeSegments = 64;
    constexpr std::size_t kMaxVertices = 50'000;
    constexpr float kPi = 3.14159265358979323846f;

    if (!std::isfinite(junction.center.x) ||
        !std::isfinite(junction.center.y) ||
        !std::isfinite(junction.center.z) ||
        !std::isfinite(junction.radius) || junction.radius <= 0.0f)
        return mesh;
    if (settings.radial_segments < 3 ||
        settings.radial_segments > kMaxRadialSegments ||
        settings.latitude_segments < 2 ||
        settings.latitude_segments > kMaxLatitudeSegments)
        return mesh;

    const std::size_t sides = settings.radial_segments;
    const std::size_t latitudes = settings.latitude_segments;
    const std::size_t vertex_count = (latitudes - 1) * sides + 2;
    if (vertex_count > kMaxVertices ||
        vertex_count > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
        return mesh;

    mesh.vertices.reserve(vertex_count);
    mesh.triangles.reserve(2 * sides * (latitudes - 1));

    mesh.vertices.push_back({
        junction.center.x,
        junction.center.y + junction.radius,
        junction.center.z
    });

    for (std::size_t latitude = 1; latitude < latitudes; ++latitude) {
        const float phi = kPi * static_cast<float>(latitude) /
                          static_cast<float>(latitudes);
        const float y = std::cos(phi) * junction.radius;
        const float ring_radius = std::sin(phi) * junction.radius;
        for (std::size_t side = 0; side < sides; ++side) {
            const float theta = 2.0f * kPi * static_cast<float>(side) /
                                static_cast<float>(sides);
            const math::Vec3 vertex{
                junction.center.x + ring_radius * std::cos(theta),
                junction.center.y + y,
                junction.center.z + ring_radius * std::sin(theta)
            };
            if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) ||
                !std::isfinite(vertex.z))
                return geometry::Mesh{};
            mesh.vertices.push_back(vertex);
        }
    }

    const auto bottom = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({
        junction.center.x,
        junction.center.y - junction.radius,
        junction.center.z
    });

    // Top cap; reverse ring order to keep faces outward-facing.
    for (std::size_t side = 0; side < sides; ++side) {
        const auto next = (side + 1) % sides;
        mesh.triangles.push_back({
            0,
            static_cast<std::uint32_t>(1 + next),
            static_cast<std::uint32_t>(1 + side)
        });
    }

    for (std::size_t latitude = 0; latitude + 1 < latitudes - 1; ++latitude) {
        const std::size_t current = 1 + latitude * sides;
        const std::size_t next_ring = current + sides;
        for (std::size_t side = 0; side < sides; ++side) {
            const std::size_t next = (side + 1) % sides;
            const auto a = static_cast<std::uint32_t>(current + side);
            const auto b = static_cast<std::uint32_t>(current + next);
            const auto c = static_cast<std::uint32_t>(next_ring + side);
            const auto d = static_cast<std::uint32_t>(next_ring + next);
            mesh.triangles.push_back({a, b, c});
            mesh.triangles.push_back({b, d, c});
        }
    }

    const std::size_t last_ring = 1 + (latitudes - 2) * sides;
    for (std::size_t side = 0; side < sides; ++side) {
        const std::size_t next = (side + 1) % sides;
        mesh.triangles.push_back({
            bottom,
            static_cast<std::uint32_t>(last_ring + side),
            static_cast<std::uint32_t>(last_ring + next)
        });
    }

    return mesh;
}

} // namespace gss::junctions
