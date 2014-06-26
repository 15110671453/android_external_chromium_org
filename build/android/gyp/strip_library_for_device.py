#!/usr/bin/env python
#
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import fnmatch
import optparse
import os
import sys

from util import build_utils

options = None

def StripLibrary(android_strip, android_strip_args, library_path, output_path):
  if build_utils.IsTimeStale(output_path, [library_path]):
    strip_cmd = ([android_strip] +
                 android_strip_args +
                 ['-o', output_path, library_path])
    build_utils.CheckOutput(strip_cmd)

#return filenames only in '_options.input_libraries'
def InputLibNames():
  with open(options.lib_absolute_path, 'r') as libfile:
    libraries = json.load(libfile)
  return libraries


#search recursively in all the folders under 'path' and check
#if file exist and return the path of file
def FindFullPath(name, path):
  matches = []
  for root, dirnames, filenames in os.walk(path):
    for filename in fnmatch.filter(filenames, name):
      abs_path = os.path.join(root)+"/"+name
      #Update match if abs_path exist in _options.input_libraries
      for file_name in fnmatch.filter(InputLibNames(), abs_path):
        matches.append(os.path.join(root))
  if(len(matches) > 0):
    return matches.pop(0)
  return None



def main():
  parser = optparse.OptionParser()

  parser.add_option('--android-strip',
      help='Path to the toolchain\'s strip binary')
  parser.add_option('--android-strip-arg', action='append',
      help='Argument to be passed to strip')
  parser.add_option('--libraries-dir',
      help='Directory for un-stripped libraries')
  parser.add_option('--stripped-libraries-dir',
      help='Directory for stripped libraries')
  parser.add_option('--libraries-file',
      help='Path to json file containing list of libraries')
  parser.add_option('--lib-absolute-path',
      help='Path to json file containing list of libs with absolute path')
  parser.add_option('--stamp', help='Path to touch on success')

  global options
  options, _ = parser.parse_args()

  with open(options.libraries_file, 'r') as libfile:
    libraries = json.load(libfile)

  build_utils.MakeDirectory(options.stripped_libraries_dir)

  for library in libraries:
    find_path = FindFullPath(library, options.libraries_dir)
    library_path = os.path.join(find_path, library)
    stripped_library_path = os.path.join(
        options.stripped_libraries_dir, library)
    StripLibrary(options.android_strip, options.android_strip_arg, library_path,
        stripped_library_path)

  if options.stamp:
    build_utils.Touch(options.stamp)


if __name__ == '__main__':
  sys.exit(main())
