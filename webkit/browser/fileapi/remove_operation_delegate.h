// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BROWSER_FILEAPI_REMOVE_OPERATION_DELEGATE_H_
#define WEBKIT_BROWSER_FILEAPI_REMOVE_OPERATION_DELEGATE_H_

#include <stack>

#include "webkit/browser/fileapi/recursive_operation_delegate.h"

namespace fileapi {

class RemoveOperationDelegate
    : public RecursiveOperationDelegate,
      public base::SupportsWeakPtr<RemoveOperationDelegate> {
 public:
  RemoveOperationDelegate(FileSystemContext* file_system_context,
                          const FileSystemURL& url,
                          const StatusCallback& callback);
  virtual ~RemoveOperationDelegate();

  // RecursiveOperationDelegate overrides:
  virtual void Run() OVERRIDE;
  virtual void RunRecursively() OVERRIDE;
  virtual void ProcessFile(const FileSystemURL& url,
                           const StatusCallback& callback) OVERRIDE;
  virtual void ProcessDirectory(const FileSystemURL& url,
                                const StatusCallback& callback) OVERRIDE;

  using base::SupportsWeakPtr<RemoveOperationDelegate>::AsWeakPtr;

 private:
  void DidTryRemoveFile(base::PlatformFileError error);
  void DidRemoveFile(const StatusCallback& callback,
                     base::PlatformFileError error);
  void RemoveNextDirectory(base::PlatformFileError error);

  FileSystemURL url_;
  StatusCallback callback_;

  std::stack<FileSystemURL> to_remove_directories_;

  DISALLOW_COPY_AND_ASSIGN(RemoveOperationDelegate);
};

}  // namespace fileapi

#endif  // WEBKIT_BROWSER_FILEAPI_REMOVE_OPERATION_DELEGATE_H_
