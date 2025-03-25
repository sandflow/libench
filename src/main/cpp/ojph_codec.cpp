#include "ojph_codec.h"
#include <assert.h>
#include <stdexcept>
#include "ojph_mem.h"
#include "ojph_params.h"

/*
 * OJPHEncoder
 */

static ojph::size IMF_PRECINCTS[] = {ojph::size(256, 256), ojph::size(256, 256),
                                     ojph::size(256, 256), ojph::size(256, 256),
                                     ojph::size(256, 256), ojph::size(256, 256),
                                     ojph::size(128, 128)};

libench::OJPHEncoder::OJPHEncoder(){}

libench::CodestreamContext libench::OJPHEncoder::encodeRGB8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::OJPHEncoder::encodeRGBA8(const ImageContext &image) {
  return this->encode8(image);
}

libench::CodestreamContext libench::OJPHEncoder::encode8(const ImageContext &image) {
  ojph::codestream cs;

  cs.set_planar(false);

  /* siz */

  ojph::param_siz siz = cs.access_siz();

  siz.set_image_extent(ojph::point(image.width, image.height));
  siz.set_num_components(image.format.comps.num_comps);
  for (ojph::ui32 c = 0; c < image.format.comps.num_comps; c++)
    siz.set_component(c, ojph::point(1, 1), 8, false);
  siz.set_image_offset(ojph::point(0, 0));
  siz.set_tile_size(ojph::size(image.width, image.height));
  siz.set_tile_offset(ojph::point(0, 0));

  /* cod */

  ojph::param_cod cod = cs.access_cod();

  cod.set_color_transform(image.format.comps.num_comps == 3 || image.format.comps.num_comps == 4);
  cod.set_reversible(true);

  /* encode */

  this->out_.close();
  this->out_.open();

  cs.write_headers(&this->out_);

  const uint8_t* line = image.planes8[0];
  ojph::ui32 next_comp = 0;
  ojph::line_buf* cur_line = cs.exchange(NULL, next_comp);

  for (uint32_t i = 0; i < image.height; ++i) {
    for (uint32_t c = 0; c < image.format.comps.num_comps; c++) {
      assert(next_comp == c);

      const uint8_t* in = line + c;
      int32_t* out = cur_line->i32;

      for (uint32_t p = 0; p < image.width; p++) {
        *out = *in;
        out += 1;
        in += image.format.comps.num_comps;
      }

      cur_line = cs.exchange(cur_line, next_comp);
    }

    line += image.format.comps.num_comps * image.width;
  }

  cs.flush();

  /* cs is not closed since that would close the file */

  if (this->out_.tell() < 0) {
    throw std::runtime_error("Memory error");
  }

  libench::CodestreamContext cb;

  cb.codestream = (uint8_t*)this->out_.get_data();
  cb.size = (size_t)this->out_.tell();

  return cb;
}

/*
 * OJPHDecoder
 */

libench::OJPHDecoder::OJPHDecoder(){}

libench::ImageContext libench::OJPHDecoder::decodeRGB8(const CodestreamContext& cs) {
  return this->decode8(cs, 3);
}

libench::ImageContext libench::OJPHDecoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode8(cs, 4);
}

libench::ImageContext libench::OJPHDecoder::decode8(const CodestreamContext& ctx, uint8_t num_comps) {
  ojph::codestream cs;

  this->in_.open(ctx.codestream, ctx.size);
  cs.read_headers(&this->in_);

  ojph::param_siz siz = cs.access_siz();
  ojph::ui32 width = siz.get_image_extent().x - siz.get_image_offset().x;
  ojph::ui32 height = siz.get_image_extent().y - siz.get_image_offset().y;

  if (num_comps != siz.get_num_components()) {
    throw std::runtime_error("Unexpected number of components");
  }

  cs.set_planar(false);

  cs.create();

  this->pixels_.resize(width * height * num_comps);

  for (uint32_t i = 0; i < height; ++i) {
    uint8_t* line = &this->pixels_.data()[width * i * num_comps];

    for (uint32_t c = 0; c < num_comps; c++) {
      ojph::ui32 next_comp = 0;
      ojph::line_buf* cur_line = cs.pull(next_comp);
      assert(next_comp == c);

      uint8_t* out = line + c;
      int32_t* in = cur_line->i32;

      for (uint32_t p = 0; p < width; p++) {
        *out = *in;
        out += num_comps;
        in += 1;
      }
    }
  }

  this->in_.close();

  libench::ImageContext image;

  image.height = (uint32_t)height;
  image.width = (uint32_t)width;
  image.format = num_comps == 3 ? libench::ImageFormat::RGB8 : libench::ImageFormat::RGBA8;
  image.planes8[0] = this->pixels_.data();

  return image;
}
