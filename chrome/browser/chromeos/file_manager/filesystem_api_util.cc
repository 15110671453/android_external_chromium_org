// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_manager/filesystem_api_util.h"

#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/chromeos/drive/file_errors.h"
#include "chrome/browser/chromeos/drive/file_system_interface.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "chrome/browser/chromeos/file_system_provider/mount_path_util.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "google_apis/drive/task_util.h"
#include "webkit/browser/fileapi/file_system_context.h"

namespace file_manager {
namespace util {

namespace {

// Helper function used to implement GetNonNativeLocalPathMimeType. It extracts
// the mime type from the passed Drive resource entry.
void GetMimeTypeAfterGetResourceEntryForDrive(
    const base::Callback<void(bool, const std::string&)>& callback,
    drive::FileError error,
    scoped_ptr<drive::ResourceEntry> entry) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (error != drive::FILE_ERROR_OK || !entry->has_file_specific_info()) {
    callback.Run(false, std::string());
    return;
  }
  callback.Run(true, entry->file_specific_info().content_mime_type());
}

// Helper function used to implement GetNonNativeLocalPathMimeType. It extracts
// the mime type from the passed metadata from a providing extension.
void GetMimeTypeAfterGetMetadataForProvidedFileSystem(
    const base::Callback<void(bool, const std::string&)>& callback,
    const chromeos::file_system_provider::EntryMetadata& metadata,
    base::File::Error result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (result != base::File::FILE_OK) {
    callback.Run(false, std::string());
    return;
  }
  callback.Run(true, metadata.mime_type);
}

// Helper function to converts a callback that takes boolean value to that takes
// File::Error, by regarding FILE_OK as the only successful value.
void BoolCallbackAsFileErrorCallback(
    const base::Callback<void(bool)>& callback,
    base::File::Error error) {
  return callback.Run(error == base::File::FILE_OK);
}

// Part of PrepareFileOnIOThread. It tries to create a new file if the given
// |url| is not already inhabited.
void PrepareFileAfterCheckExistOnIOThread(
    scoped_refptr<fileapi::FileSystemContext> file_system_context,
    const fileapi::FileSystemURL& url,
    const fileapi::FileSystemOperation::StatusCallback& callback,
    base::File::Error error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (error != base::File::FILE_ERROR_NOT_FOUND) {
    callback.Run(error);
    return;
  }

  // Call with the second argument |exclusive| set to false, meaning that it
  // is not an error even if the file already exists (it can happen if the file
  // is created after the previous FileExists call and before this CreateFile.)
  //
  // Note that the preceding call to FileExists is necessary for handling
  // read only filesystems that blindly rejects handling CreateFile().
  file_system_context->operation_runner()->CreateFile(url, false, callback);
}

// Checks whether a file exists at the given |url|, and try creating it if it
// is not already there.
void PrepareFileOnIOThread(
    scoped_refptr<fileapi::FileSystemContext> file_system_context,
    const fileapi::FileSystemURL& url,
    const base::Callback<void(bool)>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  file_system_context->operation_runner()->FileExists(
      url,
      base::Bind(&PrepareFileAfterCheckExistOnIOThread,
                 file_system_context,
                 url,
                 base::Bind(&BoolCallbackAsFileErrorCallback, callback)));
}

}  // namespace

bool IsUnderNonNativeLocalPath(Profile* profile,
                        const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  GURL url;
  if (!util::ConvertAbsoluteFilePathToFileSystemUrl(
           profile, path, kFileManagerAppId, &url)) {
    return false;
  }

  fileapi::FileSystemURL filesystem_url =
      GetFileSystemContextForExtensionId(profile,
                                         kFileManagerAppId)->CrackURL(url);
  if (!filesystem_url.is_valid())
    return false;

  switch (filesystem_url.type()) {
    case fileapi::kFileSystemTypeNativeLocal:
    case fileapi::kFileSystemTypeRestrictedNativeLocal:
      return false;
    default:
      // The path indeed corresponds to a mount point not associated with a
      // native local path.
      return true;
  }
}

void GetNonNativeLocalPathMimeType(
    Profile* profile,
    const base::FilePath& path,
    const base::Callback<void(bool, const std::string&)>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(IsUnderNonNativeLocalPath(profile, path));

  if (drive::util::IsUnderDriveMountPoint(path)) {
    drive::FileSystemInterface* file_system =
        drive::util::GetFileSystemByProfile(profile);
    if (!file_system) {
      content::BrowserThread::PostTask(
          content::BrowserThread::UI,
          FROM_HERE,
          base::Bind(callback, false, std::string()));
      return;
    }

    file_system->GetResourceEntry(
        drive::util::ExtractDrivePath(path),
        base::Bind(&GetMimeTypeAfterGetResourceEntryForDrive, callback));
    return;
  }

  if (chromeos::file_system_provider::util::IsFileSystemProviderLocalPath(
          path)) {
    chromeos::file_system_provider::util::LocalPathParser parser(profile, path);
    if (!parser.Parse()) {
      content::BrowserThread::PostTask(
          content::BrowserThread::UI,
          FROM_HERE,
          base::Bind(callback, false, std::string()));
      return;
    }

    parser.file_system()->GetMetadata(
        parser.file_path(),
        base::Bind(&GetMimeTypeAfterGetMetadataForProvidedFileSystem,
                   callback));
    return;
  }

  // As a fallback just return success with an empty mime type value.
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(callback, true /* success */, std::string()));
}

void IsNonNativeLocalPathDirectory(
    Profile* profile,
    const base::FilePath& path,
    const base::Callback<void(bool)>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(IsUnderNonNativeLocalPath(profile, path));

  GURL url;
  if (!util::ConvertAbsoluteFilePathToFileSystemUrl(
           profile, path, kFileManagerAppId, &url)) {
    // Posting to the current thread, so that we always call back asynchronously
    // independent from whether or not the operation succeeds.
    content::BrowserThread::PostTask(content::BrowserThread::UI,
                                     FROM_HERE,
                                     base::Bind(callback, false));
    return;
  }

  util::CheckIfDirectoryExists(
      GetFileSystemContextForExtensionId(profile, kFileManagerAppId),
      url,
      base::Bind(&BoolCallbackAsFileErrorCallback, callback));
}

void PrepareNonNativeLocalFileForWritableApp(
    Profile* profile,
    const base::FilePath& path,
    const base::Callback<void(bool)>& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(IsUnderNonNativeLocalPath(profile, path));

  GURL url;
  if (!util::ConvertAbsoluteFilePathToFileSystemUrl(
           profile, path, kFileManagerAppId, &url)) {
    // Posting to the current thread, so that we always call back asynchronously
    // independent from whether or not the operation succeeds.
    content::BrowserThread::PostTask(content::BrowserThread::UI,
                                     FROM_HERE,
                                     base::Bind(callback, false));
    return;
  }

  fileapi::FileSystemContext* const context =
      GetFileSystemContextForExtensionId(profile, kFileManagerAppId);
  DCHECK(context);

  // Check the existence of a file using file system API implementation on
  // behalf of the file manager app. We need to grant access beforehand.
  context->external_backend()->GrantFullAccessToExtension(kFileManagerAppId);

  content::BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(&PrepareFileOnIOThread,
                 make_scoped_refptr(context),
                 context->CrackURL(url),
                 google_apis::CreateRelayCallback(callback)));
}

}  // namespace util
}  // namespace file_manager
