# Glue & Saliva Stuff 1

A little sticky. A little messy. A whole lot of fun.

Glue & Saliva Stuff 1 lets you create stretchy strands, gooey connections, dangling threads, and liquid-filled surfaces—all without heavy simulations.

Pull it, stretch it, make a mess.

## Current foundation

- Standalone C++20 core; no Blender dependency
- 3D world transforms, perspective camera, scene objects, and renderer interface are included from the start
- Strand control points generate triangle meshes with per-point radius
- Optional OpenGL ES 3 renderer for Android/mobile and other GLES hosts
- Feature-separated source and public headers

## Build the core

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Enable the GLES 3 renderer

Configure with `-DGSS_ENABLE_GLES3=ON`. The application hosting the library must create and activate an OpenGL ES 3 context and provide the surface/swapchain. The renderer then draws the scene's 3D meshes using that context. This keeps platform windowing separate while the project already has its 3D scene and rendering path.

The GLES renderer is an initial implementation; persistent GPU mesh caching, materials, lighting controls, and translucent rendering will be refined separately. Volume fill, smooth strand junctions, and lightweight secondary motion are planned as independent modules.
