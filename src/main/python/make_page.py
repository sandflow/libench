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

@dataclasses.dataclass
class CodecInfo:
  """Preferences for a single codec"""
  color: str
  marker: str
  formats: typing.Iterable[str]

# colors from http://www.sussex.ac.uk/tel/resource/tel_website/accessiblecontrast
CODEC_PREFS = {
    "j2k_ht_ojph": CodecInfo(color="#41b6e6", marker="o", formats=["RGBA8", "RGB8"]),
    "j2k_1_kdu": CodecInfo(color="#41b6e6", marker="v", formats=["RGBA8", "RGB8", "YUV"]),
    "j2k_ht_kdu": CodecInfo(color="#41b6e6", marker="s", formats=["RGBA8", "RGB8", "YUV"]),
    "jxl": CodecInfo(color="#e56db1", marker="o", formats=["RGBA8", "RGB8"]),
    "qoi": CodecInfo(color="#dc582a", marker="o", formats=["RGBA8", "RGB8"]),
    "png": CodecInfo(color="#f2c75c", marker="o", formats=["RGBA8", "RGB8"]),
    "ffv1": CodecInfo(color="#94a596", marker="o", formats=["RGBA8", "RGB8", "YUV"])
}

def make_analysis(df, msg: str, fig_name: str, build_dir_path: str):
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
      colors = CODEC_PREFS[gdf["codec_name"].iloc[j]].color
      markers = CODEC_PREFS[gdf["codec_name"].iloc[j]].marker
      ax.scatter(gdf["encode_time"].iloc[j], gdf["coded_size"].iloc[j], s=80, marker=markers, c=colors, label=gdf["codec_name"].iloc[j])
    
    ax.set(ylabel=None)
    ax.set(xlabel=None)
    ax.set_title(label, pad=20, fontsize="medium")
    ax.ticklabel_format(style="sci", scilimits=(-2,2))

    y_padding = ax.get_ybound()[1] * 0.05
    ax.set_ybound(lower=-y_padding, upper=ax.get_ybound()[1] + y_padding)
    x_padding = ax.get_xbound()[1] * 0.05
    ax.set_xbound(lower=-x_padding, upper=ax.get_xbound()[1] + x_padding)

    if i == 0:
      ax.legend(loc='lower left')

    for r in gdf.itertuples():
      y_offset = -15 if r.coded_size > ax.get_ybound()[1]/2 else 15
      h_align = "right" if r.encode_time > ax.get_xbound()[1]/2 else "left"
      ax.annotate(
        r.codec_name,
        (r.encode_time, r.coded_size),
        horizontalalignment=h_align,
        xytext=(0, y_offset),
        textcoords="offset points",
        c="#555555"
      )

  fig.supxlabel("Encode time (s)")
  fig.supylabel("Coded size (byte)", x=0.01)
  fig.suptitle(f'Encoding performance ({msg})', fontsize=16, y=(1 - 0.01 / fig_height))
  fig.set_dpi(300)
  fig.tight_layout()
  fig.savefig(os.path.join(build_dir_path, f"{fig_name}-encode.png"))

  fig, axs = plt.subplots(n_rows, n_cols, figsize=(fig_width, fig_height), squeeze=False)

  if n_plots % n_cols > 0:
    for i in range(-1, -n_cols + n_plots % n_cols - 1, -1):
      fig.delaxes(axs[-1, i])

  for (i, (label, gdf)) in enumerate(df_by_set):
    ax = axs[i // n_cols, i % n_cols]
    for j in range(len(gdf["decode_time"])):
      colors = CODEC_PREFS[gdf["codec_name"].iloc[j]].color
      markers = CODEC_PREFS[gdf["codec_name"].iloc[j]].marker
      ax.scatter(gdf["decode_time"].iloc[j], gdf["coded_size"].iloc[j], s=80, marker=markers, c=colors, label=gdf["codec_name"].iloc[j])
      
    ax.set(ylabel=None)
    ax.set(xlabel=None)
    ax.set_title(label, pad=20, fontsize="medium")
    ax.ticklabel_format(style="sci", scilimits=(-2,2))

    y_padding = ax.get_ybound()[1] * 0.05
    ax.set_ybound(lower=-y_padding, upper=ax.get_ybound()[1] + y_padding)
    x_padding = ax.get_xbound()[1] * 0.05
    ax.set_xbound(lower=-x_padding, upper=ax.get_xbound()[1] + x_padding)

    if i == 0:
      ax.legend(loc='lower left')

    for r in gdf.itertuples():
      y_offset = -15 if r.coded_size > ax.get_ybound()[1]/2 else 15
      h_align = "right" if r.decode_time > ax.get_xbound()[1]/2 else "left"
      ax.annotate(
        r.codec_name,
        (r.decode_time, r.coded_size),
        horizontalalignment=h_align,
        xytext=(0, y_offset),
        textcoords="offset points",
        c="#555555"
      )

  fig.supxlabel("Decode time (s)")
  fig.supylabel("Coded size (byte)", x=0.01)
  fig.set_dpi(300)
  fig.suptitle(f'Decoding performance ({msg})', fontsize=16, y=(1 - 0.01 / fig_height))
  fig.tight_layout()
  fig.savefig(os.path.join(build_dir_path, f"{fig_name}-decode.png"))

def run_perf_tests(root_path: str, bin_path: str) -> typing.List[Result]:

  results = []

  sub_env = os.environ.copy()
  sub_env["OMP_NUM_THREADS"] = "1"

  for dirpath, _dirnames, filenames in os.walk(root_path):
    collection_name = os.path.relpath(dirpath, root_path)
    print(f"Collection: {collection_name}")

    for fn in filenames:

      file_path = os.path.join(dirpath, fn)

      if os.path.splitext(fn)[1] == ".png":
        _, _, _png_rows, png_info = png.Reader(filename=file_path).read(lenient=True)

        if png_info["greyscale"] or png_info["bitdepth"] != 8:
          continue

        image_format = "RGBA8" if png_info["alpha"] else "RGB8"

      elif os.path.splitext(fn)[1] == ".yuv":
        image_format = "YUV"

      else:
        continue

      rel_path = os.path.relpath(file_path, root_path)

      print(f"{rel_path} ({image_format}): ", end="")

      run_count = 3

      for codec_name, codec_info in CODEC_PREFS.items():

        if not image_format in codec_info.formats:
          continue

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
              image_height=stdout["imageHeight"],
              image_width=stdout["imageWidth"],
              image_format=image_format,
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
  parser.add_argument("--compiler", type=str, default="unknown", help="Compiler version")
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

  df = pd.read_csv(results_path)

  panels = []



  df_rgb = df[df.image_format.isin(["RGBA8", "RGB8"])]
  if not df_rgb.empty:
    make_analysis(df_rgb, "RGB(A), 8-bit, single thread", "rgb", args.build_path)
    panels.append({"name": "RGB(A)", "id": "rgb", "active": "true"})

  df_yuv = df[df.image_format.isin(["YUV"])]
  if not df_yuv.empty:
    make_analysis(df_yuv, "YCbCr, 10-bit, single thread", "yuv", args.build_path)
    panels.append({"name": "YCbCr", "id": "yuv"})

  results = {
    "date": datetime.now().isoformat(),
    "version": args.version,
    "machine": args.machine,
    "compiler": args.compiler,
    "panels": panels
  }

  # apply template
  with open("src/main/resources/hbs/index.hbs", "r", encoding="utf-8") as template_file:
    with open(os.path.join(args.build_path, "index.html"), "w", encoding="utf-8") as index_file:
      index_file.write(chevron.render(template_file, results))

if __name__ == "__main__":
  _main()
