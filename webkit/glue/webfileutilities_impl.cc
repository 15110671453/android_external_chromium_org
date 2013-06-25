// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/webfileutilities_impl.h"

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "net/base/file_stream.h"
#include "net/base/net_util.h"
#include "third_party/WebKit/public/platform/WebFileInfo.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "webkit/glue/webkit_glue.h"

using WebKit::WebString;

namespace webkit_glue {

WebFileUtilitiesImpl::WebFileUtilitiesImpl()
    : sandbox_enabled_(true) {
}

WebFileUtilitiesImpl::~WebFileUtilitiesImpl() {
}

bool WebFileUtilitiesImpl::fileExists(const WebString& path) {
  return file_util::PathExists(base::FilePath::FromUTF16Unsafe(path));
}

bool WebFileUtilitiesImpl::deleteFile(const WebString& path) {
  NOTREACHED();
  return false;
}

bool WebFileUtilitiesImpl::deleteEmptyDirectory(const WebString& path) {
  NOTREACHED();
  return false;
}

bool WebFileUtilitiesImpl::getFileInfo(const WebString& path,
                                       WebKit::WebFileInfo& web_file_info) {
  if (sandbox_enabled_) {
    NOTREACHED();
    return false;
  }
  base::PlatformFileInfo file_info;
  if (!file_util::GetFileInfo(base::FilePath::FromUTF16Unsafe(path),
                              &file_info))
    return false;

  webkit_glue::PlatformFileInfoToWebFileInfo(file_info, &web_file_info);
  web_file_info.platformPath = path;
  return true;
}

WebString WebFileUtilitiesImpl::directoryName(const WebString& path) {
  return base::FilePath::FromUTF16Unsafe(path).DirName().AsUTF16Unsafe();
}

WebString WebFileUtilitiesImpl::pathByAppendingComponent(
    const WebString& webkit_path,
    const WebString& webkit_component) {
  base::FilePath path(base::FilePath::FromUTF16Unsafe(webkit_path));
  base::FilePath component(base::FilePath::FromUTF16Unsafe(webkit_component));
  base::FilePath combined_path = path.Append(component);
  return combined_path.AsUTF16Unsafe();
}

bool WebFileUtilitiesImpl::makeAllDirectories(const WebString& path) {
  DCHECK(!sandbox_enabled_);
  return file_util::CreateDirectory(base::FilePath::FromUTF16Unsafe(path));
}

bool WebFileUtilitiesImpl::isDirectory(const WebString& path) {
  return file_util::DirectoryExists(base::FilePath::FromUTF16Unsafe(path));
}

WebKit::WebURL WebFileUtilitiesImpl::filePathToURL(const WebString& path) {
  return net::FilePathToFileURL(base::FilePath::FromUTF16Unsafe(path));
}

base::PlatformFile WebFileUtilitiesImpl::openFile(const WebString& path,
                                                  int mode) {
  if (sandbox_enabled_) {
    NOTREACHED();
    return base::kInvalidPlatformFileValue;
  }
  return base::CreatePlatformFile(
      base::FilePath::FromUTF16Unsafe(path),
      (mode == 0) ? (base::PLATFORM_FILE_OPEN | base::PLATFORM_FILE_READ)
                  : (base::PLATFORM_FILE_CREATE_ALWAYS |
                     base::PLATFORM_FILE_WRITE),
      NULL, NULL);
}

void WebFileUtilitiesImpl::closeFile(base::PlatformFile& handle) {
  if (handle == base::kInvalidPlatformFileValue)
    return;
  if (base::ClosePlatformFile(handle))
    handle = base::kInvalidPlatformFileValue;
}

long long WebFileUtilitiesImpl::seekFile(base::PlatformFile handle,
                                         long long offset,
                                         int origin) {
  if (handle == base::kInvalidPlatformFileValue)
    return -1;
  return base::SeekPlatformFile(handle,
                                static_cast<base::PlatformFileWhence>(origin),
                                offset);
}

bool WebFileUtilitiesImpl::truncateFile(base::PlatformFile handle,
                                        long long offset) {
  if (handle == base::kInvalidPlatformFileValue || offset < 0)
    return false;
  return base::TruncatePlatformFile(handle, offset);
}

int WebFileUtilitiesImpl::readFromFile(base::PlatformFile handle,
                                       char* data,
                                       int length) {
  if (handle == base::kInvalidPlatformFileValue || !data || length <= 0)
    return -1;
  return base::ReadPlatformFileCurPosNoBestEffort(handle, data, length);
}

int WebFileUtilitiesImpl::writeToFile(base::PlatformFile handle,
                                      const char* data,
                                      int length) {
  if (handle == base::kInvalidPlatformFileValue || !data || length <= 0)
    return -1;
  return base::WritePlatformFileCurPosNoBestEffort(handle, data, length);
}

}  // namespace webkit_glue
