#include "png_codec.h"
#include <climits>
#include <memory>
#include <stdexcept>

/*
 * PNGEncoder
 */

libench::PNGEncoder::PNGEncoder() {
  this->cb_ = {.codestream = NULL, .size = 0};
};

libench::CodestreamBuffer libench::PNGEncoder::encodeRGB8(const uint8_t* pixels,
                                                          uint32_t width,
                                                          uint32_t height) {
  return this->encode8(pixels, width, height, 3);
}

libench::CodestreamBuffer libench::PNGEncoder::encodeRGBA8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  return this->encode8(pixels, width, height, 4);
}

libench::CodestreamBuffer libench::PNGEncoder::encode8(const uint8_t* pixels,
                                                       uint32_t width,
                                                       uint32_t height,
                                                       uint8_t num_comps) {
  int ret;

  free(this->cb_.codestream);

  ret = lodepng_encode_memory(&this->cb_.codestream, &this->cb_.size, pixels, width, height, num_comps == 3 ? LCT_RGB : LCT_RGBA, 8);

  if (ret)
    throw std::runtime_error("PNG decode failed");

  return this->cb_;
}

/*
 * PNGDecoder
 */

libench::PNGDecoder::PNGDecoder() {
  this->pb_.pixels = NULL;
};

libench::PixelBuffer libench::PNGDecoder::decodeRGB8(const uint8_t* codestream,
                                                     size_t size) {
  return this->decode8(codestream, size, 3);
}

libench::PixelBuffer libench::PNGDecoder::decodeRGBA8(const uint8_t* codestream,
                                                      size_t size) {
  return this->decode8(codestream, size, 4);
}

libench::PixelBuffer libench::PNGDecoder::decode8(const uint8_t* codestream,
                                                  size_t size,
                                                  uint8_t num_comps) {
  int ret;

  free(this->pb_.pixels);

  this->pb_.num_comps = num_comps;

  ret = lodepng_decode_memory(&this->pb_.pixels, &this->pb_.width, &this->pb_.height,
                               codestream, size,
                               num_comps == 3 ? LCT_RGB : LCT_RGBA, 8);

  if (ret)
    throw std::runtime_error("PNG decode failed");

  return this->pb_;
}
