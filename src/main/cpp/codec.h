#ifndef LIBENCH_CODEC_H
#define LIBENCH_CODEC_H

#include <stddef.h>
#include <cstdint>
#include <stdexcept>
#include <array>
extern "C" {
#include "md5.h"
}

namespace libench {

struct CodestreamContext {
  uint8_t* codestream;
  size_t size;
  void* state;
  size_t state_size;

  CodestreamContext() {}
};

struct ImageComponents {
  std::string name;
  uint8_t num_comps;

  ImageComponents(uint8_t num_comps, std::string name): num_comps(num_comps), name(name) { }

  ImageComponents() {}

  static ImageComponents RGBA;
  static ImageComponents RGB;
  static ImageComponents YUV;
};


struct ImageFormat {
  uint8_t bit_depth;
  ImageComponents comps;
  bool is_planar;
  std::array<uint8_t, 4> x_sub_factor;
  std::array<uint8_t, 4> y_sub_factor;

  ImageFormat(uint8_t bit_depth, const ImageComponents &comps, bool is_planar, const std::array<uint8_t, 4> &x_sub_factor, const std::array<uint8_t, 4> &y_sub_factor):
    bit_depth(bit_depth), comps(comps), is_planar(is_planar), x_sub_factor(x_sub_factor), y_sub_factor(y_sub_factor) {}

  ImageFormat() {}

  uint8_t num_planes() {
    return this->is_planar ? this->comps.num_comps : 1;
  }

  static ImageFormat RGBA8;
  static ImageFormat RGB8;
  static ImageFormat YUV422P10;
};



struct ImageContext {
  uint32_t width;
  uint32_t height;
  ImageFormat format;
  union {
    uint8_t* planes8[4];
    uint16_t* planes16[4];
  };

  ImageContext() : planes8 {NULL} {}

  int component_size() {
    return this->is_plane16() ? 2 : 1;
  }

  bool is_plane16() {
    return this->format.bit_depth > 8;
  }

  size_t plane_size(int i) {
    return this->width * this->height * this->format.comps.num_comps
                       * this->component_size() / this->format.x_sub_factor[i] / this->format.y_sub_factor[i];
  }

  size_t total_bits() {
    size_t total = 0;

    for(uint8_t i = 0; i < this->format.num_planes(); i++) {
      total += (this->width / this->format.x_sub_factor[i]) * (this->height / this->format.y_sub_factor[i])
               * this->format.comps.num_comps * this->format.bit_depth;
    }

    return total;
  }

  void md5(uint8_t hash[MD5_BLOCK_SIZE]) {
    MD5_CTX md5_ctx;

    md5_init(&md5_ctx);
    
    for(uint8_t i = 0; i < this->format.num_planes(); i++) {
      md5_update(&md5_ctx, this->planes8[i], this->plane_size(i));
    }
    
    md5_final(&md5_ctx, hash);
  }
};

class Encoder {
 public:
  virtual CodestreamContext encodeRGB8(const ImageContext &image) {
    throw std::runtime_error("Not yet implemented");
  };

  virtual CodestreamContext encodeRGBA8(const ImageContext &image) {
    throw std::runtime_error("Not yet implemented");
  };

  virtual CodestreamContext encodeYUV(const ImageContext &image) {
    throw std::runtime_error("Not yet implemented");
  };

  virtual ~Encoder() {}
};

class Decoder {
 public:
  virtual ImageContext decodeRGB8(const CodestreamContext& cs) {
    throw std::runtime_error("Not yet implemented");
  };

  virtual ImageContext decodeRGBA8(const CodestreamContext& cs) {
    throw std::runtime_error("Not yet implemented");
  };

  virtual ImageContext decodeYUV(const CodestreamContext& cs) {
    throw std::runtime_error("Not yet implemented");
  };

  virtual ~Decoder() {}
};

};  // namespace libench

#endif