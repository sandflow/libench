#include <stdio.h>
#include <time.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <chrono>
#include "cxxopts.hpp"
#include "jxl_codec.h"
#include "ojph_codec.h"
#include "qoi_codec.h"
#include "kduht_codec.h"
#include "png_codec.h"
#include "ffv1_codec.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#include "stb_image.h"

struct TestContext {
  uint8_t image_hash[MD5_BLOCK_SIZE];
  std::string codestream_path;
  uint32_t image_sz;
  uint32_t codestream_sz;
  std::vector<std::chrono::system_clock::time_point::duration> encode_times;
  std::vector<std::chrono::system_clock::time_point::duration> decode_times;
};

std::ostream& operator<<(std::ostream& os, const TestContext& ctx) {
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

libench::ImageContext load_image(const std::string& filepath) {
  libench::ImageContext image;

  size_t start = filepath.find_last_of(".");

  std::string file_ext = filepath.substr(start + 1);

  if (file_ext == "png") {
    int height;
    int width;
    int num_comps;
    
    image.planes8[0] = stbi_load(filepath.c_str(), &width, &height, &num_comps, 0);
    if (! image.planes8[0]) {
      throw std::runtime_error("Cannot read image file");
    }

    image.height = height;
    image.width = width;

    switch (num_comps) {
      case 3:
        image.format = libench::ImageFormat::RGB8;
        break;
      case 4:
        image.format = libench::ImageFormat::RGBA8;
        break;
      default:
        throw std::runtime_error("Only RGB or RGBA images are supported");
    }

  } else if (file_ext == "yuv") {
    /* must be of the form XXXXXX.<width>x<height>.<pixel_fmt>.yuv */

    size_t end = start - 1;
    start = filepath.find_last_of(".", end);
    std::string pix_fmt = filepath.substr(start + 1, end - start);

    end = start - 1;
    start = filepath.find_last_of("x", end);
    image.height = std::stoi(filepath.substr(start + 1, end - start));

    end = start - 1;
    start = filepath.find_last_of(".", end);
    image.width = std::stoi(filepath.substr(start + 1, end - start));

    if (pix_fmt == "yuv422p10le") {
      image.format = libench::ImageFormat::YUV422P10;

      std::ifstream in(filepath);

      for(uint8_t i = 0; i < image.format.num_planes(); i++) {

        image.planes16[i] = (uint16_t*) malloc(image.plane_size(i));
        if (! image.planes16[i]) {
          throw std::runtime_error("Cannot allocate memory");
        }
        
        in.read((char*) image.planes16[i], image.plane_size(i));
        if (in.bad()) {
          throw std::runtime_error("Read failed");
        }
      }

    } else {
      throw std::runtime_error("Unknown pixel format: " + pix_fmt);
    }

    
  } else {
    throw std::runtime_error("Image file must be YUV or PNG");
  }

  return image;
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

  
  auto& filepath = result["file"].as<std::string>();

  libench::ImageContext in_img = load_image(filepath);

  int repetitions = result["repetitions"].as<int>();

  TestContext test;

  test.encode_times.resize(repetitions);
  test.decode_times.resize(repetitions);
  test.image_sz = in_img.total_bits() / 8;

  /*std::ofstream in_raw(filepath + "." + result["codec"].as<std::string>() + ".in.raw");
  in_raw.write((const char*) data, width * height * num_comps);
  in_raw.close();*/

  /* source hash */

  in_img.md5(test.image_hash);

  /* encode */

  for (int i = 0; i < repetitions; i++) {
    libench::CodestreamContext cs;

    auto start = std::chrono::high_resolution_clock::now();

    switch (in_img.format.comps.num_comps) {
      case 3:
        cs = encoder->encodeRGB8(in_img);
        break;
      case 4:
        cs = encoder->encodeRGBA8(in_img);
        break;
      default:
        throw std::runtime_error("Unsupported number of components");
    }

    test.encode_times[i] = std::chrono::high_resolution_clock::now() - start;

    if (i == 0) {
      test.codestream_sz = cs.size + cs.state_size;

      if (result.count("dir")) {
        /* generate the codestream path */
        std::stringstream ss;

        ss << result["dir"].as<std::string>() << "/";

        for (int i = 0; i < sizeof(TestContext::image_hash); i++) {
          ss << std::hex << std::setfill('0') << std::setw(2) << std::right
             << (int)test.image_hash[i];
        }

        test.codestream_path = ss.str();

        /* write the codestream */

        std::ofstream f(test.codestream_path);
        f.write(reinterpret_cast<char*>(cs.codestream), cs.size);
        f.close();
      }
    }

    /* decode */

    libench::ImageContext out_img;

    start = std::chrono::high_resolution_clock::now();

    switch (in_img.format.comps.num_comps) {
      case 3:
        out_img = decoder->decodeRGB8(cs);
        break;
      case 4:
        out_img = decoder->decodeRGBA8(cs);
        break;
      default:
        throw std::runtime_error("Unsupported number of components");
    }

    test.decode_times[i] = std::chrono::high_resolution_clock::now() - start;

    /*std::ofstream out_raw(filepath + "." + result["codec"].as<std::string>() + ".out.raw");
    out_raw.write((const char*) out_img.pixels, width * height * num_comps);
    out_raw.close();*/

    /* bit exact compare */

    uint8_t decoded_hash[MD5_BLOCK_SIZE];

    out_img.md5(decoded_hash);

    if (memcmp(decoded_hash, test.image_hash, MD5_BLOCK_SIZE))
      throw std::runtime_error("Image does not match");

  }

  std::cout << test;

  for(uint8_t i = 0; i < in_img.format.num_planes(); i++) {
    free(in_img.planes8[i]);
  }

}