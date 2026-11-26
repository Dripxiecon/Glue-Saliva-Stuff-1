#include "glue_saliva_stuff/strands/strand.hpp"
#include "glue_saliva_stuff/scene/scene.hpp"
#include <cassert>

int main() {
    using namespace gss;
    strands::Strand strand;
    strand.points = {{{0.0f,0.0f,0.0f},0.1f},{{0.0f,0.0f,1.0f},0.1f}};
    const auto mesh=strands::build_strand_mesh(strand);
    assert(mesh.vertices.size()==18);
    assert(mesh.triangles.size()==32);

    scene::Transform transform;
    transform.position={1.0f,2.0f,3.0f};
    const auto model=transform.matrix();
    assert(model.at(0,3)==1.0f && model.at(1,3)==2.0f && model.at(2,3)==3.0f);
    scene::Camera camera;
    const auto vp=camera.view_projection(16.0f/9.0f);
    assert(vp.at(0,0)!=0.0f && vp.at(1,1)!=0.0f);
    scene::Scene world;
    world.objects.push_back({mesh,transform,{}});
    assert(world.objects.size()==1);
    return 0;
}
