// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/file_util_proxy.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/file_util.h"
#include "base/message_loop_proxy.h"
#include "base/task_runner_util.h"

namespace base {

namespace {

void CallWithTranslatedParameter(const FileUtilProxy::StatusCallback& callback,
                                 bool value) {
  DCHECK(!callback.is_null());
  callback.Run(value ? PLATFORM_FILE_OK : PLATFORM_FILE_ERROR_FAILED);
}

// Helper classes or routines for individual methods.
class CreateOrOpenHelper {
 public:
  CreateOrOpenHelper(MessageLoopProxy* message_loop_proxy,
                     const FileUtilProxy::CloseTask& close_task)
      : message_loop_proxy_(message_loop_proxy),
        close_task_(close_task),
        file_handle_(kInvalidPlatformFileValue),
        created_(false),
        error_(PLATFORM_FILE_OK) {}

  ~CreateOrOpenHelper() {
    if (file_handle_ != kInvalidPlatformFileValue) {
      message_loop_proxy_->PostTask(
          FROM_HERE,
          base::Bind(base::IgnoreResult(close_task_), file_handle_));
    }
  }

  void RunWork(const FileUtilProxy::CreateOrOpenTask& task) {
    error_ = task.Run(&file_handle_, &created_);
  }

  void Reply(const FileUtilProxy::CreateOrOpenCallback& callback) {
    DCHECK(!callback.is_null());
    callback.Run(error_, PassPlatformFile(&file_handle_), created_);
  }

 private:
  scoped_refptr<MessageLoopProxy> message_loop_proxy_;
  FileUtilProxy::CloseTask close_task_;
  PlatformFile file_handle_;
  bool created_;
  PlatformFileError error_;
  DISALLOW_COPY_AND_ASSIGN(CreateOrOpenHelper);
};

class CreateTemporaryHelper {
 public:
  CreateTemporaryHelper(MessageLoopProxy* message_loop_proxy)
      : message_loop_proxy_(message_loop_proxy),
        file_handle_(kInvalidPlatformFileValue),
        error_(PLATFORM_FILE_OK) {}

  ~CreateTemporaryHelper() {
    if (file_handle_ != kInvalidPlatformFileValue) {
      FileUtilProxy::Close(message_loop_proxy_, file_handle_,
                           FileUtilProxy::StatusCallback());
    }
  }

  void RunWork(int additional_file_flags) {
    // TODO(darin): file_util should have a variant of CreateTemporaryFile
    // that returns a FilePath and a PlatformFile.
    file_util::CreateTemporaryFile(&file_path_);

    int file_flags =
        PLATFORM_FILE_WRITE |
        PLATFORM_FILE_TEMPORARY |
        PLATFORM_FILE_CREATE_ALWAYS |
        additional_file_flags;

    error_ = PLATFORM_FILE_OK;
    file_handle_ = CreatePlatformFile(file_path_, file_flags, NULL, &error_);
  }

  void Reply(const FileUtilProxy::CreateTemporaryCallback& callback) {
    DCHECK(!callback.is_null());
    callback.Run(error_, PassPlatformFile(&file_handle_), file_path_);
  }

 private:
  scoped_refptr<MessageLoopProxy> message_loop_proxy_;
  PlatformFile file_handle_;
  FilePath file_path_;
  PlatformFileError error_;
  DISALLOW_COPY_AND_ASSIGN(CreateTemporaryHelper);
};

class GetFileInfoHelper {
 public:
  GetFileInfoHelper()
      : error_(PLATFORM_FILE_OK) {}

  void RunWorkForFilePath(const FilePath& file_path) {
    if (!file_util::PathExists(file_path)) {
      error_ = PLATFORM_FILE_ERROR_NOT_FOUND;
      return;
    }
    if (!file_util::GetFileInfo(file_path, &file_info_))
      error_ = PLATFORM_FILE_ERROR_FAILED;
  }

  void RunWorkForPlatformFile(PlatformFile file) {
    if (!GetPlatformFileInfo(file, &file_info_))
      error_ = PLATFORM_FILE_ERROR_FAILED;
  }

  void Reply(const FileUtilProxy::GetFileInfoCallback& callback) {
    if (!callback.is_null()) {
      callback.Run(error_, file_info_);
    }
  }

 private:
  PlatformFileError error_;
  PlatformFileInfo file_info_;
  DISALLOW_COPY_AND_ASSIGN(GetFileInfoHelper);
};

class ReadHelper {
 public:
  ReadHelper(int bytes_to_read)
      : buffer_(new char[bytes_to_read]),
        bytes_to_read_(bytes_to_read),
        bytes_read_(0) {}

  void RunWork(PlatformFile file, int64 offset) {
    bytes_read_ = ReadPlatformFile(file, offset, buffer_.get(), bytes_to_read_);
  }

