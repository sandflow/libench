#ifndef LIBENCH_JXL_H
#define LIBENCH_JXL_H

#include <vector>
#include "codec.h"

namespace libench {

template <int E>
class JXLEncoder : public Encoder {
 public:
  JXLEncoder();
  virtual ~JXLEncoder();

  CodestreamContext encodeRGB8(const ImageContext &image);

  CodestreamContext encodeRGBA8(const ImageContext &image);

 private:
  CodestreamContext encode8(const ImageContext &image, uint8_t num_comps);

  CodestreamContext cb_;
};

class JXLDecoder : public Decoder {
 public:
  JXLDecoder();


  virtual ImageContext decodeRGB8(const CodestreamContext& cs);

  virtual ImageContext decodeRGBA8(const CodestreamContext& cs);

 private:
  ImageContext decode8(const CodestreamContext& cs, uint8_t num_comps);

  std::vector<uint8_t> pixels_;
};

}  // namespace libench

#endif