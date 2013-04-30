// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/remote_sync_operation_resolver.h"

#include "base/logging.h"

namespace sync_file_system {

namespace {

bool IsValidCombination(const FileChangeList& local_changes,
                        SyncFileType local_file_type,
                        bool is_conflicting) {
  if (local_changes.empty()) {
    // We never leave directory or non-existing entry in conflicting state, so
    // if there're no local changes it shouldn't be in conflicting state or
    // should be a file.
    return !(is_conflicting && local_file_type != SYNC_FILE_TYPE_FILE);
  }

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
      return local_changes.back().IsDelete();
    case SYNC_FILE_TYPE_FILE:
    case SYNC_FILE_TYPE_DIRECTORY:
      return !local_changes.back().IsDelete() &&
          local_changes.back().file_type() == local_file_type;
  }
  return false;
}

}  // namespace

RemoteSyncOperationType
RemoteSyncOperationResolver::Resolve(const FileChange& remote_file_change,
                                     const FileChangeList& local_changes,
                                     SyncFileType local_file_type,
                                     bool is_conflicting) {
  switch (remote_file_change.change()) {
    case FileChange::FILE_CHANGE_ADD_OR_UPDATE:
      switch (remote_file_change.file_type()) {
        case SYNC_FILE_TYPE_FILE:
          return is_conflicting
              ? ResolveForAddOrUpdateFileInConflict(local_changes,
                                                    local_file_type)
              : ResolveForAddOrUpdateFile(local_changes, local_file_type);
        case SYNC_FILE_TYPE_DIRECTORY:
          return is_conflicting
              ? ResolveForAddDirectoryInConflict(local_changes, local_file_type)
              : ResolveForAddDirectory(local_changes, local_file_type);
        case SYNC_FILE_TYPE_UNKNOWN:
          NOTREACHED();
          return REMOTE_SYNC_OPERATION_FAIL;
      }
      break;
    case FileChange::FILE_CHANGE_DELETE:
      switch (remote_file_change.file_type()) {
        case SYNC_FILE_TYPE_FILE:
          return is_conflicting
              ? ResolveForDeleteFileInConflict(local_changes, local_file_type)
              : ResolveForDeleteFile(local_changes, local_file_type);
        case SYNC_FILE_TYPE_DIRECTORY:
          return is_conflicting
              ? ResolveForDeleteDirectoryInConflict(local_changes,
                                                    local_file_type)
              : ResolveForDeleteDirectory(local_changes, local_file_type);
        case SYNC_FILE_TYPE_UNKNOWN:
          NOTREACHED();
          return REMOTE_SYNC_OPERATION_FAIL;
      }
      break;
  }
  NOTREACHED();
  return REMOTE_SYNC_OPERATION_FAIL;
}

RemoteSyncOperationType
RemoteSyncOperationResolver::ResolveForAddOrUpdateFile(
    const FileChangeList& local_changes,
    SyncFileType local_file_type) {
  // Invalid combination should never happen.
  if (!IsValidCombination(local_changes, local_file_type, false))
    return REMOTE_SYNC_OPERATION_FAIL;

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
      return REMOTE_SYNC_OPERATION_ADD_FILE;
    case SYNC_FILE_TYPE_FILE:
      if (local_changes.empty())
        return REMOTE_SYNC_OPERATION_UPDATE_FILE;
      return REMOTE_SYNC_OPERATION_CONFLICT;
    case SYNC_FILE_TYPE_DIRECTORY:
      // Currently we always prioritize directories over files.
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_LOCAL;
  }
  return REMOTE_SYNC_OPERATION_FAIL;
}

RemoteSyncOperationType
RemoteSyncOperationResolver::ResolveForAddOrUpdateFileInConflict(
    const FileChangeList& local_changes,
    SyncFileType local_file_type) {
  // Invalid combination should never happen.
  if (!IsValidCombination(local_changes, local_file_type, true))
    return REMOTE_SYNC_OPERATION_FAIL;

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_REMOTE;
    case SYNC_FILE_TYPE_FILE:
      return REMOTE_SYNC_OPERATION_CONFLICT;
    case SYNC_FILE_TYPE_DIRECTORY:
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_LOCAL;
  }
  return REMOTE_SYNC_OPERATION_FAIL;
}

