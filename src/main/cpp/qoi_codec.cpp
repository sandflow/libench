#include "qoi_codec.h"
#include <climits>
#include <memory>

#define QOI_IMPLEMENTATION
#include "qoi.h"

/*
 * QOIEncoder
 */

libench::QOIEncoder::QOIEncoder() {}

libench::QOIEncoder::~QOIEncoder() {
  free(this->cs_.codestream);
}

libench::CodestreamContext libench::QOIEncoder::encodeRGB8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::QOIEncoder::encodeRGBA8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::QOIEncoder::encode8(const ImageContext &image) {
  free(this->cs_.codestream);

  qoi_desc desc = {.width = image.width,
                   .height = image.height,
                   .channels = image.format.comps.num_comps,
                   .colorspace = QOI_SRGB};

  int codestream_size;

  this->cs_.codestream = (uint8_t*)qoi_encode(image.planes8[0], &desc, &codestream_size);

  this->cs_.size = codestream_size;

  return this->cs_;
}

/*
 * QOIDecoder
 */

libench::QOIDecoder::QOIDecoder() {
}

libench::QOIDecoder::~QOIDecoder() {
  free(this->image_.planes8[0]);
}

libench::ImageContext libench::QOIDecoder::decodeRGB8(const CodestreamContext& cs) {
  return this->decode8(cs);
}

libench::ImageContext libench::QOIDecoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode8(cs);
}

libench::ImageContext libench::QOIDecoder::decode8(const CodestreamContext& cs) {
  qoi_desc desc;

  free(this->image_.planes8[0]);

  this->image_.planes8[0] = (uint8_t*)qoi_decode(cs.codestream, cs.size, &desc, 0);
  this->image_.width = desc.width;
  this->image_.height = desc.height;
  this->image_.format = desc.channels == 3 ? libench::ImageFormat::RGB8 : libench::ImageFormat::RGBA8;

  return this->image_;
}
