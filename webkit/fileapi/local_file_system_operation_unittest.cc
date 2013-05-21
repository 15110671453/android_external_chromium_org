// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/local_file_system_operation.h"

#include "base/bind.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop.h"
#include "base/stringprintf.h"
#include "googleurl/src/gurl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/blob/shareable_file_reference.h"
#include "webkit/browser/fileapi/file_system_mount_point_provider.h"
#include "webkit/browser/fileapi/file_system_quota_util.h"
#include "webkit/fileapi/async_file_test_helper.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_system_file_util.h"
#include "webkit/fileapi/file_system_operation_context.h"
#include "webkit/fileapi/file_system_util.h"
#include "webkit/fileapi/local_file_system_test_helper.h"
#include "webkit/fileapi/mock_file_change_observer.h"
#include "webkit/quota/mock_quota_manager.h"
#include "webkit/quota/quota_manager.h"

using quota::QuotaManager;
using quota::QuotaManagerProxy;
using webkit_blob::ShareableFileReference;

namespace fileapi {

namespace {

const int kFileOperationStatusNotSet = 1;

void AssertFileErrorEq(base::PlatformFileError expected,
                       base::PlatformFileError actual) {
  ASSERT_EQ(expected, actual);
}

}  // namespace (anonymous)

// Test class for LocalFileSystemOperation.
class LocalFileSystemOperationTest
    : public testing::Test,
      public base::SupportsWeakPtr<LocalFileSystemOperationTest> {
 public:
  LocalFileSystemOperationTest()
      : status_(kFileOperationStatusNotSet) {
    EXPECT_TRUE(base_.CreateUniqueTempDir());
    change_observers_ = MockFileChangeObserver::CreateList(&change_observer_);
  }

 protected:
  virtual void SetUp() OVERRIDE {
    base::FilePath base_dir = base_.path().AppendASCII("filesystem");
    quota_manager_ = new quota::MockQuotaManager(
        false /* is_incognito */, base_dir,
        base::MessageLoopProxy::current(),
        base::MessageLoopProxy::current(),
        NULL /* special storage policy */);
    quota_manager_proxy_ = new quota::MockQuotaManagerProxy(
        quota_manager(),
        base::MessageLoopProxy::current());
    test_helper_.SetUp(base_dir, quota_manager_proxy_.get());
  }

  virtual void TearDown() OVERRIDE {
    // Let the client go away before dropping a ref of the quota manager proxy.
    quota_manager_proxy()->SimulateQuotaManagerDestroyed();
    quota_manager_ = NULL;
    quota_manager_proxy_ = NULL;
    test_helper_.TearDown();
  }

  LocalFileSystemOperation* NewOperation() {
    LocalFileSystemOperation* operation = test_helper_.NewOperation();
    operation->operation_context()->set_change_observers(change_observers());
    return operation;
  }

  int status() const { return status_; }
  const base::PlatformFileInfo& info() const { return info_; }
  const base::FilePath& path() const { return path_; }
  const std::vector<DirectoryEntry>& entries() const {
    return entries_;
  }

  const ShareableFileReference* shareable_file_ref() const {
    return shareable_file_ref_;
  }

  quota::MockQuotaManager* quota_manager() {
    return static_cast<quota::MockQuotaManager*>(quota_manager_.get());
  }

  quota::MockQuotaManagerProxy* quota_manager_proxy() {
    return static_cast<quota::MockQuotaManagerProxy*>(
        quota_manager_proxy_.get());
  }

 FileSystemFileUtil* file_util() {
    return test_helper_.file_util();
  }

 const ChangeObserverList& change_observers() const {
    return change_observers_;
  }

  MockFileChangeObserver* change_observer() {
    return &change_observer_;
  }

  scoped_ptr<FileSystemOperationContext> NewContext() {
    FileSystemOperationContext* context = test_helper_.NewOperationContext();
    // Grant enough quota for all test cases.
    context->set_allowed_bytes_growth(1000000);
    return make_scoped_ptr(context);
  }

  FileSystemURL URLForPath(const std::string& path) const {
    return test_helper_.CreateURLFromUTF8(path);
  }

  base::FilePath PlatformPath(const std::string& path) {
    return test_helper_.GetLocalPath(base::FilePath::FromUTF8Unsafe(path));
  }

  bool FileExists(const std::string& path) {
    return AsyncFileTestHelper::FileExists(
        test_helper_.file_system_context(), URLForPath(path),
        AsyncFileTestHelper::kDontCheckSize);
  }

  bool DirectoryExists(const std::string& path) {
    return AsyncFileTestHelper::DirectoryExists(
        test_helper_.file_system_context(), URLForPath(path));
  }

  FileSystemURL CreateFile(const std::string& path) {
    FileSystemURL url = URLForPath(path);
    bool created = false;
    EXPECT_EQ(base::PLATFORM_FILE_OK,
              file_util()->EnsureFileExists(NewContext().get(), url, &created));
    EXPECT_TRUE(created);
    return url;
  }

  FileSystemURL CreateDirectory(const std::string& path) {
    FileSystemURL url = URLForPath(path);
    EXPECT_EQ(base::PLATFORM_FILE_OK,
              file_util()->CreateDirectory(NewContext().get(), url,
                                           false /* exclusive */, true));
    return url;
  }

  int64 GetFileSize(const std::string& path) {
    base::PlatformFileInfo info;
    EXPECT_TRUE(file_util::GetFileInfo(PlatformPath(path), &info));
    return info.size;
  }

  // Callbacks for recording test results.
  FileSystemOperation::StatusCallback RecordStatusCallback() {
    return base::Bind(&LocalFileSystemOperationTest::DidFinish, AsWeakPtr());
  }

  FileSystemOperation::ReadDirectoryCallback
  RecordReadDirectoryCallback() {
    return base::Bind(&LocalFileSystemOperationTest::DidReadDirectory,
                      AsWeakPtr());
  }

  FileSystemOperation::GetMetadataCallback RecordMetadataCallback() {
    return base::Bind(&LocalFileSystemOperationTest::DidGetMetadata,
                      AsWeakPtr());
  }

  FileSystemOperation::SnapshotFileCallback RecordSnapshotFileCallback() {
    return base::Bind(&LocalFileSystemOperationTest::DidCreateSnapshotFile,
                      AsWeakPtr());
  }

  void DidFinish(base::PlatformFileError status) {
    status_ = status;
  }

  void DidReadDirectory(
      base::PlatformFileError status,
      const std::vector<DirectoryEntry>& entries,
      bool /* has_more */) {
    entries_ = entries;
    status_ = status;
  }

  void DidGetMetadata(base::PlatformFileError status,
                      const base::PlatformFileInfo& info,
                      const base::FilePath& platform_path) {
    info_ = info;
    path_ = platform_path;
    status_ = status;
  }

  void DidCreateSnapshotFile(
      base::PlatformFileError status,
      const base::PlatformFileInfo& info,
      const base::FilePath& platform_path,
      const scoped_refptr<ShareableFileReference>& shareable_file_ref) {
    info_ = info;
    path_ = platform_path;
    status_ = status;
    shareable_file_ref_ = shareable_file_ref;
  }

  void GetUsageAndQuota(int64* usage, int64* quota) {
    quota::QuotaStatusCode status =
        AsyncFileTestHelper::GetUsageAndQuota(
            quota_manager_, test_helper_.origin(), test_helper_.type(),
            usage, quota);
    base::MessageLoop::current()->RunUntilIdle();
    ASSERT_EQ(quota::kQuotaStatusOk, status);
  }

  int64 ComputePathCost(const FileSystemURL& url) {
    int64 base_usage;
    GetUsageAndQuota(&base_usage, NULL);

    AsyncFileTestHelper::CreateFile(test_helper_.file_system_context(), url);
    NewOperation()->Remove(url, false /* recursive */,
                           base::Bind(&AssertFileErrorEq,
                                      base::PLATFORM_FILE_OK));
    base::MessageLoop::current()->RunUntilIdle();
    change_observer()->ResetCount();

    int64 total_usage;
    GetUsageAndQuota(&total_usage, NULL);
    return total_usage - base_usage;
  }

  void GrantQuotaForCurrentUsage() {
    int64 usage;
    GetUsageAndQuota(&usage, NULL);
    quota_manager()->SetQuota(test_helper_.origin(),
                              test_helper_.storage_type(),
                              usage);
  }

  void AddQuota(int64 quota_delta) {
    int64 quota;
    GetUsageAndQuota(NULL, &quota);
    quota_manager()->SetQuota(test_helper_.origin(),
                              test_helper_.storage_type(),
                              quota + quota_delta);
  }

  base::MessageLoop message_loop_;
  scoped_refptr<QuotaManager> quota_manager_;
  scoped_refptr<QuotaManagerProxy> quota_manager_proxy_;

  // Common temp base for nondestructive uses.
  base::ScopedTempDir base_;

  LocalFileSystemTestOriginHelper test_helper_;

  // For post-operation status.
  int status_;
  base::PlatformFileInfo info_;
  base::FilePath path_;
  std::vector<DirectoryEntry> entries_;
  scoped_refptr<ShareableFileReference> shareable_file_ref_;

  MockFileChangeObserver change_observer_;
  ChangeObserverList change_observers_;

  DISALLOW_COPY_AND_ASSIGN(LocalFileSystemOperationTest);
};

TEST_F(LocalFileSystemOperationTest, TestMoveFailureSrcDoesntExist) {
  change_observer()->ResetCount();
  NewOperation()->Move(URLForPath("a"), URLForPath("b"),
                       RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestMoveFailureContainsPath) {
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("src/dest"));

  NewOperation()->Move(src_dir, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_INVALID_OPERATION, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestMoveFailureSrcDirExistsDestFile) {
  // Src exists and is dir. Dest is a file.
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("dest"));
  FileSystemURL dest_file(CreateFile("dest/file"));

  NewOperation()->Move(src_dir, dest_file, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_INVALID_OPERATION, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest,
       TestMoveFailureSrcFileExistsDestNonEmptyDir) {
  // Src exists and is a directory. Dest is a non-empty directory.
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("dest"));
  FileSystemURL dest_file(CreateFile("dest/file"));

  NewOperation()->Move(src_dir, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_EMPTY, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestMoveFailureSrcFileExistsDestDir) {
  // Src exists and is a file. Dest is a directory.
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL src_file(CreateFile("src/file"));
  FileSystemURL dest_dir(CreateDirectory("dest"));

  NewOperation()->Move(src_file, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_INVALID_OPERATION, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestMoveFailureDestParentDoesntExist) {
  // Dest. parent path does not exist.
  FileSystemURL src_dir(CreateDirectory("src"));
  NewOperation()->Move(src_dir, URLForPath("nonexistent/deset"),
                       RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestMoveSuccessSrcFileAndOverwrite) {
  FileSystemURL src_file(CreateFile("src"));
  FileSystemURL dest_file(CreateFile("dest"));

  NewOperation()->Move(src_file, dest_file, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(FileExists("dest"));

  EXPECT_EQ(1, change_observer()->get_and_reset_modify_file_count());
  EXPECT_EQ(1, change_observer()->get_and_reset_remove_file_count());
  EXPECT_TRUE(change_observer()->HasNoChange());

  EXPECT_EQ(1, quota_manager_proxy()->notify_storage_accessed_count());
}

TEST_F(LocalFileSystemOperationTest, TestMoveSuccessSrcFileAndNew) {
  FileSystemURL src_file(CreateFile("src"));

  NewOperation()->Move(src_file, URLForPath("new"), RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(FileExists("new"));

  EXPECT_EQ(1, change_observer()->get_and_reset_create_file_from_count());
  EXPECT_EQ(1, change_observer()->get_and_reset_remove_file_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestMoveSuccessSrcDirAndOverwrite) {
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("dest"));

  NewOperation()->Move(src_dir, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_FALSE(DirectoryExists("src"));

  EXPECT_EQ(1, change_observer()->get_and_reset_create_directory_count());
  EXPECT_EQ(2, change_observer()->get_and_reset_remove_directory_count());
  EXPECT_TRUE(change_observer()->HasNoChange());

  // Make sure we've overwritten but not moved the source under the |dest_dir|.
  EXPECT_TRUE(DirectoryExists("dest"));
  EXPECT_FALSE(DirectoryExists("dest/src"));

}

TEST_F(LocalFileSystemOperationTest, TestMoveSuccessSrcDirAndNew) {
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("dest"));

  NewOperation()->Move(src_dir, URLForPath("dest/new"), RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_FALSE(DirectoryExists("src"));
  EXPECT_TRUE(DirectoryExists("dest/new"));

  EXPECT_EQ(1, change_observer()->get_and_reset_remove_directory_count());
  EXPECT_EQ(1, change_observer()->get_and_reset_create_directory_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestMoveSuccessSrcDirRecursive) {
  FileSystemURL src_dir(CreateDirectory("src"));
  CreateDirectory("src/dir");
  CreateFile("src/dir/sub");

  FileSystemURL dest_dir(CreateDirectory("dest"));

  NewOperation()->Move(src_dir, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(DirectoryExists("dest/dir"));
  EXPECT_TRUE(FileExists("dest/dir/sub"));

  EXPECT_EQ(3, change_observer()->get_and_reset_remove_directory_count());
  EXPECT_EQ(2, change_observer()->get_and_reset_create_directory_count());
  EXPECT_EQ(1, change_observer()->get_and_reset_remove_file_count());
  EXPECT_EQ(1, change_observer()->get_and_reset_create_file_from_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopyFailureSrcDoesntExist) {
  NewOperation()->Copy(URLForPath("a"), URLForPath("b"),
                       RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopyFailureContainsPath) {
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("src/dir"));

  NewOperation()->Copy(src_dir, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_INVALID_OPERATION, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopyFailureSrcDirExistsDestFile) {
  // Src exists and is dir. Dest is a file.
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("dest"));
  FileSystemURL dest_file(CreateFile("dest/file"));

  NewOperation()->Copy(src_dir, dest_file, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_INVALID_OPERATION, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest,
       TestCopyFailureSrcFileExistsDestNonEmptyDir) {
  // Src exists and is a directory. Dest is a non-empty directory.
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("dest"));
  FileSystemURL dest_file(CreateFile("dest/file"));

  NewOperation()->Copy(src_dir, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_EMPTY, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopyFailureSrcFileExistsDestDir) {
  // Src exists and is a file. Dest is a directory.
  FileSystemURL src_file(CreateFile("src"));
  FileSystemURL dest_dir(CreateDirectory("dest"));

  NewOperation()->Copy(src_file, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_INVALID_OPERATION, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopyFailureDestParentDoesntExist) {
  // Dest. parent path does not exist.
  FileSystemURL src_dir(CreateDirectory("src"));

  NewOperation()->Copy(src_dir, URLForPath("nonexistent/dest"),
                       RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopyFailureByQuota) {
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL src_file(CreateFile("src/file"));
  FileSystemURL dest_dir(CreateDirectory("dest"));
  NewOperation()->Truncate(src_file, 6, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_EQ(6, GetFileSize("src/file"));

  FileSystemURL dest_file(URLForPath("dest/file"));
  int64 dest_path_cost = ComputePathCost(dest_file);
  GrantQuotaForCurrentUsage();
  AddQuota(6 + dest_path_cost - 1);

  NewOperation()->Copy(src_file, dest_file, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NO_SPACE, status());
  EXPECT_FALSE(FileExists("dest/file"));
}

TEST_F(LocalFileSystemOperationTest, TestCopySuccessSrcFileAndOverwrite) {
  FileSystemURL src_file(CreateFile("src"));
  FileSystemURL dest_file(CreateFile("dest"));

  NewOperation()->Copy(src_file, dest_file, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(FileExists("dest"));
  EXPECT_EQ(2, quota_manager_proxy()->notify_storage_accessed_count());

  EXPECT_EQ(1, change_observer()->get_and_reset_modify_file_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopySuccessSrcFileAndNew) {
  FileSystemURL src_file(CreateFile("src"));

  NewOperation()->Copy(src_file, URLForPath("new"),
                       RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(FileExists("new"));
  EXPECT_EQ(2, quota_manager_proxy()->notify_storage_accessed_count());

  EXPECT_EQ(1, change_observer()->get_and_reset_create_file_from_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopySuccessSrcDirAndOverwrite) {
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir(CreateDirectory("dest"));

  NewOperation()->Copy(src_dir, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());

  // Make sure we've overwritten but not copied the source under the |dest_dir|.
  EXPECT_TRUE(DirectoryExists("dest"));
  EXPECT_FALSE(DirectoryExists("dest/src"));
  EXPECT_EQ(3, quota_manager_proxy()->notify_storage_accessed_count());

  EXPECT_EQ(1, change_observer()->get_and_reset_remove_directory_count());
  EXPECT_EQ(1, change_observer()->get_and_reset_create_directory_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopySuccessSrcDirAndNew) {
  FileSystemURL src_dir(CreateDirectory("src"));
  FileSystemURL dest_dir_new(URLForPath("dest"));

  NewOperation()->Copy(src_dir, dest_dir_new, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(DirectoryExists("dest"));
  EXPECT_EQ(2, quota_manager_proxy()->notify_storage_accessed_count());

  EXPECT_EQ(1, change_observer()->get_and_reset_create_directory_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopySuccessSrcDirRecursive) {
  FileSystemURL src_dir(CreateDirectory("src"));
  CreateDirectory("src/dir");
  CreateFile("src/dir/sub");

  FileSystemURL dest_dir(CreateDirectory("dest"));

  NewOperation()->Copy(src_dir, dest_dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();

  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(DirectoryExists("dest/dir"));
  EXPECT_TRUE(FileExists("dest/dir/sub"));

  // For recursive copy we may record multiple read access.
  EXPECT_GE(quota_manager_proxy()->notify_storage_accessed_count(), 1);

  EXPECT_EQ(2, change_observer()->get_and_reset_create_directory_count());
  EXPECT_EQ(1, change_observer()->get_and_reset_remove_directory_count());
  EXPECT_EQ(1, change_observer()->get_and_reset_create_file_from_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCopyInForeignFileSuccess) {
  base::FilePath src_local_disk_file_path;
  file_util::CreateTemporaryFile(&src_local_disk_file_path);
  const char test_data[] = "foo";
  int data_size = ARRAYSIZE_UNSAFE(test_data);
  file_util::WriteFile(src_local_disk_file_path, test_data, data_size);

  FileSystemURL dest_dir(CreateDirectory("dest"));

  int64 before_usage;
  GetUsageAndQuota(&before_usage, NULL);

  // Check that the file copied and corresponding usage increased.
  NewOperation()->CopyInForeignFile(src_local_disk_file_path,
                                    URLForPath("dest/file"),
                                    RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();

  EXPECT_EQ(1, change_observer()->create_file_count());
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(FileExists("dest/file"));
  int64 after_usage;
  GetUsageAndQuota(&after_usage, NULL);
  EXPECT_GT(after_usage, before_usage);

  // Compare contents of src and copied file.
  char buffer[100];
  EXPECT_EQ(data_size, file_util::ReadFile(PlatformPath("dest/file"),
                                           buffer, data_size));
  for (int i = 0; i < data_size; ++i)
    EXPECT_EQ(test_data[i], buffer[i]);
}

TEST_F(LocalFileSystemOperationTest, TestCopyInForeignFileFailureByQuota) {
  base::FilePath src_local_disk_file_path;
  file_util::CreateTemporaryFile(&src_local_disk_file_path);
  const char test_data[] = "foo";
  file_util::WriteFile(src_local_disk_file_path, test_data,
                       ARRAYSIZE_UNSAFE(test_data));

  FileSystemURL dest_dir(CreateDirectory("dest"));

  GrantQuotaForCurrentUsage();
  NewOperation()->CopyInForeignFile(src_local_disk_file_path,
                                    URLForPath("dest/file"),
                                    RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();

  EXPECT_FALSE(FileExists("dest/file"));
  EXPECT_EQ(0, change_observer()->create_file_count());
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NO_SPACE, status());
}

TEST_F(LocalFileSystemOperationTest, TestCreateFileFailure) {
  // Already existing file and exclusive true.
  FileSystemURL file(CreateFile("file"));
  NewOperation()->CreateFile(file, true, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_EXISTS, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCreateFileSuccessFileExists) {
  // Already existing file and exclusive false.
  FileSystemURL file(CreateFile("file"));
  NewOperation()->CreateFile(file, false, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(FileExists("file"));

  // The file was already there; did nothing.
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCreateFileSuccessExclusive) {
  // File doesn't exist but exclusive is true.
  NewOperation()->CreateFile(URLForPath("new"), true, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(FileExists("new"));
  EXPECT_EQ(1, change_observer()->get_and_reset_create_file_count());
}

TEST_F(LocalFileSystemOperationTest, TestCreateFileSuccessFileDoesntExist) {
  // Non existing file.
  NewOperation()->CreateFile(URLForPath("nonexistent"), false,
                             RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_EQ(1, change_observer()->get_and_reset_create_file_count());
}

TEST_F(LocalFileSystemOperationTest,
       TestCreateDirFailureDestParentDoesntExist) {
  // Dest. parent path does not exist.
  NewOperation()->CreateDirectory(
      URLForPath("nonexistent/dir"), false, false,
      RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCreateDirFailureDirExists) {
  // Exclusive and dir existing at path.
  FileSystemURL dir(CreateDirectory("dir"));
  NewOperation()->CreateDirectory(dir, true, false,
                                  RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_EXISTS, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCreateDirFailureFileExists) {
  // Exclusive true and file existing at path.
  FileSystemURL file(CreateFile("file"));
  NewOperation()->CreateDirectory(file, true, false,
                                  RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_EXISTS, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestCreateDirSuccess) {
  // Dir exists and exclusive is false.
  FileSystemURL dir(CreateDirectory("dir"));
  NewOperation()->CreateDirectory(dir, false, false,
                                  RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(change_observer()->HasNoChange());

  // Dir doesn't exist.
  NewOperation()->CreateDirectory(URLForPath("new"), false, false,
                                  RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(DirectoryExists("new"));
  EXPECT_EQ(1, change_observer()->get_and_reset_create_directory_count());
}

TEST_F(LocalFileSystemOperationTest, TestCreateDirSuccessExclusive) {
  // Dir doesn't exist.
  NewOperation()->CreateDirectory(URLForPath("new"), true, false,
                                  RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(DirectoryExists("new"));
  EXPECT_EQ(1, change_observer()->get_and_reset_create_directory_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestExistsAndMetadataFailure) {
  NewOperation()->GetMetadata(URLForPath("nonexistent"),
                              RecordMetadataCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());

  NewOperation()->FileExists(URLForPath("nonexistent"),
                             RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());

  NewOperation()->DirectoryExists(URLForPath("nonexistent"),
                                  RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestExistsAndMetadataSuccess) {
  FileSystemURL dir(CreateDirectory("dir"));
  FileSystemURL file(CreateFile("dir/file"));
  int read_access = 0;

  NewOperation()->DirectoryExists(dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  ++read_access;

  NewOperation()->GetMetadata(dir, RecordMetadataCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(info().is_directory);
  EXPECT_EQ(base::FilePath(), path());
  ++read_access;

  NewOperation()->FileExists(file, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  ++read_access;

  NewOperation()->GetMetadata(file, RecordMetadataCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_FALSE(info().is_directory);
  EXPECT_EQ(PlatformPath("dir/file"), path());
  ++read_access;

  EXPECT_EQ(read_access,
            quota_manager_proxy()->notify_storage_accessed_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestTypeMismatchErrors) {
  FileSystemURL dir(CreateDirectory("dir"));
  NewOperation()->FileExists(dir, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_A_FILE, status());

  FileSystemURL file(CreateFile("file"));
  NewOperation()->DirectoryExists(file, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY, status());
}

TEST_F(LocalFileSystemOperationTest, TestReadDirFailure) {
  // Path doesn't exist
  NewOperation()->ReadDirectory(URLForPath("nonexistent"),
                                RecordReadDirectoryCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());

  // File exists.
  FileSystemURL file(CreateFile("file"));
  NewOperation()->ReadDirectory(file, RecordReadDirectoryCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_A_DIRECTORY, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestReadDirSuccess) {
  //      parent_dir
  //       |       |
  //  child_dir  child_file
  // Verify reading parent_dir.
  FileSystemURL parent_dir(CreateDirectory("dir"));
  FileSystemURL child_dir(CreateDirectory("dir/child_dir"));
  FileSystemURL child_file(CreateFile("dir/child_file"));

  NewOperation()->ReadDirectory(parent_dir, RecordReadDirectoryCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_EQ(2u, entries().size());

  for (size_t i = 0; i < entries().size(); ++i) {
    if (entries()[i].is_directory)
      EXPECT_EQ(FILE_PATH_LITERAL("child_dir"), entries()[i].name);
    else
      EXPECT_EQ(FILE_PATH_LITERAL("child_file"), entries()[i].name);
  }
  EXPECT_EQ(1, quota_manager_proxy()->notify_storage_accessed_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestRemoveFailure) {
  // Path doesn't exist.
  NewOperation()->Remove(URLForPath("nonexistent"), false /* recursive */,
                         RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_FOUND, status());

  // It's an error to try to remove a non-empty directory if recursive flag
  // is false.
  //      parent_dir
  //       |       |
  //  child_dir  child_file
  // Verify deleting parent_dir.
  FileSystemURL parent_dir(CreateDirectory("dir"));
  FileSystemURL child_dir(CreateDirectory("dir/child_dir"));
  FileSystemURL child_file(CreateFile("dir/child_file"));

  NewOperation()->Remove(parent_dir, false /* recursive */,
                         RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NOT_EMPTY, status());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestRemoveSuccess) {
  FileSystemURL empty_dir(CreateDirectory("empty_dir"));
  EXPECT_TRUE(DirectoryExists("empty_dir"));
  NewOperation()->Remove(empty_dir, false /* recursive */,
                         RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_FALSE(DirectoryExists("empty_dir"));

  EXPECT_EQ(1, change_observer()->get_and_reset_remove_directory_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestRemoveSuccessRecursive) {
  // Removing a non-empty directory with recursive flag == true should be ok.
  //      parent_dir
  //       |       |
  //  child_dir  child_files
  //       |
  //  child_files
  //
  // Verify deleting parent_dir.
  FileSystemURL parent_dir(CreateDirectory("dir"));
  for (int i = 0; i < 8; ++i)
    CreateFile(base::StringPrintf("dir/file-%d", i));
  FileSystemURL child_dir(CreateDirectory("dir/child_dir"));
  for (int i = 0; i < 8; ++i)
    CreateFile(base::StringPrintf("dir/child_dir/file-%d", i));

  NewOperation()->Remove(parent_dir, true /* recursive */,
                         RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_FALSE(DirectoryExists("parent_dir"));

  EXPECT_EQ(2, change_observer()->get_and_reset_remove_directory_count());
  EXPECT_EQ(16, change_observer()->get_and_reset_remove_file_count());
  EXPECT_TRUE(change_observer()->HasNoChange());
}

TEST_F(LocalFileSystemOperationTest, TestTruncate) {
  FileSystemURL file(CreateFile("file"));
  base::FilePath platform_path = PlatformPath("file");

  char test_data[] = "test data";
  int data_size = static_cast<int>(sizeof(test_data));
  EXPECT_EQ(data_size,
            file_util::WriteFile(platform_path, test_data, data_size));

  // Check that its length is the size of the data written.
  NewOperation()->GetMetadata(file, RecordMetadataCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_FALSE(info().is_directory);
  EXPECT_EQ(data_size, info().size);

  // Extend the file by truncating it.
  int length = 17;
  NewOperation()->Truncate(file, length, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());

  EXPECT_EQ(1, change_observer()->get_and_reset_modify_file_count());
  EXPECT_TRUE(change_observer()->HasNoChange());

  // Check that its length is now 17 and that it's all zeroes after the test
  // data.
  EXPECT_EQ(length, GetFileSize("file"));
  char data[100];
  EXPECT_EQ(length, file_util::ReadFile(platform_path, data, length));
  for (int i = 0; i < length; ++i) {
    if (i < static_cast<int>(sizeof(test_data)))
      EXPECT_EQ(test_data[i], data[i]);
    else
      EXPECT_EQ(0, data[i]);
  }

  // Shorten the file by truncating it.
  length = 3;
  NewOperation()->Truncate(file, length, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());

  EXPECT_EQ(1, change_observer()->get_and_reset_modify_file_count());
  EXPECT_TRUE(change_observer()->HasNoChange());

  // Check that its length is now 3 and that it contains only bits of test data.
  EXPECT_EQ(length, GetFileSize("file"));
  EXPECT_EQ(length, file_util::ReadFile(platform_path, data, length));
  for (int i = 0; i < length; ++i)
    EXPECT_EQ(test_data[i], data[i]);

  // Truncate is not a 'read' access.  (Here expected access count is 1
  // since we made 1 read access for GetMetadata.)
  EXPECT_EQ(1, quota_manager_proxy()->notify_storage_accessed_count());
}

TEST_F(LocalFileSystemOperationTest, TestTruncateFailureByQuota) {
  FileSystemURL dir(CreateDirectory("dir"));
  FileSystemURL file(CreateFile("dir/file"));

  GrantQuotaForCurrentUsage();
  AddQuota(10);

  NewOperation()->Truncate(file, 10, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_EQ(1, change_observer()->get_and_reset_modify_file_count());
  EXPECT_TRUE(change_observer()->HasNoChange());

  EXPECT_EQ(10, GetFileSize("dir/file"));

  NewOperation()->Truncate(file, 11, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_ERROR_NO_SPACE, status());
  EXPECT_TRUE(change_observer()->HasNoChange());

  EXPECT_EQ(10, GetFileSize("dir/file"));
}

TEST_F(LocalFileSystemOperationTest, TestTouchFile) {
  FileSystemURL file(CreateFile("file"));
  base::FilePath platform_path = PlatformPath("file");

  base::PlatformFileInfo info;
  EXPECT_TRUE(file_util::GetFileInfo(platform_path, &info));
  EXPECT_FALSE(info.is_directory);
  EXPECT_EQ(0, info.size);
  const base::Time last_modified = info.last_modified;
  const base::Time last_accessed = info.last_accessed;

  const base::Time new_modified_time = base::Time::UnixEpoch();
  const base::Time new_accessed_time = new_modified_time +
      base::TimeDelta::FromHours(77);
  ASSERT_NE(last_modified, new_modified_time);
  ASSERT_NE(last_accessed, new_accessed_time);

  NewOperation()->TouchFile(file, new_accessed_time, new_modified_time,
                            RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_TRUE(change_observer()->HasNoChange());

  EXPECT_TRUE(file_util::GetFileInfo(platform_path, &info));
  // We compare as time_t here to lower our resolution, to avoid false
  // negatives caused by conversion to the local filesystem's native
  // representation and back.
  EXPECT_EQ(new_modified_time.ToTimeT(), info.last_modified.ToTimeT());
  EXPECT_EQ(new_accessed_time.ToTimeT(), info.last_accessed.ToTimeT());
}

TEST_F(LocalFileSystemOperationTest, TestCreateSnapshotFile) {
  FileSystemURL dir(CreateDirectory("dir"));

  // Create a file for the testing.
  NewOperation()->DirectoryExists(dir, RecordStatusCallback());
  FileSystemURL file(CreateFile("dir/file"));
  NewOperation()->FileExists(file, RecordStatusCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());

  // See if we can get a 'snapshot' file info for the file.
  // Since LocalFileSystemOperation assumes the file exists in the local
  // directory it should just returns the same metadata and platform_path
  // as the file itself.
  NewOperation()->CreateSnapshotFile(file, RecordSnapshotFileCallback());
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(base::PLATFORM_FILE_OK, status());
  EXPECT_FALSE(info().is_directory);
  EXPECT_EQ(PlatformPath("dir/file"), path());
  EXPECT_TRUE(change_observer()->HasNoChange());

  // The FileSystemOpration implementation does not create a
  // shareable file reference.
  EXPECT_EQ(NULL, shareable_file_ref());
}

}  // namespace fileapi
