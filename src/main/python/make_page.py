from datetime import datetime
from math import ceil
import subprocess
import os
import os.path
import argparse
import json
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
  run_count: int

def make_index(build_dir_path: str, version_string: str, machine_string: str):
  # build input to template engine
  results = {
    "date": datetime.now().isoformat(),
    "version": version_string,
    "machine": machine_string
  }

  # apply template
  with open("src/main/resources/hbs/index.hbs", "r", encoding="utf-8") as template_file:
    with open(os.path.join(build_dir_path, "index.html"), "w", encoding="utf-8") as index_file:
      index_file.write(chevron.render(template_file, results))

def make_analysis(results_path: str, build_dir_path: str):
  df = pd.read_csv(results_path)
  df_by_set = df.groupby(["set_name", "codec_name"]).mean().reset_index().groupby("set_name")
  n_plots = len(df_by_set)
  n_cols = 3
  n_rows = ceil(n_plots / n_cols)
  fig_width = 15
  fig_height = 15 * n_rows / n_cols

  fig, axs = plt.subplots(n_rows, n_cols, figsize=(fig_width, fig_height), squeeze=False)

  if n_plots % n_cols > 0:
    for i in range(-1, -n_cols + n_plots % n_cols - 1, -1):
      fig.delaxes(axs[-1, i])

  for (i, (label, gdf)) in enumerate(df_by_set):
    ax = axs[i // n_cols, i % n_cols]
    for j in range(len(gdf["encode_time"])):
      ax.scatter(gdf["encode_time"].iloc[j], gdf["coded_size"].iloc[j], s=80, label=gdf["codec_name"].iloc[j])
    
    ax.set(ylabel=None)
    ax.set(xlabel=None)
    ax.set_title(label, pad=20, fontsize="medium")
    ax.ticklabel_format(style="sci", scilimits=(-2,2))

    y_padding = ax.get_ybound()[1] * 0.05
    ax.set_ybound(lower=-y_padding, upper=ax.get_ybound()[1] + y_padding)
    x_padding = ax.get_xbound()[1] * 0.05
    ax.set_xbound(lower=-x_padding, upper=ax.get_xbound()[1] + x_padding)

    if i == 1:
      ax.legend(loc='lower left')

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
  fig.savefig(os.path.join(build_dir_path, "encode-stats.png"))

  fig, axs = plt.subplots(n_rows, n_cols, figsize=(fig_width, fig_height), squeeze=False)

  if n_plots % n_cols > 0:
    for i in range(-1, -n_cols + n_plots % n_cols - 1, -1):
      fig.delaxes(axs[-1, i])

  for (i, (label, gdf)) in enumerate(df_by_set):
    ax = axs[i // n_cols, i % n_cols]
    for j in range(len(gdf["decode_time"])):
      ax.scatter(gdf["decode_time"].iloc[j], gdf["coded_size"].iloc[j], s=80, label=gdf["codec_name"].iloc[j])
      
    ax.set(ylabel=None)
    ax.set(xlabel=None)
    ax.set_title(label, pad=20, fontsize="medium")
    ax.ticklabel_format(style="sci", scilimits=(-2,2))

    y_padding = ax.get_ybound()[1] * 0.05
    ax.set_ybound(lower=-y_padding, upper=ax.get_ybound()[1] + y_padding)
    x_padding = ax.get_xbound()[1] * 0.05
    ax.set_xbound(lower=-x_padding, upper=ax.get_xbound()[1] + x_padding)

    if i == 1:
      ax.legend(loc='lower left')

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
  fig.savefig(os.path.join(build_dir_path, "decode-stats.png"))

def run_perf_tests(root_path: str, bin_path: str) -> typing.List[Result]:

  results = []

  sub_env = os.environ.copy()
  sub_env["OMP_NUM_THREADS"] = "1"

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

      run_count = 3

      for codec_name in ("j2k_ht_ojph", "jxl", "j2k_1_kdu", "qoi", "j2k_ht_kdu", "png", "ffv1"):

        try:
          stdout = json.loads(
            subprocess.run([bin_path, "--repetitions", str(run_count), codec_name, file_path], env=sub_env, check=True, stdout=subprocess.PIPE, encoding="utf-8").stdout
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
              set_name=collection_name,
              run_count=len(stdout["encodeTimes"])
          )

          results.append(result)

          print(".", end="")

        except (json.decoder.JSONDecodeError, subprocess.CalledProcessError):
          print("x", end="")
          raise

      print()

  return results

def _main():
  parser = argparse.ArgumentParser(description="Generate static web page with lossless coding results.")
  parser.add_argument("images_path", type=str, help="Root path of the image")
  parser.add_argument("--skip_run", type=bool, default=False, help="Skip the tests and only make the page")
  parser.add_argument("--build_path", type=str, default="./build/www", help="Path of the build directory")
  parser.add_argument("--bin_path", type=str, default="./build/libench", help="Path of the libench executable")
  parser.add_argument("--version", type=str, default="unknown", help="Version string")
  parser.add_argument("--machine", type=str, default="unknown", help="Machine string")
  args = parser.parse_args()

  os.makedirs(args.build_path, exist_ok=True)

  results_path = os.path.join(args.build_path, "results.csv")

  if not args.skip_run:
    results = run_perf_tests(args.images_path, args.bin_path)

    with open(results_path, "w", encoding="utf-8") as csvfile:
      writer = csv.DictWriter(csvfile, list(map(lambda x: x.name, dataclasses.fields(Result))))

      writer.writeheader()

      for result in results:
        writer.writerow(dataclasses.asdict(result))

  make_analysis(results_path, args.build_path)

  make_index(args.build_path, args.version, args.machine)

if __name__ == "__main__":
  _main()
