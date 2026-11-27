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
    // A true two-sided ribbon grid: width is in Y, thickness is in Z.
    // This avoids the old elliptical tube silhouette and produces a broad,
    // continuous sheet with a closed perimeter.
    constexpr int rings=64;
    const int across=std::clamp(radial*2,16,64);
    const int layerSize=(rings+1)*(across+1);
    mesh.vertices.reserve(static_cast<std::size_t>(layerSize)*2);
    mesh.triangles.reserve(static_cast<std::size_t>(rings*across*4 + rings*4 + across*4));
    const float pi=3.14159265359f;
    for(int layer=0;layer<2;++layer) {
        for(int i=0;i<=rings;++i) {
            const float t=static_cast<float>(i)/rings;
            const float endShape=std::pow(std::max(0.025f,std::sin(pi*t)),0.58f);
            const float width=radius*5.2f*endShape*
                (0.78f+0.24f*std::sin(t*2.0f*pi+0.4f)+0.12f*std::sin(t*5.0f*pi));
            const float x=(t-0.5f)*length;
            const float centerY=curve*std::sin(pi*t)+0.10f*curve*std::sin(2.0f*pi*t);
            const float thickness=std::max(0.003f,radius*0.11f)*endShape;
            for(int j=0;j<=across;++j) {
                const float u=2.0f*static_cast<float>(j)/across-1.0f;
                const float edgeRound=1.0f-0.10f*u*u;
                const float ripple=0.035f*radius*std::sin(t*8.0f*pi+u*2.2f)*std::sin(pi*t);
                const float y=centerY+u*width*0.5f*edgeRound;
                const float z=0.06f*radius*std::sin(t*3.0f*pi+u)*std::sin(pi*t)
                    +ripple+(layer==0?thickness*0.5f:-thickness*0.5f);
                mesh.vertices.push_back({x,y,z});
            }
        }
    }
    auto index=[&](int layer,int i,int j)->std::uint32_t {
        return static_cast<std::uint32_t>(layer*layerSize+i*(across+1)+j);
    };
    for(int i=0;i<rings;++i) for(int j=0;j<across;++j) {
        const auto a=index(0,i,j), b=index(0,i,j+1);
        const auto c=index(0,i+1,j+1), d=index(0,i+1,j);
        // Top faces point +Z; bottom faces point -Z.
        mesh.triangles.push_back({a,d,c}); mesh.triangles.push_back({a,c,b});
        const auto ba=index(1,i,j), bb=index(1,i,j+1);
        const auto bc=index(1,i+1,j+1), bd=index(1,i+1,j);
        mesh.triangles.push_back({ba,bb,bc}); mesh.triangles.push_back({ba,bc,bd});
    }
    // Seal both long edges and both end edges.
    for(int i=0;i<rings;++i) {
        for(int edge : {0,across}) {
            const auto t0=index(0,i,edge), t1=index(0,i+1,edge);
            const auto b0=index(1,i,edge), b1=index(1,i+1,edge);
            if(edge==0) { mesh.triangles.push_back({t0,b1,t1}); mesh.triangles.push_back({t0,b0,b1}); }
            else { mesh.triangles.push_back({t0,t1,b1}); mesh.triangles.push_back({t0,b1,b0}); }
        }
    }
    for(int j=0;j<across;++j) {
        for(int endcap : {0,rings}) {
            const auto t0=index(0,endcap,j), t1=index(0,endcap,j+1);
            const auto b0=index(1,endcap,j), b1=index(1,endcap,j+1);
            if(endcap==0) { mesh.triangles.push_back({t0,t1,b1}); mesh.triangles.push_back({t0,b1,b0}); }
            else { mesh.triangles.push_back({t0,b1,t1}); mesh.triangles.push_back({t0,b0,b1}); }
        }
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
