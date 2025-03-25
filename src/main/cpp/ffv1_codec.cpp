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

libench::CodestreamContext libench::FFV1Encoder::encode(const ImageContext &image) {
  int ret;
  AVDictionary *opts = NULL;

  avcodec_free_context(&this->codec_ctx_);

  this->codec_ctx_ = avcodec_alloc_context3(this->codec_);
  if (!this->codec_ctx_)
    throw std::runtime_error(
        "avcodec_alloc_codectx3 AV_CODEC_ID_FFV1 failed\n");

  this->codec_ctx_->width = image.width;
  this->codec_ctx_->height = image.height;
  this->codec_ctx_->time_base = (AVRational){1, 25};
  this->codec_ctx_->framerate = (AVRational){25, 1};
  this->codec_ctx_->thread_count = 1;

  if (image.format.comps == libench::ImageComponents::YUV) {
    this->codec_ctx_->pix_fmt = AV_PIX_FMT_YUV422P10LE;
  } else if (image.format.comps == libench::ImageComponents::RGB) {
    this->codec_ctx_->pix_fmt = AV_PIX_FMT_0RGB32;
  } else if  (image.format.comps == libench::ImageComponents::RGBA) {
    this->codec_ctx_->pix_fmt = AV_PIX_FMT_RGB32;
  } else {
    throw std::runtime_error("Unknown components");
  }

  if (image.format.bit_depth > 8) {
    ret = av_dict_set(&opts, "coder", "range_tab", 0);
    if (ret < 0)
      throw std::runtime_error("Opts allocation failed");
  }

  ret = avcodec_open2(this->codec_ctx_, this->codec_, &opts);
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

  int num_comps = image.format.comps.num_comps;

  if (this->frame_->format == AV_PIX_FMT_0RGB32) {
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
  } else if (this->frame_->format == AV_PIX_FMT_RGB32) {
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
  } else if (this->frame_->format == AV_PIX_FMT_YUV422P10LE) {
    for(int i = 0; i < image.format.comps.num_comps; i++) {
      av_image_copy_plane(this->frame_->data[i], this->frame_->linesize[i],
                          image.planes8[i], image.line_size(i),
                          image.line_size(i), image.plane_height(i));
    }
  }

  ret = avcodec_send_frame(this->codec_ctx_, this->frame_);
  if (ret < 0)
    throw std::runtime_error("Error sending a frame for encoding");

  ret = avcodec_receive_packet(this->codec_ctx_, this->pkt_);
  if (ret)
    throw std::runtime_error("Error during encoding");

  libench::CodestreamContext cs;

  cs.codestream = this->pkt_->data;
  cs.size = (size_t)this->pkt_->size;
  cs.state = this->codec_ctx_;
  cs.state_size = this->codec_ctx_->extradata_size + sizeof(this->codec_ctx_->height) + sizeof(this->codec_ctx_->width);

  return cs;
}

libench::CodestreamContext libench::FFV1Encoder::encodeRGB8(const ImageContext &image) {
  return this->encode(image);
}

libench::CodestreamContext libench::FFV1Encoder::encodeRGBA8(const ImageContext &image) {
  return this->encode(image);
}

libench::CodestreamContext libench::FFV1Encoder::encodeYUV(const ImageContext &image) {
  return this->encode(image);
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
  return this->decode(cs);
}

libench::ImageContext libench::FFV1Decoder::decodeRGBA8(const CodestreamContext& cs) {
  return this->decode(cs);
}

libench::ImageContext libench::FFV1Decoder::decodeYUV(const CodestreamContext& cs) {
  return this->decode(cs);
}


static void null_free(void*, uint8_t*) {}

libench::ImageContext libench::FFV1Decoder::decode(const CodestreamContext& cs) {
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
  ctx->pix_fmt = encoder_ctx->pix_fmt;
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


  libench::ImageContext image;

  image.height = this->frame_->height;
  image.width = this->frame_->width;

  if (ctx->pix_fmt == AV_PIX_FMT_0RGB32) {
    image.format = libench::ImageFormat::RGB8;
    this->planes_[0].resize(image.plane_size(0));
    image.planes8[0] = this->planes_[0].data();
    pixels = this->planes_[0].data();

      for (int i = 0; i < ctx->height; i++) {
      const uint8_t* src_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      uint8_t* dst_line = pixels + (i * ctx->width * image.format.comps.num_comps);
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
  } else if (ctx->pix_fmt == AV_PIX_FMT_RGB32) {
    image.format = libench::ImageFormat::RGBA8;
    this->planes_[0].resize(image.plane_size(0));
    image.planes8[0] = this->planes_[0].data();
    pixels = this->planes_[0].data();

    for (int i = 0; i < ctx->height; i++) {
      const uint8_t* src_line =
          this->frame_->data[0] + (i * this->frame_->linesize[0]);
      uint8_t* dst_line = pixels + (i * ctx->width * image.format.comps.num_comps);
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
  }  else if (this->frame_->format == AV_PIX_FMT_YUV422P10LE) {
    image.format = libench::ImageFormat::YUV422P10;

    for(int i = 0; i < image.format.comps.num_comps; i++) {
      this->planes_[i].resize(image.plane_size(i));
      image.planes8[i] = this->planes_[i].data();
      pixels = this->planes_[i].data();
      av_image_copy_plane(image.planes8[i], image.line_size(i),
                          this->frame_->data[i], this->frame_->linesize[i],
                          image.line_size(i), image.plane_height(i));
    }
  } else {
    throw std::runtime_error("Bad pixel format");
  }

  ctx->extradata = NULL; /* this was allocated outside of ffmpeg */
  avcodec_free_context(&ctx);

  return image;
}
