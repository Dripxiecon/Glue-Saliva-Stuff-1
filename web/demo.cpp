#include "glue_saliva_stuff/junctions/junction.hpp"
#include <emscripten/emscripten.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
std::vector<float> vertices;
std::vector<unsigned> indices;
gss::strands::Strand make_strand(float length, float radius, float taper, float curve, float side) {
    gss::strands::Strand s;
    const float r1=std::max(0.002f,radius*(1.0f-taper));
    s.points={{{-length*0.5f,0.0f,0.0f},radius},
              {{0.0f,curve,side*0.12f},(radius+r1)*0.5f},
              {{length*0.5f,0.0f,side*0.25f},r1}};
    return s;
}
gss::geometry::Mesh build_stretched_sheet(float length, float radius, float taper, float curve, int radial) {
    using namespace gss;
    geometry::Mesh mesh;
    const int rings=64, sides=std::clamp(radial,8,48);
    const float sheet_width=radius*3.4f;
    for(int i=0;i<=rings;++i){
        const float t=static_cast<float>(i)/rings;
        const float x=(t-0.5f)*length;
        const float envelope=std::pow(std::max(0.015f,std::sin(3.14159265f*t)),0.65f);
        const float width=sheet_width*envelope*(0.75f+0.45f*std::sin(t*6.2831853f));
        const float thickness=std::max(0.006f,radius*0.18f)*envelope;
        const float y=curve*std::sin(t*3.14159265f);
        for(int j=0;j<sides;++j){
            const float a=6.2831853f*static_cast<float>(j)/sides;
            mesh.vertices.push_back({x,y+std::cos(a)*width*(1.0f-taper*0.25f*t),std::sin(a)*thickness});
        }
    }
    for(int i=0;i<rings;++i)for(int j=0;j<sides;++j){
        const auto a=static_cast<std::uint32_t>(i*sides+j);
        const auto b=static_cast<std::uint32_t>(i*sides+(j+1)%sides);
        const auto c=static_cast<std::uint32_t>((i+1)*sides+(j+1)%sides);
        const auto d=static_cast<std::uint32_t>((i+1)*sides+j);
        mesh.triangles.push_back({a,b,c});mesh.triangles.push_back({a,c,d});
    }
    mesh.mark_dirty();
    return mesh;
}
void store(const gss::geometry::Mesh& m) {
    vertices.clear(); indices.clear();
    vertices.reserve(m.vertices.size()*3); indices.reserve(m.triangles.size()*3);
    for(const auto& v:m.vertices){vertices.push_back(v.x);vertices.push_back(v.y);vertices.push_back(v.z);}
    for(const auto& t:m.triangles){indices.push_back(t.a);indices.push_back(t.b);indices.push_back(t.c);}
}
}
extern "C" {
EMSCRIPTEN_KEEPALIVE int gss_generate(int mode,int radial,int resolution,float radius,float length,float taper,float curve,float blend) {
    using namespace gss;
    strands::StrandMeshSettings ss; ss.radial_segments=static_cast<std::size_t>(std::clamp(radial,3,64));
    const auto strand=make_strand(std::max(0.2f,length),std::max(0.005f,radius),std::clamp(taper,0.0f,0.95f),curve,0.0f);
    if(mode==0){
        store(strands::build_strand_mesh(strand,ss));
    } else if(mode==2) {
        store(build_stretched_sheet(std::max(0.2f,length),std::max(0.005f,radius),std::clamp(taper,0.0f,0.95f),curve,radial));
    } else {
        junctions::Junction hub{{0,0,0},std::max(0.03f,radius*1.25f)};
        auto left=make_strand(length,radius,taper,curve,-1.0f);
        auto right=make_strand(length,radius,taper,-curve,1.0f);
        for(auto& p:left.points)p.position.x-=length*0.32f;
        for(auto& p:right.points)p.position.x+=length*0.32f;
        junctions::BlendedJunctionSettings bs;
        bs.grid_resolution=static_cast<std::size_t>(std::clamp(resolution,8,48));
        bs.blend_radius=std::max(0.0f,blend);
        store(junctions::build_blended_junction_mesh(hub,{left,right},bs));
    }
    return static_cast<int>(indices.size()/3);
}
EMSCRIPTEN_KEEPALIVE int gss_vertex_count(){return static_cast<int>(vertices.size()/3);}
EMSCRIPTEN_KEEPALIVE int gss_index_count(){return static_cast<int>(indices.size());}
EMSCRIPTEN_KEEPALIVE float* gss_vertices_ptr(){return vertices.data();}
EMSCRIPTEN_KEEPALIVE unsigned* gss_indices_ptr(){return indices.data();}
}
