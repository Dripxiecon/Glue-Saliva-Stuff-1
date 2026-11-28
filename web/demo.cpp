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
    const int rings=56;
    const int sides=std::clamp(radial*2,20,40);
    const float scale=std::clamp(length/2.0f,0.55f,1.65f);
    const float r=std::max(0.018f,radius);

    // Lattice-style deformation of a tube. Each sphere influence is represented
    // as a smooth radial field along the tube, so the circular bump is part of
    // the same surface instead of a disconnected sphere intersecting it.
    auto add_lattice_tube=[&](Vec3 start,Vec3 finish,Vec3 bendAxis,
                               float tubeRadius,float sheetWidth,
                               float bulgeT,float bulgeAmount,float bulgeSpread,
                               float phase) {
        Vec3 path=(finish-start).normalized();
        Vec3 reference{0.0f,0.0f,1.0f};
        Vec3 widthAxis=path.cross(reference).normalized();
        if(widthAxis.length()<0.01f) widthAxis=path.cross({0.0f,1.0f,0.0f}).normalized();
        Vec3 normal=widthAxis.cross(path).normalized();
        const std::uint32_t base=static_cast<std::uint32_t>(mesh.vertices.size());
        for(int i=0;i<=rings;++i) {
            const float t=static_cast<float>(i)/rings;
            const float endTaper=0.18f+0.82f*std::pow(std::max(0.0f,std::sin(pi*t)),0.38f);
            const float sphereInfluence=std::exp(-std::pow((t-bulgeT)/std::max(0.035f,bulgeSpread),2.0f));
            const float endSphereA=0.55f*std::exp(-std::pow((t-0.045f)/0.075f,2.0f));
            const float endSphereB=0.55f*std::exp(-std::pow((t-0.955f)/0.075f,2.0f));
            const float bump=bulgeAmount*sphereInfluence+ tubeRadius*(endSphereA+endSphereB);
            const float lattice=1.0f+0.10f*std::sin(t*4.0f*pi+phase)
                                      +0.045f*std::sin(t*9.0f*pi-phase);
            Vec3 center=start+(finish-start)*t;
            center=center+bendAxis*(curve*std::sin(pi*t));
            center=center+normal*(tubeRadius*0.22f*std::sin(2.0f*pi*t+phase));

            // Fixed contact circles distributed across a deliberate,
            // mostly planar web layout. Each tube is deflected locally around
            // the contact instead of all paths converging into one tangled hub.
            struct ContactCircle { Vec3 center; float radius; };
            const std::array<ContactCircle,7> contacts{{
                {{-1.55f*scale,-1.15f*scale,0.0f},r*0.62f},
                {{-0.42f*scale,-0.10f*scale,0.0f},r*0.78f},
                {{1.25f*scale,1.10f*scale,0.0f},r*0.58f},
                {{1.35f*scale,-1.20f*scale,0.0f},r*0.70f},
                {{-0.05f*scale,1.45f*scale,0.0f},r*0.52f},
                {{-1.65f*scale,0.45f*scale,0.0f},r*0.60f},
                {{0.15f*scale,-1.55f*scale,0.0f},r*0.62f}
            }};
            for(const auto& contact:contacts) {
                Vec3 away=center-contact.center;
                away=away-path*away.dot(path);
                const float distance=away.length();
                if(distance<0.0001f) away=widthAxis;
                else away=away/distance;
                const float clearance=contact.radius+tubeRadius*0.28f;
                const float penetration=clearance-distance;
                if(penetration>0.0f) {
                    const float influence=std::clamp(penetration/clearance,0.0f,1.0f);
                    const float smooth=influence*influence*(3.0f-2.0f*influence);
                    center=center+away*(penetration*0.78f*smooth);
                }
            }
            const float widthRadius=(tubeRadius*sheetWidth*endTaper*lattice)+bump;
            const float depthRadius=(tubeRadius*0.42f*endTaper)+bump*0.72f;
            for(int j=0;j<sides;++j) {
                const float a=2.0f*pi*static_cast<float>(j)/sides;
                const float angularLattice=1.0f+0.055f*std::sin(3.0f*a+phase+t*5.0f);
                Vec3 p=center+widthAxis*(std::cos(a)*widthRadius*angularLattice);
                p=p+normal*(std::sin(a)*depthRadius);
                mesh.vertices.push_back(p);
            }
        }
        for(int i=0;i<rings;++i) for(int j=0;j<sides;++j) {
            const auto a=base+static_cast<std::uint32_t>(i*sides+j);
            const auto b=base+static_cast<std::uint32_t>(i*sides+(j+1)%sides);
            const auto c=base+static_cast<std::uint32_t>((i+1)*sides+(j+1)%sides);
            const auto d=base+static_cast<std::uint32_t>((i+1)*sides+j);
            mesh.triangles.push_back({a,d,c});
            mesh.triangles.push_back({a,c,b});
        }
    };

    const Vec3 A{-1.55f*scale,-1.15f*scale,0.0f};
    const Vec3 B{-0.42f*scale,-0.10f*scale,0.18f*scale};
    const Vec3 C{1.25f*scale,1.10f*scale,0.05f*scale};
    const Vec3 D{1.35f*scale,-1.20f*scale,-0.12f*scale};
    const Vec3 E{-0.05f*scale,1.45f*scale,0.22f*scale};
    const Vec3 F{-1.65f*scale,0.45f*scale,-0.08f*scale};
    const Vec3 G{0.15f*scale,-1.55f*scale,0.12f*scale};

    // Main web perimeter: six connected-looking, non-crossing routes.
    // Keep the large membranes on the outside so the center stays open.
    add_lattice_tube(A,F,{0.0f,0.18f,0.0f},r*0.25f,4.6f,0.46f,r*1.45f,0.13f,0.2f);
    add_lattice_tube(F,E,{0.0f,0.18f,0.0f},r*0.24f,4.2f,0.58f,r*1.30f,0.13f,0.8f);
    add_lattice_tube(E,C,{0.0f,-0.15f,0.0f},r*0.23f,3.9f,0.48f,r*1.25f,0.12f,1.4f);
    add_lattice_tube(C,D,{0.0f,-0.18f,0.0f},r*0.25f,4.4f,0.52f,r*1.45f,0.13f,2.0f);
    add_lattice_tube(D,G,{0.0f,0.12f,0.0f},r*0.26f,4.5f,0.56f,r*1.50f,0.14f,2.6f);
    add_lattice_tube(G,A,{0.0f,-0.12f,0.0f},r*0.23f,4.0f,0.42f,r*1.25f,0.12f,3.2f);
    // Three short interior attachments. They create small local contact
    // clusters without long diagonal strands slicing across the whole web.
    add_lattice_tube(B,F,{0.0f,0.10f,0.0f},r*0.18f,2.1f,0.55f,r*0.95f,0.10f,3.8f);
    add_lattice_tube(B,D,{0.0f,-0.10f,0.0f},r*0.18f,2.0f,0.44f,r*0.95f,0.10f,4.4f);
    add_lattice_tube(B,E,{0.0f,0.12f,0.0f},r*0.16f,1.8f,0.62f,r*0.85f,0.09f,5.0f);
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
