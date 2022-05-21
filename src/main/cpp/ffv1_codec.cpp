#include "ffv1_codec.h"
#include <inttypes.h>
#include <climits>
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
}

libench::FFV1Encoder::~FFV1Encoder() {
  av_packet_free(&this->pkt_);
  av_frame_free(&this->frame_);
}

libench::CodestreamBuffer libench::FFV1Encoder::encode8(const uint8_t* pixels,
                                                        uint32_t width,
                                                        uint32_t height,
                                                        uint8_t num_comps) {
  int ret;
  AVBufferRef* buffer_ref;
  AVCodecContext* ctx;

  ctx = avcodec_alloc_context3(this->codec_);
  if (!ctx)
    throw std::runtime_error(
        "avcodec_alloc_codectx3 AV_CODEC_ID_FFV1 failed\n");

  av_opt_set(ctx->priv_data, "threads", "1", 0);
  av_opt_set(ctx->priv_data, "g", "1", 0);

  ctx->width = width;
  ctx->height = height;
  ctx->pix_fmt = num_comps == 3 ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_RGB32;

  ret = avcodec_open2(ctx, this->codec_, NULL);
  if (ret < 0)
    throw std::runtime_error("Could not open codec");

  av_frame_unref(this->frame_);

  this->frame_->format = ctx->pix_fmt;
  this->frame_->width = ctx->width;
  this->frame_->height = ctx->height;
  this->frame_->pts = 0;

  ret = av_frame_get_buffer(this->frame_, 0);
  if (ret < 0)
    throw std::runtime_error("Could not allocate the video frame data");

  ret = av_frame_make_writable(this->frame_);
  if (ret < 0)
    throw std::runtime_error("Frame is not writable");

  av_image_copy_plane(this->frame_->data[0], this->frame_->linesize[0], pixels,
                      width, width, height);

  ret = avcodec_send_frame(ctx, this->frame_);
  if (ret < 0)
    throw std::runtime_error("Error sending a frame for encoding");

  ret = avcodec_receive_packet(ctx, this->pkt_);
  if (ret)
    throw std::runtime_error("Error during encoding");

  avcodec_free_context(&ctx);

  libench::CodestreamBuffer cb;

  cb.codestream = this->pkt_->data;

  cb.size = (size_t)this->pkt_->size;

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
  this->codec_ = avcodec_find_encoder(AV_CODEC_ID_FFV1);
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
                                                      size_t size) {
  return this->decode8(codestream, size, 3);
}

libench::PixelBuffer libench::FFV1Decoder::decodeRGBA8(
    const uint8_t* codestream,
    size_t size) {
  return this->decode8(codestream, size, 4);
}

libench::PixelBuffer libench::FFV1Decoder::decode8(const uint8_t* codestream,
                                                   size_t size,
                                                   uint8_t num_comps) {
  int ret;

  
  
  libench::PixelBuffer pb;

  pb.num_comps = num_comps;

  return pb;
}
