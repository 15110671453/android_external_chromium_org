// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SHARED_IMPL_FILE_SYSTEM_UTIL_H_
#define PPAPI_SHARED_IMPL_FILE_SYSTEM_UTIL_H_

#include "ppapi/c/pp_file_info.h"
#include "ppapi/shared_impl/ppapi_shared_export.h"
#include "webkit/common/fileapi/file_system_types.h"

namespace ppapi {

PPAPI_SHARED_EXPORT
fileapi::FileSystemType PepperFileSystemTypeToFileSystemType(
    PP_FileSystemType type);

PPAPI_SHARED_EXPORT bool FileSystemTypeIsValid(PP_FileSystemType type);

PPAPI_SHARED_EXPORT bool FileSystemTypeHasQuota(PP_FileSystemType type);

}  // namespace ppapi

#endif  // PPAPI_SHARED_IMPL_FILE_SYSTEM_UTIL_H_
