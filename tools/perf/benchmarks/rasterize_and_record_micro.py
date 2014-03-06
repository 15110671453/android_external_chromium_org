# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from benchmarks import silk_flags
from measurements import rasterize_and_record_micro
from telemetry import test


@test.Disabled('android', 'linux')
class RasterizeAndRecordMicroTop25(test.Test):
  """Measures rasterize and record performance on the top 25 web pages.

  http://www.chromium.org/developers/design-documents/rendering-benchmarks"""
  test = rasterize_and_record_micro.RasterizeAndRecordMicro
  page_set = 'page_sets/top_25.json'


class RasterizeAndRecordMicroKeyMobileSites(test.Test):
  """Measures rasterize and record performance on the key mobile sites.

  http://www.chromium.org/developers/design-documents/rendering-benchmarks"""
  test = rasterize_and_record_micro.RasterizeAndRecordMicro
  page_set = 'page_sets/key_mobile_sites.json'


class RasterizeAndRecordMicroKeySilkCases(test.Test):
  """Measures rasterize and record performance on the silk sites.

  http://www.chromium.org/developers/design-documents/rendering-benchmarks"""
  test = rasterize_and_record_micro.RasterizeAndRecordMicro
  page_set = 'page_sets/key_silk_cases.json'


class RasterizeAndRecordMicroFastPathKeySilkCases(test.Test):
  """Measures rasterize and record performance on the silk sites.

  Uses bleeding edge rendering fast paths.

  http://www.chromium.org/developers/design-documents/rendering-benchmarks"""
  tag = 'fast_path'
  test = rasterize_and_record_micro.RasterizeAndRecordMicro
  page_set = 'page_sets/key_silk_cases.json'
  def CustomizeBrowserOptions(self, options):
    silk_flags.CustomizeBrowserOptionsForFastPath(options)
