#ifndef LIBENCH_AVIF_H
#define LIBENCH_AVIF_H

#include "codec.h"

#include "avif/avif.h"

namespace libench {

class AVIFEncoder : public Encoder {
 public:
  AVIFEncoder();
  ~AVIFEncoder() override;

  CodestreamContext encodeRGB8(const ImageContext &image) override;

  CodestreamContext encodeRGBA8(const ImageContext &image) override;

 private:
  CodestreamContext encode8(const ImageContext &image);

  avifRWData output_;
};

class AVIFDecoder : public Decoder {
 public:
  AVIFDecoder();
  ~AVIFDecoder() override;

  ImageContext decodeRGB8(const CodestreamContext& cs) override;

  ImageContext decodeRGBA8(const CodestreamContext& cs) override;

 private:
  ImageContext decode8(const CodestreamContext& cs, uint8_t num_comps);

  avifRGBImage rgb_;
};

}  // namespace libench

#endif
