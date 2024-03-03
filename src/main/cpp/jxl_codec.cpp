#include "jxl_codec.h"
#include "jxl/decode_cxx.h"
#include "jxl/encode_cxx.h"
#include "jxl/types.h"
#include <climits>
#include <cstdlib>
#include <inttypes.h>
#include <jxl/encode.h>
#include <memory>
#include <stdexcept>
#include <vector>

/*
 * JXLEncoder
 */

template class libench::JXLEncoder<1>;
template class libench::JXLEncoder<2>;
template class libench::JXLEncoder<3>;

template <int E> libench::JXLEncoder<E>::JXLEncoder(){};

template <int E> libench::JXLEncoder<E>::~JXLEncoder() {
  free(this->cb_.codestream);
};

template <int E, bool rgba>
static size_t JxlEncode(void *image_data, size_t width, size_t height,
                        uint8_t **encoded) {
  auto enc = JxlEncoderMake(/*memory_manager=*/nullptr);

  JxlPixelFormat pixel_format = {rgba ? 4 : 3, JXL_TYPE_UINT8,
                                 JXL_NATIVE_ENDIAN, 0};

  JxlBasicInfo basic_info;
  JxlEncoderInitBasicInfo(&basic_info);
  basic_info.xsize = width;
  basic_info.ysize = height;
  basic_info.bits_per_sample = 8;
  basic_info.exponent_bits_per_sample = 0;
  basic_info.uses_original_profile = JXL_TRUE;
  if (rgba) {
    basic_info.alpha_bits = 8;
    basic_info.num_extra_channels = 1;
  }
  if (JXL_ENC_SUCCESS != JxlEncoderSetBasicInfo(enc.get(), &basic_info)) {
    throw std::runtime_error("JxlEncoderSetBasicInfo failed\n");
  }

  JxlColorEncoding color_encoding = {};
  JxlColorEncodingSetToSRGB(&color_encoding, false);
  if (JXL_ENC_SUCCESS !=
      JxlEncoderSetColorEncoding(enc.get(), &color_encoding)) {
    throw std::runtime_error("JxlEncoderSetColorEncoding failed\n");
  }

  JxlEncoderFrameSettings *frame_settings =
      JxlEncoderFrameSettingsCreate(enc.get(), nullptr);

  if (JXL_ENC_SUCCESS != JxlEncoderSetFrameLossless(frame_settings, JXL_TRUE)) {
    throw std::runtime_error("JxlEncoderSetFrameLossless failed\n");
  }

  if (JXL_ENC_SUCCESS != JxlEncoderFrameSettingsSetOption(
                             frame_settings, JXL_ENC_FRAME_SETTING_EFFORT, E)) {
    throw std::runtime_error("JxlEncoderFrameSettingsSetOption failed\n");
  }

  if (JXL_ENC_SUCCESS !=
      JxlEncoderAddImageFrame(frame_settings, &pixel_format,
                              static_cast<const void *>(image_data),
                              pixel_format.num_channels * width * height)) {
    throw std::runtime_error("JxlEncoderAddImageFrame failed\n");
  }
  JxlEncoderCloseInput(enc.get());

  size_t size = pixel_format.num_channels * width * height;
  *encoded = (uint8_t *)malloc(size);
  uint8_t *next_out = *encoded;
  size_t avail_out = size - (next_out - *encoded);
  JxlEncoderStatus process_result = JXL_ENC_NEED_MORE_OUTPUT;
  while (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
    process_result = JxlEncoderProcessOutput(enc.get(), &next_out, &avail_out);
    if (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
      size_t offset = next_out - *encoded;
      size *= 2;
      *encoded = (uint8_t *)realloc(*encoded, size);
      next_out = *encoded + offset;
      avail_out = size - offset;
    }
  }
  if (JXL_ENC_SUCCESS != process_result) {
    throw std::runtime_error("JxlEncoderProcessOutput failed\n");
  }

  return next_out - *encoded;
}

template <int E>
libench::CodestreamContext
libench::JXLEncoder<E>::encodeRGB8(const ImageContext &image) {
  free(this->cb_.codestream);
  cb_.size = JxlEncode<E, false>(image.planes8[0], image.width, image.height,
                                 &cb_.codestream);
  return this->cb_;
}

template <int E>
libench::CodestreamContext
libench::JXLEncoder<E>::encodeRGBA8(const ImageContext &image) {
  free(this->cb_.codestream);
  cb_.size = JxlEncode<E, true>(image.planes8[0], image.width, image.height,
                                &cb_.codestream);
  return this->cb_;
}

/*
 * JXLDecoder
 */

libench::JXLDecoder::JXLDecoder(){};

libench::ImageContext
libench::JXLDecoder::decodeRGB8(const CodestreamContext &cs) {
  return this->decode8(cs, 3);
}

libench::ImageContext
libench::JXLDecoder::decodeRGBA8(const CodestreamContext &cs) {
  return this->decode8(cs, 4);
}

libench::ImageContext libench::JXLDecoder::decode8(const CodestreamContext &cs,
                                                   uint8_t num_comps) {
  libench::ImageContext image;

  image.format =
      num_comps == 3 ? libench::ImageFormat::RGB8 : libench::ImageFormat::RGBA8;

  auto dec = JxlDecoderMake(nullptr);
  if (JXL_DEC_SUCCESS !=
      JxlDecoderSubscribeEvents(dec.get(), JXL_DEC_BASIC_INFO |
                                               JXL_DEC_COLOR_ENCODING |
                                               JXL_DEC_FULL_IMAGE)) {
    throw std::runtime_error("JxlDecoderSubscribeEvents failed\n");
  }

  JxlBasicInfo info;
  JxlPixelFormat format = {num_comps, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};

  JxlDecoderSetInput(dec.get(), cs.codestream, cs.size);
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
      image.width = info.xsize;
      image.height = info.ysize;
    } else if (status == JXL_DEC_COLOR_ENCODING) {
      // Get the ICC color profile of the pixel data (but ignore it)
      size_t icc_size;
      if (JXL_DEC_SUCCESS !=
          JxlDecoderGetICCProfileSize(dec.get(), JXL_COLOR_PROFILE_TARGET_DATA,
                                      &icc_size)) {
        throw std::runtime_error("JxlDecoderGetICCProfileSize failed\n");
      }
      std::vector<uint8_t> icc_profile(icc_size);
      if (JXL_DEC_SUCCESS != JxlDecoderGetColorAsICCProfile(
                                 dec.get(), JXL_COLOR_PROFILE_TARGET_DATA,
                                 icc_profile.data(), icc_profile.size())) {
        throw std::runtime_error("JxlDecoderGetColorAsICCProfile failed\n");
      }
    } else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
      size_t buffer_size;
      if (JXL_DEC_SUCCESS !=
          JxlDecoderImageOutBufferSize(dec.get(), &format, &buffer_size)) {
        throw std::runtime_error("JxlDecoderImageOutBufferSize failed\n");
      }
      if (buffer_size != image.height * image.width * num_comps) {
        throw std::runtime_error("Invalid out buffer size");
      }
      this->pixels_.resize(image.height * image.width * num_comps);
      void *pixels_buffer = (void *)this->pixels_.data();
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

  image.planes8[0] = this->pixels_.data();

  return image;
}
