#ifndef LIBENCH_PNG_H
#define LIBENCH_PNG_H

#include <vector>
#include "codec.h"

#include "lodepng.h"

namespace libench {

class PNGEncoder : public Encoder {
 public:
  PNGEncoder();

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

class PNGDecoder : public Decoder {
 public:
  PNGDecoder();

  virtual PixelBuffer decodeRGB8(const uint8_t* codestream, size_t size);

  virtual PixelBuffer decodeRGBA8(const uint8_t* codestream, size_t size);

 private:
  PixelBuffer decode8(const uint8_t* codestream,
                      size_t size,
                      uint8_t num_comps);

  PixelBuffer pb_;
};

}  // namespace libench

#endif