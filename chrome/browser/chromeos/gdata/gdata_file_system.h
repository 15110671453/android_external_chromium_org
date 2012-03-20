// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_GDATA_GDATA_FILE_SYSTEM_H_
#define CHROME_BROWSER_CHROMEOS_GDATA_GDATA_FILE_SYSTEM_H_

#include <sys/stat.h>

#include <map>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop.h"
#include "base/platform_file.h"
#include "base/synchronization/lock.h"
#include "chrome/browser/chromeos/gdata/gdata_files.h"
#include "chrome/browser/chromeos/gdata/gdata_operation_registry.h"
#include "chrome/browser/chromeos/gdata/gdata_params.h"
#include "chrome/browser/chromeos/gdata/gdata_parser.h"
#include "chrome/browser/chromeos/gdata/gdata_uploader.h"
#include "chrome/browser/profiles/profile_keyed_service.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace base {
class WaitableEvent;
}

namespace gdata {

class DocumentsServiceInterface;
class GDataDownloadObserver;
class GDataSyncClientInterface;

// Callback for completion of cache operation.
typedef base::Callback<void(base::PlatformFileError error,
                            const std::string& resource_id,
                            const std::string& md5)> CacheOperationCallback;

// Callback for GetFromCache.
typedef base::Callback<void(base::PlatformFileError error,
                            const std::string& resource_id,
                            const std::string& md5,
                            const FilePath& gdata_file_path,
                            const FilePath& cache_file_path)>
    GetFromCacheCallback;

// Used to get result of file search. Please note that |file| is a live
// object provided to this callback under lock. It must not be used outside
// of the callback method. This callback can be invoked on different thread
// than one that started FileFileByPath() request.
typedef base::Callback<void(base::PlatformFileError error,
                            const FilePath& directory_path,
                            GDataFileBase* file)>
    FindFileCallback;

// Used for file operations like removing files.
typedef base::Callback<void(base::PlatformFileError error)>
    FileOperationCallback;

// Used to get files from the file system.
typedef base::Callback<void(base::PlatformFileError error,
                            const FilePath& file_path,
                            GDataFileType file_type)>
    GetFileCallback;

// Used for file operations like removing files.
typedef base::Callback<void(base::PlatformFileError error,
                            scoped_ptr<base::Value> value)>
    GetJsonDocumentCallback;

// Used to get available space for the account from GData.
typedef base::Callback<void(base::PlatformFileError error,
                            int bytes_total,
                            int bytes_used)>
    GetAvailableSpaceCallback;


// Delegate class used to deal with results synchronous read-only search
// over virtual file system.
class FindFileDelegate {
 public:
  virtual ~FindFileDelegate();

  // Called when FindFileByPathSync() completes search.
  virtual void OnDone(base::PlatformFileError error,
                      const FilePath& directory_path,
                      GDataFileBase* file) = 0;
};

// Delegate used to find a directory element for file system updates.
class ReadOnlyFindFileDelegate : public FindFileDelegate {
 public:
  ReadOnlyFindFileDelegate();

  // Returns found file.
  GDataFileBase* file() { return file_; }

 private:
  // FindFileDelegate overrides.
  virtual void OnDone(base::PlatformFileError error,
                      const FilePath& directory_path,
                      GDataFileBase* file) OVERRIDE;

  // File entry that was found.
  GDataFileBase* file_;
};

// Helper structure used for extracting key properties from GDataFile object.
struct GDataFileProperties {
  GDataFileProperties();
  ~GDataFileProperties();

  base::PlatformFileInfo file_info;
  std::string resource_id;
  std::string file_md5;
  GURL content_url;
  GURL edit_url;
  bool is_hosted_document;
};

// GData file system abstraction layer.
// The interface is defined to make GDataFileSystem mockable.
class GDataFileSystemInterface {
 public:
  virtual ~GDataFileSystemInterface() {}

  // Used to notify events on the file system.
  class Observer {
   public:
    // Trigerred when a file has been pinned, after the cache state is
    // updated.
    virtual void OnFilePinned(const std::string& resource_id,
                              const std::string& md5) {}

   protected:
    virtual ~Observer() {}
  };

  // Initializes the object. This function should be called before any
  // other functions.
  virtual void Initialize() = 0;

