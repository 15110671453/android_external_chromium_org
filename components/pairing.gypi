# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'pairing',
      'type': 'static_library',
      'include_dirs': [
        '..',
      ],
      'dependencies': [
        'pairing_api_proto',
        '../base/base.gyp:base',
        '../device/bluetooth/bluetooth.gyp:device_bluetooth',
      ],
      'sources': [
        'pairing/fake_controller_pairing_controller.cc',
        'pairing/fake_controller_pairing_controller.h',
        'pairing/fake_host_pairing_controller.cc',
        'pairing/fake_host_pairing_controller.h',
        'pairing/controller_pairing_controller.cc',
        'pairing/controller_pairing_controller.h',
        'pairing/host_pairing_controller.cc',
        'pairing/host_pairing_controller.h',
      ],
    },
    {
      # Protobuf compiler / generator for the pairing api protocol buffer.
      'target_name': 'pairing_api_proto',
      'type': 'static_library',
      'sources': [ 'pairing/pairing_api.proto' ],
      'variables': {
        'proto_in_dir': 'pairing',
        'proto_out_dir': 'components/pairing',
      },
      'includes': [ '../build/protoc.gypi' ]
    },
  ],
}
