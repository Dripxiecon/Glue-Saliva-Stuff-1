#include "glue_saliva_stuff/scene/scene.hpp"
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