  // Adds and removes the observer.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Enum defining GCache subdirectory location.
  enum CacheSubdir {  // This indexes into |cache_paths_| vector.
    CACHE_TYPE_META = 0,
    CACHE_TYPE_PINNED,
    CACHE_TYPE_OUTGOING,
    CACHE_TYPE_PERSISTENT,
    CACHE_TYPE_TMP,
  };

  // Enum defining origin of a cached file.
  enum CachedFileOrigin {
    CACHED_FILE_FROM_SERVER = 0,
    CACHED_FILE_LOCALLY_MODIFIED,
  };

  // Authenticates the user by fetching the auth token as
  // needed. |callback| will be run with the error code and the auth
  // token, on the thread this function is run.
  //
  // Must be called on UI thread.
  virtual void Authenticate(const AuthStatusCallback& callback) = 0;

  // Finds file info by using virtual |file_path|. This call will also
  // retrieve and refresh file system content from server and disk cache.
  //
  // Can be called from any thread.
  virtual void FindFileByPathAsync(const FilePath& file_path,
                                   const FindFileCallback& callback) = 0;

  // Finds file info by using virtual |file_path|. This call does not initiate
  // content refreshing and will invoke one of |delegate| methods directly as
  // it executes.
  //
  // Can be called from any thread.
  virtual void FindFileByPathSync(const FilePath& file_path,
                                  FindFileDelegate* delegate) = 0;

  // Copies |src_file_path| to |dest_file_path| on the file system.
  // |src_file_path| can be a hosted document (see limitations below).
  // |dest_file_path| is expected to be of the same type of |src_file_path|
  // (i.e. if |src_file_path| is a file, |dest_file_path| will be created as
  // a file).
  //
  // This method also has the following assumptions/limitations that may be
  // relaxed or addressed later:
  // - |src_file_path| cannot be a regular file (i.e. non-hosted document)
  //   or a directory.
  // - |dest_file_path| must not exist.
  // - The parent of |dest_file_path| must already exist.
  //
  // The file entries represented by |src_file_path| and the parent directory
  // of |dest_file_path| need to be present in the in-memory representation
  // of the file system.
  //
  // Can be called from any thread.
  virtual void Copy(const FilePath& src_file_path,
                    const FilePath& dest_file_path,
                    const FileOperationCallback& callback) = 0;

  // Moves |src_file_path| to |dest_file_path| on the file system.
  // |src_file_path| can be a file (regular or hosted document) or a directory.
  // |dest_file_path| is expected to be of the same type of |src_file_path|
  // (i.e. if |src_file_path| is a file, |dest_file_path| will be created as
  // a file).
  //
  // This method also has the following assumptions/limitations that may be
  // relaxed or addressed later:
  // - |dest_file_path| must not exist.
  // - The parent of |dest_file_path| must already exist.
  //
  // The file entries represented by |src_file_path| and the parent directory
  // of |dest_file_path| need to be present in the in-memory representation
  // of the file system.
  //
  // Can be called from any thread.
  virtual void Move(const FilePath& src_file_path,
                    const FilePath& dest_file_path,
                    const FileOperationCallback& callback) = 0;

  // Removes |file_path| from the file system.  If |is_recursive| is set and
  // |file_path| represents a directory, we will also delete all of its
  // contained children elements. The file entry represented by |file_path|
  // needs to be present in in-memory representation of the file system that
  // in order to be removed.
  //
  // TODO(zelidrag): Wire |is_recursive| through gdata api
  // (find appropriate calls for it).
  //
  // Can be called from any thread. |callback| is run on the calling thread.
  virtual void Remove(const FilePath& file_path,
                      bool is_recursive,
                      const FileOperationCallback& callback) = 0;

  // Creates new directory under |directory_path|. If |is_exclusive| is true,
  // an error is raised in case a directory is already present at the
  // |directory_path|. If |is_recursive| is true, the call creates parent
  // directories as needed just like mkdir -p does.
  //
  // Can be called from any thread. |callback| is run on the calling thread.
  virtual void CreateDirectory(const FilePath& directory_path,
                               bool is_exclusive,
                               bool is_recursive,
                               const FileOperationCallback& callback) = 0;

  // Gets |file_path| from the file system. The file entry represented by
  // |file_path| needs to be present in in-memory representation of the file
  // system in order to be retrieved. If the file is not cached, the file
  // will be downloaded through gdata api.
  //
  // Can be called from any thread. |callback| is run on the calling thread.
  virtual void GetFile(const FilePath& file_path,
                       const GetFileCallback& callback) = 0;

