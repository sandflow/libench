#ifndef LIBENCH_OJPH_H
#define LIBENCH_OJPH_H

#include <vector>
#include "codec.h"
#include "ojph_arch.h"
#include "ojph_codestream.h"
#include "ojph_file.h"

namespace libench {

class OJPHEncoder : public Encoder {
 public:
  OJPHEncoder();

  CodestreamContext encodeRGB8(const ImageContext &image);

  CodestreamContext encodeRGBA8(const ImageContext &image);

 private:
  CodestreamContext encode8(const ImageContext &image);

  ojph::mem_outfile out_;
};

class OJPHDecoder : public Decoder {
 public:
  OJPHDecoder();

  virtual ImageContext decodeRGB8(const CodestreamContext& cs);

  virtual ImageContext decodeRGBA8(const CodestreamContext& cs);

 private:
  ImageContext decode8(const CodestreamContext& cs, uint8_t num_comps);

  ojph::mem_infile in_;
  std::vector<uint8_t> pixels_;
};

}  // namespace libench

#endif