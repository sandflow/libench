#ifndef LIBENCH_CODEC_H
#define LIBENCH_CODEC_H

#include <stddef.h>
#include <cstdint>
#include <stdexcept>
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

struct ImageContext {
  uint32_t width;
  uint32_t height;
  uint8_t bit_depth;
  uint8_t num_comps;
  uint8_t num_planes;
  uint8_t x_sub_factor[4];
  uint8_t y_sub_factor[4];
  union {
    uint8_t* planes8[4];
    uint16_t* planes16[4];
  };

  ImageContext() : x_sub_factor {1}, y_sub_factor {1}, planes8 {NULL}, num_comps(0), num_planes(0), width(0), height(0), bit_depth(0) {}

  int component_size() {
    return this->is_plane16() ? 2 : 1;
  }

  bool is_plane16() {
    return this->bit_depth > 8;
  }

  size_t plane_size(int i) {
    return this->width * this->height * this->num_comps * this->component_size() / x_sub_factor[i] / y_sub_factor[i];
  }

  size_t total_bits() {
    size_t total = 0;

    for(uint8_t i = 0; i < this->num_planes; i++) {
      total += (this->width / x_sub_factor[i]) * (this->height / y_sub_factor[i]) * this->num_comps * this->bit_depth;
    }

    return total;
  }

  void md5(uint8_t hash[MD5_BLOCK_SIZE]) {
    MD5_CTX md5_ctx;

    md5_init(&md5_ctx);
    
    for(uint8_t i = 0; i < this->num_planes; i++) {
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