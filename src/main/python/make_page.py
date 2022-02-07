from datetime import datetime
import os
import os.path
import argparse
import json
import typing
from dataclasses import dataclass, field
import matplotlib.pyplot as plt
import chevron


@dataclass
class Codec:
  """Codec-specific results"""
  name: str
  encode_time: float
  decode_time: float
  coded_size: float

@dataclass
class Image:
  """Image information"""
  name: str
  preview_path: str
  src_path: str
  size: int = 0
  codecs : typing.Iterable[Codec] = field(default_factory=list)

def _link_overwrite(src, dst):
  if os.path.exists(dst):
    os.remove(dst)
  os.symlink(src, dst)

def _apply_common_style(axes):
  axes.tick_params(which='both', bottom=False, top=False, left=False, right=False)
  axes.grid(True)
  axes.set(xlim=(0, None))

def build(build_dir, images: typing.Iterable[Image]):

  # build input to template engine
  results = {
    "date": datetime.now().isoformat(),
    "version": "unknown",
    "images": []
  }

  for image in images:

    image_results = {
      "name" : image.name,
      "size" : image.size,
      "preview_url": os.path.basename(image.preview_path),
      "src_url": os.path.basename(image.src_path)
    }

    # preview image
    _link_overwrite(image.preview_path, os.path.join(build_dir, image_results["preview_url"]))

    # source image
    _link_overwrite(image.src_path, os.path.join(build_dir, image_results["src_url"]))
    
    image_sizes = tuple(map(lambda x : x.coded_size/1000, image.codecs))

    # decode plot
    decode_times = tuple(map(lambda y : y.decode_time * 1000, image.codecs))
    decode_plot_fn = image.name + "_decode.svg"
    _, ax = plt.subplots(figsize=(6, 6))

    ax.scatter(image_sizes, decode_times)
    for i, codec in enumerate(image.codecs):
      ax.annotate(codec.name, (image_sizes[i], decode_times[i]))

    _apply_common_style(ax)
    ax.set_ylabel("Decode time (ms)")
    ax.set_xlabel("Coded size (kiB)")
    ax.set_title("Decode performance")  
    plt.show()
    plt.savefig(os.path.join(build_dir, decode_plot_fn))

    image_results["decode_plot_fn"] = decode_plot_fn

    # encode plot
    encode_times = tuple(map(lambda y : y.encode_time * 1000, image.codecs))
    encode_plot_fn = image.name + "_encode.svg"
    _, ax = plt.subplots(figsize=(6, 6))

    ax.scatter(image_sizes, encode_times)
    for i, codec in enumerate(image.codecs):
      ax.annotate(codec.name, (image_sizes[i], encode_times[i]))

    _apply_common_style(ax)
    ax.set_ylabel("Encode time (ms)")
    ax.set_xlabel("Coded size (kiB)")
    ax.set_title("Encode performance")
    plt.show()
    plt.savefig(os.path.join(build_dir, encode_plot_fn))

    image_results["encode_plot_fn"] = encode_plot_fn

    # codec results
    image_results["codecs"] = []

    for codec in image.codecs:
      image_results["codecs"].append({
        "name": codec.name,
        "encode_time": codec.encode_time * 1000,
        "decode_time": codec.decode_time * 1000,
        "coded_size" : round(codec.coded_size / 1000)
    })

    results["images"].append(image_results)

  # apply template

  with open("src/main/resources/hbs/index.hbs", "r", encoding="utf-8") as template_file:
    with open(os.path.join(build_dir, "index.html"), "w", encoding="utf-8") as index_file:
      index_file.write(chevron.render(template_file, results))

def _main():
  parser = argparse.ArgumentParser(description="Generate static web page with lossless coding results.")
  parser.add_argument("manifest_path", type=str, help="Path of the manifest file")
  parser.add_argument("build_path", type=str, help="Path of the build directory")
  parser.add_argument("--bin_path", type=str, default="./build/libench", help="Path of the libench executable")
  args = parser.parse_args()

  os.makedirs(args.build_path, exist_ok=True)

  with open(args.manifest_path, encoding="utf-8") as f:
    image_list = json.load(f)

  image_root_dir = os.path.dirname(args.manifest_path)

  images = []

  for image in image_list:

    image = Image(
      name=os.path.basename(image['path']),
      src_path=os.path.join(image_root_dir, image['path']),
      preview_path=os.path.join(image_root_dir, image['preview'])
    )

    for codec_name in ("ojph", "jxl", "qoi"):
      result = json.load(os.popen(f"{args.bin_path} {codec_name} {image.src_path}"))
      image.codecs.append(
        Codec(
          name=codec_name,
          encode_time=sum(result["encodeTimes"])/len(result["encodeTimes"]),
          decode_time=sum(result["decodeTimes"])/len(result["decodeTimes"]),
          coded_size=result["codestreamSize"]
        )
      )
      image.size=result["imageSize"]

    images.append(image)

  build(args.build_path, images)

if __name__ == "__main__":
  _main()