  // Gets absolute path of cache file corresponding to |gdata_file_path|.
  // Upon completion, |callback| is invoked on the same thread where this method
  // was called, with path if it exists and is accessible or empty FilePath
  // otherwise.
  virtual void GetFromCacheForPath(const FilePath& gdata_file_path,
                                   const GetFromCacheCallback& callback) = 0;

  // Obtains the list of currently active operations.
  virtual std::vector<GDataOperationRegistry::ProgressStatus>
  GetProgressStatusList() = 0;

  // Cancels ongoing operation for a given |file_path|. Returns true if
  // the operation was found and canceled.
  virtual bool CancelOperation(const FilePath& file_path) = 0;

  // Add operation observer.
  virtual void AddOperationObserver(
      GDataOperationRegistry::Observer* observer) = 0;

  // Remove operation observer.
  virtual void RemoveOperationObserver(
      GDataOperationRegistry::Observer* observer) = 0;

  // Gets the cache state of file corresponding to |resource_id| and |md5| if it
  // exists on disk.
  // Initializes cache if it has not been initialized.
  // Upon completion, |callback| is invoked on the thread where this method was
  // called with the cache state if file exists in cache or CACHE_STATE_NONE
  // otherwise.
  virtual void GetCacheState(const std::string& resource_id,
                             const std::string& md5,
                             const GetCacheStateCallback& callback) = 0;

  // Finds file object by |file_path| and returns its key |properties|.
  // Returns true if file was found.
  virtual bool GetFileInfoFromPath(const FilePath& gdata_file_path,
                                   GDataFileProperties* properties) = 0;

  // Returns the tmp sub-directory under gdata cache directory, i.e.
  // <user_profile_dir>/GCache/v1/tmp
  virtual FilePath GetGDataCacheTmpDirectory() = 0;

