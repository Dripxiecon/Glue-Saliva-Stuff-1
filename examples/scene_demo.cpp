#include "glue_saliva_stuff/scene/scene.hpp"
#include "glue_saliva_stuff/strands/strand.hpp"
#include <utility>

// Host-side scene setup example. Call scene.render(renderer, surfaceWidth,
// surfaceHeight) each frame after the host activates its graphics context.
gss::scene::Scene make_demo_scene() {
    using namespace gss;
    scene::Scene world;
    strands::Strand sample;
    sample.points={{{-0.8f,0.0f,0.0f},0.12f},{{-0.3f,0.25f,0.0f},0.10f},{{0.3f,-0.15f,0.0f},0.08f},{{0.8f,0.0f,0.0f},0.04f}};
    scene::Object object;
    object.mesh=strands::build_strand_mesh(sample);
    object.material.base_color={0.62f,0.78f,0.96f};
    world.objects.push_back(std::move(object));
    world.camera.position={0.0f,0.0f,3.5f};
    world.camera.target={0.0f,0.0f,0.0f};
    return world;
}
