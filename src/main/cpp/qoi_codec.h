#ifndef LIBENCH_QOI_H
#define LIBENCH_QOI_H

#include <vector>
#include "codec.h"

#include "qoi.h"

namespace libench {

class QOIEncoder : public Encoder {
 public:
  QOIEncoder();

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

class QOIDecoder : public Decoder {
 public:
  QOIDecoder();

  virtual PixelBuffer decodeRGB8(const uint8_t* codestream,
                                 size_t size,
                                 uint32_t width,
                                 uint32_t height,
                                 const uint8_t* init_data,
                                 size_t init_data_size);

  virtual PixelBuffer decodeRGBA8(const uint8_t* codestream,
                                  size_t size,
                                  uint32_t width,
                                  uint32_t height,
                                  const uint8_t* init_data,
                                  size_t init_data_size);

 private:
  PixelBuffer decode8(const uint8_t* codestream,
                      size_t size,
                      uint8_t num_comps);

  PixelBuffer pb_;
};

}  // namespace libench

#endif