  void Reply(const FileUtilProxy::ReadCallback& callback) {
    if (!callback.is_null()) {
      PlatformFileError error =
          (bytes_read_ < 0) ? PLATFORM_FILE_ERROR_FAILED : PLATFORM_FILE_OK;
      callback.Run(error, buffer_.get(), bytes_read_);
    }
  }

 private:
  scoped_array<char> buffer_;
  int bytes_to_read_;
  int bytes_read_;
  DISALLOW_COPY_AND_ASSIGN(ReadHelper);
};

class WriteHelper {
 public:
  WriteHelper(const char* buffer, int bytes_to_write)
      : buffer_(new char[bytes_to_write]),
        bytes_to_write_(bytes_to_write),
        bytes_written_(0) {
    memcpy(buffer_.get(), buffer, bytes_to_write);
  }

  void RunWork(PlatformFile file, int64 offset) {
    bytes_written_ = WritePlatformFile(file, offset, buffer_.get(),
                                       bytes_to_write_);
  }

  void Reply(const FileUtilProxy::WriteCallback& callback) {
    if (!callback.is_null()) {
      PlatformFileError error =
          (bytes_written_ < 0) ? PLATFORM_FILE_ERROR_FAILED : PLATFORM_FILE_OK;
      callback.Run(error, bytes_written_);
    }
  }

