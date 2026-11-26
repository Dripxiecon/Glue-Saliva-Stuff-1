#include "glue_saliva_stuff/strands/strand.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace gss::strands {
namespace {
constexpr float kPi = 3.14159265358979323846f;
}

geometry::Mesh build_strand_mesh(const Strand& strand, const StrandMeshSettings& settings) {
    geometry::Mesh mesh;
    constexpr std::size_t kMaxRadialSegments = 256;
    // Keep accidental high-resolution strands from exhausting mobile memory.
    constexpr std::size_t kMaxGeneratedVertices = 1'000'000;
    constexpr std::size_t kMaxControlPoints = 65'536;
    if (strand.points.size() < 2 || strand.points.size() > kMaxControlPoints ||
        settings.radial_segments < 3 ||
        settings.radial_segments > kMaxRadialSegments) return mesh;

    std::vector<StrandPoint> points;
    points.reserve(strand.points.size());
    constexpr float kDuplicateDistance = 1.0e-6f;
    for (const auto& point : strand.points) {
        if (points.empty() || (point.position - points.back().position).length() > kDuplicateDistance) {
            points.push_back(point);
        } else {
            // Keep the widest radius when several control points occupy one location.
            points.back().radius = std::max(points.back().radius, point.radius);
        }
    }
    if (points.size() < 2) return mesh;

    const std::size_t ring_count = points.size();
    const std::size_t sides = settings.radial_segments;
    const std::size_t max_index = static_cast<std::size_t>(
        std::numeric_limits<std::uint32_t>::max());
    if (ring_count > (max_index - (settings.cap_ends ? 2u : 0u)) / sides)
        return mesh;
    if (ring_count > (kMaxGeneratedVertices - (settings.cap_ends ? 2u : 0u)) / sides)
        return mesh;
    if (ring_count - 1 > std::numeric_limits<std::size_t>::max() / (sides * 2))
        return mesh;

    for (const auto& point : strand.points) {
        if (!std::isfinite(point.position.x) || !std::isfinite(point.position.y) ||
            !std::isfinite(point.position.z) || !std::isfinite(point.radius))
            return mesh;
    }
    mesh.vertices.reserve(ring_count * sides + (settings.cap_ends ? 2 : 0));
    mesh.triangles.reserve((ring_count - 1) * sides * 2 + (settings.cap_ends ? sides * 2 : 0));

    math::Vec3 previous_tangent{};
    math::Vec3 frame_normal{};
    math::Vec3 frame_binormal{};

    for (std::size_t i = 0; i < ring_count; ++i) {
        const auto& point = points[i];
        // Find the nearest distinct neighbors, so duplicate control points
        // do not force a zero tangent. At a cusp, favor the outgoing direction.
        std::size_t before = i;
        std::size_t after = i;
        while (before > 0 && (point.position - points[before - 1].position).length() <= 1.0e-6f) --before;
        while (after + 1 < ring_count && (points[after + 1].position - point.position).length() <= 1.0e-6f) ++after;
        const bool has_before = before > 0;
        const bool has_after = after + 1 < ring_count;
        math::Vec3 incoming = has_before ? (point.position - points[before - 1].position).normalized() : math::Vec3{};
        math::Vec3 outgoing = has_after ? (points[after + 1].position - point.position).normalized() : math::Vec3{};
        math::Vec3 tangent;
        if (has_before && has_after) {
            const math::Vec3 combined = (points[after + 1].position - points[before - 1].position).normalized();
            tangent = (combined.length() <= 1.0e-6f || incoming.dot(outgoing) < -0.95f) ? outgoing : combined;
        } else if (has_after) tangent = outgoing;
        else if (has_before) tangent = incoming;
        if (!std::isfinite(tangent.x) || !std::isfinite(tangent.y) || !std::isfinite(tangent.z))
            return geometry::Mesh{};
        if (tangent.length() <= 1.0e-8f) tangent = i > 0 ? previous_tangent : math::Vec3{0, 0, 1};

        if (i == 0) {
            const math::Vec3 reference = std::abs(tangent.y) < 0.9f
                ? math::Vec3{0, 1, 0} : math::Vec3{1, 0, 0};
            frame_normal = tangent.cross(reference).normalized();
        } else {
            // Parallel-transport approximation: keep the previous frame normal,
            // projected onto the new tangent's perpendicular plane.
            frame_normal = (frame_normal - tangent * frame_normal.dot(tangent)).normalized();
            if (frame_normal.length() <= 1.0e-8f) {
                const math::Vec3 reference = std::abs(tangent.y) < 0.9f
                    ? math::Vec3{0, 1, 0} : math::Vec3{1, 0, 0};
                frame_normal = tangent.cross(reference).normalized();
            }
        }
        frame_binormal = tangent.cross(frame_normal).normalized();
        frame_normal = frame_binormal.cross(tangent).normalized();
        previous_tangent = tangent;

        const float radius = std::max(0.0f, point.radius);
        for (std::size_t side = 0; side < sides; ++side) {
            const float angle = 2.0f * kPi * static_cast<float>(side) / static_cast<float>(sides);
            const math::Vec3 offset = frame_normal * std::cos(angle) + frame_binormal * std::sin(angle);
            const math::Vec3 vertex = point.position + offset * radius;
            if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) ||
                !std::isfinite(vertex.z)) return geometry::Mesh{};
            mesh.vertices.push_back(vertex);
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
        mesh.vertices.push_back(points.front().position);
        const auto end_center = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(points.back().position);
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
