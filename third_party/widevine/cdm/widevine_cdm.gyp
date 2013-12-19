# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'widevine_cdm_version_h_file%': 'widevine_cdm_version.h',
    'widevine_cdm_binary_files%': [],
    'conditions': [
      [ 'branding == "Chrome"', {
        'conditions': [
          [ 'chromeos == 1', {
            'widevine_cdm_version_h_file%':
                'symbols/chromeos/<(target_arch)/widevine_cdm_version.h',
            'widevine_cdm_binary_files%': [
              'binaries/chromeos/<(target_arch)/libwidevinecdm.so',
            ],
          }],
          [ 'OS == "linux" and chromeos == 0', {
            'widevine_cdm_version_h_file%':
                'symbols/linux/<(target_arch)/widevine_cdm_version.h',
            'widevine_cdm_binary_files%': [
              'binaries/linux/<(target_arch)/libwidevinecdm.so',
            ],
          }],
          [ 'OS == "mac"', {
            'widevine_cdm_version_h_file%':
                'symbols/mac/<(target_arch)/widevine_cdm_version.h',
            'widevine_cdm_binary_files%': [
              'binaries/mac/<(target_arch)/libwidevinecdm.dylib',
            ],
          }],
          [ 'OS == "win"', {
            'widevine_cdm_version_h_file%':
                'symbols/win/<(target_arch)/widevine_cdm_version.h',
            'widevine_cdm_binary_files%': [
              'binaries/win/<(target_arch)/widevinecdm.dll',
              'binaries/win/<(target_arch)/widevinecdm.dll.lib',
            ],
          }],
        ],
      }],
      [ 'OS == "android" and google_tv != 1', {
        'widevine_cdm_version_h_file%':
            'android/widevine_cdm_version.h',
      }],
    ],
  },
  # Always provide a target, so we can put the logic about whether there's
  # anything to be done in this file (instead of a higher-level .gyp file).
  'targets': [
    {
      'target_name': 'widevinecdmadapter',
      'type': 'none',
      'conditions': [
        [ 'branding == "Chrome" and enable_pepper_cdms==1', {
          'dependencies': [
            '<(DEPTH)/ppapi/ppapi.gyp:ppapi_cpp',
            '<(DEPTH)/media/media_cdm_adapter.gyp:cdmadapter',
            'widevine_cdm_version_h',
            'widevine_cdm_binaries',
          ],
          'conditions': [
            [ 'os_posix == 1 and OS != "mac"', {
              'libraries': [
                # Copied by widevine_cdm_binaries.
                '<(PRODUCT_DIR)/libwidevinecdm.so',
              ],
            }],
            [ 'OS == "win"', {
              'libraries': [
                # Copied by widevine_cdm_binaries.
                '<(PRODUCT_DIR)/widevinecdm.dll.lib',
              ],
            }],
            [ 'OS == "mac"', {
              'libraries': [
                # Copied by widevine_cdm_binaries.
                '<(PRODUCT_DIR)/libwidevinecdm.dylib',
              ],
            }],
          ],
        }],
      ],
    },
    {
      'target_name': 'widevine_cdm_version_h',
      'type': 'none',
      'copies': [{
        'destination': '<(SHARED_INTERMEDIATE_DIR)',
        'files': [ '<(widevine_cdm_version_h_file)' ],
      }],
    },
    {
      'target_name': 'widevine_cdm_binaries',
      'type': 'none',
      'conditions': [
        [ 'OS=="mac"', {
          'xcode_settings': {
            'COPY_PHASE_STRIP': 'NO',
          }
        }],
      ],
      'copies': [{
        # TODO(ddorwin): Do we need a sub-directory? We either need a
        # sub-directory or to rename manifest.json before we can copy it.
        'destination': '<(PRODUCT_DIR)',
        'files': [ '<@(widevine_cdm_binary_files)' ],
      }],
    },
  ],
}
