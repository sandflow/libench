cmake_minimum_required (VERSION 3.2)

project(lossless-image-benchmark)

# OpenJPH support

add_subdirectory(ext/OpenJPH)
set_property(DIRECTORY ext/OpenJPH PROPERTY OJPH_ENABLE_TIFF_SUPPORT OFF)

include_directories(ext/OpenJPH/src/core/common)

# stb

include_directories(ext/stb)

# qoi

include_directories(ext/qoi)

# cxxopts

include_directories(ext/cxxopts/include)

# jxl

set(JPEGXL_ENABLE_BENCHMARK FALSE CACHE INTERNAL "" FORCE)
set(JPEGXL_ENABLE_JNI FALSE CACHE INTERNAL "" FORCE)
set(JPEGXL_ENABLE_SJPEG FALSE CACHE INTERNAL "" FORCE)
set(JPEGXL_ENABLE_OPENEXR FALSE CACHE INTERNAL "" FORCE)
set(JPEGXL_ENABLE_TRANSCODE_JPEG FALSE CACHE INTERNAL "" FORCE)
set(JPEGXL_BUNDLE_SKCMS FALSE CACHE INTERNAL "" FORCE)
set(JPEGXL_ENABLE_MANPAGES FALSE CACHE INTERNAL "" FORCE)
include_directories(ext/libjxl/experimental/fast_lossless ext/libjxl/lib/include "${PROJECT_BINARY_DIR}/ext/libjxl/lib/include")
add_library(fast_lossless ext/libjxl/experimental/fast_lossless/fast_lossless.cc)
add_subdirectory(ext/libjxl)

# md5

include_directories(ext/crypto-algorithms)
add_library(md5 ext/crypto-algorithms/md5.c)

# main executable

add_executable(libench src/main/cpp/main.cpp src/main/cpp/ojph_codec.cpp src/main/cpp/qoi_codec.cpp src/main/cpp/jxl_codec.cpp)
target_link_libraries(libench openjph md5 jxl jxl_threads fast_lossless)

# tests

enable_testing()

add_test(NAME "ojph" COMMAND libench ojph ${PROJECT_SOURCE_DIR}/src/test/resources/test1.png)
add_test(NAME "qoi" COMMAND libench qoi ${PROJECT_SOURCE_DIR}/src/test/resources/test1.png)
add_test(NAME "jxl-rgb" COMMAND libench jxl ${PROJECT_SOURCE_DIR}/src/test/resources/test1.png)
add_test(NAME "jxl-rgba" COMMAND libench jxl -r 1 --dir . ${PROJECT_SOURCE_DIR}/src/test/resources/rgba.png)