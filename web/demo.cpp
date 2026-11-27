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
    using math::Vec3;
    geometry::Mesh mesh;
    constexpr float pi=3.14159265359f;
    const int rings=48;
    const int across=std::clamp(radial*2,20,48);
    const float scale=std::clamp(length/2.0f,0.55f,1.65f);
    const float r=std::max(0.018f,radius);

    // The reference is a web of stretched membranes and filaments,
    // not one elongated tube or one isolated ribbon.
    auto add_patch=[&](Vec3 start,Vec3 finish,Vec3 widthAxis,Vec3 bendAxis,
                        float width,float bulge,float twist) {
        const Vec3 path=(finish-start).normalized();
        widthAxis=(widthAxis-path*path.dot(widthAxis)).normalized();
        if(widthAxis.length()<0.01f) widthAxis=path.cross({0,0,1}).normalized();
        const Vec3 normal=path.cross(widthAxis).normalized();
        const std::uint32_t base=static_cast<std::uint32_t>(mesh.vertices.size());
        const int layerCount=2;
        const std::uint32_t layerSize=static_cast<std::uint32_t>((rings+1)*(across+1));
        for(int layer=0;layer<layerCount;++layer) {
            for(int i=0;i<=rings;++i) {
                const float t=static_cast<float>(i)/rings;
                const float envelope=std::pow(std::max(0.001f,std::sin(pi*t)),0.55f);
                const float lobeA=std::exp(-std::pow((t-0.32f)/0.22f,2.0f));
                const float lobeB=0.42f*std::exp(-std::pow((t-0.76f)/0.16f,2.0f));
                const float widthProfile=0.055f+1.05f*lobeA+lobeB;
                const float w=width*envelope*widthProfile;
                Vec3 center=start+(finish-start)*t;
                center=center+bendAxis*(bulge*std::sin(pi*t));
                center=center+normal*(twist*std::sin(2.0f*pi*t));
                for(int j=0;j<=across;++j) {
                    const float u=2.0f*static_cast<float>(j)/across-1.0f;
                    const float edge=1.0f-0.18f*u*u;
                    const float ripple=0.045f*r*std::sin(t*9.0f*pi+u*3.1f)*envelope;
                    const float thick=std::max(0.0015f,r*0.055f)*envelope*(1.0f-0.35f*u*u);
                    Vec3 p=center+widthAxis*(u*w*0.5f*edge);
                    p=p+normal*(ripple+(layer==0?thick:-thick));
                    mesh.vertices.push_back(p);
                }
            }
        }
        auto idx=[&](int layer,int i,int j) {
            return base+static_cast<std::uint32_t>(layer)*layerSize+
                   static_cast<std::uint32_t>(i*(across+1)+j);
        };
        for(int i=0;i<rings;++i) for(int j=0;j<across;++j) {
            const auto a=idx(0,i,j),b=idx(0,i,j+1),c=idx(0,i+1,j+1),d=idx(0,i+1,j);
            mesh.triangles.push_back({a,d,c}); mesh.triangles.push_back({a,c,b});
            const auto ba=idx(1,i,j),bb=idx(1,i,j+1),bc=idx(1,i+1,j+1),bd=idx(1,i+1,j);
            mesh.triangles.push_back({ba,bb,bc}); mesh.triangles.push_back({ba,bc,bd});
        }
        // Close the two sides and both ends of each membrane.
        for(int i=0;i<rings;++i) for(int edge : {0,across}) {
            auto t0=idx(0,i,edge),t1=idx(0,i+1,edge),b0=idx(1,i,edge),b1=idx(1,i+1,edge);
            mesh.triangles.push_back({t0,t1,b1}); mesh.triangles.push_back({t0,b1,b0});
        }
        for(int j=0;j<across;++j) for(int e : {0,rings}) {
            auto t0=idx(0,e,j),t1=idx(0,e,j+1),b0=idx(1,e,j),b1=idx(1,e,j+1);
            mesh.triangles.push_back({t0,t1,b1}); mesh.triangles.push_back({t0,b1,b0});
        }
    };

    auto add_blob=[&](Vec3 center,float blobRadius) {
        constexpr int lat=10,lon=18;
        const std::uint32_t base=static_cast<std::uint32_t>(mesh.vertices.size());
        for(int y=0;y<=lat;++y) {
            const float v=pi*static_cast<float>(y)/lat;
            for(int x=0;x<=lon;++x) {
                const float u=2.0f*pi*static_cast<float>(x)/lon;
                const float organic=1.0f+0.10f*std::sin(3.0f*u+v)*std::sin(2.0f*v);
                mesh.vertices.push_back({center.x+blobRadius*organic*std::sin(v)*std::cos(u),
                    center.y+blobRadius*organic*std::cos(v),
                    center.z+blobRadius*organic*std::sin(v)*std::sin(u)});
            }
        }
        for(int y=0;y<lat;++y) for(int x=0;x<lon;++x) {
            const auto a=base+y*(lon+1)+x,b=a+1,c=a+(lon+1)+1,d=a+(lon+1);
            mesh.triangles.push_back({a,c,b}); mesh.triangles.push_back({a,d,c});
        }
    };

    const Vec3 A{-1.55f*scale,-1.15f*scale,0.0f};
    const Vec3 B{-0.42f*scale,-0.10f*scale,0.18f*scale};
    const Vec3 C{1.25f*scale,1.10f*scale,0.05f*scale};
    const Vec3 D{1.35f*scale,-1.20f*scale,-0.12f*scale};
    const Vec3 E{-0.05f*scale,1.45f*scale,0.22f*scale};
    const Vec3 F{-1.65f*scale,0.45f*scale,-0.08f*scale};

    // Wide sheets flare from pooled anchors and pull into fine web-like threads.
    add_patch(A,B,{0.0f,0.0f,1.0f},{0.0f,0.35f,0.15f},r*5.8f,curve*0.45f,0.08f*r);
    add_patch(B,C,{0.0f,0.0f,1.0f},{0.0f,-0.25f,0.25f},r*2.0f,curve*0.35f,0.04f*r);
    add_patch(B,D,{0.0f,0.0f,1.0f},{0.0f,0.15f,-0.25f},r*4.8f,curve*0.65f,0.06f*r);
    add_patch(F,B,{0.0f,0.0f,1.0f},{0.0f,-0.25f,0.12f},r*3.2f,curve*0.25f,0.05f*r);
    add_patch(B,E,{0.0f,0.0f,1.0f},{0.0f,0.35f,-0.18f},r*1.7f,curve*0.4f,0.04f*r);
    add_patch(E,C,{0.0f,0.0f,1.0f},{0.0f,-0.3f,0.18f},r*2.5f,curve*0.3f,0.05f*r);
    // Fine connecting strands create the long, high-tension transitions.
    add_patch(A,C,{0.0f,0.0f,1.0f},{0.0f,0.25f,0.1f},r*0.42f,curve*0.55f,0.02f*r);
    add_patch(F,D,{0.0f,0.0f,1.0f},{0.0f,-0.15f,0.2f},r*0.34f,curve*0.35f,0.02f*r);

    add_blob(A,r*2.5f); add_blob(B,r*2.1f); add_blob(C,r*1.6f);
    add_blob(D,r*2.3f); add_blob(E,r*1.35f); add_blob(F,r*1.8f);
    // Small suspended droplets around the membrane network.
    add_blob({-0.82f*scale,0.42f*scale,0.28f*scale},r*0.34f);
    add_blob({0.42f*scale,0.72f*scale,-0.20f*scale},r*0.27f);
    add_blob({0.86f*scale,-0.38f*scale,0.25f*scale},r*0.30f);
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
