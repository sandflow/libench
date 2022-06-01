#ifndef LIBENCH_OPENHTJ2K_H
#define LIBENCH_OPENHTJ2K_H

#include <vector>

#include "codec.h"
#include "decoder.hpp"
#include "encoder.hpp"

namespace libench {

class OHTJ2KEncoder : public Encoder {
 public:
  OHTJ2KEncoder();

  virtual CodestreamBuffer encodeRGB8(const uint8_t* pixels,
                                      const uint32_t width, uint32_t height);

  virtual CodestreamBuffer encodeRGBA8(const uint8_t* pixels, uint32_t width,
                                       uint32_t height);

 private:
  CodestreamBuffer encode8(const uint8_t* pixels, uint32_t width,
                           uint32_t height, uint8_t num_comps);
  std::vector<uint8_t> outbuf_;
  uint32_t num_threads;
};

class OHTJ2KDecoder : public Decoder {
 public:
  OHTJ2KDecoder();

  virtual PixelBuffer decodeRGB8(const uint8_t* codestream, size_t size,
                                 uint32_t width, uint32_t height,
                                 const uint8_t* init_data,
                                 size_t init_data_size);

  virtual PixelBuffer decodeRGBA8(const uint8_t* codestream, size_t size,
                                  uint32_t width, uint32_t height,
                                  const uint8_t* init_data,
                                  size_t init_data_size);

 private:
  PixelBuffer decode8(const uint8_t* codestream, size_t size,
                      uint8_t num_comps);
  std::vector<uint8_t> pixels_;
  uint32_t num_threads;
};
}  // namespace libench
#endif