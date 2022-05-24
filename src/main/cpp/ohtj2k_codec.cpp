#include "ohtj2k_codec.h"

#include <assert.h>

#include <memory>
#include <stdexcept>

/*
 * OHTJ2KEncoder
 */

libench::OHTJ2KEncoder::OHTJ2KEncoder() : num_threads(0){};

libench::CodestreamBuffer libench::OHTJ2KEncoder::encodeRGB8(
    const uint8_t* pixels, uint32_t width, uint32_t height) {
  return this->encode8(pixels, width, height, 3);
}

libench::CodestreamBuffer libench::OHTJ2KEncoder::encodeRGBA8(
    const uint8_t* pixels, uint32_t width, uint32_t height) {
  return this->encode8(pixels, width, height, 4);
}

libench::CodestreamBuffer libench::OHTJ2KEncoder::encode8(const uint8_t* pixels,
                                                          uint32_t width,
                                                          uint32_t height,
                                                          uint8_t num_comps) {
  // siz
  open_htj2k::siz_params siz;
  siz.Rsiz = 0;
  siz.Xsiz = width;
  siz.Ysiz = height;
  siz.XOsiz = 0;
  siz.YOsiz = 0;
  siz.XTsiz = 0;
  siz.YTsiz = 0;
  siz.XTOsiz = 0;
  siz.YTOsiz = 0;
  siz.Csiz = num_comps;
  for (auto c = 0; c < siz.Csiz; ++c) {
    siz.Ssiz.push_back(7);
    auto compw = width;
    auto comph = height;
    siz.XRsiz.push_back(((siz.Xsiz - siz.XOsiz) + compw - 1) / compw);
    siz.YRsiz.push_back(((siz.Ysiz - siz.YOsiz) + comph - 1) / comph);
  }

  // cod
  open_htj2k::cod_params cod;
  cod.blkwidth = 5;   // 2^(5+2) = 128
  cod.blkheight = 3;  // 2^(3+2) = 32
  cod.is_max_precincts = false;
  cod.use_SOP = false;
  cod.use_EPH = false;
  cod.progression_order = 2;
  cod.number_of_layers = 1;
  cod.use_color_trafo = 1;
  cod.dwt_levels = 6;
  cod.codeblock_style = 0x040;
  cod.transformation = 1;

  for (size_t i = 0; i < 6; ++i) {
    cod.PPx.push_back(8);  // 2^8 = 256
    cod.PPy.push_back(8);
  }
  cod.PPx.push_back(7);  // 2^7 = 128
  cod.PPy.push_back(7);

  // qcd
  open_htj2k::qcd_params qcd;
  qcd.is_derived = false;
  qcd.number_of_guardbits = 1;
  qcd.base_step = 1.0 / 256;
  if (qcd.base_step == 0.0) {
    qcd.base_step = 1.0f / static_cast<float>(1 << 8);
  }

  bool isJPH = false;
  uint8_t Qfactor = 0xff;   // 0xff means no qfactor value is specified
  uint8_t color_space = 0;  // 0:RGB, 1:YCC

  auto buf = std::make_unique<std::unique_ptr<int32_t[]>[]>(num_comps);
  for (size_t i = 0; i < num_comps; ++i) {
    buf[i] = std::make_unique<int32_t[]>(width * height);
  }
  std::vector<int32_t*> input_buf;
  for (auto c = 0; c < num_comps; ++c) {
    input_buf.push_back(buf[c].get());
  }
  const uint8_t* line = pixels;
  for (uint32_t i = 0; i < height; ++i) {
    for (uint32_t c = 0; c < num_comps; c++) {
      const uint8_t* in = line + c;
      int32_t* out = buf[c].get() + i * width;
      for (uint32_t p = 0; p < width; p++) {
        *out = *in;
        out += 1;
        in += num_comps;
      }
    }
    line += num_comps * width;
  }
  open_htj2k::openhtj2k_encoder encoder("", input_buf, siz, cod, qcd, Qfactor,
                                        isJPH, color_space, num_threads);
  encoder.set_output_buffer(this->outbuf_);

  size_t total_size = total_size = encoder.invoke();

  libench::CodestreamBuffer cb;

  cb.codestream = outbuf_.data();
  cb.size = total_size;

  return cb;
}

/*
 * OHTJ2KDecoder
 */

libench::OHTJ2KDecoder::OHTJ2KDecoder() : num_threads(0){};

libench::PixelBuffer libench::OHTJ2KDecoder::decodeRGB8(
    const uint8_t* codestream, size_t size, uint32_t width, uint32_t height,
    const uint8_t* init_data, size_t init_data_size) {
  return this->decode8(codestream, size, 3);
}

libench::PixelBuffer libench::OHTJ2KDecoder::decodeRGBA8(
    const uint8_t* codestream, size_t size, uint32_t width, uint32_t height,
    const uint8_t* init_data, size_t init_data_size) {
  return this->decode8(codestream, size, 4);
}

libench::PixelBuffer libench::OHTJ2KDecoder::decode8(const uint8_t* codestream,
                                                     size_t size,
                                                     uint8_t num_comps) {
  open_htj2k::openhtj2k_decoder decoder(codestream, size, 0, num_threads);
  std::vector<int32_t*> buf;
  std::vector<uint32_t> img_width;
  std::vector<uint32_t> img_height;
  std::vector<uint8_t> img_depth;
  std::vector<bool> img_signed;
  decoder.invoke(buf, img_width, img_height, img_depth, img_signed);
  uint32_t width = img_width[0];
  uint32_t height = img_height[0];
  this->pixels_.resize(width * height * num_comps);
  for (uint32_t i = 0; i < height; ++i) {
    uint8_t* line = &this->pixels_.data()[width * i * num_comps];
    for (uint32_t c = 0; c < num_comps; c++) {
      uint8_t* out = line + c;
      int32_t* in = buf[c] + width * i;
      for (uint32_t p = 0; p < width; p++) {
        *out = *in;
        out += num_comps;
        in += 1;
      }
    }
  }
  libench::PixelBuffer pb = {.height = height,
                             .width = width,
                             .num_comps = num_comps,
                             .pixels = &this->pixels_.data()[0]};
  return pb;
}