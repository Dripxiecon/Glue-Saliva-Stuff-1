#pragma once
#include <atomic>
#include <cstdint>
#include <utility>
#include <vector>
#include "glue_saliva_stuff/math/vec3.hpp"
namespace gss::geometry {
struct Triangle { std::uint32_t a{0},b{0},c{0}; };
struct Mesh {
 std::vector<math::Vec3> vertices; std::vector<Triangle> triangles;
 Mesh() noexcept: id_(next_id()) {}
 Mesh(const Mesh& o):vertices(o.vertices),triangles(o.triangles),id_(next_id()),revision_(o.revision_){}
 Mesh(Mesh&& o) noexcept:vertices(std::move(o.vertices)),triangles(std::move(o.triangles)),id_(o.id_),revision_(o.revision_){o.id_=next_id();++o.revision_;}
 Mesh& operator=(const Mesh& o){if(this!=&o){vertices=o.vertices;triangles=o.triangles;mark_dirty();}return *this;}
 Mesh& operator=(Mesh&& o) noexcept{if(this!=&o){vertices=std::move(o.vertices);triangles=std::move(o.triangles);mark_dirty();}return *this;}
 void clear() noexcept{vertices.clear();triangles.clear();mark_dirty();}
 bool empty() const noexcept{return vertices.empty()||triangles.empty();}
 std::uint64_t cache_id()const noexcept{return id_;} std::uint64_t revision()const noexcept{return revision_;}
 // Call after directly editing vertices or triangles.
 void mark_dirty() noexcept{++revision_;}
 private:
 static std::uint64_t next_id() noexcept{static std::atomic<std::uint64_t> n{1};return n.fetch_add(1,std::memory_order_relaxed);}
 std::uint64_t id_{0},revision_{1};
};
}