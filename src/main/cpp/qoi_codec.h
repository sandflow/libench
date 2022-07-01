#ifndef LIBENCH_QOI_H
#define LIBENCH_QOI_H

#include <vector>
#include "codec.h"

#include "qoi.h"

namespace libench {

class QOIEncoder : public Encoder {
 public:
  QOIEncoder();
  ~QOIEncoder();

  CodestreamContext encodeRGB8(const ImageContext &image);

  CodestreamContext encodeRGBA8(const ImageContext &image);

 private:
  CodestreamContext encode8(const ImageContext &image);

  CodestreamContext cs_;
};

class QOIDecoder : public Decoder {
 public:
  QOIDecoder();
  ~QOIDecoder();

  virtual ImageContext decodeRGB8(const CodestreamContext& cs);

  virtual ImageContext decodeRGBA8(const CodestreamContext& cs);

 private:
  ImageContext decode8(const CodestreamContext& cs);

  ImageContext image_;
};

}  // namespace libench

#endif