 private:
  scoped_array<char> buffer_;
  int bytes_to_write_;
  int bytes_written_;
  DISALLOW_COPY_AND_ASSIGN(WriteHelper);
};


PlatformFileError CreateOrOpenAdapter(
    const FilePath& file_path, int file_flags,
    PlatformFile* file_handle, bool* created) {
  DCHECK(file_handle);
  DCHECK(created);
  if (!file_util::DirectoryExists(file_path.DirName())) {
    // If its parent does not exist, should return NOT_FOUND error.
    return PLATFORM_FILE_ERROR_NOT_FOUND;
  }
  PlatformFileError error = PLATFORM_FILE_OK;
  *file_handle = CreatePlatformFile(file_path, file_flags, created, &error);
  return error;
}

PlatformFileError CloseAdapter(PlatformFile file_handle) {
  if (!ClosePlatformFile(file_handle)) {
    return PLATFORM_FILE_ERROR_FAILED;
  }
  return PLATFORM_FILE_OK;
}

PlatformFileError DeleteAdapter(const FilePath& file_path, bool recursive) {
  if (!file_util::PathExists(file_path)) {
    return PLATFORM_FILE_ERROR_NOT_FOUND;
  }
  if (!file_util::Delete(file_path, recursive)) {
    if (!recursive && !file_util::IsDirectoryEmpty(file_path)) {
      return PLATFORM_FILE_ERROR_NOT_EMPTY;
    }
    return PLATFORM_FILE_ERROR_FAILED;
  }
  return PLATFORM_FILE_OK;
}

}  // namespace

// static
bool FileUtilProxy::CreateOrOpen(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    const FilePath& file_path, int file_flags,
    const CreateOrOpenCallback& callback) {
  return RelayCreateOrOpen(
      message_loop_proxy,
      base::Bind(&CreateOrOpenAdapter, file_path, file_flags),
      base::Bind(&CloseAdapter),
      callback);
}

// static
bool FileUtilProxy::CreateTemporary(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    int additional_file_flags,
    const CreateTemporaryCallback& callback) {
  CreateTemporaryHelper* helper = new CreateTemporaryHelper(message_loop_proxy);
  return message_loop_proxy->PostTaskAndReply(
      FROM_HERE,
      Bind(&CreateTemporaryHelper::RunWork, Unretained(helper),
           additional_file_flags),
      Bind(&CreateTemporaryHelper::Reply, Owned(helper), callback));
}

// static
bool FileUtilProxy::Close(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    base::PlatformFile file_handle,
    const StatusCallback& callback) {
  return RelayClose(
      message_loop_proxy,
      base::Bind(&CloseAdapter),
      file_handle, callback);
}

// Retrieves the information about a file. It is invalid to pass NULL for the
// callback.
bool FileUtilProxy::GetFileInfo(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    const FilePath& file_path,
    const GetFileInfoCallback& callback) {
  GetFileInfoHelper* helper = new GetFileInfoHelper;
  return message_loop_proxy->PostTaskAndReply(
      FROM_HERE,
      Bind(&GetFileInfoHelper::RunWorkForFilePath,
           Unretained(helper), file_path),
      Bind(&GetFileInfoHelper::Reply, Owned(helper), callback));
}

// static
bool FileUtilProxy::GetFileInfoFromPlatformFile(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    PlatformFile file,
    const GetFileInfoCallback& callback) {
  GetFileInfoHelper* helper = new GetFileInfoHelper;
  return message_loop_proxy->PostTaskAndReply(
      FROM_HERE,
      Bind(&GetFileInfoHelper::RunWorkForPlatformFile,
           Unretained(helper), file),
      Bind(&GetFileInfoHelper::Reply, Owned(helper), callback));
}

// static
bool FileUtilProxy::Delete(scoped_refptr<MessageLoopProxy> message_loop_proxy,
                           const FilePath& file_path,
                           bool recursive,
                           const StatusCallback& callback) {
  return RelayFileTask(
      message_loop_proxy, FROM_HERE,
      Bind(&DeleteAdapter, file_path, recursive),
      callback);
}

// static
bool FileUtilProxy::RecursiveDelete(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    const FilePath& file_path,
    const StatusCallback& callback) {
  return RelayFileTask(
      message_loop_proxy, FROM_HERE,
      Bind(&DeleteAdapter, file_path, true /* recursive */),
      callback);
}

// static
bool FileUtilProxy::Read(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    PlatformFile file,
    int64 offset,
    int bytes_to_read,
    const ReadCallback& callback) {
  if (bytes_to_read < 0) {
    return false;
  }
  ReadHelper* helper = new ReadHelper(bytes_to_read);
  return message_loop_proxy->PostTaskAndReply(
      FROM_HERE,
      Bind(&ReadHelper::RunWork, Unretained(helper), file, offset),
      Bind(&ReadHelper::Reply, Owned(helper), callback));
}

// static
bool FileUtilProxy::Write(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    PlatformFile file,
    int64 offset,
    const char* buffer,
    int bytes_to_write,
    const WriteCallback& callback) {
  if (bytes_to_write <= 0 || buffer == NULL) {
    return false;
  }
  WriteHelper* helper = new WriteHelper(buffer, bytes_to_write);
  return message_loop_proxy->PostTaskAndReply(
      FROM_HERE,
      Bind(&WriteHelper::RunWork, Unretained(helper), file, offset),
      Bind(&WriteHelper::Reply, Owned(helper), callback));
}

// static
bool FileUtilProxy::Touch(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    PlatformFile file,
    const Time& last_access_time,
    const Time& last_modified_time,
    const StatusCallback& callback) {
  return base::PostTaskAndReplyWithResult(
      message_loop_proxy,
      FROM_HERE,
      Bind(&TouchPlatformFile, file,
           last_access_time, last_modified_time),
      Bind(&CallWithTranslatedParameter, callback));
}

// static
bool FileUtilProxy::Touch(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    const FilePath& file_path,
    const Time& last_access_time,
    const Time& last_modified_time,
    const StatusCallback& callback) {
  return base::PostTaskAndReplyWithResult(
      message_loop_proxy,
      FROM_HERE,
      Bind(&file_util::TouchFile, file_path,
           last_access_time, last_modified_time),
      Bind(&CallWithTranslatedParameter, callback));
}

// static
bool FileUtilProxy::Truncate(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    PlatformFile file,
    int64 length,
    const StatusCallback& callback) {
  return base::PostTaskAndReplyWithResult(
      message_loop_proxy,
      FROM_HERE,
      Bind(&TruncatePlatformFile, file, length),
      Bind(&CallWithTranslatedParameter, callback));
}

// static
bool FileUtilProxy::Flush(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    PlatformFile file,
    const StatusCallback& callback) {
  return base::PostTaskAndReplyWithResult(
      message_loop_proxy,
      FROM_HERE,
      Bind(&FlushPlatformFile, file),
      Bind(&CallWithTranslatedParameter, callback));
}

// static
bool FileUtilProxy::RelayFileTask(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    const tracked_objects::Location& from_here,
    const FileTask& file_task,
    const StatusCallback& callback) {
  return base::PostTaskAndReplyWithResult(
      message_loop_proxy, from_here, file_task, callback);
}

// static
bool FileUtilProxy::RelayCreateOrOpen(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    const CreateOrOpenTask& open_task,
    const CloseTask& close_task,
    const CreateOrOpenCallback& callback) {
  CreateOrOpenHelper* helper = new CreateOrOpenHelper(
      message_loop_proxy, close_task);
  return message_loop_proxy->PostTaskAndReply(
      FROM_HERE,
      Bind(&CreateOrOpenHelper::RunWork, Unretained(helper), open_task),
      Bind(&CreateOrOpenHelper::Reply, Owned(helper), callback));
}

// static
bool FileUtilProxy::RelayClose(
    scoped_refptr<MessageLoopProxy> message_loop_proxy,
    const CloseTask& close_task,
    PlatformFile file_handle,
    const StatusCallback& callback) {
  return base::PostTaskAndReplyWithResult(
      message_loop_proxy, FROM_HERE, Bind(close_task, file_handle), callback);
}

}  // namespace base