  // Fetches the user's Account Metadata to find out current quota information
  // and returns it to the callback.
  virtual void GetAvailableSpace(const GetAvailableSpaceCallback& callback) = 0;
};

// The production implementation of GDataFileSystemInterface.
class GDataFileSystem : public GDataFileSystemInterface,
                        public ProfileKeyedService {
 public:
  // ProfileKeyedService override:
  virtual void Shutdown() OVERRIDE;

  // GDataFileSystem overrides.
  virtual void Initialize() OVERRIDE;
  virtual void AddObserver(Observer* observer) OVERRIDE;
  virtual void RemoveObserver(Observer* observer) OVERRIDE;
  virtual void Authenticate(const AuthStatusCallback& callback) OVERRIDE;
  virtual void FindFileByPathAsync(const FilePath& file_path,
                                   const FindFileCallback& callback) OVERRIDE;
  virtual void FindFileByPathSync(const FilePath& file_path,
                                  FindFileDelegate* delegate) OVERRIDE;
  virtual void Copy(const FilePath& src_file_path,
                    const FilePath& dest_file_path,
                    const FileOperationCallback& callback) OVERRIDE;
  virtual void Move(const FilePath& src_file_path,
                    const FilePath& dest_file_path,
                    const FileOperationCallback& callback) OVERRIDE;
  virtual void Remove(const FilePath& file_path,
                      bool is_recursive,
                      const FileOperationCallback& callback) OVERRIDE;
  virtual void CreateDirectory(const FilePath& directory_path,
                               bool is_exclusive,
                               bool is_recursive,
                               const FileOperationCallback& callback) OVERRIDE;
  virtual void GetFile(const FilePath& file_path,
                       const GetFileCallback& callback) OVERRIDE;
  virtual void GetFromCacheForPath(
      const FilePath& gdata_file_path,
      const GetFromCacheCallback& callback) OVERRIDE;
  virtual std::vector<GDataOperationRegistry::ProgressStatus>
  GetProgressStatusList() OVERRIDE;
  virtual bool CancelOperation(const FilePath& file_path) OVERRIDE;
  virtual void AddOperationObserver(
      GDataOperationRegistry::Observer* observer) OVERRIDE;
  virtual void RemoveOperationObserver(
      GDataOperationRegistry::Observer* observer) OVERRIDE;
  virtual void GetCacheState(const std::string& resource_id,
                             const std::string& md5,
                             const GetCacheStateCallback& callback) OVERRIDE;
  virtual bool GetFileInfoFromPath(const FilePath& gdata_file_path,
                                   GDataFileProperties* properties) OVERRIDE;
  virtual FilePath GetGDataCacheTmpDirectory() OVERRIDE;
  virtual void GetAvailableSpace(
      const GetAvailableSpaceCallback& callback) OVERRIDE;

 private:
  friend class GDataUploader;
  friend class GDataFileSystemFactory;
  friend class GDataFileSystemTest;
  FRIEND_TEST_ALL_PREFIXES(GDataFileSystemTest,
                           FindFirstMissingParentDirectory);
  FRIEND_TEST_ALL_PREFIXES(GDataFileSystemTest,
                           GetGDataFileInfoFromPath);
  FRIEND_TEST_ALL_PREFIXES(GDataFileSystemTest,
                           GetFromCacheForPath);

  // Defines possible search results of FindFirstMissingParentDirectory().
  enum FindMissingDirectoryResult {
    // Target directory found, it's not a directory.
    FOUND_INVALID,
    // Found missing directory segment while searching for given directory.
    FOUND_MISSING,
    // Found target directory, it already exists.
    DIRECTORY_ALREADY_PRESENT,
  };

  // Defines set of parameters passes to intermediate callbacks during
  // execution of CreateDirectory() method.
  struct CreateDirectoryParams {
    CreateDirectoryParams(const FilePath& created_directory_path,
                          const FilePath& target_directory_path,
                          bool is_exclusive,
                          bool is_recursive,
                          const FileOperationCallback& callback);
    ~CreateDirectoryParams();

    const FilePath created_directory_path;
    const FilePath target_directory_path;
    const bool is_exclusive;
    const bool is_recursive;
    FileOperationCallback callback;
  };

  // Defines set of parameters passed to intermediate callbacks during
  // execution of GetFile() method.
  struct GetFileFromCacheParams {
    GetFileFromCacheParams(const FilePath& virtual_file_path,
        const FilePath& local_tmp_path,
        const GURL& content_url,
        const std::string& resource_id,
        const std::string& md5,
        scoped_refptr<base::MessageLoopProxy> proxy,
        const GetFileCallback& callback);
    ~GetFileFromCacheParams();

    FilePath virtual_file_path;
    FilePath local_tmp_path;
    GURL content_url;
    std::string resource_id;
    std::string md5;
    scoped_refptr<base::MessageLoopProxy> proxy;
    const GetFileCallback callback;
  };

  // Callback similar to FileOperationCallback but with a given
  // |file_path|.
  typedef base::Callback<void(base::PlatformFileError error,
                              const FilePath& file_path)>
      FilePathUpdateCallback;

  GDataFileSystem(Profile* profile,
                  DocumentsServiceInterface* documents_service,
                  GDataSyncClientInterface* sync_client);
  virtual ~GDataFileSystem();

  // Finds file object by |file_path| and returns the file info.
  // Returns NULL if it does not find the file.
  GDataFileBase* GetGDataFileInfoFromPath(const FilePath& file_path);

  // Initiates upload operation of file defined with |file_name|,
  // |content_type| and |content_length|. The operation will place the newly
  // created file entity into |destination_directory|.
  //
  // Can be called from any thread. |callback| is run on the calling thread.
  void InitiateUpload(const std::string& file_name,
                      const std::string& content_type,
                      int64 content_length,
                      const FilePath& destination_directory,
                      const FilePath& virtual_path,
                      const InitiateUploadCallback& callback);

  // Resumes upload operation for chunk of file defined in |params..
  //
  // Can be called from any thread. |callback| is run on the calling thread.
  void ResumeUpload(const ResumeUploadParams& params,
                    const ResumeUploadCallback& callback);

  // Unsafe (unlocked) version of FindFileByPathSync method.
  void UnsafeFindFileByPath(const FilePath& file_path,
                            FindFileDelegate* delegate);

  // Converts document feed from gdata service into DirectoryInfo. On failure,
  // returns NULL and fills in |error| with an appropriate value.
  GDataDirectory* ParseGDataFeed(GDataErrorCode status,
                                 base::Value* data,
                                 base::PlatformFileError *error);

  // Creates a temporary JSON file representing a document with |edit_url|
  // and |resource_id| on IO thread pool. Upon completion it will invoke
  // |callback| with the path of the created temporary file on thread
  // represented by |relay_proxy|.
  static void CreateDocumentJsonFileOnIOThreadPool(
      const GURL& edit_url,
      const std::string& resource_id,
      const GetFileCallback& callback,
      scoped_refptr<base::MessageLoopProxy> relay_proxy);

  // Renames a file or directory at |file_path| to |new_name|.
  void Rename(const FilePath& file_path,
              const FilePath::StringType& new_name,
              const FilePathUpdateCallback& callback);

  // Adds a file or directory at |file_path| to the directory at |dir_path|.
  void AddFileToDirectory(const FilePath& dir_path,
                          const FileOperationCallback& callback,
                          base::PlatformFileError error,
                          const FilePath& file_path);

  // Removes a file or directory at |file_path| from the directory at
  // |dir_path| and moves it to the root directory.
  void RemoveFileFromDirectory(const FilePath& dir_path,
                               const FilePathUpdateCallback& callback,
                               base::PlatformFileError error,
                               const FilePath& file_path);

  // Removes file under |file_path| from in-memory snapshot of the file system.
  // |resource_id| contains the resource id of the removed file if it was a
  // file.
  // Return PLATFORM_FILE_OK if successful.
  base::PlatformFileError RemoveFileFromGData(const FilePath& file_path,
                                              std::string* resource_id);

  // Callback for handling feed content fetching while searching for file info.
  // This callback is invoked after async feed fetch operation that was
  // invoked by StartDirectoryRefresh() completes. This callback will update
  // the content of the refreshed directory object and continue initially
  // started FindFileByPath() request.
  void OnGetDocuments(const FilePath& search_file_path,
                      scoped_ptr<base::ListValue> feed_list,
                      scoped_refptr<base::MessageLoopProxy> proxy,
                      const FindFileCallback& callback,
                      GDataErrorCode status,
                      scoped_ptr<base::Value> data);

  // A pass-through callback used for bridging from
  // FilePathUpdateCallback to FileOperationCallback.
  void OnFilePathUpdated(const FileOperationCallback& cllback,
                         base::PlatformFileError error,
                         const FilePath& file_path);

  // Callback for handling resource rename attempt.
  void OnRenameResourceCompleted(const FilePath& file_path,
                                 const FilePath::StringType& new_name,
                                 const FilePathUpdateCallback& callback,
                                 GDataErrorCode status,
                                 const GURL& document_url);

  // Callback for handling document copy attempt.
  void OnCopyDocumentCompleted(const FilePathUpdateCallback& callback,
                               GDataErrorCode status,
                               scoped_ptr<base::Value> data);

  // Callback for handling an attempt to add a file or directory to another
  // directory.
  void OnAddFileToDirectoryCompleted(const FileOperationCallback& callback,
                                     const FilePath& file_path,
                                     const FilePath& dir_path,
                                     GDataErrorCode status,
                                     const GURL& document_url);

  // Callback for handling an attempt to remove a file or directory from
  // another directory.
  void OnRemoveFileFromDirectoryCompleted(
      const FilePathUpdateCallback& callback,
      const FilePath& file_path,
      const FilePath& dir_path,
      GDataErrorCode status,
      const GURL& document_url);

  // Callback for handling account metadata fetch.
  void OnGetAvailableSpace(
      const GetAvailableSpaceCallback& callback,
      GDataErrorCode status,
      scoped_ptr<base::Value> data);

  // Callback for handling document remove attempt.
  void OnRemovedDocument(
      const FileOperationCallback& callback,
      const FilePath& file_path,
      GDataErrorCode status,
      const GURL& document_url);

  // Callback for handling directory create requests.
  void OnCreateDirectoryCompleted(
      const CreateDirectoryParams& params,
      GDataErrorCode status,
      scoped_ptr<base::Value> created_entry);

  // Callback for handling file downloading requests.
  void OnFileDownloaded(
    const GetFileFromCacheParams& params,
    GDataErrorCode status,
    const GURL& content_url,
    const FilePath& downloaded_file_path);

  // Callback for handling internal StoreToCache() calls after downloading
  // file content.
  void OnDownloadStoredToCache(base::PlatformFileError error,
                               const std::string& resource_id,
                               const std::string& md5);

  // Callback for handling file upload initialization requests.
  void OnUploadLocationReceived(
      const InitiateUploadCallback& callback,
      scoped_refptr<base::MessageLoopProxy> message_loop_proxy,
      GDataErrorCode code,
      const GURL& upload_location);

  // Callback for handling file upload resume requests.
  void OnResumeUpload(const ResumeUploadCallback& callback,
      scoped_refptr<base::MessageLoopProxy> message_loop_proxy,
      const ResumeUploadResponse& response);

  // Renames a file or directory at |file_path| on in-memory snapshot
  // of the file system. Returns PLATFORM_FILE_OK if successful.
  base::PlatformFileError RenameFileOnFilesystem(
      const FilePath& file_path, const FilePath::StringType& new_name,
      FilePath* updated_file_path);

  // Adds a file or directory at |file_path| to another directory at
  // |dir_path| on in-memory snapshot of the file system.
  // Returns PLATFORM_FILE_OK if successful.
  base::PlatformFileError AddFileToDirectoryOnFilesystem(
      const FilePath& file_path, const FilePath& dir_path);

  // Removes a file or directory at |file_path| from another directory at
  // |dir_path| on in-memory snapshot of the file system.
  // Returns PLATFORM_FILE_OK if successful.
  base::PlatformFileError RemoveFileFromDirectoryOnFilesystem(
      const FilePath& file_path, const FilePath& dir_path,
      FilePath* updated_file_path);

  // Removes file under |file_path| from in-memory snapshot of the file system
  // and the corresponding file from cache if it exists.
  // Return PLATFORM_FILE_OK if successful.
  base::PlatformFileError RemoveFileFromFileSystem(const FilePath& file_path);

  // Parses the content of |feed_data| and returns DocumentFeed instance
  // represeting it.
  DocumentFeed* ParseDocumentFeed(base::Value* feed_data);

  // Updates whole directory structure feeds collected in |feed_list|.
  // On success, returns PLATFORM_FILE_OK.
  base::PlatformFileError UpdateDirectoryWithDocumentFeed(
      base::ListValue* feed_list,
      ContentOrigin origin);

  // Converts |entry_value| into GFileDocument instance and adds it
  // to virtual file system at |directory_path|.
  base::PlatformFileError AddNewDirectory(const FilePath& directory_path,
                                          base::Value* entry_value);

  // Given non-existing |directory_path|, finds the first missing parent
  // directory of |directory_path|.
  FindMissingDirectoryResult FindFirstMissingParentDirectory(
      const FilePath& directory_path,
      GURL* last_dir_content_url,
      FilePath* first_missing_parent_path);

  // Starts root feed load from the server. If successful, it will try to find
  // the file upon retrieval completion.
  void LoadFeedFromServer(const FilePath& search_file_path,
                          scoped_refptr<base::MessageLoopProxy> proxy,
                          const FindFileCallback& callback);

  // Starts root feed load from the cache. If successful, it will try to find
  // the file upon retrieval completion. In addition to that, it will
  // initate retrieval of the root feed from the server if |load_from_server|
  // is set.
  void LoadRootFeedFromCache(const FilePath& search_file_path,
                             bool load_from_server,
                             scoped_refptr<base::MessageLoopProxy> proxy,
                             const FindFileCallback& callback);

  // Loads root feed content from |file_path| on IO thread pool. Upon
  // completion it will invoke |callback| on thread represented by
  // |relay_proxy|.
  static void LoadRootFeedOnIOThreadPool(
      const FilePath& meta_cache_path,
      scoped_refptr<base::MessageLoopProxy> relay_proxy,
      const GetJsonDocumentCallback& callback);

  // Callback for handling root directory refresh from the cache.
  void OnLoadRootFeed(const FilePath& search_file_path,
                      bool load_from_server,
                      scoped_refptr<base::MessageLoopProxy> proxy,
                      FindFileCallback callback,
                      base::PlatformFileError error,
                      scoped_ptr<base::Value> feed_list);

  // Saves a collected feed in GCache directory under
  // <user_profile_dir>/GCache/v1/meta/|name| for later reloading when offline.
  void SaveFeed(scoped_ptr<base::Value> feed_vector,
                const FilePath& name);
  static void SaveFeedOnIOThreadPool(
      const FilePath& meta_cache_path,
      scoped_ptr<base::Value> feed_vector,
      const FilePath& name);

  // Finds and returns upload url of a given directory. Returns empty url
  // if directory can't be found.
  GURL GetUploadUrlForDirectory(const FilePath& destination_directory);

  void NotifyDirectoryChanged(const FilePath& directory_path);

  // Cache entry points from within GDataFileSystem.
  // The functionalities of GData blob cache include:
  // - stores downloaded gdata files on disk, indexed by the resource_id and md5
  //   of the gdata file.
  // - provides absolute path for files to be cached or cached.
  // - updates the cached file on disk after user has edited it locally
  // - handles eviction when disk runs out of space
  // - uploads dirty files to gdata server.
  // - etc.

  // Returns absolute path of the file if it were cached or to be cached.
  FilePath GetCacheFilePath(const std::string& resource_id,
                            const std::string& md5,
                            CacheSubdir subdir_id,
                            CachedFileOrigin file_orign);

  // Stores |source_path| corresponding to |resource_id| and |md5| to cache.
  // Initializes cache if it has not been initialized.
  // Upon completion, |callback| is invoked on the thread where this method was
  // called.
  void StoreToCache(const std::string& resource_id,
                    const std::string& md5,
                    const FilePath& source_path,
                    const CacheOperationCallback& callback);

  // Checks if file corresponding to |resource_id| and |md5| exist on disk and
  // can be accessed i.e. not corrupted by previous file operations that didn't
  // complete for whatever reasons.
  // Initializes cache if it has not been initialized.
  // Upon completion, |callback| is invoked on the thread where this method was
  // called with the cache file path if it exists and is accessible or empty
  // otherwise.
  void GetFromCache(const std::string& resource_id,
                    const std::string& md5,
                    const GetFromCacheCallback& callback);

  // Removes all files corresponding to |resource_id| from cache.
  // Initializes cache if it has not been initialized.
  // Upon completion, |callback| is invoked on the thread where this method was
  // called.
  void RemoveFromCache(const std::string& resource_id,
                       const CacheOperationCallback& callback);

  // Pin file corresponding to |resource_id| and |md5| by setting the
  // appropriate file attributes.
  // We'll try not to evict pinned cache files unless there's not enough space
  // on disk and pinned files are the only ones left.
  // If the file to be pinned is not stored in the cache,
  // net::ERR_FILE_NOT_FOUND will be passed to the |callback|.
  // Initializes cache if it has not been initialized.
  // Upon completion, |callback| is invoked on the thread where this method was
  // called.
  void Pin(const std::string& resource_id,
           const std::string& md5,
           const CacheOperationCallback& callback);

  // Unpin file corresponding to |resource_id| and |md5| by setting the
  // appropriate file attributes.
  // Unpinned files would be evicted when space on disk runs out.
  // Initializes cache if it has not been initialized.
  // Upon completion, |callback| is invoked on the thread where this method was
  // called.
  void Unpin(const std::string& resource_id,
             const std::string& md5,
             const CacheOperationCallback& callback);

  // Initializes cache if it hasn't been initialized by posting
  // InitializeCacheOnIOThreadPool task to IO thread pool.
  void InitializeCacheIfNecessary();
  // Traverses cache sundirectory |subdir| and build |cache_map| with found
  // file blobs.
  void TraverseCacheDirectory(CacheSubdir subdir,
      GDataRootDirectory::CacheMap* cache_map);

  // Task posted from InitializeCacheIfNecessary to run on IO thread pool.
  // Creates cache directory and its sub-directories if they don't exist,
  // or scans blobs sub-directory for files and their attributes and updates the
  // info into cache map.
  void InitializeCacheOnIOThreadPool();

  // Cache callbacks from cache tasks that were run on IO thread pool.

  // Callback for StoreToCache that updates the data members with results from
  // StoreToCacheOnIOThreadPool.
  void OnStoredToCache(base::PlatformFileError error,
                       const std::string& resource_id,
                       const std::string& md5,
                       mode_t mode_bits,
                       const CacheOperationCallback& callback);

  // Callback for GetFromCache that checks if file corresponding to
  // |resource_id| and |md5| exist in cache map.
  void OnGetFromCache(const std::string& resource_id,
                      const std::string& md5,
                      const FilePath& gdata_file_path,
                      const GetFromCacheCallback& callback);

  // Default dummy callback for RemoveFromCache.
  void OnRemovedFromCache(base::PlatformFileError error,
                          const std::string& resource_id,
                          const std::string& md5);

  // Callback for Pin. Calls OnCacheStatusModified() and notifies the
  // observers.
  void OnFilePinned(base::PlatformFileError error,
                    const std::string& resource_id,
                    const std::string& md5,
                    mode_t mode_bits,
                    const CacheOperationCallback& callback);

  // Callback for Unpin. Calls OnCacheStatusModified() and notifies the
  // observers.
  void OnFileUnpinned(base::PlatformFileError error,
                      const std::string& resource_id,
                      const std::string& md5,
                      mode_t mode_bits,
                      const CacheOperationCallback& callback);

  // Helper function for OnFilePinned() and OnFileUnpinned().
  void OnCacheStatusModified(base::PlatformFileError error,
                             const std::string& resource_id,
                             const std::string& md5,
                             mode_t mode_bits,
                             const CacheOperationCallback& callback);

  // Helper function for internally handling responses from GetFromCache()
  // calls during processing of GetFile() request.
  void OnGetFileFromCache(const GetFileFromCacheParams& params,
                          base::PlatformFileError error,
                          const std::string& resource_id,
                          const std::string& md5,
                          const FilePath& gdata_file_path,
                          const FilePath& cache_file_path);

  // Callback for GetCacheState that gets cache state of file corresponding to
  // |resource_id| and |md5|.
  void OnGetCacheState(const std::string& resource_id,
                       const std::string& md5,
                       const GetCacheStateCallback& callback);

  // Cache internal helper functions.

  void GetFromCacheInternal(const std::string& resource_id,
                            const std::string& md5,
                            const FilePath& gdata_file_path,
                            const GetFromCacheCallback& callback);

  // Unsafe (unlocked) version of InitializeCacheIfnecessary method.
  void UnsafeInitializeCacheIfNecessary();

  // Helper function used to perform file search on the calling thread of
  // FindFileByPath() request.
  void FindFileByPathOnCallingThread(const FilePath& search_file_path,
                                     const FindFileCallback& callback);

  scoped_ptr<GDataRootDirectory> root_;

  base::Lock lock_;

  // The profile hosts the GDataFileSystem.
  Profile* profile_;

  // The document service for the GDataFileSystem.
  scoped_ptr<DocumentsServiceInterface> documents_service_;

  // File content uploader.
  scoped_ptr<GDataUploader> gdata_uploader_;
  // Downloads observer.
  scoped_ptr<GDataDownloadObserver> gdata_download_observer_;

  // Base path for GData cache, e.g. <user_profile_dir>/user/GCache/v1.
  FilePath gdata_cache_path_;

  // Paths for all subdirectories of GCache, one for each CacheSubdir
  // enum.
  std::vector<FilePath> cache_paths_;

  scoped_ptr<base::WaitableEvent> on_cache_initialized_;

  // True if cache initialization has started, is in progress or has completed,
  // we only want to initialize cache once.
  bool cache_initialization_started_;

  base::WeakPtrFactory<GDataFileSystem> weak_ptr_factory_;

  ObserverList<Observer> observers_;
  scoped_ptr<GDataSyncClientInterface> sync_client_;
};

// Singleton that owns all GDataFileSystems and associates them with
// Profiles.
class GDataFileSystemFactory : public ProfileKeyedServiceFactory {
 public:
  // Returns the GDataFileSystem for |profile|, creating it if it is not
  // yet created.
  static GDataFileSystem* GetForProfile(Profile* profile);
  // Returns the GDataFileSystem that is already associated with |profile|,
  // if it is not yet created it will return NULL.
  static GDataFileSystem* FindForProfile(Profile* profile);

  // Returns the GDataFileSystemFactory instance.
  static GDataFileSystemFactory* GetInstance();

 private:
  friend struct DefaultSingletonTraits<GDataFileSystemFactory>;

  GDataFileSystemFactory();
  virtual ~GDataFileSystemFactory();

  // ProfileKeyedServiceFactory:
  virtual ProfileKeyedService* BuildServiceInstanceFor(
      Profile* profile) const OVERRIDE;
};

}  // namespace gdata

#endif  // CHROME_BROWSER_CHROMEOS_GDATA_GDATA_FILE_SYSTEM_H_
