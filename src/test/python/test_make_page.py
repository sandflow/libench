import unittest
import os
import make_page


class MakePageTest(unittest.TestCase):

  BUILD_DIR = "build/python_test"
  BIN_PATH = "./build/libench"

  def setUp(self):
    os.makedirs(MakePageTest.BUILD_DIR, exist_ok=True)

  def test_perf_tests(self):
    results = make_page.run_perf_tests("src/test/resources/images", MakePageTest.BIN_PATH)

    self.assertIsNotNone(results)
    self.assertEqual(len(results), 14)

  def test_make_analysis(self):
    make_page.make_analysis("src/test/resources/results/results.csv", MakePageTest.BUILD_DIR)

  def test_make_index(self):
    make_page.make_index(MakePageTest.BUILD_DIR, "unknown", "unknown")
