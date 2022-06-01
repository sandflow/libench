#include "ffv1_codec.h"
#include <inttypes.h>
#include <climits>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>

#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

/*
 * FFV1Encoder
 */

libench::FFV1Encoder::FFV1Encoder() {
  this->codec_ = avcodec_find_encoder(AV_CODEC_ID_FFV1);
  if (!this->codec_)
    throw std::runtime_error("avcodec_find_encoder AV_CODEC_ID_FFV1 failed");

  this->pkt_ = av_packet_alloc();
  if (!this->pkt_)
    throw std::runtime_error("Cannot allocate packet");

  this->frame_ = av_frame_alloc();
  if (!this->frame_)
    throw std::runtime_error("Could not allocate image frame");

  this->codec_ctx_ = NULL;
}

libench::FFV1Encoder::~FFV1Encoder() {
  av_packet_unref(this->pkt_);
  av_frame_unref(this->frame_);
  av_packet_free(&this->pkt_);
  av_frame_free(&this->frame_);
  avcodec_free_context(&this->codec_ctx_);
}

libench::CodestreamBuffer libench::FFV1Encoder::encode8(const uint8_t* pixels,
                                                        uint32_t width,
                                                        uint32_t height,
                                                        uint8_t num_comps) {
  AVCodecContext* codec_ctx_;
  int ret;

  avcodec_free_context(&this->codec_ctx_);

  this->codec_ctx_ = avcodec_alloc_context3(this->codec_);
  if (!this->codec_ctx_)
    throw std::runtime_error(
        "avcodec_alloc_codectx3 AV_CODEC_ID_FFV1 failed\n");

  this->codec_ctx_->width = width;
  this->codec_ctx_->height = height;
  this->codec_ctx_->pix_fmt =
      num_comps == 3 ? AV_PIX_FMT_0RGB32 : AV_PIX_FMT_RGB32;
  this->codec_ctx_->time_base = (AVRational){1, 25};
  this->codec_ctx_->framerate = (AVRational){25, 1};
  this->codec_ctx_->thread_count = 1;

  ret = avcodec_open2(this->codec_ctx_, this->codec_, NULL);
  if (ret < 0)
    throw std::runtime_error("Could not open codec");

  av_packet_unref(this->pkt_);
  av_frame_unref(this->frame_);

  this->frame_->format = this->codec_ctx_->pix_fmt;
  this->frame_->width = this->codec_ctx_->width;
  this->frame_->height = this->codec_ctx_->height;
  this->frame_->pts = 0;

  ret = av_frame_get_buffer(this->frame_, 0);
  if (ret < 0)
    throw std::runtime_error("Could not allocate the video frame data");

  ret = av_frame_make_writable(this->frame_);
  if (ret < 0)
    throw std::runtime_error("Frame is not writable");

  if (num_comps == 3) {
    for (int i = 0; i < height; i++) {
      uint8_t* dst_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      const uint8_t* src_line = pixels + (i * width * num_comps);
      for (int j = 0; j < width; j++) {
#if HAVE_BIGENDIAN
        /* RGB -> 0RGB */
        dst_line[4 * j + 0] = 255;
        dst_line[4 * j + 1] = src_line[3 * j + 0];
        dst_line[4 * j + 2] = src_line[3 * j + 1];
        dst_line[4 * j + 3] = src_line[3 * j + 2];
#else
        /* RGB -> BGR0 */
        dst_line[4 * j + 0] = src_line[3 * j + 2];
        dst_line[4 * j + 1] = src_line[3 * j + 1];
        dst_line[4 * j + 2] = src_line[3 * j + 0];
        dst_line[4 * j + 3] = 0;
#endif
      }
    }
  } else {
    for (int i = 0; i < height; i++) {
      uint8_t* dst_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      const uint8_t* src_line = pixels + (i * width * num_comps);
      for (int j = 0; j < width; j++) {
#if HAVE_BIGENDIAN
        /* RGBA -> ARGB */
        dst_line[4 * j + 0] = src_line[4 * j + 3];
        dst_line[4 * j + 3] = src_line[4 * j + 2];
        dst_line[4 * j + 2] = src_line[4 * j + 1];
        dst_line[4 * j + 1] = src_line[4 * j + 0];
#else
        /* RGBA -> BGRA */
        dst_line[4 * j + 0] = src_line[4 * j + 2];
        dst_line[4 * j + 1] = src_line[4 * j + 1];
        dst_line[4 * j + 2] = src_line[4 * j + 0];
        dst_line[4 * j + 3] = src_line[4 * j + 3];
#endif
      }
    }
  }

  ret = avcodec_send_frame(this->codec_ctx_, this->frame_);
  if (ret < 0)
    throw std::runtime_error("Error sending a frame for encoding");

  ret = avcodec_receive_packet(this->codec_ctx_, this->pkt_);
  if (ret)
    throw std::runtime_error("Error during encoding");

  libench::CodestreamBuffer cb;

  cb.codestream = this->pkt_->data;
  cb.size = (size_t)this->pkt_->size;
  cb.init_data = this->codec_ctx_->extradata;
  cb.init_data_size = this->codec_ctx_->extradata_size;

  return cb;
}

libench::CodestreamBuffer libench::FFV1Encoder::encodeRGB8(
    const uint8_t* pixels,
    const uint32_t width,
    uint32_t height) {
  return this->encode8(pixels, width, height, 3);
}

libench::CodestreamBuffer libench::FFV1Encoder::encodeRGBA8(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height) {
  return this->encode8(pixels, width, height, 4);
}

