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
endif()

if(GSS_BUILD_TESTS)
    enable_testing()
    add_executable(gss_tests tests/test_strand_mesh.cpp)
    target_link_libraries(gss_tests PRIVATE GSS::glue_saliva_stuff)
    add_test(NAME gss_tests COMMAND gss_tests)
endif()
