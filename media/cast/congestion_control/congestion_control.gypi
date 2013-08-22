# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'congestion_control',
      'type': 'static_library',
      'include_dirs': [
        '../../../',
      ],
      'sources': [
        'congestion_control.h',
        'congestion_control.cc',
      ], # source
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:test_support_base',
      ],
  },
  ],
}

