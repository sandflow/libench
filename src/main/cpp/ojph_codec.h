#ifndef LIBENCH_OJPH_H
#define LIBENCH_OJPH_H

#include <vector>
#include "codec.h"
#include "ojph_arch.h"
#include "ojph_codestream.h"
#include "ojph_file.h"

namespace libench {

class OJPHEncoder : public Encoder {
 public:
  OJPHEncoder();

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
  ojph::mem_outfile out_;
};

class OJPHDecoder : public Decoder {
 public:
  OJPHDecoder();

  virtual PixelBuffer decodeRGB8(const uint8_t* codestream, size_t size);

  virtual PixelBuffer decodeRGBA8(const uint8_t* codestream, size_t size);

 private:
  PixelBuffer decode8(const uint8_t* codestream,
                      size_t size,
                      uint8_t num_comps);
  ojph::mem_infile in_;
  std::vector<uint8_t> pixels_;
};

}  // namespace libench

#endif