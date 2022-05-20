#include "ffv1_codec.h"
#include <inttypes.h>
#include <climits>
#include <memory>
#include <stdexcept>
#include <vector>
extern "C" {
  #include "libavcodec/codec.h"
}

/*
 * FFV1Encoder
 */

libench::FFV1Encoder::FFV1Encoder() {
  this->cb_ = {.codestream = NULL, .size = 0};

  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_FFV1);
  if (!codec)
    throw std::runtime_error("avcodec_find_encoder AV_CODEC_ID_FFV1 failed");
};

libench::CodestreamBuffer libench::FFV1Encoder::encodeRGB8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  free(this->cb_.codestream);

  // this->cb_.size = FastLosslessEncode(pixels, width, width * 3, height, 3, 8,
  // E, &this->cb_.codestream);

  return this->cb_;
}

libench::CodestreamBuffer libench::FFV1Encoder::encodeRGBA8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  free(this->cb_.codestream);

  // this->cb_.size = FastLosslessEncode(pixels, width, width * 4, height, 4, 8,
  // E, &this->cb_.codestream);

  return this->cb_;
}

/*
 * FFV1Decoder
 */

libench::FFV1Decoder::FFV1Decoder(){};

libench::PixelBuffer libench::FFV1Decoder::decodeRGB8(const uint8_t* codestream,
                                                      size_t size) {
  return this->decode8(codestream, size, 3);
}

libench::PixelBuffer libench::FFV1Decoder::decodeRGBA8(
    const uint8_t* codestream,
    size_t size) {
  return this->decode8(codestream, size, 4);
}

libench::PixelBuffer libench::FFV1Decoder::decode8(const uint8_t* codestream,
                                                   size_t size,
                                                   uint8_t num_comps) {
  libench::PixelBuffer pb;

  pb.num_comps = num_comps;

  return pb;
}
