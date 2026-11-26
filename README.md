# Glue & Saliva Stuff 1

A little sticky. A little messy. A whole lot of fun.

Glue & Saliva Stuff 1 lets you create stretchy strands, gooey connections, dangling threads, and liquid-filled surfaces—all without heavy simulations.

Pull it, stretch it, make a mess.

## Status

Early standalone C++20 library. The first module generates lightweight tube meshes from strand control points. Volume fill, smooth junctions, and secondary motion are planned as separate modules.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The core has no Blender dependency and is designed to be integrated into other projects through CMake.
