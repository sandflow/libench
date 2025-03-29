#include "webp_codec.h"
#include "webp/decode.h"
#include "webp/encode.h"
#include <cstdint>
#include <stdexcept>

/*
 * WEBPEncoder
 */

libench::WEBPEncoder::WEBPEncoder() {
  WebPMemoryWriterInit(&this->writer_);
};

libench::WEBPEncoder::~WEBPEncoder() {
  WebPMemoryWriterClear(&this->writer_);
};

libench::CodestreamContext libench::WEBPEncoder::encodeRGB8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::WEBPEncoder::encodeRGBA8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::WEBPEncoder::encode8(const ImageContext &image) {
  WebPMemoryWriterClear(&this->writer_);

  WebPConfig config;
  WebPPicture pic;
  const int level = 6;  // 0 (faster) - 9 (slower)
  if (!WebPConfigInit(&config) || !WebPConfigLosslessPreset(&config, level) || !WebPPictureInit(&pic))
    throw std::runtime_error("WEBP encode failed");

  const int num_comps = image.format.comps.num_comps;
  const int rgb_stride = image.width * num_comps;
  pic.width = image.width;
  pic.height = image.height;
  pic.use_argb = 1;
  int ret = num_comps == 3 ?
      WebPPictureImportRGB(&pic, image.planes8[0], rgb_stride) :
      WebPPictureImportRGBA(&pic, image.planes8[0], rgb_stride);
  if (!ret) {
    WebPPictureFree(&pic);
    throw std::runtime_error("WEBP encode failed");
  }

  // Retain pixel values in fully transparent areas. The default will modify
  // the (invisible) pixels to improve compression.
  config.exact = 1;

  pic.writer = WebPMemoryWrite;
  pic.custom_ptr = &this->writer_;

  if (!WebPEncode(&config, &pic)) {
    WebPPictureFree(&pic);
    WebPMemoryWriterClear(&this->writer_);
    throw std::runtime_error("WEBP encode failed");
  }

  CodestreamContext cs;
  cs.codestream = this->writer_.mem;
  cs.size = this->writer_.size;
  return cs;
}

/*
 * WEBPDecoder
 */

libench::WEBPDecoder::WEBPDecoder() {
};

libench::WEBPDecoder::~WEBPDecoder() {
  WebPFree(this->image_.planes8[0]);
};

libench::ImageContext libench::WEBPDecoder::decodeRGB8(const CodestreamContext& cs) {
  return this->decode8(cs, 3);
}

libench::ImageContext libench::WEBPDecoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode8(cs, 4);
}

libench::ImageContext libench::WEBPDecoder::decode8(const CodestreamContext& cs, uint8_t num_comps) {
  WebPFree(this->image_.planes8[0]);

  this->image_.format = num_comps == 3 ? libench::ImageFormat::RGB8 : libench::ImageFormat::RGBA8;

  int width, height;
  this->image_.planes8[0] = num_comps == 3 ?
      WebPDecodeRGB(cs.codestream, cs.size, &width, &height) :
      WebPDecodeRGBA(cs.codestream, cs.size, &width, &height);

  if (!this->image_.planes8[0])
    throw std::runtime_error("WEBP decode failed");

  this->image_.width = static_cast<uint32_t>(width);
  this->image_.height = static_cast<uint32_t>(height);

  return this->image_;
}
