# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'optimize_with_syzygy%': 0,
  },
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'chrome_dll',
          'type': 'none',
          'sources' : [],
          'dependencies': [
            'chrome_dll_initial',
            'chrome',
          ],
          'conditions': [
            ['optimize_with_syzygy==1 and fastbuild==0', {
              # Optimize the initial chrome DLL file, placing the optimized
              # output and corresponding PDB file into the product directory.
              # If fastbuild!=0 then no PDB files are generated by the build
              # and the syzygy optimizations cannot run (they use the PDB
              # information to properly understand the DLLs contents), so
              # syzygy optimization cannot be performed.
              'actions': [
                {
                  'action_name': 'Optimize Chrome binaries with syzygy',
                  'msvs_cygwin_shell': 0,
                  'inputs': [
                    '<(PRODUCT_DIR)\\initial\\chrome.dll',
                    '<(PRODUCT_DIR)\\initial\\chrome_dll.pdb',
                  ],
                  'outputs': [
                    '<(PRODUCT_DIR)\\chrome.dll',
                    '<(PRODUCT_DIR)\\chrome_dll.pdb',
                  ],
                  'action': [
                    '<(DEPTH)\\third_party\\syzygy\\binaries\\optimize.bat',
                    '--verbose',
                    '--input-dir="<(PRODUCT_DIR)"',
                    '--input-dll="<(PRODUCT_DIR)\\initial\\chrome.dll"',
                    '--input-pdb="<(PRODUCT_DIR)\\initial\\chrome_dll.pdb"',
                    '--output-dir="<(INTERMEDIATE_DIR)\\optimized"',
                    '--copy-to="<(PRODUCT_DIR)"',
                  ],
                },
              ],
            }, {  # optimize_with_syzygy!=1 or fastbuild!=0           
              # Copy the chrome DLL and PDB files into the product directory.
              # If fastbuild!= 0 then there is no PDB file to copy.
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)',
                  'conditions': [
                    ['fastbuild==0', {
                      'files': [
                        '<(PRODUCT_DIR)\\initial\\chrome.dll',
                        '<(PRODUCT_DIR)\\initial\\chrome_dll.pdb',
                      ],
                    }, {
                      'files': [
                        '<(PRODUCT_DIR)\\initial\\chrome.dll',
                      ],
                    }],
                  ],
                },
              ],
            }],  # optimize_with_syzygy==0 or fastbuild==1
          ],  # conditions
        },
      ],
    }],
  ],
}
