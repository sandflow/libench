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
  name: str
  encode_time: float
  decode_time: float
  coded_size: float

@dataclass
class Image:
  name: str
  size: int = 0
  codecs : typing.Iterable[Codec] = field(default_factory=list)

def build(build_dir, images: typing.Iterable[Image]):

  # render image results

  results = {
    "date": datetime.now().isoformat(),
    "version": "unknown",
    "images": []
  }

  for image in images:
    rendered_image = {}

    rendered_image["name"] = image.name

    # decode plot
    efficiencies = tuple(map(lambda x : x.coded_size/1000, image.codecs))
    decode_times = tuple(map(lambda y : y.decode_time * 1000, image.codecs))
    decode_plot_fn = image.name + "_decode.svg"
    _, ax = plt.subplots(figsize=(6, 6))
    ax.scatter(efficiencies, decode_times)

    for i, codec in enumerate(image.codecs):
      ax.annotate(codec.name, (efficiencies[i], decode_times[i]))
    ax.tick_params(which='both', bottom=False, top=False, left=False, right=False)
    ax.grid(True)
    ax.set(xlim=(0, None))
    ax.set_ylabel("Decode time (ms)")
    ax.set_xlabel("Coded size (kiB)")
    ax.set_title("Decode performance")  
    plt.show()
    plt.savefig(os.path.join(build_dir, decode_plot_fn))

    rendered_image["decode_plot_fn"] = decode_plot_fn

    # encode plot
    encode_times = tuple(map(lambda y : y.encode_time * 1000, image.codecs))
    encode_plot_fn = image.name + "_encode.svg"
    _, ax = plt.subplots(figsize=(6, 6))
    ax.scatter(efficiencies, encode_times)

    for i, codec in enumerate(image.codecs):
      ax.annotate(codec.name, (efficiencies[i], encode_times[i]))
    ax.tick_params(which='both', bottom=False, top=False, left=False, right=False)
    ax.grid(True)
    ax.set(xlim=(0, None))
    ax.set_ylabel("Encode time (ms)")
    ax.set_xlabel("Coded size (kiB)")
    ax.set_title("Encode performance")
    plt.show()
    plt.savefig(os.path.join(build_dir, encode_plot_fn))

    rendered_image["encode_plot_fn"] = encode_plot_fn

    rendered_image["codecs"] = []

    for codec in image.codecs:
      rendered_image["codecs"].append({
        "name": codec.name,
        "encode_time": codec.encode_time * 1000,
        "decode_time": codec.decode_time * 1000,
        "coded_size" : round(codec.coded_size / 1000)
    })

    results["images"].append(rendered_image)

  # apply template

  with open("src/main/resources/hbs/index.hbs", "r", encoding="utf-8") as template_file:
    with open(os.path.join(build_dir, "index.html"), "w", encoding="utf-8") as index_file:
      index_file.write(chevron.render(template_file, results))

def main():
  BUILD_DIR = "build/python_test"

  os.makedirs(BUILD_DIR, exist_ok=True)

  parser = argparse.ArgumentParser(description="Generate static web page with lossless coding results.")
  parser.add_argument("manifest_path", type=str, help="Path of the manifest file")
  args = parser.parse_args()

  with open(args.manifest_path, encoding="utf-8") as f:
    image_list = json.load(f)

  image_root_dir = os.path.dirname(args.manifest_path)

  images = []

  for image in image_list:
    image_path = os.path.join(image_root_dir, image['path'])

    image = Image(name=os.path.basename(image_path))

    for codec_name in ("ojph", "jxl", "qoi"):
      result = json.load(os.popen(f"./build/libench {codec_name} {image_path}"))
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

  build(BUILD_DIR, images)


if __name__ == "__main__":
  main()
