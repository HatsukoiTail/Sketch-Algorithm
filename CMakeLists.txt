set(CMAKE_CXX_STANDARD 20)

add_library(SketchAlgorithm STATIC
    FunnelSketch/FunnelSketch.cpp
)

target_include_directories(
    SketchAlgorithm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/FunnelSketch
)