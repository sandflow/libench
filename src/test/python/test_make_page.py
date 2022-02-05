import unittest
import os
import make_page


class MakePageTest(unittest.TestCase):

  BUILD_DIR = "build/python_test"

  def setUp(self):
    os.makedirs(MakePageTest.BUILD_DIR, exist_ok=True)

  def test_make(self):
    images = (
      make_page.Image(
        name = "Sample PNG RGBA image",
        size=64000, 
        codecs = (
          make_page.Codec(name="HT", coded_size=12000, encode_time=0.5, decode_time=0.5),
          make_page.Codec(name="J2K1", coded_size=10000, encode_time=0.1, decode_time=0.1),
          make_page.Codec(name="PNG", coded_size=14000, encode_time=0.3, decode_time=0.3)
        )
      ),
      make_page.Image(
        name="Sample TIFF image",
        size=20000000, 
        codecs = (
          make_page.Codec(name="HT", coded_size=10000000, encode_time=0.8, decode_time=0.9),
          make_page.Codec(name="J2K1", coded_size=9000000, encode_time=0.12, decode_time=0.12),
          make_page.Codec(name="PNG", coded_size=13000000, encode_time=0.2, decode_time=0.3)
        )
      )
    )

    make_page.build(MakePageTest.BUILD_DIR, images)
