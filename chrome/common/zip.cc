// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/zip.h"

#include "base/bind.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/string16.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "net/base/file_stream.h"
#include "third_party/zlib/contrib/minizip/unzip.h"
#include "third_party/zlib/contrib/minizip/zip.h"
#if defined(OS_WIN)
#include "third_party/zlib/contrib/minizip/iowin32.h"
#endif

namespace {

const int kZipMaxPath = 256;
const int kZipBufSize = 8192;

// Extract the 'current' selected file from the zip into dest_dir.
// Output filename is stored in out_file.  Returns true on success.
bool ExtractCurrentFile(unzFile zip_file,
                        const FilePath& dest_dir) {
  // We assume that the file names in zip files to be UTF-8. This is true
  // for zip files created with Zip() and friends in the file.
  char filename_in_zip_utf8[kZipMaxPath] = {0};
  unz_file_info file_info;
  int err = unzGetCurrentFileInfo(
      zip_file, &file_info, filename_in_zip_utf8,
      sizeof(filename_in_zip_utf8) - 1, NULL, 0, NULL, 0);
  if (err != UNZ_OK)
    return false;
  if (filename_in_zip_utf8[0] == '\0')
    return false;

  err = unzOpenCurrentFile(zip_file);
  if (err != UNZ_OK)
    return false;

  // Use of "Unsafe" function looks not good, but there is no safe way to
  // do this on Linux anyway. See file_path.h for details.
  FilePath file_path_in_zip = FilePath::FromUTF8Unsafe(filename_in_zip_utf8);

  // Check the filename here for directory traversal issues. In the name of
  // simplicity and security, we might reject a valid filename such as "a..b".
  if (file_path_in_zip.value().find(FILE_PATH_LITERAL(".."))
      != FilePath::StringType::npos)
    return false;

  FilePath dest_file = dest_dir.Append(file_path_in_zip);

  // If this is a directory, just create it and return.
  if (EndsWith(filename_in_zip_utf8, "/", false)) {
    if (!file_util::CreateDirectory(dest_file))
      return false;
    return true;
  }

  // We can't rely on parent directory entries being specified in the zip, so we
  // make sure they are created.
  FilePath dir = dest_file.DirName();
  if (!file_util::CreateDirectory(dir))
    return false;

  net::FileStream stream;
  int flags = base::PLATFORM_FILE_CREATE_ALWAYS | base::PLATFORM_FILE_WRITE;
  if (stream.Open(dest_file, flags) != 0)
    return false;

  bool ret = true;
  int num_bytes = 0;
  char buf[kZipBufSize];
  do {
    num_bytes = unzReadCurrentFile(zip_file, buf, kZipBufSize);
    if (num_bytes < 0) {
      // If num_bytes < 0, then it's a specific UNZ_* error code.
      // While we're not currently handling these codes specifically, save
      // it away in case we want to in the future.
      err = num_bytes;
      break;
    }
    if (num_bytes > 0) {
      if (num_bytes != stream.Write(buf, num_bytes,
                                    net::CompletionCallback())) {
        ret = false;
        break;
      }
    }
  } while (num_bytes > 0);

  stream.Close();
  if (err == UNZ_OK)
    err = unzCloseCurrentFile(zip_file);
  else
    unzCloseCurrentFile(zip_file);  // Don't lose the original error code.
  if (err != UNZ_OK)
    ret = false;
  return ret;
}

#if defined(OS_WIN)
typedef struct {
  HANDLE hf;
  int error;
} WIN32FILE_IOWIN;

// This function is derived from third_party/minizip/iowin32.c.
// Its only difference is that it treats the char* as UTF8 and
// uses the Unicode version of CreateFile.
void* ZipOpenFunc(void *opaque, const char* filename, int mode) {
  DWORD desired_access, creation_disposition;
  DWORD share_mode, flags_and_attributes;
  HANDLE file = 0;
  void* ret = NULL;

  desired_access = share_mode = flags_and_attributes = 0;

  if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ) {
    desired_access = GENERIC_READ;
    creation_disposition = OPEN_EXISTING;
    share_mode = FILE_SHARE_READ;
  } else if (mode & ZLIB_FILEFUNC_MODE_EXISTING) {
    desired_access = GENERIC_WRITE | GENERIC_READ;
    creation_disposition = OPEN_EXISTING;
  } else if (mode & ZLIB_FILEFUNC_MODE_CREATE) {
    desired_access = GENERIC_WRITE | GENERIC_READ;
    creation_disposition = CREATE_ALWAYS;
  }

  string16 filename16 = UTF8ToUTF16(filename);
  if ((filename != NULL) && (desired_access != 0)) {
    file = CreateFile(filename16.c_str(), desired_access, share_mode,
        NULL, creation_disposition, flags_and_attributes, NULL);
  }

  if (file == INVALID_HANDLE_VALUE)
    file = NULL;

  if (file != NULL) {
    WIN32FILE_IOWIN file_ret;
    file_ret.hf = file;
    file_ret.error = 0;
    ret = malloc(sizeof(WIN32FILE_IOWIN));
    if (ret == NULL)
      CloseHandle(file);
    else
      *(static_cast<WIN32FILE_IOWIN*>(ret)) = file_ret;
  }
  return ret;
}
#endif

// Opens the given file name in UTF-8 for unzipping, with some setup for
// Windows.
unzFile OpenForUnzipping(const std::string& file_name_utf8) {
  zlib_filefunc_def* zip_func_ptrs = NULL;
#if defined(OS_WIN)
  zlib_filefunc_def zip_funcs;
  fill_win32_filefunc(&zip_funcs);
  zip_funcs.zopen_file = ZipOpenFunc;
  zip_func_ptrs = &zip_funcs;
#endif
  return unzOpen2(file_name_utf8.c_str(), zip_func_ptrs);
}

