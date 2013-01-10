# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'sfntly',
      'type': 'static_library',
      'sources': [
        'cpp/src/sfntly/data/byte_array.cc',
        'cpp/src/sfntly/data/byte_array.h',
        'cpp/src/sfntly/data/font_data.cc',
        'cpp/src/sfntly/data/font_data.h',
        'cpp/src/sfntly/data/font_input_stream.cc',
        'cpp/src/sfntly/data/font_input_stream.h',
        'cpp/src/sfntly/data/font_output_stream.cc',
        'cpp/src/sfntly/data/font_output_stream.h',
        'cpp/src/sfntly/data/growable_memory_byte_array.cc',
        'cpp/src/sfntly/data/growable_memory_byte_array.h',
        'cpp/src/sfntly/data/memory_byte_array.cc',
        'cpp/src/sfntly/data/memory_byte_array.h',
        'cpp/src/sfntly/data/readable_font_data.cc',
        'cpp/src/sfntly/data/readable_font_data.h',
        'cpp/src/sfntly/data/writable_font_data.cc',
        'cpp/src/sfntly/data/writable_font_data.h',
        'cpp/src/sfntly/font.cc',
        'cpp/src/sfntly/font.h',
        'cpp/src/sfntly/font_factory.cc',
        'cpp/src/sfntly/font_factory.h',
        'cpp/src/sfntly/math/fixed1616.h',
        'cpp/src/sfntly/math/font_math.h',
        'cpp/src/sfntly/port/atomic.h',
        'cpp/src/sfntly/port/config.h',
        'cpp/src/sfntly/port/endian.h',
        'cpp/src/sfntly/port/exception_type.h',
        'cpp/src/sfntly/port/file_input_stream.cc',
        'cpp/src/sfntly/port/file_input_stream.h',
        'cpp/src/sfntly/port/input_stream.h',
        'cpp/src/sfntly/port/lock.cc',
        'cpp/src/sfntly/port/lock.h',
        'cpp/src/sfntly/port/memory_input_stream.cc',
        'cpp/src/sfntly/port/memory_input_stream.h',
        'cpp/src/sfntly/port/memory_output_stream.cc',
        'cpp/src/sfntly/port/memory_output_stream.h',
        'cpp/src/sfntly/port/output_stream.h',
        'cpp/src/sfntly/port/refcount.h',
        'cpp/src/sfntly/port/type.h',
        'cpp/src/sfntly/table/bitmap/big_glyph_metrics.cc',
        'cpp/src/sfntly/table/bitmap/big_glyph_metrics.h',
        'cpp/src/sfntly/table/bitmap/bitmap_glyph.cc',
        'cpp/src/sfntly/table/bitmap/bitmap_glyph.h',
        'cpp/src/sfntly/table/bitmap/bitmap_glyph_info.cc',
        'cpp/src/sfntly/table/bitmap/bitmap_glyph_info.h',
        'cpp/src/sfntly/table/bitmap/bitmap_size_table.cc',
        'cpp/src/sfntly/table/bitmap/bitmap_size_table.h',
        'cpp/src/sfntly/table/bitmap/composite_bitmap_glyph.cc',
        'cpp/src/sfntly/table/bitmap/composite_bitmap_glyph.h',
        'cpp/src/sfntly/table/bitmap/ebdt_table.cc',
        'cpp/src/sfntly/table/bitmap/ebdt_table.h',
        'cpp/src/sfntly/table/bitmap/eblc_table.cc',
        'cpp/src/sfntly/table/bitmap/eblc_table.h',
        'cpp/src/sfntly/table/bitmap/ebsc_table.cc',
        'cpp/src/sfntly/table/bitmap/ebsc_table.h',
        'cpp/src/sfntly/table/bitmap/glyph_metrics.cc',
        'cpp/src/sfntly/table/bitmap/glyph_metrics.h',
        'cpp/src/sfntly/table/bitmap/index_sub_table.cc',
        'cpp/src/sfntly/table/bitmap/index_sub_table.h',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format1.cc',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format1.h',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format2.cc',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format2.h',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format3.cc',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format3.h',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format4.cc',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format4.h',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format5.cc',
        'cpp/src/sfntly/table/bitmap/index_sub_table_format5.h',
        'cpp/src/sfntly/table/bitmap/simple_bitmap_glyph.cc',
        'cpp/src/sfntly/table/bitmap/simple_bitmap_glyph.h',
        'cpp/src/sfntly/table/bitmap/small_glyph_metrics.cc',
        'cpp/src/sfntly/table/bitmap/small_glyph_metrics.h',
        'cpp/src/sfntly/table/byte_array_table_builder.cc',
        'cpp/src/sfntly/table/byte_array_table_builder.h',
        'cpp/src/sfntly/table/core/cmap_table.cc',
        'cpp/src/sfntly/table/core/cmap_table.h',
        'cpp/src/sfntly/table/core/font_header_table.cc',
        'cpp/src/sfntly/table/core/font_header_table.h',
        'cpp/src/sfntly/table/core/horizontal_device_metrics_table.cc',
        'cpp/src/sfntly/table/core/horizontal_device_metrics_table.h',
        'cpp/src/sfntly/table/core/horizontal_header_table.cc',
        'cpp/src/sfntly/table/core/horizontal_header_table.h',
        'cpp/src/sfntly/table/core/horizontal_metrics_table.cc',
        'cpp/src/sfntly/table/core/horizontal_metrics_table.h',
        'cpp/src/sfntly/table/core/maximum_profile_table.cc',
        'cpp/src/sfntly/table/core/maximum_profile_table.h',
        'cpp/src/sfntly/table/core/name_table.cc',
        'cpp/src/sfntly/table/core/name_table.h',
        'cpp/src/sfntly/table/core/os2_table.cc',
        'cpp/src/sfntly/table/core/os2_table.h',
        'cpp/src/sfntly/table/font_data_table.cc',
        'cpp/src/sfntly/table/font_data_table.h',
        'cpp/src/sfntly/table/generic_table_builder.cc',
        'cpp/src/sfntly/table/generic_table_builder.h',
        'cpp/src/sfntly/table/header.cc',
        'cpp/src/sfntly/table/header.h',
        'cpp/src/sfntly/table/subtable.cc',
        'cpp/src/sfntly/table/subtable.h',
        'cpp/src/sfntly/table/subtable_container_table.h',
        'cpp/src/sfntly/table/table.cc',
        'cpp/src/sfntly/table/table.h',
        'cpp/src/sfntly/table/table_based_table_builder.cc',
        'cpp/src/sfntly/table/table_based_table_builder.h',
        'cpp/src/sfntly/table/truetype/glyph_table.cc',
        'cpp/src/sfntly/table/truetype/glyph_table.h',
        'cpp/src/sfntly/table/truetype/loca_table.cc',
        'cpp/src/sfntly/table/truetype/loca_table.h',
        'cpp/src/sfntly/tag.cc',
        'cpp/src/sfntly/tag.h',
        'cpp/src/sample/chromium/font_subsetter.cc',
        'cpp/src/sample/chromium/font_subsetter.h',
        'cpp/src/sample/chromium/subsetter_impl.cc',
        'cpp/src/sample/chromium/subsetter_impl.h',
      ],
      'include_dirs': [
        'cpp/src', '../..',
      ],
      # This macro must be define to suppress the use of exception
      'defines': [
        'SFNTLY_NO_EXCEPTION',
      ],
      'dependencies' : [
        '../icu/icu.gyp:icuuc',
      ],
      # TODO(jschuh): http://crbug.com/167187
      'msvs_disabled_warnings': [ 4267 ],
    },
  ]
}
