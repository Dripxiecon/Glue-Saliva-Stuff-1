#include "glue_saliva_stuff/render/gles3_renderer.hpp"
#ifdef GSS_ENABLE_GLES3
#include <vector>
#include <cstddef>
namespace gss::render {
namespace {
GLuint compile_shader(GLenum type,const char* source) {
    GLuint shader=glCreateShader(type); glShaderSource(shader,1,&source,nullptr); glCompileShader(shader);
    GLint ok=0; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok); if(!ok){glDeleteShader(shader);return 0;} return shader;
}
}
Gles3Renderer::~Gles3Renderer(){shutdown();}
bool Gles3Renderer::initialize(){
    const char* vs="#version 300 es\nprecision highp float; layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal; uniform mat4 uMVP; uniform mat4 uModel; out vec3 vNormal; void main(){vNormal=mat3(uModel)*aNormal;gl_Position=uMVP*vec4(aPos,1.0);}";
    const char* fs="#version 300 es\nprecision mediump float; in vec3 vNormal; uniform vec3 uColor; uniform float uOpacity; out vec4 fragColor; void main(){vec3 n=normalize(vNormal);float light=0.28+0.72*max(dot(n,normalize(vec3(0.4,0.8,0.5))),0.0);fragColor=vec4(uColor*light,uOpacity);} ";
    GLuint v=compile_shader(GL_VERTEX_SHADER,vs), f=compile_shader(GL_FRAGMENT_SHADER,fs); if(!v||!f){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return false;}
    program_=glCreateProgram();glAttachShader(program_,v);glAttachShader(program_,f);glLinkProgram(program_);glDeleteShader(v);glDeleteShader(f);
    GLint linked=0;glGetProgramiv(program_,GL_LINK_STATUS,&linked);if(!linked){shutdown();return false;}
    mvp_location_=glGetUniformLocation(program_,"uMVP");model_location_=glGetUniformLocation(program_,"uModel");color_location_=glGetUniformLocation(program_,"uColor");return true;
}
void Gles3Renderer::shutdown() noexcept {if(program_){glDeleteProgram(program_);program_=0;}}
bool Gles3Renderer::begin_frame(int width,int height,const math::Mat4& vp){if(!program_||width<=0||height<=0)return false;view_projection_=vp;glViewport(0,0,width,height);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glClearColor(0.035f,0.045f,0.065f,1.0f);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glUseProgram(program_);return true;}
void Gles3Renderer::draw_mesh(const geometry::Mesh& mesh,const math::Mat4& model,const Material& material){
    if(mesh.empty())return;
    struct Vertex{float p[3];float n[3];};std::vector<math::Vec3> normals(mesh.vertices.size());
    for(const auto& t:mesh.triangles){if(t.a>=mesh.vertices.size()||t.b>=mesh.vertices.size()||t.c>=mesh.vertices.size())continue;auto n=(mesh.vertices[t.b]-mesh.vertices[t.a]).cross(mesh.vertices[t.c]-mesh.vertices[t.a]);normals[t.a]=normals[t.a]+n;normals[t.b]=normals[t.b]+n;normals[t.c]=normals[t.c]+n;}
    std::vector<Vertex> vertices;vertices.reserve(mesh.vertices.size());for(std::size_t i=0;i<mesh.vertices.size();++i){auto n=normals[i].normalized();vertices.push_back({{mesh.vertices[i].x,mesh.vertices[i].y,mesh.vertices[i].z},{n.x,n.y,n.z}});}
    std::vector<std::uint32_t> indices;indices.reserve(mesh.triangles.size()*3);for(const auto&t:mesh.triangles){if(t.a>=vertices.size()||t.b>=vertices.size()||t.c>=vertices.size())continue;indices.insert(indices.end(),{t.a,t.b,t.c});}
    if(indices.empty())return;
    GLuint vao=0,vbo=0,ebo=0;glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glGenBuffers(1,&ebo);glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(std::uint32_t),indices.data(),GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,p)));
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,n)));
    const auto mvp=view_projection_*model;glUniformMatrix4fv(mvp_location_,1,GL_FALSE,mvp.v.data());glUniformMatrix4fv(model_location_,1,GL_FALSE,model.v.data());glUniform3f(color_location_,material.base_color.x,material.base_color.y,material.base_color.z);
    GLint opacity=glGetUniformLocation(program_,"uOpacity");if(opacity>=0)glUniform1f(opacity,material.opacity);
    glDrawElements(GL_TRIANGLES,static_cast<GLsizei>(indices.size()),GL_UNSIGNED_INT,nullptr);
    glBindVertexArray(0);glDeleteBuffers(1,&ebo);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);
}
void Gles3Renderer::end_frame(){glUseProgram(0);}
}
#endif
