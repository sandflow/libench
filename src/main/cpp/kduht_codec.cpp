#include "kduht_codec.h"
#include <assert.h>
#include <stdexcept>
#include "kdu_compressed.h"
#include "kdu_file_io.h"
#include "kdu_messaging.h"
#include "kdu_sample_processing.h"
#include "kdu_stripe_compressor.h"
#include "kdu_stripe_decompressor.h"

/*
 * KDUEncoder
 */

using namespace kdu_supp;

libench::KDUEncoder::KDUEncoder(bool isHT) : isHT_(isHT){};

libench::CodestreamBuffer libench::KDUEncoder::encodeRGB8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  return this->encode8(pixels, width, height, 3);
}

libench::CodestreamBuffer libench::KDUEncoder::encodeRGBA8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  return this->encode8(pixels, width, height, 4);
}

libench::CodestreamBuffer libench::KDUEncoder::encode8(const uint8_t* pixels,
                                                         uint32_t width,
                                                         uint32_t height,
                                                         uint8_t num_comps) {
  siz_params siz;
  siz.set(Scomponents, 0, 0, num_comps);
  siz.set(Sdims, 0, 0, static_cast<int>(height));
  siz.set(Sdims, 0, 1, static_cast<int>(width));
  siz.set(Sprecision, 0, 0, 8);
  siz.set(Ssigned, 0, 0, false);
  static_cast<kdu_params&>(siz).finalize();

  this->out_.close();
  
  kdu_codestream codestream;

  codestream.create(&siz, &this->out_);
  codestream.access_siz()->access_cluster(COD_params)->set(Creversible, 0, 0, true);
  if (this->isHT_)
    codestream.access_siz()->access_cluster(COD_params)->set(Cmodes, 0, 0, Cmodes_HT);

  codestream.access_siz()->finalize_all();

  kdu_stripe_compressor compressor;
  compressor.start(codestream,
    0,
    NULL,
    NULL,
    0,
    false,
    false,
    false,
    0,
    0,
    true
  );
  int stripe_heights[4] = {(int)height, (int)height, (int)height, (int)height};
  compressor.push_stripe((kdu_byte*)pixels, stripe_heights);
  compressor.finish();

  libench::CodestreamBuffer cb = {.codestream = this->out_.get_buffer().data(),
                                  .size = this->out_.get_buffer().size()};

  return cb;
}

/*
 * KDUDecoder
 */

libench::KDUDecoder::KDUDecoder(){};

libench::PixelBuffer libench::KDUDecoder::decodeRGB8(
    const uint8_t* codestream,
    size_t size) {
  return this->decode8(codestream, size, 3);
}

libench::PixelBuffer libench::KDUDecoder::decodeRGBA8(
    const uint8_t* codestream,
    size_t size) {
  return this->decode8(codestream, size, 4);
}

libench::PixelBuffer libench::KDUDecoder::decode8(const uint8_t* codestream,
                                                    size_t size,
                                                    uint8_t num_comps) {
  kdu_compressed_source_buffered buffer((kdu_byte*)codestream, size);

  kdu_codestream c;

  c.create(&buffer);

  kdu_dims dims;
  c.get_dims(0, dims);

  int height = dims.size.y;
  int width = dims.size.x;

  kdu_stripe_decompressor d;

  this->pixels_.resize(width * height * num_comps);

  int stripe_heights[4] = {height, height, height, height};

  d.start(c);

  d.pull_stripe(this->pixels_.data(), stripe_heights);

  d.finish();

  libench::PixelBuffer pb = {.height = (uint32_t)height,
                             .width = (uint32_t)width,
                             .num_comps = num_comps,
                             .pixels = this->pixels_.data()};

  return pb;
}