// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_PERMISSIONS_API_PERMISSION_H_
#define CHROME_COMMON_EXTENSIONS_PERMISSIONS_API_PERMISSION_H_

#include <map>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/pickle.h"
#include "chrome/common/extensions/permissions/permission_message.h"

namespace base {
class Value;
}

namespace IPC {
class Message;
}

namespace extensions {

class APIPermissionInfo;
class PermissionsInfo;

// APIPermission is for handling some complex permissions. Please refer to
// extensions::SocketPermission as an example.
// There is one instance per permission per loaded extension.
class APIPermission : public base::RefCounted<APIPermission> {
 public:
  enum ID {
    // Error codes.
    kInvalid = -2,
    kUnknown = -1,

    // Real permissions.
    kActiveTab,
    kAlarms,
    kAppNotifications,
    kAppRuntime,
    kAppWindow,
    kAudioCapture,
    kBackground,
    kBookmark,
    kBrowserTag,
    kBrowsingData,
    kChromeosInfoPrivate,
    kClipboardRead,
    kClipboardWrite,
    kCloudPrintPrivate,
    kCommands,
    kContentSettings,
    kContextMenus,
    kCookie,
    kDebugger,
    kDeclarative,
    kDeclarativeWebRequest,
    kDevtools,
    kEchoPrivate,
    kDownloads,
    kExperimental,
    kFileBrowserHandler,
    kFileBrowserHandlerInternal,
    kFileBrowserPrivate,
    kFileSystem,
    kFileSystemWrite,
    kFontSettings,
    kGeolocation,
    kHistory,
    kIdle,
    kInput,
    kInputMethodPrivate,
    kManagedModePrivate,
    kManagement,
    kMediaGalleries,
    kMediaGalleriesAllGalleries,
    kMediaGalleriesRead,
    kMediaPlayerPrivate,
    kMetricsPrivate,
    kNotification,
    kPageCapture,
    kPlugin,
    kPrivacy,
    kProxy,
    kSerial,
    kSocket,
    kStorage,
    kSystemPrivate,
    kTab,
    kTerminalPrivate,
    kTopSites,
    kTts,
    kTtsEngine,
    kUnlimitedStorage,
    kVideoCapture,
    kWallpaperPrivate,
    kWebNavigation,
    kWebRequest,
    kWebRequestBlocking,
    kWebRequestInternal,
    kWebSocketProxyPrivate,
    kWebstorePrivate,
    kEnumBoundary
  };

  struct CheckParam {
  };

  explicit APIPermission(const APIPermissionInfo* info)
    : info_(info) {
    DCHECK(info_);
  }

  // Returns the id of this permission.
  ID id() const;

  // Returns the name of this permission.
  const char* name() const;

  // Returns the APIPermission of this permission.
  const APIPermissionInfo* info() const {
    return info_;
  }

  // Returns true if the given permission is allowed.
  virtual bool Check(const CheckParam* param) const = 0;

  // Returns true if |rhs| is a subset of this.
  virtual bool Contains(const APIPermission* rhs) const = 0;

  // Returns true if |rhs| is equal to this.
  virtual bool Equal(const APIPermission* rhs) const = 0;

  // Parses the rhs from |value|. Returns false if error happens.
  virtual bool FromValue(const base::Value* value) = 0;

  // Stores this into a new created |value|.
  virtual void ToValue(base::Value** value) const = 0;

  // Clones this.
  virtual APIPermission* Clone() const = 0;

  // Returns a new API permission rhs which equals this - |rhs|.
  virtual APIPermission* Diff(const APIPermission* rhs) const = 0;

  // Returns a new API permission rhs which equals the union of this and
  // |rhs|.
  virtual APIPermission* Union(const APIPermission* rhs) const = 0;

  // Returns a new API permission rhs which equals the intersect of this and
  // |rhs|.
  virtual APIPermission* Intersect(const APIPermission* rhs) const = 0;

  // IPC functions
  // Writes this into the given IPC message |m|.
  virtual void Write(IPC::Message* m) const = 0;

  // Reads from the given IPC message |m|.
  virtual bool Read(const IPC::Message* m, PickleIterator* iter) = 0;

  // Logs this permission.
  virtual void Log(std::string* log) const = 0;

 protected:
  friend class base::RefCounted<APIPermission>;
  virtual ~APIPermission();

 private:
  const APIPermissionInfo* const info_;
};


// The APIPermissionInfo is an immutable class that describes a single
// named permission (API permission).
// There is one instance per permission.
class APIPermissionInfo {
 public:
  enum Flag {
    kFlagNone = 0,

    // Indicates if the permission implies full access (native code).
    kFlagImpliesFullAccess = 1 << 0,

    // Indicates if the permission implies full URL access.
    kFlagImpliesFullURLAccess = 1 << 1,

    // Indicates that extensions cannot specify the permission as optional.
    kFlagCannotBeOptional = 1 << 3
  };

  typedef APIPermission* (*APIPermissionConstructor)(const APIPermissionInfo*);

  typedef std::set<APIPermission::ID> IDSet;

  ~APIPermissionInfo();

  // Creates a APIPermission instance.
  scoped_refptr<APIPermission> CreateAPIPermission() const;

  // Returns the localized permission message associated with this api.
  // Use GetMessage_ to avoid name conflict with macro GetMessage on Windows.
  PermissionMessage GetMessage_() const;

  int flags() const { return flags_; }

  APIPermission::ID id() const { return id_; }

  // Returns the message id associated with this permission.
  PermissionMessage::ID message_id() const {
    return message_id_;
  }

  // Returns the name of this permission.
  const char* name() const { return name_; }

  // Returns true if this permission implies full access (e.g., native code).
  bool implies_full_access() const {
    return (flags_ & kFlagImpliesFullAccess) != 0;
  }

  // Returns true if this permission implies full URL access.
  bool implies_full_url_access() const {
    return (flags_ & kFlagImpliesFullURLAccess) != 0;
  }

  // Returns true if this permission can be added and removed via the
  // optional permissions extension API.
  bool supports_optional() const {
    return (flags_ & kFlagCannotBeOptional) == 0;
  }

 private:
  // Instances should only be constructed from within PermissionsInfo.
  friend class PermissionsInfo;

  explicit APIPermissionInfo(
      APIPermission::ID id,
      const char* name,
      int l10n_message_id,
      PermissionMessage::ID message_id,
      int flags,
      APIPermissionConstructor api_permission_constructor);

  // Register ALL the permissions!
  static void RegisterAllPermissions(PermissionsInfo* info);

  const APIPermission::ID id_;
  const char* const name_;
  const int flags_;
  const int l10n_message_id_;
  const PermissionMessage::ID message_id_;
  const APIPermissionConstructor api_permission_constructor_;
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_PERMISSIONS_API_PERMISSION_H_
