#include "jxl_codec.h"
#include <inttypes.h>
#include <climits>
#include <memory>
#include <stdexcept>
#include <vector>
#include "fast_lossless.h"
#include "jxl/decode_cxx.h"
#include "jxl/encode_cxx.h"
#include "jxl/resizable_parallel_runner_cxx.h"
#include "jxl/thread_parallel_runner_cxx.h"

/*
 * JXLEncoder
 */

template class libench::JXLEncoder<0>;
template class libench::JXLEncoder<2>;

template <int E>
libench::JXLEncoder<E>::JXLEncoder(){};

template <int E>
libench::CodestreamBuffer libench::JXLEncoder<E>::encodeRGB8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  free(this->cb_.codestream);

  this->cb_.size = FastLosslessEncode(pixels, width, width * 3, height, 3, 8, E,
                                      &this->cb_.codestream);

  return this->cb_;
}

template <int E>
libench::CodestreamBuffer libench::JXLEncoder<E>::encodeRGBA8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  free(this->cb_.codestream);

  this->cb_.size = FastLosslessEncode(pixels, width, width * 4, height, 4, 8, E,
                                      &this->cb_.codestream);

  return this->cb_;
}

/*
 * JXLDecoder
 */

libench::JXLDecoder::JXLDecoder(){};

libench::PixelBuffer libench::JXLDecoder::decodeRGB8(const uint8_t* codestream,
                                                     size_t size,
                                                     uint32_t width,
                                                     uint32_t height,
                                                     const uint8_t* init_data,
                                                     size_t init_data_size) {
  return this->decode8(codestream, size, 3);
}

libench::PixelBuffer libench::JXLDecoder::decodeRGBA8(const uint8_t* codestream,
                                                      size_t size,
                                                      uint32_t width,
                                                      uint32_t height,
                                                      const uint8_t* init_data,
                                                      size_t init_data_size) {
  return this->decode8(codestream, size, 4);
}

libench::PixelBuffer libench::JXLDecoder::decode8(const uint8_t* codestream,
                                                  size_t size,
                                                  uint8_t num_comps) {
  libench::PixelBuffer pb;

  pb.num_comps = num_comps;

  // Multi-threaded parallel runner.
  auto runner = JxlResizableParallelRunnerMake(nullptr);

  auto dec = JxlDecoderMake(nullptr);
  if (JXL_DEC_SUCCESS !=
      JxlDecoderSubscribeEvents(dec.get(), JXL_DEC_BASIC_INFO |
                                               JXL_DEC_COLOR_ENCODING |
                                               JXL_DEC_FULL_IMAGE)) {
    throw std::runtime_error("JxlDecoderSubscribeEvents failed\n");
  }

  if (JXL_DEC_SUCCESS != JxlDecoderSetParallelRunner(dec.get(),
                                                     JxlResizableParallelRunner,
                                                     runner.get())) {
    throw std::runtime_error("JxlDecoderSetParallelRunner failed\n");
  }

  JxlBasicInfo info;
  JxlPixelFormat format = {num_comps, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};

  JxlDecoderSetInput(dec.get(), codestream, size);
  JxlDecoderCloseInput(dec.get());

  for (;;) {
    JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());

    if (status == JXL_DEC_ERROR) {
      throw std::runtime_error("Decoder error\n");
    } else if (status == JXL_DEC_NEED_MORE_INPUT) {
      throw std::runtime_error("Error, already provided all input\n");
    } else if (status == JXL_DEC_BASIC_INFO) {
      if (JXL_DEC_SUCCESS != JxlDecoderGetBasicInfo(dec.get(), &info)) {
        throw std::runtime_error("JxlDecoderGetBasicInfo failed\n");
      }
      pb.width = info.xsize;
      pb.height = info.ysize;
      JxlResizableParallelRunnerSetThreads(
          runner.get(),
          JxlResizableParallelRunnerSuggestThreads(info.xsize, info.ysize));
    } else if (status == JXL_DEC_COLOR_ENCODING) {
      // Get the ICC color profile of the pixel data (but ignore it)
      size_t icc_size;
      if (JXL_DEC_SUCCESS !=
          JxlDecoderGetICCProfileSize(
              dec.get(), &format, JXL_COLOR_PROFILE_TARGET_DATA, &icc_size)) {
        throw std::runtime_error("JxlDecoderGetICCProfileSize failed\n");
      }
      std::vector<uint8_t> icc_profile(icc_size);
      if (JXL_DEC_SUCCESS != JxlDecoderGetColorAsICCProfile(
                                 dec.get(), &format,
                                 JXL_COLOR_PROFILE_TARGET_DATA,
                                 icc_profile.data(), icc_profile.size())) {
        throw std::runtime_error("JxlDecoderGetColorAsICCProfile failed\n");
      }
    } else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
      size_t buffer_size;
      if (JXL_DEC_SUCCESS !=
          JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size)) {
        throw std::runtime_error("JxlDecoderImageOutBufferSize failed\n");
      }
      if (buffer_size != pb.height * pb.width * num_comps) {
        throw std::runtime_error("Invalid out buffer size");
      }
      this->pixels_.resize(pb.height * pb.width * num_comps);
      void* pixels_buffer = (void*)this->pixels_.data();
      size_t pixels_buffer_size = this->pixels_.size() * sizeof(float);
      if (JXL_DEC_SUCCESS != JxlDecoderSetImageOutBuffer(dec.get(), &format,
                                                         pixels_buffer,
                                                         pixels_buffer_size)) {
        throw std::runtime_error("JxlDecoderSetImageOutBuffer failed\n");
      }
    } else if (status == JXL_DEC_FULL_IMAGE) {
      // Nothing to do. Do not yet return. If the image is an animation, more
      // full frames may be decoded. This example only keeps the last one.
    } else if (status == JXL_DEC_SUCCESS) {
      // All decoding successfully finished.
      // It's not required to call JxlDecoderReleaseInput(dec.get()) here since
      // the decoder will be destroyed.
      break;
    } else {
      throw std::runtime_error("Unknown decoder status\n");
    }
  }

  pb.pixels = this->pixels_.data();

  return pb;
}
