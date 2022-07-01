#include "codec.h"

libench::ImageComponents libench::ImageComponents::RGBA = libench::ImageComponents(4, "RGBA");
libench::ImageComponents libench::ImageComponents::RGB = libench::ImageComponents(3, "RGB");
libench::ImageComponents libench::ImageComponents::YUV = libench::ImageComponents(3, "YUV");

libench::ImageFormat libench::ImageFormat::RGBA8 = libench::ImageFormat(8, libench::ImageComponents::RGBA, false, {1, 1, 1, 1}, {1, 1, 1, 1});
libench::ImageFormat libench::ImageFormat::RGB8 = libench::ImageFormat(8, libench::ImageComponents::RGB, false, {1, 1, 1, 1}, {1, 1, 1, 1});
libench::ImageFormat libench::ImageFormat::YUV422P10 = libench::ImageFormat(10, libench::ImageComponents::YUV, true, {1, 2, 2, 1}, {1, 1, 1, 1});
