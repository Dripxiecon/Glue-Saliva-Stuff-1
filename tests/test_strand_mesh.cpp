#include "glue_saliva_stuff/scene/scene.hpp"
#include "glue_saliva_stuff/junctions/junction.hpp"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace gss;
    strands::Strand strand;
    strand.points={{{0.0f,0.0f,0.0f},0.1f},{{0.0f,0.0f,1.0f},0.1f}};
    const auto generated_mesh=strands::build_strand_mesh(strand);
    assert(generated_mesh.vertices.size()==18);
    assert(generated_mesh.triangles.size()==32);

    // Invalid coordinates and unreasonable tessellation settings fail safely.
    strands::Strand invalid = strand;
    invalid.points[1].position.y = std::numeric_limits<float>::quiet_NaN();
    assert(strands::build_strand_mesh(invalid).empty());
    strands::StrandMeshSettings excessive;
    excessive.radial_segments = 257;
    assert(strands::build_strand_mesh(strand, excessive).empty());

    // A valid but excessive mesh request must not allocate unbounded buffers.
    strands::Strand oversized;
    oversized.points.resize(4000);
    for (std::size_t i = 0; i < oversized.points.size(); ++i) {
        oversized.points[i].position = {0.0f, 0.0f, static_cast<float>(i)};
        oversized.points[i].radius = 0.01f;
    }
    strands::StrandMeshSettings dense;
    dense.radial_segments = 256;
    assert(strands::build_strand_mesh(oversized, dense).empty());

    // Finite inputs can still overflow during vertex construction.
    strands::Strand overflow = strand;
    overflow.points[0].position.x = std::numeric_limits<float>::max();
    overflow.points[0].radius = std::numeric_limits<float>::max();
    assert(strands::build_strand_mesh(overflow).empty());

    // Repeated control points and a sharp reversal should not create NaNs.
    strands::Strand duplicate_and_cusp;
    duplicate_and_cusp.points = {
        {{0.0f, 0.0f, 0.0f}, 0.08f},
        {{0.0f, 0.0f, 0.0f}, 0.08f},
        {{0.0f, 0.0f, 1.0f}, 0.08f},
        {{0.0f, 0.0f, 2.0f}, 0.08f},
        {{0.0f, 0.0f, 1.0f}, 0.08f},
        {{0.0f, 0.0f, 0.0f}, 0.08f}
    };
    const auto cusp_mesh = strands::build_strand_mesh(duplicate_and_cusp);
    assert(!cusp_mesh.empty());
    // Consecutive duplicate positions collapse to one ring, avoiding zero-area strips.
    assert(cusp_mesh.vertices.size() == 5 * 8 + 2);
    strands::Strand zero_length;
    zero_length.points = {
        {{2.0f, 2.0f, 2.0f}, 0.1f},
        {{2.0f, 2.0f, 2.0f}, 0.2f},
        {{2.0f, 2.0f, 2.0f}, 0.1f}
    };
    assert(strands::build_strand_mesh(zero_length).empty());
    for (const auto& v : cusp_mesh.vertices)
        assert(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z));

    // A curved, non-planar strand should produce finite tube vertices.
    strands::Strand curved;
    curved.points={
        {{0.0f,0.0f,0.0f},0.12f},
        {{0.0f,0.8f,0.7f},0.10f},
        {{0.7f,1.0f,1.2f},0.08f},
        {{1.1f,0.5f,1.8f},0.05f}
    };
    const auto curved_mesh=strands::build_strand_mesh(curved);
    assert(curved_mesh.vertices.size()==curved.points.size()*8+2);
    for(const auto& v:curved_mesh.vertices)
        assert(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z));

    // Rounded junction hub has bounded, finite geometry.
    junctions::Junction hub;
    hub.center = {1.0f, 2.0f, 3.0f};
    hub.radius = 0.25f;
    junctions::JunctionMeshSettings hub_settings;
    hub_settings.radial_segments = 12;
    hub_settings.latitude_segments = 8;
    const auto hub_mesh = junctions::build_junction_mesh(hub, hub_settings);
    assert(hub_mesh.vertices.size() == 86);
    assert(hub_mesh.triangles.size() == 168);
    for (const auto& v : hub_mesh.vertices)
        assert(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z));
    // Every hub triangle must have nonzero area and face away from the center.
    for (const auto& triangle : hub_mesh.triangles) {
        const auto& a = hub_mesh.vertices[triangle.a];
        const auto& b = hub_mesh.vertices[triangle.b];
        const auto& c = hub_mesh.vertices[triangle.c];
        const auto normal = (b - a).cross(c - a);
        const auto centroid = (a + b + c) / 3.0f;
        assert(normal.length() > 1.0e-8f);
        assert(normal.dot(centroid - hub.center) > 0.0f);
    }

    // Branch tubes are combined into one mesh with their ends hidden inside
    // the hub. Open branch ends avoid internal cap disks.
    strands::Strand branch_a;
    branch_a.points = {{{1.0f, 2.0f, 3.0f}, 0.08f},
                       {{2.0f, 2.0f, 3.0f}, 0.08f}};
    strands::Strand branch_b;
    branch_b.points = {{{1.0f, 2.0f, 3.0f}, 0.08f},
                       {{0.0f, 2.0f, 3.0f}, 0.08f}};
    strands::StrandMeshSettings branch_settings;
    branch_settings.radial_segments = 8;
    branch_settings.cap_ends = true; // assembly deliberately disables caps
    const auto assembly = junctions::build_junction_assembly(
        hub, {branch_a, branch_b}, hub_settings, branch_settings);
    assert(assembly.vertices.size() == 86 + 2 * 2 * 8);
    assert(assembly.triangles.size() == 168 + 2 * 8 * 2);

    // Smooth implicit junction surface blends the hub and branch rather
    // than retaining intersecting shells.
    junctions::BlendedJunctionSettings blended_settings;
    blended_settings.grid_resolution = 10;
    blended_settings.blend_radius = 0.12f;
    const auto blended = junctions::build_blended_junction_mesh(
        hub, {branch_a, branch_b}, blended_settings);
    assert(!blended.empty());
    for (const auto& vertex : blended.vertices)
        assert(std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z));
    for (const auto& triangle : blended.triangles) {
        assert(triangle.a < blended.vertices.size());
        assert(triangle.b < blended.vertices.size());
        assert(triangle.c < blended.vertices.size());
    }
    auto tiny_budget = blended_settings;
    tiny_budget.max_output_vertices = 5;
    assert(junctions::build_blended_junction_mesh(
        hub, {branch_a}, tiny_budget).empty());

    junctions::Junction invalid_hub = hub;
    invalid_hub.radius = std::numeric_limits<float>::infinity();
    assert(junctions::build_junction_mesh(invalid_hub).empty());
    junctions::Junction overflow_hub = hub;
    overflow_hub.center.y = std::numeric_limits<float>::max();
    overflow_hub.radius = std::numeric_limits<float>::max();
    assert(junctions::build_junction_mesh(overflow_hub).empty());
    junctions::JunctionMeshSettings huge_hub;
    huge_hub.radial_segments = 129;
    assert(junctions::build_junction_mesh(hub, huge_hub).empty());

    scene::Transform transform;
    transform.position={1.0f,2.0f,3.0f};
    const auto model=transform.matrix();
    assert(model.at(0,3)==1.0f && model.at(1,3)==2.0f && model.at(2,3)==3.0f);
    scene::Camera camera;
    const auto vp=camera.view_projection(16.0f/9.0f);
    assert(vp.at(0,0)!=0.0f && vp.at(1,1)!=0.0f);

    scene::Scene world;
    auto& object=world.add_strand(strand,{},transform,{});
    assert(world.objects.size()==1);
    assert(&object==&world.objects.front());
    assert(object.mesh.vertices.size()==generated_mesh.vertices.size());
    assert(object.mesh.triangles.size()==generated_mesh.triangles.size());
    assert(object.mesh.vertices[0].z==generated_mesh.vertices[0].z);
    const auto handle = object.handle;
    const auto old_mesh_id = object.mesh.cache_id();
    const auto old_mesh_revision = object.mesh.revision();
    const auto old_transform = object.transform.position;
    const auto old_material = object.material.base_color;
    strand.points[1].position.x = 0.75f;
    assert(world.update_strand(handle, strand));
    assert(world.objects.size() == 1);
    assert(world.find(handle) == &world.objects.front());
    assert(object.mesh.cache_id() == old_mesh_id);
    assert(object.mesh.revision() > old_mesh_revision);
    assert(object.transform.position.x == old_transform.x);
    assert(object.material.base_color.x == old_material.x);
    assert(object.mesh.vertices[0].x != generated_mesh.vertices[0].x ||
           object.mesh.vertices[1].x != generated_mesh.vertices[1].x);
    assert(!world.update_strand(999999, strand));

    geometry::Mesh tracked;
    const auto original_id=tracked.cache_id();
    const auto original_revision=tracked.revision();
    tracked.vertices.push_back({0.0f,0.0f,0.0f});
    tracked.mark_dirty();
    assert(tracked.revision()>original_revision);
    geometry::Mesh copied=tracked;
    assert(copied.cache_id()!=original_id);
    return 0;
}