RemoteSyncOperationType
RemoteSyncOperationResolver::ResolveForAddDirectory(
    const FileChangeList& local_changes,
    SyncFileType local_file_type) {
  // Invalid combination should never happen.
  if (!IsValidCombination(local_changes, local_file_type, false))
    return REMOTE_SYNC_OPERATION_FAIL;

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
      if (local_changes.empty())
        return REMOTE_SYNC_OPERATION_ADD_DIRECTORY;
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_REMOTE;
    case SYNC_FILE_TYPE_FILE:
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_REMOTE;
    case SYNC_FILE_TYPE_DIRECTORY:
      return REMOTE_SYNC_OPERATION_NONE;
  }
  return REMOTE_SYNC_OPERATION_FAIL;
}

RemoteSyncOperationType
RemoteSyncOperationResolver::ResolveForAddDirectoryInConflict(
    const FileChangeList& local_changes,
    SyncFileType local_file_type) {
  // Invalid combination should never happen.
  if (!IsValidCombination(local_changes, local_file_type, true))
    return REMOTE_SYNC_OPERATION_FAIL;

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
    case SYNC_FILE_TYPE_FILE:
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_REMOTE;
    case SYNC_FILE_TYPE_DIRECTORY:
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_LOCAL;
  }
  return REMOTE_SYNC_OPERATION_FAIL;
}

RemoteSyncOperationType
RemoteSyncOperationResolver::ResolveForDeleteFile(
    const FileChangeList& local_changes,
    SyncFileType local_file_type) {
  // Invalid combination should never happen.
  if (!IsValidCombination(local_changes, local_file_type, false))
    return REMOTE_SYNC_OPERATION_FAIL;

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
      return REMOTE_SYNC_OPERATION_DELETE_METADATA;
    case SYNC_FILE_TYPE_FILE:
      if (local_changes.empty())
        return REMOTE_SYNC_OPERATION_DELETE_FILE;
      return REMOTE_SYNC_OPERATION_NONE;
    case SYNC_FILE_TYPE_DIRECTORY:
      return REMOTE_SYNC_OPERATION_NONE;
  }
  return REMOTE_SYNC_OPERATION_FAIL;
}

RemoteSyncOperationType
RemoteSyncOperationResolver::ResolveForDeleteFileInConflict(
    const FileChangeList& local_changes,
    SyncFileType local_file_type) {
  // Invalid combination should never happen.
  if (!IsValidCombination(local_changes, local_file_type, true))
    return REMOTE_SYNC_OPERATION_FAIL;

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
      return REMOTE_SYNC_OPERATION_DELETE_METADATA;
    case SYNC_FILE_TYPE_FILE:
    case SYNC_FILE_TYPE_DIRECTORY:
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_LOCAL;
  }
  return REMOTE_SYNC_OPERATION_FAIL;
}

RemoteSyncOperationType
RemoteSyncOperationResolver::ResolveForDeleteDirectory(
    const FileChangeList& local_changes,
    SyncFileType local_file_type) {
  // Invalid combination should never happen.
  if (!IsValidCombination(local_changes, local_file_type, false))
    return REMOTE_SYNC_OPERATION_FAIL;

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
      return REMOTE_SYNC_OPERATION_NONE;
    case SYNC_FILE_TYPE_FILE:
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_LOCAL;
    case SYNC_FILE_TYPE_DIRECTORY:
      if (local_changes.empty())
        return REMOTE_SYNC_OPERATION_DELETE_DIRECTORY;
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_LOCAL;
  }
  return REMOTE_SYNC_OPERATION_FAIL;
}

RemoteSyncOperationType
RemoteSyncOperationResolver::ResolveForDeleteDirectoryInConflict(
    const FileChangeList& local_changes,
    SyncFileType local_file_type) {
  // Invalid combination should never happen.
  if (!IsValidCombination(local_changes, local_file_type, true))
    return REMOTE_SYNC_OPERATION_FAIL;

  switch (local_file_type) {
    case SYNC_FILE_TYPE_UNKNOWN:
      return REMOTE_SYNC_OPERATION_DELETE_METADATA;
    case SYNC_FILE_TYPE_FILE:
    case SYNC_FILE_TYPE_DIRECTORY:
      return REMOTE_SYNC_OPERATION_RESOLVE_TO_LOCAL;
  }
  return REMOTE_SYNC_OPERATION_FAIL;
}

}  // namespace sync_file_system
