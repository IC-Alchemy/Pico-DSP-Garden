cmake_minimum_required(VERSION 3.16)
project(rpdsp_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# rpdsp headers resolve the same way the examples resolve them.
include_directories(${CMAKE_SOURCE_DIR}/../libraries/rpdsp/src)
include_directories(${CMAKE_SOURCE_DIR}/../libraries/HarmonyEngine/src)

add_executable(rpdsp_tests
    main.cpp
    test_compile_all.cpp
    test_algorithm.cpp
    test_counterpoint_pipeline.cpp
    test_control_surface.cpp
    test_oscillator.cpp
    test_tension_sculptor_pipeline.cpp
)

enable_testing()
add_test(NAME rpdsp_tests COMMAND rpdsp_tests)
