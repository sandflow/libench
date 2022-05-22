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
  AVPacket* pkt_;
  AVFrame* frame_;
  const AVCodec* codec_;
  AVCodecContext* codec_ctx_;
};

class FFV1Decoder : public Decoder {
 public:
  FFV1Decoder();
  ~FFV1Decoder();

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
                      uint8_t num_comps,
                      uint32_t width,
                      uint32_t height,
                      const uint8_t* init_data,
                      size_t init_data_size);

  AVPacket* pkt_;
  AVFrame* frame_;
  const AVCodec* codec_;
  std::vector<uint8_t> pixels_;
};

}  // namespace libench

#endif