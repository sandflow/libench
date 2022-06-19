#ifndef LIBENCH_FFV1_H
#define LIBENCH_FFV1_H

#include <vector>
#include "codec.h"

extern "C" {
#include "libavcodec/codec.h"
#include "libavcodec/packet.h"
#include <libavcodec/avcodec.h>
}

namespace libench {

class FFV1Encoder : public Encoder {
 public:
  FFV1Encoder();
  ~FFV1Encoder();

  CodestreamContext encodeRGB8(const ImageContext &image);

  CodestreamContext encodeRGBA8(const ImageContext &image);

 private:
  CodestreamContext encode8(const ImageContext &image, uint8_t num_comps);

  AVPacket* pkt_;
  AVFrame* frame_;
  const AVCodec* codec_;
  AVCodecContext* codec_ctx_;
};

class FFV1Decoder : public Decoder {
 public:
  FFV1Decoder();
  ~FFV1Decoder();
  
  virtual ImageContext decodeRGB8(const CodestreamContext& cs);

  virtual ImageContext decodeRGBA8(const CodestreamContext& cs);

 private:
  ImageContext decode8(const CodestreamContext& cs, uint8_t num_comps);

  AVPacket* pkt_;
  AVFrame* frame_;
  const AVCodec* codec_;
  std::vector<uint8_t> pixels_;
};

}  // namespace libench

#endif