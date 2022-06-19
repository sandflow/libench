#ifndef LIBENCH_PNG_H
#define LIBENCH_PNG_H

#include <vector>
#include "codec.h"

#include "lodepng.h"

namespace libench {

class PNGEncoder : public Encoder {
 public:
  PNGEncoder();
  ~PNGEncoder();

  CodestreamContext encodeRGB8(const ImageContext &image);

  CodestreamContext encodeRGBA8(const ImageContext &image);

 private:
  CodestreamContext encode8(const ImageContext &image);

  CodestreamContext cb_;
};

class PNGDecoder : public Decoder {
 public:
  PNGDecoder();
  ~PNGDecoder();

  virtual ImageContext decodeRGB8(const CodestreamContext& cs);

  virtual ImageContext decodeRGBA8(const CodestreamContext& cs);

 private:
  ImageContext decode8(const CodestreamContext& cs, uint8_t num_comps);

  ImageContext pb_;
};

}  // namespace libench

#endif