/*
 * FFV1Decoder
 */

libench::FFV1Decoder::FFV1Decoder() {
  this->codec_ = avcodec_find_decoder(AV_CODEC_ID_FFV1);
  if (!this->codec_)
    throw std::runtime_error("avcodec_find_encoder AV_CODEC_ID_FFV1 failed");

  this->pkt_ = av_packet_alloc();
  if (!this->pkt_)
    throw std::runtime_error("Cannot allocate packet");

  this->frame_ = av_frame_alloc();
  if (!this->frame_)
    throw std::runtime_error("Could not allocate image frame");
}

libench::FFV1Decoder::~FFV1Decoder() {
  av_packet_free(&this->pkt_);
  av_frame_free(&this->frame_);
}

libench::PixelBuffer libench::FFV1Decoder::decodeRGB8(const uint8_t* codestream,
                                                      size_t size,
                                                      uint32_t width,
                                                      uint32_t height,
                                                      const uint8_t* init_data,
                                                      size_t init_data_size) {
  return this->decode8(codestream, size, 3, width, height, init_data,
                       init_data_size);
}

libench::PixelBuffer libench::FFV1Decoder::decodeRGBA8(
    const uint8_t* codestream,
    size_t size,
    uint32_t width,
    uint32_t height,
    const uint8_t* init_data,
    size_t init_data_size) {
  return this->decode8(codestream, size, 4, width, height, init_data,
                       init_data_size);
}

static void null_free(void*, uint8_t*) {}

libench::PixelBuffer libench::FFV1Decoder::decode8(const uint8_t* codestream,
                                                   size_t size,
                                                   uint8_t num_comps,
                                                   uint32_t width,
                                                   uint32_t height,
                                                   const uint8_t* init_data,
                                                   size_t init_data_size) {
  int ret;
  AVCodecContext* ctx;
  uint8_t* pixels;
  AVBufferRef* buf;

  ctx = avcodec_alloc_context3(this->codec_);
  if (!ctx)
    throw std::runtime_error(
        "avcodec_alloc_codectx3 AV_CODEC_ID_FFV1 failed\n");

  ctx->width = width;
  ctx->height = height;
  ctx->pix_fmt = num_comps == 3 ? AV_PIX_FMT_0RGB32 : AV_PIX_FMT_RGB32;
  ctx->time_base = (AVRational){1, 25};
  ctx->framerate = (AVRational){25, 1};
  ctx->extradata = (uint8_t*) init_data;
  ctx->extradata_size = init_data_size;
  ctx->thread_count = 1;

  ret = avcodec_open2(ctx, this->codec_, NULL);
  if (ret < 0)
    throw std::runtime_error("Could not open codec");

  av_frame_unref(this->frame_);

  // av_packet_unref(this->pkt_);

  /*buf = av_buffer_create((uint8_t*)codestream, size, &null_free, NULL,
                         AV_BUFFER_FLAG_READONLY);*/

  // this->pkt_->buf = NULL;
  this->pkt_->data = (uint8_t*)codestream;
  this->pkt_->size = size;

  ret = avcodec_send_packet(ctx, this->pkt_);
  if (ret < 0)
    throw std::runtime_error("Error sending a packet for decoding");

  ret = avcodec_receive_frame(ctx, this->frame_);
  if (ret < 0)
    throw std::runtime_error("Error during decoding");

  this->pixels_.resize(this->frame_->height * this->frame_->width * num_comps);

  pixels = this->pixels_.data();

  if (num_comps == 3) {
    for (int i = 0; i < height; i++) {
      const uint8_t* src_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      uint8_t* dst_line = pixels + (i * width * num_comps);
      for (int j = 0; j < width; j++) {
#if HAVE_BIGENDIAN
        /* 0RGB -> RGB */
        dst_line[3 * j + 0] = src_line[4 * j + 1];
        dst_line[3 * j + 1] = src_line[4 * j + 2];
        dst_line[3 * j + 2] = src_line[4 * j + 3];
#else
        /* BGR0 -> RGB */
        dst_line[3 * j + 0] = src_line[4 * j + 2];
        dst_line[3 * j + 1] = src_line[4 * j + 1];
        dst_line[3 * j + 2] = src_line[4 * j + 0];
#endif
      }
    }
  } else {
    for (int i = 0; i < height; i++) {
      const uint8_t* src_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      uint8_t* dst_line = pixels + (i * width * num_comps);
      for (int j = 0; j < width; j++) {
#if HAVE_BIGENDIAN
        /* ARGB -> RGBA */
        dst_line[4 * j + 0] = src_line[4 * j + 1];
        dst_line[4 * j + 1] = src_line[4 * j + 2];
        dst_line[4 * j + 2] = src_line[4 * j + 3];
        dst_line[4 * j + 3] = src_line[4 * j + 0];
#else
        /* BGRA -> RGBA */
        dst_line[4 * j + 0] = src_line[4 * j + 2];
        dst_line[4 * j + 1] = src_line[4 * j + 1];
        dst_line[4 * j + 2] = src_line[4 * j + 0];
        dst_line[4 * j + 3] = src_line[4 * j + 3];
#endif
      }
    }
  }

  ctx->extradata = NULL; /* this was allocated outside of ffmpeg */
  avcodec_free_context(&ctx);

  libench::PixelBuffer pb;

  pb.num_comps = num_comps;
  pb.pixels = this->pixels_.data();
  pb.height = this->frame_->height;
  pb.width = this->frame_->width;

  return pb;
}
