#include <stdio.h>
#include <time.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <chrono>
#include "cxxopts.hpp"
extern "C" {
#include "md5.h"
}
#include "jxl_codec.h"
#include "ojph_codec.h"
#include "qoi_codec.h"
#include "kduht_codec.h"
#include "png_codec.h"
#include "ffv1_codec.h"
#include "ohtj2k_codec.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#include "stb_image.h"

struct CodestreamContext {
  uint8_t image_hash[MD5_BLOCK_SIZE];
  std::string codestream_path;
  uint32_t image_sz;
  uint32_t codestream_sz;
  std::vector<std::chrono::system_clock::time_point::duration> encode_times;
  std::vector<std::chrono::system_clock::time_point::duration> decode_times;
};

std::ostream& operator<<(std::ostream& os, const CodestreamContext& ctx) {
  os << "{" << std::endl;

  os << "\"decodeTimes\" : [";
  for (const auto& t : ctx.decode_times) {
    os << std::chrono::duration<double>(t).count();
    if (&t != &ctx.decode_times.back()) {
      os << ", ";
    }
  }
  os << "]," << std::endl;

  os << "\"encodeTimes\" : [";
  for (const auto& t : ctx.encode_times) {
    os << std::chrono::duration<double>(t).count();
    if (&t != &ctx.encode_times.back()) {
      os << ", ";
    }
  }
  os << "]," << std::endl;

  os << "\"imageSize\" : " << ctx.image_sz << "," << std::endl;

  os << "\"codestreamSize\" : " << ctx.codestream_sz  << std::endl;

  os << "}" << std::endl;

  return os;
}

int main(int argc, char* argv[]) {
  cxxopts::Options options("libench", "Lossless image codec benchmark");

  options.add_options()("dir", "Codesteeam directory path",
                        cxxopts::value<std::string>())(
      "r,repetitions", "Codesteeam directory path",
      cxxopts::value<int>()->default_value("5"))(
      "file", "Input image", cxxopts::value<std::string>())(
      "codec", "Coded to profile", cxxopts::value<std::string>());

  options.parse_positional({"codec", "file"});

  std::unique_ptr<libench::Encoder> encoder;
  std::unique_ptr<libench::Decoder> decoder;

  auto result = options.parse(argc, argv);

  if (result["codec"].as<std::string>() == "j2k_ht_ojph") {
    encoder.reset(new libench::OJPHEncoder());
    decoder.reset(new libench::OJPHDecoder());
  } else if (result["codec"].as<std::string>() == "qoi") {
    encoder.reset(new libench::QOIEncoder());
    decoder.reset(new libench::QOIDecoder());
  } else if (result["codec"].as<std::string>() == "j2k_ht_ohtj2k") {
    encoder.reset(new libench::OHTJ2KEncoder());
    decoder.reset(new libench::OHTJ2KDecoder());
  } else if (result["codec"].as<std::string>() == "jxl") {
    encoder.reset(new libench::JXLEncoder<2>());
    decoder.reset(new libench::JXLDecoder());
  } else if (result["codec"].as<std::string>() == "jxl_0") {
    encoder.reset(new libench::JXLEncoder<0>());
    decoder.reset(new libench::JXLDecoder());
  } else if (result["codec"].as<std::string>() == "j2k_ht_kdu") {
    encoder.reset(new libench::KDUEncoder(true));
    decoder.reset(new libench::KDUDecoder());
  } else if (result["codec"].as<std::string>() == "j2k_1_kdu") {
    encoder.reset(new libench::KDUEncoder(false));
    decoder.reset(new libench::KDUDecoder());
  } else if (result["codec"].as<std::string>() == "png") {
    encoder.reset(new libench::PNGEncoder());
    decoder.reset(new libench::PNGDecoder());
  } else if (result["codec"].as<std::string>() == "ffv1") {
    encoder.reset(new libench::FFV1Encoder());
    decoder.reset(new libench::FFV1Decoder());
  } else {
    throw std::runtime_error("Unknown encoder");
  }

  std::vector<CodestreamContext> ctxs;

  auto& filepath = result["file"].as<std::string>();
  int width;
  int height;
  int num_comps;


  unsigned char* data =
      stbi_load(filepath.c_str(), &width, &height, &num_comps, 0);

  if (!data) {
    throw std::runtime_error("Cannot read image file");
  }

  if (num_comps < 3 || num_comps > 4) {
    std::cerr << "Only RGB or RGBA images are supported";
    return 1;
  }

  int repetitions = result["repetitions"].as<int>();

  CodestreamContext ctx;

  ctx.encode_times.resize(repetitions);
  ctx.decode_times.resize(repetitions);
  ctx.image_sz = height * width * num_comps;

  /*std::ofstream in_raw(filepath + "." + result["codec"].as<std::string>() + ".in.raw");
  in_raw.write((const char*) data, width * height * num_comps);
  in_raw.close();*/

  /* source hash */

  MD5_CTX md5_ctx;
  md5_init(&md5_ctx);
  md5_update(&md5_ctx, data, width * height * num_comps);
  md5_final(&md5_ctx, ctx.image_hash);

  /* encode */

  for (int i = 0; i < repetitions; i++) {
    libench::CodestreamBuffer cb;

    auto start = std::chrono::high_resolution_clock::now();

    switch (num_comps) {
      case 3:
        cb = encoder->encodeRGB8(data, width, height);
        break;
      case 4:
        cb = encoder->encodeRGBA8(data, width, height);
        break;
      default:
        throw std::runtime_error("Unsupported number of components");
    }

    ctx.encode_times[i] = std::chrono::high_resolution_clock::now() - start;

    if (i == 0) {
      ctx.codestream_sz = cb.size + cb.init_data_size;

      if (result.count("dir")) {
        /* generate the codestream path */
        std::stringstream ss;

        ss << result["dir"].as<std::string>() << "/";

        for (int i = 0; i < sizeof(CodestreamContext::image_hash); i++) {
          ss << std::hex << std::setfill('0') << std::setw(2) << std::right
             << (int)ctx.image_hash[i];
        }

        ctx.codestream_path = ss.str();

        /* write the codestream */

        std::ofstream f(ctx.codestream_path);
        f.write(reinterpret_cast<char*>(cb.codestream), cb.size);
        f.close();
      }
    }

    /* decode */

    libench::PixelBuffer pb;

    start = std::chrono::high_resolution_clock::now();

    switch (num_comps) {
      case 3:
        pb = decoder->decodeRGB8(cb.codestream, cb.size, width, height, cb.init_data, cb.init_data_size);
        break;
      case 4:
        pb = decoder->decodeRGBA8(cb.codestream, cb.size, width, height, cb.init_data, cb.init_data_size);
        break;
      default:
        throw std::runtime_error("Unsupported number of components");
    }

    ctx.decode_times[i] = std::chrono::high_resolution_clock::now() - start;

    /*std::ofstream out_raw(filepath + "." + result["codec"].as<std::string>() + ".out.raw");
    out_raw.write((const char*) pb.pixels, width * height * num_comps);
    out_raw.close();*/

    /* bit exact compare */

    uint8_t decoded_hash[MD5_BLOCK_SIZE];

    md5_init(&md5_ctx);
    md5_update(&md5_ctx, pb.pixels, width * height * num_comps);
    md5_final(&md5_ctx, decoded_hash);

    if (memcmp(decoded_hash, ctx.image_hash, MD5_BLOCK_SIZE))
      throw std::runtime_error("Image does not match");

  }

  std::cout << ctx;

  ctxs.push_back(ctx);

  stbi_image_free(data);
}