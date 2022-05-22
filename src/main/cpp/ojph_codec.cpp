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

libench::OJPHEncoder::OJPHEncoder(){};

libench::CodestreamBuffer libench::OJPHEncoder::encodeRGB8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  return this->encode8(pixels, width, height, 3);
}

libench::CodestreamBuffer libench::OJPHEncoder::encodeRGBA8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  return this->encode8(pixels, width, height, 4);
}

libench::CodestreamBuffer libench::OJPHEncoder::encode8(const uint8_t* pixels,
                                                        uint32_t width,
                                                        uint32_t height,
                                                        uint8_t num_comps) {
  ojph::codestream cs;

  cs.set_planar(false);

  /* siz */

  ojph::param_siz siz = cs.access_siz();

  siz.set_image_extent(ojph::point(width, height));
  siz.set_num_components(num_comps);
  for (ojph::ui32 c = 0; c < num_comps; c++)
    siz.set_component(c, ojph::point(1, 1), 8, false);
  siz.set_image_offset(ojph::point(0, 0));
  siz.set_tile_size(ojph::size(width, height));
  siz.set_tile_offset(ojph::point(0, 0));

  /* cod */

  ojph::param_cod cod = cs.access_cod();

  cod.set_color_transform(num_comps == 3 || num_comps == 4);
  cod.set_reversible(true);

  /* encode */

  this->out_.close();
  this->out_.open();

  cs.write_headers(&this->out_);

  const uint8_t* line = pixels;
  ojph::ui32 next_comp = 0;
  ojph::line_buf* cur_line = cs.exchange(NULL, next_comp);

  for (uint32_t i = 0; i < height; ++i) {
    for (uint32_t c = 0; c < num_comps; c++) {
      assert(next_comp == c);

      const uint8_t* in = line + c;
      int32_t* out = cur_line->i32;

      for (uint32_t p = 0; p < width; p++) {
        *out = *in;
        out += 1;
        in += num_comps;
      }

      cur_line = cs.exchange(cur_line, next_comp);
    }

    line += num_comps * width;
  }

  cs.flush();

  /* cs is not closed since that would close the file */

  if (this->out_.tell() < 0) {
    throw std::runtime_error("Memory error");
  }

  libench::CodestreamBuffer cb;

  cb.codestream = (uint8_t*)this->out_.get_data();
  cb.size = (size_t)this->out_.tell();

  return cb;
}

/*
 * OJPHDecoder
 */

libench::OJPHDecoder::OJPHDecoder(){};

libench::PixelBuffer libench::OJPHDecoder::decodeRGB8(const uint8_t* codestream,
                                                      size_t size,
                                                      uint32_t width,
                                                      uint32_t height,
                                                      const uint8_t* init_data,
                                                      size_t init_data_size) {
  return this->decode8(codestream, size, 3);
}

libench::PixelBuffer libench::OJPHDecoder::decodeRGBA8(
    const uint8_t* codestream,
    size_t size,
    uint32_t width,
    uint32_t height,
    const uint8_t* init_data,
    size_t init_data_size) {
  return this->decode8(codestream, size, 4);
}

libench::PixelBuffer libench::OJPHDecoder::decode8(const uint8_t* codestream,
                                                   size_t size,
                                                   uint8_t num_comps) {
  ojph::codestream cs;

  this->in_.open(codestream, size);
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

  libench::PixelBuffer pb = {.height = height,
                             .width = width,
                             .num_comps = num_comps,
                             .pixels = &this->pixels_.data()[0]};

  return pb;
}