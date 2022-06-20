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

libench::CodestreamContext libench::FFV1Encoder::encode8(const ImageContext &image, uint8_t num_comps) {
  int ret;

  avcodec_free_context(&this->codec_ctx_);

  this->codec_ctx_ = avcodec_alloc_context3(this->codec_);
  if (!this->codec_ctx_)
    throw std::runtime_error(
        "avcodec_alloc_codectx3 AV_CODEC_ID_FFV1 failed\n");

  this->codec_ctx_->width = image.width;
  this->codec_ctx_->height = image.height;
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
    for (int i = 0; i < image.height; i++) {
      uint8_t* dst_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      const uint8_t* src_line = image.planes8[0] + (i * image.width * num_comps);
      for (int j = 0; j < image.width; j++) {
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
    for (int i = 0; i < image.height; i++) {
      uint8_t* dst_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      const uint8_t* src_line = image.planes8[0] + (i * image.width * num_comps);
      for (int j = 0; j < image.width; j++) {
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

  libench::CodestreamContext cb;

  cb.codestream = this->pkt_->data;
  cb.size = (size_t)this->pkt_->size;
  cb.state = this->codec_ctx_;
  cb.state_size = this->codec_ctx_->extradata_size + sizeof(this->codec_ctx_->height) + sizeof(this->codec_ctx_->width);

  return cb;
}

libench::CodestreamContext libench::FFV1Encoder::encodeRGB8(const ImageContext &image) {
  return this->encode8(image, 3);
}

libench::CodestreamContext libench::FFV1Encoder::encodeRGBA8(const ImageContext &image) {
  return this->encode8(image, 4);
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

libench::ImageContext libench::FFV1Decoder::decodeRGB8(const CodestreamContext& cs) {
  return this->decode8(cs, 3);
}

libench::ImageContext libench::FFV1Decoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode8(cs, 4);
}

static void null_free(void*, uint8_t*) {}

libench::ImageContext libench::FFV1Decoder::decode8(const CodestreamContext& cs, uint8_t num_comps) {
  int ret;
  AVCodecContext* ctx;
  AVCodecContext* encoder_ctx = (AVCodecContext*) cs.state;
  uint8_t* pixels;
  AVBufferRef* buf;

  ctx = avcodec_alloc_context3(this->codec_);
  if (!ctx)
    throw std::runtime_error(
        "avcodec_alloc_codectx3 AV_CODEC_ID_FFV1 failed\n");

  ctx->width = encoder_ctx->width;
  ctx->height = encoder_ctx->height;
  ctx->pix_fmt = num_comps == 3 ? AV_PIX_FMT_0RGB32 : AV_PIX_FMT_RGB32;
  ctx->time_base = (AVRational){1, 25};
  ctx->framerate = (AVRational){25, 1};
  ctx->extradata = encoder_ctx->extradata;
  ctx->extradata_size = encoder_ctx->extradata_size;
  ctx->thread_count = 1;

  ret = avcodec_open2(ctx, this->codec_, NULL);
  if (ret < 0)
    throw std::runtime_error("Could not open codec");

  av_frame_unref(this->frame_);

  // av_packet_unref(this->pkt_);

  /*buf = av_buffer_create((uint8_t*)codestream, size, &null_free, NULL,
                         AV_BUFFER_FLAG_READONLY);*/

  // this->pkt_->buf = NULL;
  this->pkt_->data = (uint8_t*)cs.codestream;
  this->pkt_->size = cs.size;

  ret = avcodec_send_packet(ctx, this->pkt_);
  if (ret < 0)
    throw std::runtime_error("Error sending a packet for decoding");

  ret = avcodec_receive_frame(ctx, this->frame_);
  if (ret < 0)
    throw std::runtime_error("Error during decoding");

  this->pixels_.resize(this->frame_->height * this->frame_->width * num_comps);

  pixels = this->pixels_.data();

  if (num_comps == 3) {
    for (int i = 0; i < ctx->height; i++) {
      const uint8_t* src_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      uint8_t* dst_line = pixels + (i * ctx->width * num_comps);
      for (int j = 0; j < ctx->width; j++) {
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
    for (int i = 0; i < ctx->height; i++) {
      const uint8_t* src_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      uint8_t* dst_line = pixels + (i * ctx->width * num_comps);
      for (int j = 0; j < ctx->width; j++) {
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

  libench::ImageContext image;

  image.format = num_comps == 3 ? libench::ImageFormat::RGB8 : libench::ImageFormat::RGBA8;
  image.planes8[0] = this->pixels_.data();
  image.height = this->frame_->height;
  image.width = this->frame_->width;

  return image;
}
