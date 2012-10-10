// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_FILEAPI_FILE_SYSTEM_FILE_STREAM_READER_H_
#define WEBKIT_FILEAPI_FILE_SYSTEM_FILE_STREAM_READER_H_

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/platform_file.h"
#include "webkit/fileapi/file_system_url.h"
#include "webkit/blob/file_stream_reader.h"
#include "webkit/blob/shareable_file_reference.h"

class FilePath;

namespace base {
class SequencedTaskRunner;
}

namespace webkit_blob {
class LocalFileStreamReader;
}

namespace fileapi {

class FileSystemContext;

// TODO(kinaba,satorux): This generic implementation would work for any
// filesystems but remote filesystem should implement its own reader
// rather than relying on FileSystemOperation::GetSnapshotFile() which
// may force downloading the entire contents for remote files.
class FileSystemFileStreamReader : public webkit_blob::FileStreamReader {
 public:
  // Creates a new FileReader for a filesystem URL |url| form |initial_offset|.
  FileSystemFileStreamReader(FileSystemContext* file_system_context,
                             const FileSystemURL& url,
                             int64 initial_offset);
  virtual ~FileSystemFileStreamReader();

  // FileStreamReader overrides.
  virtual int Read(net::IOBuffer* buf, int buf_len,
                   const net::CompletionCallback& callback) OVERRIDE;
  virtual int GetLength(
      const net::Int64CompletionCallback& callback) OVERRIDE;

 private:
  int CreateSnapshot(const base::Closure& callback,
                     const net::CompletionCallback& error_callback);
  void DidCreateSnapshot(
      const base::Closure& callback,
      const net::CompletionCallback& error_callback,
      base::PlatformFileError file_error,
      const base::PlatformFileInfo& file_info,
      const FilePath& platform_path,
      const scoped_refptr<webkit_blob::ShareableFileReference>& file_ref);

  scoped_refptr<FileSystemContext> file_system_context_;
  FileSystemURL url_;
  const int64 initial_offset_;
  scoped_ptr<webkit_blob::LocalFileStreamReader> local_file_reader_;
  scoped_refptr<webkit_blob::ShareableFileReference> snapshot_ref_;
  bool has_pending_create_snapshot_;
  base::WeakPtrFactory<FileSystemFileStreamReader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemFileStreamReader);
};

}  // namespace fileapi

#endif  // WEBKIT_FILEAPI_FILE_SYSTEM_FILE_STREAM_READER_H_
