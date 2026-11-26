#include "glue_saliva_stuff/junctions/junction.hpp"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
using gss::geometry::Mesh;
using gss::math::Vec3;

struct EdgeKey {
    std::uint32_t a, b;
    bool operator==(const EdgeKey&) const = default;
};
struct EdgeHash {
    std::size_t operator()(const EdgeKey& e) const noexcept {
        return (static_cast<std::size_t>(e.a) << 32U) ^ e.b;
    }
};
struct Stats {
    std::size_t invalid_indices{}, degenerate_faces{}, boundary_edges{}, nonmanifold_edges{};
    bool finite{true};
};

Stats inspect(const Mesh& mesh) {
    Stats s;
    std::unordered_map<EdgeKey, unsigned, EdgeHash> edges;
    for (const auto& v : mesh.vertices)
        s.finite &= std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    for (const auto& t : mesh.triangles) {
        if (t.a >= mesh.vertices.size() || t.b >= mesh.vertices.size() || t.c >= mesh.vertices.size()) {
            ++s.invalid_indices; continue;
        }
        const auto n = (mesh.vertices[t.b] - mesh.vertices[t.a]).cross(
                       mesh.vertices[t.c] - mesh.vertices[t.a]);
        if (!std::isfinite(n.length()) || n.length() <= 1.0e-8f) ++s.degenerate_faces;
        const std::uint32_t ids[3]{t.a,t.b,t.c};
        for (int i=0;i<3;++i) {
            auto a=ids[i], b=ids[(i+1)%3];
            if (a>b) std::swap(a,b);
            ++edges[{a,b}];
        }
    }
    for (const auto& [edge,count] : edges) {
        (void)edge;
        if (count==1) ++s.boundary_edges;
        else if (count>2) ++s.nonmanifold_edges;
    }
    return s;
}

bool write_obj(const std::string& path, const std::vector<std::pair<std::string,Mesh>>& meshes) {
    std::ofstream out(path);
    if (!out) return false;
    out << "# Glue & Saliva Stuff 1 mesh verification scene\n";
    std::size_t offset=1;
    for (const auto& [name,mesh] : meshes) {
        out << "\ng " << name << "\n";
        for (const auto& v : mesh.vertices) out << "v " << v.x << ' ' << v.y << ' ' << v.z << "\n";
        for (const auto& t : mesh.triangles)
            out << "f " << t.a+offset << ' ' << t.b+offset << ' ' << t.c+offset << "\n";
        offset += mesh.vertices.size();
    }
    return static_cast<bool>(out);
}
}

int main(int argc, char** argv) {
    using namespace gss;
    const std::string output = argc>1 ? argv[1] : "gss_mesh_verification.obj";
    std::vector<std::pair<std::string,Mesh>> meshes;

    strands::Strand curved;
    curved.points={{{-2.2f,0.0f,0.0f},0.16f},{{-1.7f,0.55f,0.25f},0.13f},
                   {{-1.15f,0.15f,0.55f},0.09f},{{-0.65f,0.55f,0.15f},0.045f}};
    meshes.emplace_back("tapered_curved_strand",strands::build_strand_mesh(curved));

    strands::Strand cusp;
    cusp.points={{{-2.0f,-1.0f,0.0f},0.09f},{{-2.0f,-1.0f,0.0f},0.11f},
                 {{-1.4f,-0.45f,0.0f},0.08f},{{-1.95f,0.1f,0.0f},0.06f}};
    meshes.emplace_back("duplicate_point_and_cusp",strands::build_strand_mesh(cusp));

    junctions::Junction hub{{0.0f,0.0f,0.0f},0.28f};
    strands::Strand left,right,up;
    left.points={{{-0.05f,0.0f,0.0f},0.12f},{{-0.9f,0.0f,0.0f},0.10f}};
    right.points={{{0.05f,0.0f,0.0f},0.12f},{{0.9f,0.0f,0.0f},0.07f}};
    up.points={{{0.0f,0.05f,0.0f},0.11f},{{0.0f,0.85f,0.35f},0.06f}};
    meshes.emplace_back("overlap_hub_and_branch_assembly",
        junctions::build_junction_assembly(hub,{left,right,up}));

    junctions::BlendedJunctionSettings blend;
    blend.grid_resolution=24;
    blend.blend_radius=0.18f;
    meshes.emplace_back("smooth_blended_junction",
        junctions::build_blended_junction_mesh(hub,{left,right,up},blend));

    bool ok=true;
    for (const auto& [name,mesh] : meshes) {
        const auto s=inspect(mesh);
        std::cout << name << ": vertices=" << mesh.vertices.size()
                  << " triangles=" << mesh.triangles.size()
                  << " finite=" << s.finite
                  << " invalid_indices=" << s.invalid_indices
                  << " degenerate_faces=" << s.degenerate_faces
                  << " boundary_edges=" << s.boundary_edges
                  << " nonmanifold_edges=" << s.nonmanifold_edges << '\n';
        ok &= !mesh.empty() && s.finite && s.invalid_indices==0 && s.degenerate_faces==0;
    }
    if (!write_obj(output,meshes)) {
        std::cerr << "Failed to write OBJ: " << output << '\n';
        return 2;
    }
    std::cout << "Wrote visual inspection mesh: " << output << '\n';
    return ok ? 0 : 1;
}
