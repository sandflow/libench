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

  CodestreamContext encodeRGB8(const ImageContext &image);

  CodestreamContext encodeRGBA8(const ImageContext &image);

  CodestreamContext encodeYUV(const ImageContext &image);

 private:
  CodestreamContext encode(const ImageContext &image);

  mem_compressed_target out_;
  bool isHT_;
};

class KDUDecoder : public Decoder {
 public:
  KDUDecoder();

  virtual ImageContext decodeRGB8(const CodestreamContext& cs);

  virtual ImageContext decodeRGBA8(const CodestreamContext& cs);

 private:
  ImageContext decode(const CodestreamContext& cs);

  std::vector<uint8_t> planes_[3];
};

}  // namespace libench

#endif