// Opens the given file name in UTF-8 for zipping, with some setup for
// Windows.  |append_flag| will be passed to zipOpen2().
zipFile OpenForZipping(const std::string& file_name_utf8, int append_flag) {
  zlib_filefunc_def* zip_func_ptrs = NULL;
#if defined(OS_WIN)
  zlib_filefunc_def zip_funcs;
  fill_win32_filefunc(&zip_funcs);
  zip_funcs.zopen_file = ZipOpenFunc;
  zip_func_ptrs = &zip_funcs;
#endif
  return zipOpen2(file_name_utf8.c_str(),
                  append_flag,
                  NULL,  // global comment
                  zip_func_ptrs);
}

bool AddFileToZip(zipFile zip_file, const FilePath& src_dir) {
  net::FileStream stream;
  int flags = base::PLATFORM_FILE_OPEN | base::PLATFORM_FILE_READ;
  if (stream.Open(src_dir, flags) != 0) {
    DLOG(ERROR) << "Could not open stream for path "
                << src_dir.value();
    return false;
  }

  int num_bytes;
  char buf[kZipBufSize];
  do {
    num_bytes = stream.Read(buf, kZipBufSize, net::CompletionCallback());
    if (num_bytes > 0) {
      if (ZIP_OK != zipWriteInFileInZip(zip_file, buf, num_bytes)) {
        DLOG(ERROR) << "Could not write data to zip for path "
                    << src_dir.value();
        return false;
      }
    }
  } while (num_bytes > 0);

  return true;
}

bool AddEntryToZip(zipFile zip_file, const FilePath& path,
                   const FilePath& root_path) {
  std::string str_path =
      path.AsUTF8Unsafe().substr(root_path.value().length() + 1);
#if defined(OS_WIN)
  ReplaceSubstringsAfterOffset(&str_path, 0u, "\\", "/");
#endif

  bool is_directory = file_util::DirectoryExists(path);
  if (is_directory)
    str_path += "/";

  if (ZIP_OK != zipOpenNewFileInZip(
      zip_file, str_path.c_str(),
      NULL, NULL, 0u, NULL, 0u, NULL,  // file info, extrafield local, length,
                                       // extrafield global, length, comment
      Z_DEFLATED, Z_DEFAULT_COMPRESSION)) {
    DLOG(ERROR) << "Could not open zip file entry " << str_path;
    return false;
  }

  bool success = true;
  if (!is_directory) {
    success = AddFileToZip(zip_file, path);
  }

  if (ZIP_OK != zipCloseFileInZip(zip_file)) {
    DLOG(ERROR) << "Could not close zip file entry " << str_path;
    return false;
  }

  return success;
}

bool ExcludeNoFilesFilter(const FilePath& file_path) {
  return true;
}

bool ExcludeHiddenFilesFilter(const FilePath& file_path) {
  return file_path.BaseName().value()[0] != '.';
}

}  // namespace

namespace zip {

bool Unzip(const FilePath& src_file, const FilePath& dest_dir) {
  unzFile zip_file = OpenForUnzipping(src_file.AsUTF8Unsafe());
  if (!zip_file) {
    DLOG(WARNING) << "couldn't create file " << src_file.value();
    return false;
  }
  unz_global_info zip_info;
  int err;
  err = unzGetGlobalInfo(zip_file, &zip_info);
  if (err != UNZ_OK) {
    DLOG(WARNING) << "couldn't open zip " << src_file.value();
    return false;
  }
  bool ret = true;
  for (unsigned int i = 0; i < zip_info.number_entry; ++i) {
    if (!ExtractCurrentFile(zip_file, dest_dir)) {
      ret = false;
      break;
    }

    if (i + 1 < zip_info.number_entry) {
      err = unzGoToNextFile(zip_file);
      if (err != UNZ_OK) {
        DLOG(WARNING) << "error %d in unzGoToNextFile";
        ret = false;
        break;
      }
    }
  }
  unzClose(zip_file);
  return ret;
}

bool ZipWithFilterCallback(const FilePath& src_dir, const FilePath& dest_file,
                           const FilterCallback& filter_cb) {
  DCHECK(file_util::DirectoryExists(src_dir));

  zipFile zip_file = OpenForZipping(dest_file.AsUTF8Unsafe(),
                                    APPEND_STATUS_CREATE);

  if (!zip_file) {
    DLOG(WARNING) << "couldn't create file " << dest_file.value();
    return false;
  }

  bool success = true;
  file_util::FileEnumerator file_enumerator(
      src_dir, true,  // recursive
      static_cast<file_util::FileEnumerator::FileType>(
          file_util::FileEnumerator::FILES |
          file_util::FileEnumerator::DIRECTORIES));
  for (FilePath path = file_enumerator.Next(); !path.value().empty();
       path = file_enumerator.Next()) {
    if (!filter_cb.Run(path)) {
      continue;
    }

    if (!AddEntryToZip(zip_file, path, src_dir)) {
      success = false;
      return false;
    }
  }

  if (ZIP_OK != zipClose(zip_file, NULL)) {  // global comment
    DLOG(ERROR) << "Error closing zip file " << dest_file.value();
    return false;
  }

  return success;
}

bool Zip(const FilePath& src_dir, const FilePath& dest_file,
         bool include_hidden_files) {
  if (include_hidden_files) {
    return ZipWithFilterCallback(
        src_dir, dest_file, base::Bind(&ExcludeNoFilesFilter));
  } else {
    return ZipWithFilterCallback(
        src_dir, dest_file, base::Bind(&ExcludeHiddenFilesFilter));
  }
}

}  // namespace zip
