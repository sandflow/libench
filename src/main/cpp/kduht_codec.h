#ifndef LIBENCH_KDUHT_H
#define LIBENCH_KDUHT_H

#include <vector>
#include "codec.h"
#include "kdu_elementary.h"
#include "kdu_params.h"
#include "kdu_stripe_compressor.h"

namespace libench {

using namespace kdu_supp;

class mem_compressed_target : public kdu_compressed_target {
 public:
  mem_compressed_target() {}

  bool close() {
    this->buf.clear();
    return true;
  }

  bool write(const kdu_byte* buf, int num_bytes) {
    std::copy(buf, buf + num_bytes, std::back_inserter(this->buf));
    return true;
  }

  void set_target_size(kdu_long num_bytes) { this->buf.reserve(num_bytes); }

  bool prefer_large_writes() const { return false; }

  std::vector<uint8_t>& get_buffer() { return this->buf; }

 private:
  std::vector<uint8_t> buf;
};

class KDUEncoder : public Encoder {
 public:
  KDUEncoder(bool isHT = true);

  virtual CodestreamBuffer encodeRGB8(const uint8_t* pixels,
                                      const uint32_t width,
                                      uint32_t height);

  virtual CodestreamBuffer encodeRGBA8(const uint8_t* pixels,
                                       uint32_t width,
                                       uint32_t height);

 private:
  CodestreamBuffer encode8(const uint8_t* pixels,
                           uint32_t width,
                           uint32_t height,
                           uint8_t num_comps);
  mem_compressed_target out_;
  bool isHT_;
};

class KDUDecoder : public Decoder {
 public:
  KDUDecoder();

  virtual PixelBuffer decodeRGB8(const uint8_t* codestream,
                                 size_t size,
                                 uint32_t width,
                                 uint32_t height,
                                 const uint8_t* init_data,
                                 size_t init_data_size);

  virtual PixelBuffer decodeRGBA8(const uint8_t* codestream,
                                  size_t size,
                                  uint32_t width,
                                  uint32_t height,
                                 const uint8_t* init_data,
                                 size_t init_data_size);

 private:
  PixelBuffer decode8(const uint8_t* codestream,
                      size_t size,
                      uint8_t num_comps);

  std::vector<uint8_t> pixels_;
};

}  // namespace libench

#endif