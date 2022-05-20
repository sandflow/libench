#ifndef LIBENCH_FFV1_H
#define LIBENCH_FFV1_H

#include "codec.h"
#include <vector>

namespace libench {

class FFV1Encoder : public Encoder {
 public:
  FFV1Encoder();

  virtual CodestreamBuffer encodeRGB8(const uint8_t* pixels,
                                      const uint32_t width,
                                      uint32_t height);

  virtual CodestreamBuffer encodeRGBA8(const uint8_t* pixels,
                                       uint32_t width,
                                       uint32_t height);

 private:
  CodestreamBuffer encode8(const uint8_t* pixels,
                           uint32_t width,
                           uint32_t height,
                           uint8_t num_comps);
  CodestreamBuffer cb_;
};

class FFV1Decoder : public Decoder {
 public:
  FFV1Decoder();

  virtual PixelBuffer decodeRGB8(const uint8_t* codestream, size_t size);

  virtual PixelBuffer decodeRGBA8(const uint8_t* codestream, size_t size);

 private:
  PixelBuffer decode8(const uint8_t* codestream,
                      size_t size,
                      uint8_t num_comps);

  std::vector<uint8_t> pixels_;
};

}  // namespace libench

#endif