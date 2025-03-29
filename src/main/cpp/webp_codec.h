#ifndef LIBENCH_WEBP_H
#define LIBENCH_WEBP_H

#include "codec.h"
#include "webp/encode.h"

namespace libench {

class WEBPEncoder : public Encoder {
 public:
  WEBPEncoder();
  ~WEBPEncoder() override;

  CodestreamContext encodeRGB8(const ImageContext &image) override;

  CodestreamContext encodeRGBA8(const ImageContext &image) override;

 private:
  CodestreamContext encode8(const ImageContext &image);

  WebPMemoryWriter writer_;
};

class WEBPDecoder : public Decoder {
 public:
  WEBPDecoder();
  ~WEBPDecoder() override;

  ImageContext decodeRGB8(const CodestreamContext& cs) override;

  ImageContext decodeRGBA8(const CodestreamContext& cs) override;

 private:
  ImageContext decode8(const CodestreamContext& cs, uint8_t num_comps);

  ImageContext image_;
};

}  // namespace libench

#endif
