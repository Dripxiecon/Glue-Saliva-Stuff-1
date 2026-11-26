#include "glue_saliva_stuff/scene/scene.hpp"
#include <cassert>
#include <cmath>

int main() {
    using namespace gss;
    strands::Strand strand;
    strand.points={{{0.0f,0.0f,0.0f},0.1f},{{0.0f,0.0f,1.0f},0.1f}};
    const auto generated_mesh=strands::build_strand_mesh(strand);
    assert(generated_mesh.vertices.size()==18);
    assert(generated_mesh.triangles.size()==32);

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
