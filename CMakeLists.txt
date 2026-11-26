cmake_minimum_required(VERSION 3.20)
project(GlueSalivaStuff1 VERSION 0.1.0 LANGUAGES CXX)

option(GSS_BUILD_TESTS "Build the lightweight test executable" ON)

add_library(glue_saliva_stuff
    src/strands/strand_mesh.cpp
)
add_library(GSS::glue_saliva_stuff ALIAS glue_saliva_stuff)

target_compile_features(glue_saliva_stuff PUBLIC cxx_std_20)
target_include_directories(glue_saliva_stuff
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(GSS_BUILD_TESTS)
    enable_testing()
    add_executable(gss_tests tests/test_strand_mesh.cpp)
    target_link_libraries(gss_tests PRIVATE GSS::glue_saliva_stuff)
    add_test(NAME gss_tests COMMAND gss_tests)
endif()
