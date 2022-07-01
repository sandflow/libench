#include "png_codec.h"
#include <climits>
#include <memory>
#include <stdexcept>

/*
 * PNGEncoder
 */

libench::PNGEncoder::PNGEncoder() {};
libench::PNGEncoder::~PNGEncoder() {
  free(this->cs_.codestream);
};

libench::CodestreamContext libench::PNGEncoder::encodeRGB8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::PNGEncoder::encodeRGBA8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::PNGEncoder::encode8(const ImageContext &image) {
  int ret;

  free(this->cs_.codestream);

  ret = lodepng_encode_memory(&this->cs_.codestream, &this->cs_.size, image.planes8[0],
                              image.width, image.height,
                              image.format.comps.num_comps == 3 ? LCT_RGB : LCT_RGBA, 8);

  if (ret)
    throw std::runtime_error("PNG decode failed");

  return this->cs_;
}

/*
 * PNGDecoder
 */

libench::PNGDecoder::PNGDecoder() {
};

libench::PNGDecoder::~PNGDecoder() {
  free(this->image_.planes8[0]);
};

libench::ImageContext libench::PNGDecoder::decodeRGB8(const CodestreamContext& cs) {
  return this->decode8(cs, 3);
}

libench::ImageContext libench::PNGDecoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode8(cs, 4);
}

libench::ImageContext libench::PNGDecoder::decode8(const CodestreamContext& cs, uint8_t num_comps) {
  int ret;

  free(this->image_.planes8[0]);

  this->image_.format = num_comps == 3 ? libench::ImageFormat::RGB8 : libench::ImageFormat::RGBA8;

  ret = lodepng_decode_memory(&this->image_.planes8[0], &this->image_.width,
                              &this->image_.height, cs.codestream, cs.size,
                              num_comps == 3 ? LCT_RGB : LCT_RGBA, 8);

  if (ret)
    throw std::runtime_error("PNG decode failed");

  return this->image_;
}
