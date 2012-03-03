// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_FILEAPI_FILE_UTIL_HELPER_H_
#define WEBKIT_FILEAPI_FILE_UTIL_HELPER_H_

#include "base/platform_file.h"

namespace fileapi {

class FileSystemFileUtil;
class FileSystemOperationContext;
class FileSystemPath;

// A collection of static methods that are usually called by
// FileSystemFileUtilProxy.  The method should be called on FILE thread.
class FileUtilHelper {
 public:
  static base::PlatformFileError Delete(
      FileSystemOperationContext* context,
      FileSystemFileUtil* file_util,
      const FileSystemPath& path,
      bool recursive);

 private:
  static base::PlatformFileError DeleteDirectoryRecursive(
      FileSystemOperationContext* context,
      FileSystemFileUtil* file_util,
      const FileSystemPath& path);
};

}  // namespace fileapi

#endif  // WEBKIT_FILEAPI_FILE_UTIL_HELPER_H_
