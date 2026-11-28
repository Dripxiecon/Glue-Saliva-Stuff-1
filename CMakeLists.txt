cmake_minimum_required(VERSION 3.20)
project(GlueSalivaStuff1 VERSION 0.2.0 LANGUAGES CXX)

option(GSS_BUILD_TESTS "Build the lightweight test executable" ON)
option(GSS_BUILD_EXAMPLES "Compile scene setup examples" ON)
option(GSS_ENABLE_GLES3 "Build the OpenGL ES 3 renderer (host must provide a current GLES context)" OFF)

add_library(glue_saliva_stuff
    src/strands/strand_mesh.cpp
    src/junctions/junction_mesh.cpp
    src/junctions/junction_assembly.cpp
    src/junctions/blended_junction_mesh.cpp
)
add_library(GSS::glue_saliva_stuff ALIAS glue_saliva_stuff)
target_compile_features(glue_saliva_stuff PUBLIC cxx_std_20)
target_include_directories(glue_saliva_stuff PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(GSS_ENABLE_GLES3)
    target_sources(glue_saliva_stuff PRIVATE src/render/gles3_renderer.cpp)
    target_compile_definitions(glue_saliva_stuff PUBLIC GSS_ENABLE_GLES3=1)
    find_library(GSS_GLES3_LIBRARY GLESv3 REQUIRED)
    target_link_libraries(glue_saliva_stuff PUBLIC ${GSS_GLES3_LIBRARY})
endif()

if(GSS_BUILD_EXAMPLES)
    add_library(gss_scene_example OBJECT examples/scene_demo.cpp)
    target_link_libraries(gss_scene_example PRIVATE GSS::glue_saliva_stuff)

    add_executable(gss_mesh_inspector examples/mesh_inspector.cpp)
    target_link_libraries(gss_mesh_inspector PRIVATE GSS::glue_saliva_stuff)
endif()

if(GSS_BUILD_TESTS)
    enable_testing()
    add_executable(gss_tests tests/test_strand_mesh.cpp)
    target_link_libraries(gss_tests PRIVATE GSS::glue_saliva_stuff)
    add_test(NAME gss_tests COMMAND gss_tests)
endif()

if(EMSCRIPTEN)
    # Keep the browser module compact so mobile browsers have less WASM to compile.
    target_compile_options(glue_saliva_stuff PRIVATE -Oz)
    set_property(TARGET glue_saliva_stuff PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

    add_executable(gss_web_demo web/demo.cpp)
    target_compile_options(gss_web_demo PRIVATE -Oz)
    set_property(TARGET gss_web_demo PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    target_link_libraries(gss_web_demo PRIVATE GSS::glue_saliva_stuff)
    set_target_properties(gss_web_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/site
        OUTPUT_NAME gss_demo
    )
    target_link_options(gss_web_demo PRIVATE
        "--no-entry"
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=GSSModule"
        "-sENVIRONMENT=web"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sFILESYSTEM=0"
        "-sASSERTIONS=0"
        "-Oz"
        "-sEXPORTED_FUNCTIONS=['_gss_generate','_gss_vertex_count','_gss_index_count','_gss_vertices_ptr','_gss_indices_ptr']"
        "-sEXPORTED_RUNTIME_METHODS=['HEAPF32','HEAPU32']"
    )
endif()
