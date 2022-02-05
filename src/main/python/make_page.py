from datetime import datetime
import os
import argparse
import json
import typing
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

  with open("src/main/resources/hbs/index.hbs", "r", encoding="utf-8") as template_file:
    with open(os.path.join(build_dir, "index.html"), "w", encoding="utf-8") as index_file:
      index_file.write(chevron.render(template_file, {"date": datetime.now().isoformat(), "version": "unknown", "images": images}))

if __name__ == "main":
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
