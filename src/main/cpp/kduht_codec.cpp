#include "kduht_codec.h"
#include <iostream>
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

class error_message_handler : public kdu_core::kdu_message {
 public:

  void put_text(const char* msg) {
    std::cout << msg;
  }

  virtual void flush(bool end_of_message = false) {
    if (end_of_message) {
        std::cout << std::endl;
    }
  }
};

static error_message_handler error_handler;

libench::KDUEncoder::KDUEncoder(bool isHT) : isHT_(isHT){
  kdu_core::kdu_customize_errors(&error_handler);
};

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

  codestream.set_disabled_auto_comments(0xFFFFFFFF);

  codestream.access_siz()
      ->access_cluster(COD_params)
      ->set(Creversible, 0, 0, true);

  codestream.access_siz()
      ->access_cluster(COD_params)
      ->set(Corder, 0, 0, Corder_CPRL);

  if (this->isHT_) {
    codestream.access_siz()
        ->access_cluster(COD_params)
        ->set(Cmodes, 0, 0, Cmodes_HT);
  }

  codestream.access_siz()->finalize_all();

  kdu_stripe_compressor compressor;
  compressor.start(codestream, 0, NULL, NULL, 0, false, false, false, 0, 0,
                   true);
  int stripe_heights[4] = {(int)image.height, (int)image.height, (int)image.height, (int)image.height};

  if (image.format.is_planar && image.is_plane16()) {

    int precisions[4];
    bool is_signed[4];
    
    for(uint8_t i = 0; i < image.format.comps.num_comps; i++) {
      precisions[i] = (int) image.format.bit_depth;
      is_signed[i] = false;
    }
    compressor.push_stripe((kdu_int16 **) image.planes16, stripe_heights, NULL, NULL, precisions, is_signed);

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
  return this->decode(cs);
}

libench::ImageContext libench::KDUDecoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode(cs);
}

libench::ImageContext libench::KDUDecoder::decodeYUV(const CodestreamContext& cs) {
  return this->decode(cs);
}

libench::ImageContext libench::KDUDecoder::decode(const CodestreamContext& cs) {
  libench::ImageContext image;

  kdu_compressed_source_buffered buffer((kdu_byte*)cs.codestream, cs.size);

  kdu_codestream c;

  c.create(&buffer);

  kdu_dims dims;
  c.get_dims(0, dims);
  image.height = dims.size.y;
  image.width = dims.size.x;

  int num_comps = c.get_num_components();

  if (num_comps < 3 || num_comps > 4) {
    throw std::runtime_error("Bad number of components");
  }

  image.format.is_planar = false;

  for(int i = 0; i < num_comps; i++) {
    kdu_core::kdu_coords coords;

    c.get_subsampling(i, coords);

    image.format.x_sub_factor[i] = coords.x;
    image.format.y_sub_factor[i] = coords.y;

    if (coords.x > 1 || coords.y > 1)
      image.format.is_planar = true;
  }

  image.format.bit_depth = c.get_bit_depth(0);

  if (image.format.is_planar) {
    image.format.comps = libench::ImageComponents::YUV;
  } else if (num_comps == 3) {
    image.format.comps = libench::ImageComponents::RGB;
  } else if (num_comps == 4) {
    image.format.comps = libench::ImageComponents::RGBA;
  }

  kdu_stripe_decompressor d;

  int stripe_heights[4] = {(int) image.height, (int) image.height, (int) image.height, (int) image.height};
  
  d.start(c);

  if (image.format.is_planar) {

    int precisions[4];
    bool is_signed[4];

    for(int i = 0; i < num_comps; i++) {
      this->planes_[i].resize(image.plane_size(i));
      image.planes16[i] = (uint16_t*) this->planes_[i].data();
      precisions[i] = (int) image.format.bit_depth;
      is_signed[i] = false;
    }
    
    if (! image.is_plane16()) {
      throw std::runtime_error("Only YUV 10 bits supported.");
    }

    kdu_int16 *planes[3] = {(kdu_int16*) this->planes_[0].data(),
                            (kdu_int16*) this->planes_[1].data(),
                            (kdu_int16*) this->planes_[2].data()};

    d.pull_stripe(planes, stripe_heights, NULL, NULL, precisions, is_signed);

  } else {

    this->planes_[0].resize(image.plane_size(0));
    image.planes8[0] = this->planes_[0].data();

    d.pull_stripe(this->planes_[0].data(), stripe_heights);
    
  }

  d.finish();

  return image;
}