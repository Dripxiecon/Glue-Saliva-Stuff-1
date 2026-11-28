#include "glue_saliva_stuff/junctions/junction.hpp"
#include <emscripten/emscripten.h>
#include <algorithm>
#include <array>
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
    const float scale=std::clamp(length/2.0f,0.55f,1.5f);
    const float r=std::max(0.018f,radius);
    const int sides=std::clamp(radial*2,16,36);
    const int rings=80;

    auto add_tube=[&](auto centerAt, auto radiusAt, int ringCount) {
        const std::uint32_t base=static_cast<std::uint32_t>(mesh.vertices.size());
        for(int i=0;i<=ringCount;++i) {
            const float t=static_cast<float>(i)/ringCount;
            const Vec3 center=centerAt(t);
            const Vec3 before=centerAt(std::max(0.0f,t-0.002f));
            const Vec3 after=centerAt(std::min(1.0f,t+0.002f));
            const Vec3 tangent=(after-before).normalized();
            Vec3 axisA=tangent.cross({0.0f,0.0f,1.0f}).normalized();
            if(axisA.length()<0.001f) axisA=tangent.cross({0.0f,1.0f,0.0f}).normalized();
            const Vec3 axisB=tangent.cross(axisA).normalized();
            const float rr=std::max(0.002f,radiusAt(t));
            for(int j=0;j<sides;++j) {
                const float a=2.0f*pi*static_cast<float>(j)/sides;
                mesh.vertices.push_back(center+axisA*(std::cos(a)*rr)+axisB*(std::sin(a)*rr));
            }
        }
        for(int i=0;i<ringCount;++i) for(int j=0;j<sides;++j) {
            const auto a=base+static_cast<std::uint32_t>(i*sides+j);
            const auto b=base+static_cast<std::uint32_t>(i*sides+(j+1)%sides);
            const auto c=base+static_cast<std::uint32_t>((i+1)*sides+(j+1)%sides);
            const auto d=base+static_cast<std::uint32_t>((i+1)*sides+j);
            mesh.triangles.push_back({a,d,c}); mesh.triangles.push_back({a,c,b});
        }
    };

    auto add_blob=[&](Vec3 center,Vec3 radii,int lat=12,int lon=20) {
        const std::uint32_t base=static_cast<std::uint32_t>(mesh.vertices.size());
        for(int y=0;y<=lat;++y) {
            const float v=static_cast<float>(y)/lat;
            const float phi=pi*v;
            for(int x=0;x<=lon;++x) {
                const float u=2.0f*pi*static_cast<float>(x)/lon;
                mesh.vertices.push_back({center.x+radii.x*std::sin(phi)*std::cos(u),
                                        center.y+radii.y*std::cos(phi),
                                        center.z+radii.z*std::sin(phi)*std::sin(u)});
            }
        }
        for(int y=0;y<lat;++y) for(int x=0;x<lon;++x) {
            const auto a=base+static_cast<std::uint32_t>(y*(lon+1)+x);
            const auto b=a+1, d=a+static_cast<std::uint32_t>(lon+1), c=d+1;
            mesh.triangles.push_back({a,d,b}); mesh.triangles.push_back({b,d,c});
        }
    };

    // One emitted liquid stream. The central sphere-like bulge is a local
    // radius deformation of this same tube, not a second intersecting tube.
    auto mainCenter=[=](float t) {
        const float impact=std::clamp(t/0.84f,0.0f,1.0f);
        return Vec3{-0.28f*scale+0.18f*scale*std::sin(t*pi*1.4f)+curve*0.10f*std::sin(t*pi),
                     1.65f*scale-2.30f*scale*t,
                     0.04f*scale*std::sin(t*pi*2.0f)};
    };
    auto mainRadius=[=](float t) {
        const float bulge=std::exp(-std::pow((t-0.43f)/0.105f,2.0f));
        const float taperEnd=std::pow(std::max(0.04f,1.0f-t),0.22f);
        return r*(0.20f+0.72f*bulge)*taperEnd;
    };
    add_tube(mainCenter,mainRadius,rings);

    // Impact split: the stream separates into two ground-hugging branches
    // only near the landing point, then spreads into shallow puddles.
    const Vec3 impact{-0.20f*scale,-0.65f*scale,0.0f};
    auto branch=[&](float side) {
        auto center=[=](float t) {
            const float x=impact.x+side*(0.10f+0.88f*t)*scale;
            const float y=impact.y-0.02f*scale+0.08f*scale*std::sin(pi*t);
            const float z=impact.z+side*0.05f*scale*std::sin(pi*t);
            return Vec3{x,y,z};
        };
        auto rad=[=](float t) {
            const float start=0.78f, end=0.20f;
            return r*(start*(1.0f-t)+end*t)*(0.72f+0.28f*std::sin(pi*t));
        };
        add_tube(center,rad,32);
        const Vec3 end=center(1.0f);
        add_blob({end.x,end.y-0.035f*scale,end.z},{r*0.95f,r*0.10f,r*0.58f},10,18);
    };
    branch(-1.0f); branch(1.0f);

    // Falling droplets detach from the stream; the small flattened deposits
    // on the floor are their impact puddles. A few droplets remain airborne.
    add_blob({-0.18f*scale,0.72f*scale,0.08f*scale},{r*0.13f,r*0.18f,r*0.13f},8,12);
    add_blob({-0.10f*scale,0.28f*scale,0.12f*scale},{r*0.10f,r*0.14f,r*0.10f},8,12);
    add_blob({-0.22f*scale,-0.02f*scale,-0.10f*scale},{r*0.11f,r*0.15f,r*0.11f},8,12);
    add_blob({-1.02f*scale,-0.70f*scale,0.02f},{r*0.32f,r*0.07f,r*0.24f},10,16);
    add_blob({0.72f*scale,-0.70f*scale,-0.02f},{r*0.28f,r*0.06f,r*0.20f},10,16);
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
