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

libench::CodestreamContext libench::KDUEncoder::encodeRGB8(const ImageContext &image) {
  return this->encode(image);
}

libench::CodestreamContext libench::KDUEncoder::encodeRGBA8(const ImageContext &image) {
  return this->encode(image);
}

libench::CodestreamContext libench::KDUEncoder::encodeYUV(const ImageContext &image) {
  return this->encode(image);
}

libench::CodestreamContext libench::KDUEncoder::encode(const ImageContext &image) {
  siz_params siz;
  siz.set(Scomponents, 0, 0, image.format.comps.num_comps);
  for(uint8_t i = 0; i < image.format.num_planes(); i++) {
    siz.set(Sdims, i, 0, static_cast<int>(image.height) / image.format.y_sub_factor[i]);
    siz.set(Sdims, i, 1, static_cast<int>(image.width) / image.format.x_sub_factor[i]);
  }
  siz.set(Sprecision, 0, 0, image.format.bit_depth);
  siz.set(Ssigned, 0, 0, false);
  static_cast<kdu_params&>(siz).finalize();

  this->out_.close();

  kdu_codestream codestream;

  codestream.create(&siz, &this->out_);
  codestream.access_siz()
      ->access_cluster(COD_params)
      ->set(Creversible, 0, 0, true);
  if (this->isHT_)
    codestream.access_siz()
        ->access_cluster(COD_params)
        ->set(Cmodes, 0, 0, Cmodes_HT);

  codestream.access_siz()->finalize_all();

  kdu_stripe_compressor compressor;
  compressor.start(codestream, 0, NULL, NULL, 0, false, false, false, 0, 0,
                   true);
  int stripe_heights[4] = {(int)image.height, (int)image.height, (int)image.height, (int)image.height};

  if (image.format.is_planar && image.is_plane16()) {
    compressor.push_stripe((kdu_int16 **) &image.planes16, stripe_heights);
  } else if ((!image.format.is_planar) && (!image.is_plane16())) {
    compressor.push_stripe((kdu_byte*)image.planes8[0], stripe_heights);
  }
  compressor.finish();

  libench::CodestreamContext cb;
  
  cb.codestream = this->out_.get_buffer().data();
  cb.size = this->out_.get_buffer().size();

  return cb;
}

/*
 * KDUDecoder
 */

libench::KDUDecoder::KDUDecoder(){};

libench::ImageContext libench::KDUDecoder::decodeRGB8(const CodestreamContext& cs) {
  return this->decode8(cs, 3);
}

libench::ImageContext libench::KDUDecoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode8(cs, 4);
}

libench::ImageContext libench::KDUDecoder::decode8(const CodestreamContext& cs, uint8_t num_comps) {
  kdu_compressed_source_buffered buffer((kdu_byte*)cs.codestream, cs.size);

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

  libench::ImageContext image;
  
  image.height = (uint32_t)height;
  image.width = (uint32_t)width;
  image.format = num_comps == 3 ? libench::ImageFormat::RGB8 : libench::ImageFormat::RGBA8;
  image.planes8[0] = this->pixels_.data();

  return image;
}