from asyncio.subprocess import PIPE
from cgitb import enable
from datetime import datetime
from math import ceil
import os
import subprocess
import os.path
import argparse
import json
from termios import N_STRIP
import typing
import dataclasses
import csv
import matplotlib.pyplot as plt
import chevron
import png
import pandas as pd


@dataclasses.dataclass
class Result:
  """Single result"""
  codec_name: str
  encode_time: float
  decode_time: float
  coded_size: float
  image_path: str
  image_width: int
  image_height: int
  image_format: str
  image_size: int
  set_name: str

def _link_overwrite(src, dst):
  if os.path.exists(dst):
    os.remove(dst)
  os.symlink(src, dst)

def _apply_common_style(axes):
  axes.tick_params(which='both', bottom=False, top=False, left=False, right=False)
  axes.grid(True)
  axes.set(xlim=(0, None))

def build(build_dir, images):

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
    _link_overwrite(os.path.abspath(image.preview_path), os.path.join(build_dir, image_results["preview_url"]))

    # source image
    if image.src_path != image.preview_path:
      _link_overwrite(os.path.abspath(image.src_path), os.path.join(build_dir, image_results["src_url"]))
    
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

def make_analysis(results_path: str, build_dir_path: str):
  df = pd.read_csv(results_path)
  df_by_set = df.groupby(["set_name", "codec_name"]).mean().reset_index().groupby("set_name")
  n_rows = ceil(len(df_by_set) / 3)
  fig_height = 15 * n_rows / 3

  fig, axs = plt.subplots(n_rows, 3, figsize=(15, fig_height))
  if len(df_by_set) % 3 > 0:
    for i in range(len(df_by_set) % 3, 3):
      fig.delaxes(axs[n_rows - 1, i])

  for (i, (label, gdf)) in enumerate(df_by_set):
    ax = axs[i // 3, i % 3]
    gdf.plot(x="encode_time", y="coded_size", kind='scatter', s=80, c=['red', 'green', 'blue', 'orange', 'purple'], ax=ax)
    ax.set(ylabel=None)
    ax.set(xlabel=None)
    ax.set_title(label, pad=20, fontsize="medium")
    ax.ticklabel_format(style="sci", scilimits=(-2,2))

    y_padding = ax.get_ybound()[1] * 0.05
    ax.set_ybound(lower=-y_padding, upper=ax.get_ybound()[1] + y_padding)
    x_padding = ax.get_xbound()[1] * 0.05
    ax.set_xbound(lower=-x_padding, upper=ax.get_xbound()[1] + x_padding)

    for r in gdf.itertuples():
      y_offset = -15 if r.coded_size > ax.get_ybound()[1]/2 else 15
      h_align = "right" if r.encode_time > ax.get_xbound()[1]/2 else "left"
      ax.annotate(
        r.codec_name,
        (r.encode_time, r.coded_size),
        horizontalalignment=h_align,
        xytext=(0, y_offset),
        textcoords="offset points"
      )
    
  fig.supxlabel("Encode time (s)")
  fig.supylabel("Coded size (byte)", x=0.01)
  fig.suptitle('Encoding performance (RGB(A), 8-bit)', fontsize=16, va="bottom", y=0.99)
  fig.set_dpi(300)
  fig.tight_layout()
  fig.savefig(os.path.join(build_dir_path, "encode-stats.svg"))

  fig, axs = plt.subplots(n_rows, 3, figsize=(15, fig_height))
  if len(df_by_set) % 3 > 0:
    for i in range(len(df_by_set) % 3, 3):
      fig.delaxes(axs[n_rows - 1, i])

  for (i, (label, gdf)) in enumerate(df_by_set):
    ax = axs[i // 3, i % 3]
    gdf.plot(x="decode_time", y="coded_size", kind='scatter', s=80, c=['red', 'green', 'blue', 'orange', 'purple'], ax=ax)
    ax.set(ylabel=None)
    ax.set(xlabel=None)
    ax.set_title(label, pad=20, fontsize="medium")
    ax.ticklabel_format(style="sci", scilimits=(-2,2))

    y_padding = ax.get_ybound()[1] * 0.05
    ax.set_ybound(lower=-y_padding, upper=ax.get_ybound()[1] + y_padding)
    x_padding = ax.get_xbound()[1] * 0.05
    ax.set_xbound(lower=-x_padding, upper=ax.get_xbound()[1] + x_padding)

    for r in gdf.itertuples():
      y_offset = -15 if r.coded_size > ax.get_ybound()[1]/2 else 15
      h_align = "right" if r.decode_time > ax.get_xbound()[1]/2 else "left"
      ax.annotate(
        r.codec_name,
        (r.decode_time, r.coded_size),
        horizontalalignment=h_align,
        xytext=(0, y_offset),
        textcoords="offset points"
      )

  fig.supxlabel("Decode time (s)")
  fig.supylabel("Coded size (byte)", x=0.01)
  fig.set_dpi(300)
  fig.suptitle('Decoding performance (RGB(A), 8-bit)', fontsize=16, va="bottom", y=0.99)
  fig.tight_layout()
  fig.savefig(os.path.join(build_dir_path, "decode-stats.svg"))
  

def run_perf_tests(root_path: str, bin_path: str) -> typing.List[Result]:

  results = []

  for dirpath, _dirnames, filenames in os.walk(root_path):
    collection_name = os.path.relpath(dirpath, root_path)
    print(f"Collection: {collection_name}")
    for fn in filenames:
      if os.path.splitext(fn)[1] != ".png":
        continue

      file_path = os.path.join(dirpath, fn)

      png_width, png_height, _png_rows, png_info = png.Reader(filename=file_path).read(lenient=True)

      if png_info["greyscale"] or png_info["bitdepth"] != 8:
        continue

      png_format = "RGBA8" if png_info["alpha"] else "RGB8"

      rel_path = os.path.relpath(file_path, root_path)

      print(f"{rel_path} ({png_format}): ", end="")

      for codec_name in ("j2k_ojph", "jxl", "qoi", "j2k_kduht", "png"):

        try:
          stdout = json.loads(
            subprocess.run([bin_path, "--repetitions", "3", codec_name, file_path], check=True, stdout=subprocess.PIPE, encoding="utf-8").stdout
            )

          result = Result(
              codec_name=codec_name,
              encode_time=sum(stdout["encodeTimes"])/len(stdout["encodeTimes"]),
              decode_time=sum(stdout["decodeTimes"])/len(stdout["decodeTimes"]),
              coded_size=stdout["codestreamSize"],
              image_size=stdout["imageSize"],
              image_height=png_height,
              image_width=png_width,
              image_format=png_format,
              image_path=rel_path,
              set_name=collection_name
          )

          results.append(result)

          print(".", end="")

        except (json.decoder.JSONDecodeError, subprocess.CalledProcessError):
          print("x", end="")

      print()

  return results

def _main():
  parser = argparse.ArgumentParser(description="Generate static web page with lossless coding results.")
  parser.add_argument("images_path", type=str, help="Root path of the image")
  parser.add_argument("build_path", type=str, help="Path of the build directory")
  parser.add_argument("--bin_path", type=str, default="./build/libench", help="Path of the libench executable")
  args = parser.parse_args()

  os.makedirs(args.build_path, exist_ok=True)

  results = run_perf_tests(args.images_path, args.bin_path)

  with open(os.path.join(args.build_path, "results.csv"), "w", encoding="utf-8") as csvfile:
    writer = csv.DictWriter(csvfile, list(map(lambda x: x.name, dataclasses.fields(Result))))

    writer.writeheader()

    for result in results:
      writer.writerow(dataclasses.asdict(result))

if __name__ == "__main__":
  _main()
