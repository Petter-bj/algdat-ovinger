cmake_minimum_required(VERSION 3.16)
project(oving6 LANGUAGES CXX)

# Explicit list: simpleLZalgo.cpp is a separate exercise program with its own
# main() and is intentionally not part of this target.
add_executable(lzh
    main.cpp
    compressor.cpp
    decompressor.cpp
    huffman.cpp
    format.cpp
)
target_compile_features(lzh PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(lzh PRIVATE /W4 /O2)
else()
    target_compile_options(lzh PRIVATE -O2 -Wall -Wextra -Wpedantic)
endif()
