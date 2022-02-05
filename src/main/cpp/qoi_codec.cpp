#include "qoi_codec.h"
#include <climits>
#include <memory>

#define QOI_IMPLEMENTATION
#include "qoi.h"

/*
 * QOIEncoder
 */

libench::QOIEncoder::QOIEncoder() {
  this->cb_ = {.codestream = NULL, .size = 0};
};

libench::CodestreamBuffer libench::QOIEncoder::encodeRGB8(const uint8_t* pixels,
                                                          uint32_t width,
                                                          uint32_t height) {
  return this->encode8(pixels, width, height, 3);
}

libench::CodestreamBuffer libench::QOIEncoder::encodeRGBA8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  return this->encode8(pixels, width, height, 4);
}

libench::CodestreamBuffer libench::QOIEncoder::encode8(const uint8_t* pixels,
                                                       uint32_t width,
                                                       uint32_t height,
                                                       uint8_t num_comps) {
  free(this->cb_.codestream);

  qoi_desc desc = {.width = width,
                   .height = height,
                   .channels = num_comps,
                   .colorspace = QOI_SRGB};

  int codestream_size;

  this->cb_.codestream = (uint8_t*)qoi_encode(pixels, &desc, &codestream_size);

  this->cb_.size = codestream_size;

  return this->cb_;
}

/*
 * QOIDecoder
 */

libench::QOIDecoder::QOIDecoder(){
  this->pb_.pixels = NULL;
};

libench::PixelBuffer libench::QOIDecoder::decodeRGB8(const uint8_t* codestream,
                                                     size_t size) {
  return this->decode8(codestream, size, 3);
}

libench::PixelBuffer libench::QOIDecoder::decodeRGBA8(const uint8_t* codestream,
                                                      size_t size) {
  return this->decode8(codestream, size, 4);
}

libench::PixelBuffer libench::QOIDecoder::decode8(const uint8_t* codestream,
                                                  size_t size,
                                                  uint8_t num_comps) {
  qoi_desc desc;

  free(this->pb_.pixels);

  this->pb_.pixels = (uint8_t*)qoi_decode(codestream, size, &desc, num_comps);
  this->pb_.width = desc.width;
  this->pb_.height = desc.height;
  this->pb_.num_comps = num_comps;

  return this->pb_;
}