#include "glue_saliva_stuff/scene/scene.hpp"
#include <cassert>

int main() {
    using namespace gss;
    strands::Strand strand;
    strand.points={{{0.0f,0.0f,0.0f},0.1f},{{0.0f,0.0f,1.0f},0.1f}};
    const auto generated_mesh=strands::build_strand_mesh(strand);
    assert(generated_mesh.vertices.size()==18);
    assert(generated_mesh.triangles.size()==32);

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
    return 0;
}
