// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/web_app.h"

#import <Cocoa/Cocoa.h>

#include "base/file_util.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/scoped_temp_dir.h"
#include "base/sys_string_conversions.h"
#include "chrome/common/mac/app_mode_common.h"
#include "content/public/browser/browser_thread.h"
#include "grit/chromium_strings.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace {

// Returns a path to the app loader.
FilePath GetAppLoaderPath() {
  NSString* app_loader = [l10n_util::GetNSString(IDS_PRODUCT_NAME)
      stringByAppendingString:@" App Mode Loader.app"];
  return base::mac::PathForFrameworkBundleResource(
      base::mac::NSToCFCast(app_loader));
}

// Returns a path to the destination where the app should be written to.
FilePath GetDestinationPath() {
  FilePath path;
  if (base::mac::GetLocalDirectory(NSApplicationDirectory, &path) &&
      file_util::PathIsWritable(path)) {
    return path;
  }

  if (base::mac::GetUserDirectory(NSApplicationDirectory, &path))
    return path;

  return FilePath();
}

// Updates the plist inside |app_path| with the information in |info|.
bool UpdatePlist(const FilePath& app_path,
                 const ShellIntegration::ShortcutInfo& info) {
  NSString* plist_path = base::mac::FilePathToNSString(
      app_path.Append("Contents").Append("Info.plist"));
  NSMutableDictionary* dict =
      [NSMutableDictionary dictionaryWithContentsOfFile:plist_path];

  [dict setObject:base::SysUTF8ToNSString(info.extension_id)
           forKey:app_mode::kCrAppModeShortcutIDKey];
  [dict setObject:base::SysUTF16ToNSString(info.title)
           forKey:app_mode::kCrAppModeShortcutNameKey];
  [dict setObject:base::SysUTF8ToNSString(info.url.spec())
           forKey:app_mode::kCrAppModeShortcutURLKey];
  return [dict writeToFile:plist_path atomically:YES];
}

bool UpdateIcon() {
  // TODO:(sail) Need to implement this.
  return true;
}

}  // namespace

namespace web_app {
namespace internals {

void CreateShortcutTask(const FilePath& web_app_path,
                        const FilePath& profile_path,
                        const ShellIntegration::ShortcutInfo& shortcut_info) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::FILE));
  FilePath app_name = GetSanitizedFileName(shortcut_info.title);
  FilePath app_file_name = app_name.ReplaceExtension("app");
  ScopedTempDir scoped_temp_dir;
  if (!scoped_temp_dir.CreateUniqueTempDir())
    return;
  FilePath staging_path = scoped_temp_dir.path().Append(app_file_name);

  // Update the app's plist and icon in a temp directory. This works around
  // a Finder bug where the app's icon doesn't properly update.
  if (!file_util::CopyDirectory(GetAppLoaderPath(), staging_path, true)) {
    LOG(ERROR) << "Copying app to staging path: " << staging_path.value()
               << " failed";
    return;
  }

  if (!UpdatePlist(staging_path, shortcut_info))
    return;

  if (!UpdateIcon())
    return;

  FilePath dst_path = GetDestinationPath().Append(app_file_name);
  if (!file_util::CopyDirectory(staging_path, dst_path, true)) {
    LOG(ERROR) << "Copying app to dst path: " << dst_path.value() << " failed";
    return;
  }

  [[NSWorkspace sharedWorkspace]
      selectFile:base::mac::FilePathToNSString(dst_path)
      inFileViewerRootedAtPath:nil];
}

}  // namespace internals
}  // namespace web_app
