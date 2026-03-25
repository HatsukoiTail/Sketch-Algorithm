set(CMAKE_CXX_STANDARD 20)

add_library(SketchAlgorithm STATIC
    "FunnelSketch/FunnelSketch.cpp"
    "SP-Sketch/SP-Sketch.cpp"
    "PS-Sketch/PS-Sketch.cpp"
    "PISketch/PISketch.cpp"
    "On-Off Sketch/On-Off Sketch.cpp"
    "BloomFilter/BloomFilter.cpp"
    "HyperLogLog/HyperLogLog.cpp"
    "LinearCounting/LinearCounting.cpp"
    "VHLL/VHLL.cpp"
    "Hash/Murmur.cpp"
    "Hash/Hash.h"
)

target_include_directories(
    SketchAlgorithm PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/FunnelSketch"
    "${CMAKE_CURRENT_SOURCE_DIR}/SP-Sketch"
    "${CMAKE_CURRENT_SOURCE_DIR}/PS-Sketch"
    "${CMAKE_CURRENT_SOURCE_DIR}/PISketch"
    "${CMAKE_CURRENT_SOURCE_DIR}/On-Off Sketch"
    "${CMAKE_CURRENT_SOURCE_DIR}/BloomFilter"
    "${CMAKE_CURRENT_SOURCE_DIR}/HyperLogLog"
    "${CMAKE_CURRENT_SOURCE_DIR}/LinearCounting"
    "${CMAKE_CURRENT_SOURCE_DIR}/VHLL"
    "${CMAKE_CURRENT_SOURCE_DIR}/Hash"
)