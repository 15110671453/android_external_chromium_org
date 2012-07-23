// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_TAB_ANDROID_H_
#define CHROME_BROWSER_ANDROID_TAB_ANDROID_H_

#include <jni.h>

#include "base/android/jni_helper.h"
#include "base/android/scoped_java_ref.h"
#include "base/string16.h"

class GURL;
class OnContextMenuItemSelectedCallBack;
class SkBitmap;

namespace browser_sync {
class SyncedTabDelegate;
}

namespace content {
struct ContextMenuParams;
class WebContents;
}

class TabAndroid {
 public:
  TabAndroid();

  // Convenience method to retrieve the Tab associated with the passed
  // WebContents.  Can return NULL.
  static TabAndroid* FromWebContents(content::WebContents* web_contents);

  static TabAndroid* GetNativeTab(JNIEnv* env, jobject obj);

  virtual browser_sync::SyncedTabDelegate* GetSyncedTabDelegate() = 0;

  int id() const {
    return tab_id_;
  }

  // Called to show the regular context menu that is triggered by a long press.
  virtual void ShowContextMenu(const content::ContextMenuParams& params) = 0;

  // Called to show a custom context menu. Used by the NTP.
  virtual void ShowCustomContextMenu(
      const content::ContextMenuParams& params,
      OnContextMenuItemSelectedCallBack* callback) = 0;

  virtual void ShowSelectFileDialog(
      const base::android::ScopedJavaLocalRef<jobject>& select_file) = 0;

  // --------------------------------------------------------------------------
  // Public methods that call to Java via JNI
  // --------------------------------------------------------------------------
  // Called when context menu option to create the bookmark shortcut on
  // homescreen is called.
  virtual void AddShortcutToBookmark(
      const GURL& url, const string16& title, const SkBitmap& skbitmap,
      int r_value, int g_value, int b_value) = 0;

  // TODO(felipeg,tedchoc): Remove this when possible.
  // http://crbug.com/138216
  // Called when the mobile promo action asks to send email.
  virtual void PromoSendEmail(const string16& data_email,
                              const string16& data_subj,
                              const string16& data_body,
                              const string16& data_inv) = 0;

 protected:
  virtual ~TabAndroid();

  int tab_id_;
};

#endif  // CHROME_BROWSER_ANDROID_TAB_ANDROID_H_
