#include <stddef.h>
#include <cstdint>

#ifndef LIBENCH_CODEC_H
#define LIBENCH_CODEC_H

namespace libench {

struct CodestreamBuffer {
  uint8_t* codestream;
  size_t size;
};

struct PixelBuffer {
  uint32_t height;
  uint32_t width;
  uint8_t num_comps;
  uint8_t* pixels;
};

class Encoder {
 public:
  virtual CodestreamBuffer encodeRGB8(const uint8_t* pixels,
                                             uint32_t width,
                                             uint32_t height) = 0;

  virtual CodestreamBuffer encodeRGBA8(const uint8_t* pixels,
                                              uint32_t width,
                                              uint32_t height) = 0;
};

class Decoder {
 public:
  virtual PixelBuffer decodeRGB8(const uint8_t* codestream, size_t size) = 0;

  virtual PixelBuffer decodeRGBA8(const uint8_t* codestream, size_t size) = 0;
};

};  // namespace libench

#endif