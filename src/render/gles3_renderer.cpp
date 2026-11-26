#include "glue_saliva_stuff/render/gles3_renderer.hpp"
#ifdef GSS_ENABLE_GLES3
#include <vector>
#include <cstddef>
namespace gss::render { namespace {
struct V{float p[3],n[3];};
GLuint shader(GLenum type,const char*s){GLuint x=glCreateShader(type);glShaderSource(x,1,&s,nullptr);glCompileShader(x);GLint ok=0;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok){glDeleteShader(x);return 0;}return x;}
}
Gles3Renderer::~Gles3Renderer(){shutdown();}
bool Gles3Renderer::initialize(){
 const char*vs="#version 300 es\nprecision highp float;layout(location=0)in vec3 p;layout(location=1)in vec3 n;uniform mat4 uMVP;uniform mat4 uModel;out vec3 N;void main(){N=mat3(uModel)*n;gl_Position=uMVP*vec4(p,1.0);}";
 const char*fs="#version 300 es\nprecision mediump float;in vec3 N;uniform vec3 uColor;uniform float uOpacity;out vec4 c;void main(){float l=.28+.72*max(dot(normalize(N),normalize(vec3(.4,.8,.5))),0.);c=vec4(uColor*l,uOpacity);}";
 GLuint a=shader(GL_VERTEX_SHADER,vs),b=shader(GL_FRAGMENT_SHADER,fs);if(!a||!b){if(a)glDeleteShader(a);if(b)glDeleteShader(b);return false;}
 program_=glCreateProgram();glAttachShader(program_,a);glAttachShader(program_,b);glLinkProgram(program_);glDeleteShader(a);glDeleteShader(b);GLint ok=0;glGetProgramiv(program_,GL_LINK_STATUS,&ok);if(!ok){shutdown();return false;}
 mvp_=glGetUniformLocation(program_,"uMVP");model_=glGetUniformLocation(program_,"uModel");color_=glGetUniformLocation(program_,"uColor");opacity_=glGetUniformLocation(program_,"uOpacity");return true;}
void Gles3Renderer::destroy(CachedMesh&m)noexcept{if(m.ebo)glDeleteBuffers(1,&m.ebo);if(m.vbo)glDeleteBuffers(1,&m.vbo);if(m.vao)glDeleteVertexArrays(1,&m.vao);m={};}
void Gles3Renderer::shutdown()noexcept{for(auto&e:cache_)destroy(e.second);cache_.clear();if(program_){glDeleteProgram(program_);program_=0;}}
bool Gles3Renderer::begin_frame(int w,int h,const math::Mat4&vp){if(!program_||w<=0||h<=0)return false;++frame_;vp_=vp;glViewport(0,0,w,h);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glClearColor(.035f,.045f,.065f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glUseProgram(program_);return true;}
void Gles3Renderer::draw_mesh(const geometry::Mesh&m,const math::Mat4&model,const Material&mat){if(m.empty())return;auto&g=cache_[m.cache_id()];g.last_seen=frame_;
 if(!g.vao||g.revision!=m.revision()){
 std::vector<math::Vec3> ns(m.vertices.size());for(auto&t:m.triangles)if(t.a<m.vertices.size()&&t.b<m.vertices.size()&&t.c<m.vertices.size()){auto n=(m.vertices[t.b]-m.vertices[t.a]).cross(m.vertices[t.c]-m.vertices[t.a]);ns[t.a]=ns[t.a]+n;ns[t.b]=ns[t.b]+n;ns[t.c]=ns[t.c]+n;}
 std::vector<V> vs;vs.reserve(m.vertices.size());for(size_t i=0;i<m.vertices.size();++i){auto n=ns[i].normalized();vs.push_back({{m.vertices[i].x,m.vertices[i].y,m.vertices[i].z},{n.x,n.y,n.z}});}
 std::vector<std::uint32_t> ix;ix.reserve(m.triangles.size()*3);for(auto&t:m.triangles)if(t.a<vs.size()&&t.b<vs.size()&&t.c<vs.size())ix.insert(ix.end(),{t.a,t.b,t.c});if(ix.empty())return;
 if(!g.vao){glGenVertexArrays(1,&g.vao);glGenBuffers(1,&g.vbo);glGenBuffers(1,&g.ebo);}glBindVertexArray(g.vao);glBindBuffer(GL_ARRAY_BUFFER,g.vbo);glBufferData(GL_ARRAY_BUFFER,vs.size()*sizeof(V),vs.data(),GL_STATIC_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,g.ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,ix.size()*sizeof(std::uint32_t),ix.data(),GL_STATIC_DRAW);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(V),reinterpret_cast<void*>(offsetof(V,p)));glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(V),reinterpret_cast<void*>(offsetof(V,n)));glBindVertexArray(0);g.count=static_cast<GLsizei>(ix.size());g.revision=m.revision();}
 auto mvp=vp_*model;glUniformMatrix4fv(mvp_,1,GL_FALSE,mvp.v.data());glUniformMatrix4fv(model_,1,GL_FALSE,model.v.data());glUniform3f(color_,mat.base_color.x,mat.base_color.y,mat.base_color.z);if(opacity_>=0)glUniform1f(opacity_,mat.opacity);glBindVertexArray(g.vao);glDrawElements(GL_TRIANGLES,g.count,GL_UNSIGNED_INT,nullptr);glBindVertexArray(0);}
void Gles3Renderer::end_frame(){for(auto i=cache_.begin();i!=cache_.end();)if(frame_>i->second.last_seen+120){destroy(i->second);i=cache_.erase(i);}else ++i;glUseProgram(0);}
}
#endif