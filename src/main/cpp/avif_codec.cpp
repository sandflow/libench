#include "avif_codec.h"
#include "avif/avif_cxx.h"
#include <cstring>
#include <stdexcept>

/*
 * AVIFEncoder
 */

libench::AVIFEncoder::AVIFEncoder() {
  this->output_ = AVIF_DATA_EMPTY;
}

libench::AVIFEncoder::~AVIFEncoder() {
  avifRWDataFree(&this->output_);
}

libench::CodestreamContext libench::AVIFEncoder::encodeRGB8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::AVIFEncoder::encodeRGBA8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::AVIFEncoder::encode8(const ImageContext &image) {
  avifRWDataFree(&this->output_);

  avif::ImagePtr avif(avifImageCreate(image.width, image.height, 8,
                                      AVIF_PIXEL_FORMAT_YUV444));
  if (!avif)
    throw std::runtime_error("avifImageCreate failed");
  avif->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_IDENTITY;
  avifRGBImage rgb;
  avifRGBImageSetDefaults(&rgb, avif.get());
  rgb.format = image.format.comps.num_comps == 3 ? AVIF_RGB_FORMAT_RGB
                                                 : AVIF_RGB_FORMAT_RGBA;
  rgb.pixels = image.planes8[0];
  rgb.rowBytes = image.width * image.format.comps.num_comps;
  avifResult result = avifImageRGBToYUV(avif.get(), &rgb);
  if (result != AVIF_RESULT_OK)
    throw std::runtime_error("avifImageRGBToYUV failed");
  avif::EncoderPtr encoder(avifEncoderCreate());
  if (!encoder)
    throw std::runtime_error("avifEncoderCreate failed");
  encoder->speed = 6;
  encoder->quality = AVIF_QUALITY_LOSSLESS;
  encoder->qualityAlpha = encoder->quality;
  encoder->autoTiling = AVIF_TRUE;
  result = avifEncoderAddImage(encoder.get(), avif.get(), 1,
                               AVIF_ADD_IMAGE_FLAG_SINGLE);
  if (result != AVIF_RESULT_OK)
    throw std::runtime_error("avifEncoderAddImage failed");
  result = avifEncoderFinish(encoder.get(), &this->output_);
  if (result != AVIF_RESULT_OK)
    throw std::runtime_error("avifEncoderFinish failed");

  CodestreamContext cs;
  cs.codestream = this->output_.data;
  cs.size = this->output_.size;
  return cs;
}

/*
 * AVIFDecoder
 */

libench::AVIFDecoder::AVIFDecoder() {
  memset(&this->rgb_, 0, sizeof(this->rgb_));
}

libench::AVIFDecoder::~AVIFDecoder() {
  avifRGBImageFreePixels(&this->rgb_);
}

libench::ImageContext libench::AVIFDecoder::decodeRGB8(const CodestreamContext& cs) {
  return this->decode8(cs, 3);
}

libench::ImageContext libench::AVIFDecoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode8(cs, 4);
}

libench::ImageContext libench::AVIFDecoder::decode8(const CodestreamContext& cs, uint8_t num_comps) {
  avif::DecoderPtr decoder(avifDecoderCreate());
  if (!decoder)
    throw std::runtime_error("avifDecoderCreate failed");
  avifResult result = avifDecoderSetIOMemory(decoder.get(), cs.codestream,
                                             cs.size);
  if (result != AVIF_RESULT_OK)
    throw std::runtime_error("avifDecoderSetIOMemory failed");
  result = avifDecoderParse(decoder.get());
  if (result != AVIF_RESULT_OK)
    throw std::runtime_error("avifDecoderParse failed");
  result = avifDecoderNextImage(decoder.get());
  if (result != AVIF_RESULT_OK)
    throw std::runtime_error("avifDecoderNextImage failed");

  if (decoder->image->depth != 8)
    throw std::runtime_error("Bit depth must be 8");
  if (decoder->image->yuvFormat != AVIF_PIXEL_FORMAT_YUV444)
    throw std::runtime_error("YUV format must be 4:4:4 for lossless");
  if (decoder->image->matrixCoefficients != AVIF_MATRIX_COEFFICIENTS_IDENTITY)
    throw std::runtime_error("Matrix coefficients must be identity for lossless");

  avifRGBImageSetDefaults(&this->rgb_, decoder->image);
  this->rgb_.format = num_comps == 3 ? AVIF_RGB_FORMAT_RGB
                                     : AVIF_RGB_FORMAT_RGBA;
  result = avifRGBImageAllocatePixels(&this->rgb_);
  if (result != AVIF_RESULT_OK)
    throw std::runtime_error("avifRGBImageAllocatePixels failed");
  result = avifImageYUVToRGB(decoder->image, &this->rgb_);
  if (result != AVIF_RESULT_OK)
    throw std::runtime_error("avifImageYUVToRGB failed");

  ImageContext image;
  image.width = decoder->image->width;
  image.height = decoder->image->height;
  image.format = num_comps == 3 ? libench::ImageFormat::RGB8
                                : libench::ImageFormat::RGBA8;
  image.planes8[0] = this->rgb_.pixels;
  return image;
}
