cmake_minimum_required (VERSION 3.2)

project(lossless-image-benchmark)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Kakadu SDK support

find_path(KDU_INCLUDE_DIR kdu_args.h PATH_SUFFIXES kakadu)
find_library(KDU_LIBRARY NAMES libkdu_a82R)

if(KDU_INCLUDE_DIR AND KDU_LIBRARY)
  set(KDU_PRESENT 1)
  message("Kakadu SDK found: ${KDU_LIBRARY} and ${KDU_INCLUDE_DIR}")
  include_directories(${KDU_INCLUDE_DIR})
else()
  message("Kakadu SDK not found.")
endif()

# OpenJPH support

add_subdirectory(ext/OpenJPH)
set_property(DIRECTORY ext/OpenJPH PROPERTY OJPH_ENABLE_TIFF_SUPPORT OFF)

include_directories(ext/OpenJPH/src/core/common)

# stb

include_directories(ext/stb)

# lodepng

include_directories(ext/lodepng)

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

# ffmpeg

#   CONFIGURE_COMMAND ./configure --disable-avdevice --disable-avformat --disable-swresample --disable-swscale --disable-avfilter --disable-doc --disable-programs --prefix=${FFMPEG_INSTALL_DIR}


include(ExternalProject)
set(FFMPEG_INSTALL_DIR ${CMAKE_BINARY_DIR}/ext/ffmpeg)
ExternalProject_Add(ffmpeg
  SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/ext/ffmpeg
  CONFIGURE_COMMAND ./configure --disable-avdevice --disable-avformat --disable-swresample --disable-swscale --disable-avfilter --disable-doc --disable-programs --disable-everything --enable-encoder=ffv1 --enable-decoder=ffv1 --prefix=${FFMPEG_INSTALL_DIR}
  BUILD_COMMAND     make -j${CONCURRENCY}
  BUILD_IN_SOURCE   TRUE
)
include_directories(${PROJECT_BINARY_DIR}/ext/ffmpeg/include)
add_library(libavcodec STATIC IMPORTED)
add_dependencies(libavcodec ffmpeg)
set_target_properties(libavcodec PROPERTIES IMPORTED_LOCATION ${FFMPEG_INSTALL_DIR}/lib/libavcodec.a)
add_library(libavutil STATIC IMPORTED)
add_dependencies(libavutil ffmpeg)
set_target_properties(libavutil PROPERTIES IMPORTED_LOCATION ${FFMPEG_INSTALL_DIR}/lib/libavutil.a)

# md5

include_directories(ext/crypto-algorithms)
add_library(md5 ext/crypto-algorithms/md5.c)

# main executable

file(GLOB LIBENCH_SRC_FILES src/main/cpp/*)
add_executable(libench ${LIBENCH_SRC_FILES} ext/lodepng/lodepng.cpp)
target_link_libraries(libench openjph md5 jxl jxl_threads fast_lossless libavcodec libavutil ${KDU_LIBRARY} ${CMAKE_DL_LIBS})

# tests

enable_testing()

add_test(NAME "j2k_ht_ojph" COMMAND libench j2k_ht_ojph ${PROJECT_SOURCE_DIR}/src/test/resources/images/test1.png)
add_test(NAME "qoi-rgb" COMMAND libench qoi ${PROJECT_SOURCE_DIR}/src/test/resources/images/test1.png)
add_test(NAME "qoi-rgba" COMMAND libench qoi ${PROJECT_SOURCE_DIR}/src/test/resources/images/rgba.png)
add_test(NAME "jxl-rgb" COMMAND libench jxl ${PROJECT_SOURCE_DIR}/src/test/resources/images/test1.png)
add_test(NAME "jxl-rgba" COMMAND libench jxl -r 1 --dir . ${PROJECT_SOURCE_DIR}/src/test/resources/images/rgba.png)
add_test(NAME "j2k_ht_kdu" COMMAND libench j2k_ht_kdu ${PROJECT_SOURCE_DIR}/src/test/resources/images/test1.png)
add_test(NAME "j2k_1_kdu" COMMAND libench j2k_1_kdu ${PROJECT_SOURCE_DIR}/src/test/resources/images/test1.png)
add_test(NAME "png" COMMAND libench png ${PROJECT_SOURCE_DIR}/src/test/resources/images/test1.png)
add_test(NAME "ffv1" COMMAND libench ffv1 ${PROJECT_SOURCE_DIR}/src/test/resources/images/test1.png)
