#include "glue_saliva_stuff/junctions/junction.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <vector>

namespace gss::junctions {
namespace {
constexpr std::size_t kMaxGridResolution = 48;
constexpr std::size_t kMaxGridSamples = 125'000;
constexpr std::size_t kMaxBranchSegments = 128;
constexpr std::size_t kMaxBranches = 128;

bool finite_vec(const math::Vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

float smooth_min(float a, float b, float k) {
    if (k <= 1.0e-6f) return std::min(a, b);
    const float h = std::max(k - std::abs(a - b), 0.0f) / k;
    return std::min(a, b) - h * h * k * 0.25f;
}

float capsule_distance(const math::Vec3& p, const math::Vec3& a,
                       const math::Vec3& b, float radius_a, float radius_b) {
    const auto ab = b - a;
    const float length_sq = ab.dot(ab);
    const float t = length_sq > 1.0e-12f
        ? std::clamp((p - a).dot(ab) / length_sq, 0.0f, 1.0f) : 0.0f;
    const auto closest = a + ab * t;
    const float radius = radius_a + (radius_b - radius_a) * t;
    return (p - closest).length() - radius;
}

void emit_triangle(geometry::Mesh& mesh, std::uint32_t a, std::uint32_t b,
                   std::uint32_t c, const math::Vec3& outward_hint) {
    const auto normal = (mesh.vertices[b] - mesh.vertices[a]).cross(mesh.vertices[c] - mesh.vertices[a]);
    if (normal.length() <= 1.0e-9f) return;
    if (normal.dot(outward_hint) < 0.0f) std::swap(b, c);
    mesh.triangles.push_back({a, b, c});
}
} // namespace

geometry::Mesh build_blended_junction_mesh(
    const Junction& junction,
    const std::vector<strands::Strand>& branches,
    const BlendedJunctionSettings& settings) {
    geometry::Mesh empty;
    if (!finite_vec(junction.center) || !std::isfinite(junction.radius) ||
        junction.radius <= 0.0f || !std::isfinite(settings.blend_radius) ||
        settings.blend_radius < 0.0f || settings.grid_resolution < 8 ||
        settings.grid_resolution > kMaxGridResolution ||
        settings.max_output_vertices < 6)
        return empty;

    if (branches.size() > kMaxBranches) return empty;

    std::size_t segment_count = 0;
    math::Vec3 minimum = junction.center - math::Vec3{junction.radius, junction.radius, junction.radius};
    math::Vec3 maximum = junction.center + math::Vec3{junction.radius, junction.radius, junction.radius};
    for (const auto& branch : branches) {
        if (branch.points.size() > 4096) return empty;
        for (const auto& point : branch.points) {
            if (!finite_vec(point.position) || !std::isfinite(point.radius) || point.radius < 0.0f)
                return empty;
            const math::Vec3 extent{point.radius, point.radius, point.radius};
            minimum.x = std::min(minimum.x, point.position.x - extent.x);
            minimum.y = std::min(minimum.y, point.position.y - extent.y);
            minimum.z = std::min(minimum.z, point.position.z - extent.z);
            maximum.x = std::max(maximum.x, point.position.x + extent.x);
            maximum.y = std::max(maximum.y, point.position.y + extent.y);
            maximum.z = std::max(maximum.z, point.position.z + extent.z);
        }
        if (branch.points.size() > 1) {
            if (branch.points.size() - 1 > kMaxBranchSegments - segment_count)
                return empty;
            segment_count += branch.points.size() - 1;
        }
    }

    const float padding = std::max(settings.blend_radius, 0.02f);
    minimum = minimum - math::Vec3{padding, padding, padding};
    maximum = maximum + math::Vec3{padding, padding, padding};
    const auto extent = maximum - minimum;
    const float largest_extent = std::max({extent.x, extent.y, extent.z});
    if (!std::isfinite(largest_extent) || largest_extent <= 1.0e-6f) return empty;
    const std::size_t resolution = settings.grid_resolution;
    const std::size_t side = resolution + 1;
    if (side > kMaxGridSamples / side || side * side > kMaxGridSamples / side)
        return empty;
    const std::size_t sample_count = side * side * side;
    const math::Vec3 step = extent / static_cast<float>(resolution);
    const float blend = settings.blend_radius;

    const auto field = [&](const math::Vec3& p) {
        float value = (p - junction.center).length() - junction.radius;
        for (const auto& branch : branches) {
            for (std::size_t i = 0; i + 1 < branch.points.size(); ++i) {
                const auto& a = branch.points[i];
                const auto& b = branch.points[i + 1];
                const float distance = capsule_distance(
                    p, a.position, b.position, a.radius, b.radius);
                value = smooth_min(value, distance, blend);
            }
        }
        return value;
    };

    std::vector<float> samples(sample_count);
    const auto index = [side](std::size_t x, std::size_t y, std::size_t z) {
        return (z * side + y) * side + x;
    };
    for (std::size_t z = 0; z < side; ++z)
        for (std::size_t y = 0; y < side; ++y)
            for (std::size_t x = 0; x < side; ++x) {
                const math::Vec3 p{minimum.x + step.x * static_cast<float>(x),
                                    minimum.y + step.y * static_cast<float>(y),
                                    minimum.z + step.z * static_cast<float>(z)};
                samples[index(x, y, z)] = field(p);
            }

    geometry::Mesh mesh;
    constexpr std::uint64_t edge_bits = 17;
    std::unordered_map<std::uint64_t, std::uint32_t> edge_cache;
    const auto sample_id = [side](std::size_t x, std::size_t y, std::size_t z) {
        return static_cast<std::uint32_t>((z * side + y) * side + x);
    };
    const auto get_edge_vertex = [&](std::uint32_t ia, std::uint32_t ib, const math::Vec3& a,
                                    const math::Vec3& b, float va, float vb) {
        const auto lo = std::min(ia, ib), hi = std::max(ia, ib);
        const std::uint64_t key = (static_cast<std::uint64_t>(lo) << edge_bits) | hi;
        if (const auto it = edge_cache.find(key); it != edge_cache.end()) return it->second;
        if (mesh.vertices.size() >= settings.max_output_vertices || mesh.vertices.size() >= std::numeric_limits<std::uint32_t>::max())
            return std::numeric_limits<std::uint32_t>::max();
        const float d = va - vb;
        const float t = std::abs(d) > 1.0e-12f ? std::clamp(va / d, 0.0f, 1.0f) : 0.5f;
        const auto id = static_cast<std::uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(a + (b - a) * t);
        edge_cache.emplace(key, id);
        return id;
    };
    const std::array<math::Vec3, 8> corner_offsets{{
        {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0},
        {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}
    }};
    const std::array<std::array<int, 4>, 6> tetrahedra{{
        {{0,5,1,6}}, {{0,1,2,6}}, {{0,2,3,6}},
        {{0,3,7,6}}, {{0,7,4,6}}, {{0,4,5,6}}
    }};
    const std::array<std::array<int, 2>, 6> tetra_edges{{
        {{0,1}}, {{0,2}}, {{0,3}}, {{1,2}}, {{1,3}}, {{2,3}}
    }};

    for (std::size_t z = 0; z < resolution; ++z)
        for (std::size_t y = 0; y < resolution; ++y)
            for (std::size_t x = 0; x < resolution; ++x) {
                std::array<math::Vec3, 8> positions;
                std::array<float, 8> values;
                std::array<std::uint32_t, 8> ids;
                for (std::size_t corner = 0; corner < 8; ++corner) {
                    const auto& offset = corner_offsets[corner];
                    const auto cx = x + static_cast<std::size_t>(offset.x);
                    const auto cy = y + static_cast<std::size_t>(offset.y);
                    const auto cz = z + static_cast<std::size_t>(offset.z);
                    positions[corner] = {minimum.x + step.x * static_cast<float>(cx),
                                         minimum.y + step.y * static_cast<float>(cy),
                                         minimum.z + step.z * static_cast<float>(cz)};
                    values[corner] = samples[index(cx, cy, cz)];
                    ids[corner] = sample_id(cx, cy, cz);
                }
                for (const auto& tetra : tetrahedra) {
                    std::array<math::Vec3, 4> p;
                    std::array<float, 4> v;
                    std::array<std::uint32_t, 4> id;
                    for (int i = 0; i < 4; ++i) {
                        p[i] = positions[tetra[i]];
                        v[i] = values[tetra[i]];
                        id[i] = ids[tetra[i]];
                    }
                    std::array<std::uint32_t, 4> crossings;
                    std::size_t crossing_count = 0;
                    math::Vec3 inside_center{}, outside_center{};
                    std::size_t inside_count = 0, outside_count = 0;
                    for (int i = 0; i < 4; ++i) {
                        if (v[i] < 0.0f) { inside_center = inside_center + p[i]; ++inside_count; }
                        else { outside_center = outside_center + p[i]; ++outside_count; }
                    }
                    if (inside_count == 0 || outside_count == 0) continue;
                    inside_center = inside_center / static_cast<float>(inside_count);
                    outside_center = outside_center / static_cast<float>(outside_count);
                    const auto outward_hint = outside_center - inside_center;
                    for (const auto& edge : tetra_edges) {
                        const int a = edge[0], b = edge[1];
                        if ((v[a] < 0.0f) == (v[b] < 0.0f)) continue;
                        const auto vertex = get_edge_vertex(id[a], id[b], p[a], p[b], v[a], v[b]);
                        if (vertex == std::numeric_limits<std::uint32_t>::max()) return geometry::Mesh{};
                        crossings[crossing_count++] = vertex;
                    }
                    if (crossing_count == 3) {
                        emit_triangle(mesh, crossings[0], crossings[1], crossings[2], outward_hint);
                    } else if (crossing_count == 4) {
                        emit_triangle(mesh, crossings[0], crossings[1], crossings[2], outward_hint);
                        emit_triangle(mesh, crossings[0], crossings[2], crossings[3], outward_hint);
                    }
                    if (mesh.vertices.size() > settings.max_output_vertices - 6)
                        return geometry::Mesh{};
                }
            }
    return mesh;
}

} // namespace gss::junctions
