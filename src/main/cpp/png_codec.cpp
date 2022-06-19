#include "png_codec.h"
#include <climits>
#include <memory>
#include <stdexcept>

/*
 * PNGEncoder
 */

libench::PNGEncoder::PNGEncoder() {};
libench::PNGEncoder::~PNGEncoder() {
  free(this->cb_.codestream);
};

libench::CodestreamContext libench::PNGEncoder::encodeRGB8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::PNGEncoder::encodeRGBA8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::PNGEncoder::encode8(const ImageContext &image) {
  int ret;

  free(this->cb_.codestream);

  ret = lodepng_encode_memory(&this->cb_.codestream, &this->cb_.size, image.planes8[0],
                              image.width, image.height,
                              image.num_comps == 3 ? LCT_RGB : LCT_RGBA, 8);

  if (ret)
    throw std::runtime_error("PNG decode failed");

  return this->cb_;
}

/*
 * PNGDecoder
 */

libench::PNGDecoder::PNGDecoder() {
    this->pb_.bit_depth = 8;
    this->pb_.num_planes = 1;
};

libench::PNGDecoder::~PNGDecoder() {
  free(this->pb_.planes8[0]);
};

libench::ImageContext libench::PNGDecoder::decodeRGB8(const CodestreamContext& cs) {
  return this->decode8(cs, 3);
}

libench::ImageContext libench::PNGDecoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode8(cs, 4);
}

libench::ImageContext libench::PNGDecoder::decode8(const CodestreamContext& cs, uint8_t num_comps) {
  int ret;

  free(this->pb_.planes8[0]);

  this->pb_.num_comps = num_comps;

  ret = lodepng_decode_memory(&this->pb_.planes8[0], &this->pb_.width,
                              &this->pb_.height, cs.codestream, cs.size,
                              num_comps == 3 ? LCT_RGB : LCT_RGBA, 8);

  if (ret)
    throw std::runtime_error("PNG decode failed");

  return this->pb_;
}
