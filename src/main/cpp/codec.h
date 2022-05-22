#include <stddef.h>
#include <cstdint>

#ifndef LIBENCH_CODEC_H
#define LIBENCH_CODEC_H

namespace libench {

struct CodestreamBuffer {
  uint8_t* codestream;
  size_t size;
  uint8_t* init_data;
  size_t init_data_size;

  CodestreamBuffer()
      : codestream(NULL), size(0), init_data(NULL), init_data_size(0) {}
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

  virtual ~Encoder() {}
};

class Decoder {
 public:
  virtual PixelBuffer decodeRGB8(const uint8_t* codestream,
                                 size_t size,
                                 uint32_t width,
                                 uint32_t height,
                                 const uint8_t* init_data,
                                 size_t init_data_size) = 0;

  virtual PixelBuffer decodeRGBA8(const uint8_t* codestream,
                                  size_t size,
                                  uint32_t width,
                                  uint32_t height,
                                  const uint8_t* init_data,
                                  size_t init_data_size) = 0;

  virtual ~Decoder() {}
};

};  // namespace libench

#endif