from datetime import datetime
import os
import argparse
import json
import typing
import subprocess
from dataclasses import dataclass, field
import os.path
import matplotlib.pyplot as plt
import chevron


@dataclass
class Codec:
  name: str
  encode_time: float
  decode_time: float
  coding_efficiency: float

@dataclass
class Image:
  name: str
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
    efficiencies = tuple(map(lambda x : x.coding_efficiency * 100, image.codecs))
    decode_times = tuple(map(lambda y : y.decode_time * 1000, image.codecs))
    decode_plot_fn = image.name + "_decode.svg"
    _, ax = plt.subplots()
    ax.scatter(efficiencies, decode_times)

    for i, codec in enumerate(image.codecs):
      ax.annotate(codec.name, (efficiencies[i], decode_times[i]))
    plt.tick_params(which='both', bottom=False, top=False, left=False, right=False)
    ax.grid(True)
    ax.set(xlim=(0, 100))
    ax.set_ylabel("Decoding speed (ms)")
    ax.set_xlabel("Coding efficiency (%)")
    ax.set_title("Decoding performance")
    plt.show()
    plt.savefig(os.path.join(build_dir, decode_plot_fn))

    rendered_image["decode_plot_fn"] = decode_plot_fn

    # encode plot
    encode_times = tuple(map(lambda y : y.encode_time * 1000, image.codecs))
    encode_plot_fn = image.name + "_encode.svg"
    _, ax = plt.subplots()
    ax.scatter(efficiencies, encode_times)

    for i, codec in enumerate(image.codecs):
      ax.annotate(codec.name, (efficiencies[i], encode_times[i]))
    plt.tick_params(which='both', bottom=False, top=False, left=False, right=False)
    ax.grid(True)
    ax.set(xlim=(0, 100))
    ax.set_ylabel("Encoding speed (ms)")
    ax.set_xlabel("Coding efficiency (%)")
    ax.set_title("Encoding performance")
    plt.show()
    plt.savefig(os.path.join(build_dir, encode_plot_fn))

    rendered_image["encode_plot_fn"] = encode_plot_fn

    rendered_image["codecs"] = []

    for codec in image.codecs:
      rendered_image["codecs"].append({
        "name": codec.name,
        "encode_time": codec.encode_time * 1000,
        "decode_time": codec.decode_time * 1000,
        "coding_efficiency" : round(codec.coding_efficiency * 100)
    })

    results["images"].append(rendered_image)

  # apply template

  with open("src/main/resources/hbs/index.hbs", "r", encoding="utf-8") as template_file:
    with open(os.path.join(build_dir, "index.html"), "w", encoding="utf-8") as index_file:
      index_file.write(chevron.render(template_file, results))

def main():
  parser = argparse.ArgumentParser(description="Generate static web page with lossless coding results.")
  parser.add_argument("manifest_path", type=str, help="Path of the manifest file")
  args = parser.parse_args()

  image_list = json.load(args["manifest_path"])

  for image in image_list:
    pass

  stream = os.popen("./build/libench ojph images/rgba.png")
  output = stream.read()
  print(output)

  fig, ax = plt.subplots()
  ax.scatter([1,2], [3,4])
  plt.show()
  plt.savefig('build/a.png')

if __name__ == "main":
  main()
