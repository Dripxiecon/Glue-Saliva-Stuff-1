#include "glue_saliva_stuff/strands/strand.hpp"

#include <cassert>

int main() {
    using namespace gss;
    strands::Strand strand;
    strand.points = {{{0.0f, 0.0f, 0.0f}, 0.1f}, {{0.0f, 0.0f, 1.0f}, 0.1f}};
    const auto mesh = strands::build_strand_mesh(strand);
    assert(mesh.vertices.size() == 18); // two 8-sided rings plus two cap centers
    assert(mesh.triangles.size() == 32); // 16 side triangles plus 16 cap triangles
    return 0;
}
