// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_PROMPT_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_PROMPT_H_
#pragma once

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/string16.h"
#include "chrome/browser/extensions/crx_installer_error.h"
#include "chrome/browser/extensions/image_loading_tracker.h"
#include "chrome/common/extensions/url_pattern.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/native_widget_types.h"

class Browser;
class ExtensionInstallUI;
class ExtensionPermissionSet;
class MessageLoop;
class InfoBarDelegate;

namespace base {
class DictionaryValue;
}  // namespace base

namespace extensions {
class BundleInstaller;
class Extension;
class ExtensionWebstorePrivateApiTest;
}  // namespace extensions

// Displays all the UI around extension installation.
class ExtensionInstallPrompt : public ImageLoadingTracker::Observer {
 public:
  enum PromptType {
    UNSET_PROMPT_TYPE = -1,
    INSTALL_PROMPT = 0,
    INLINE_INSTALL_PROMPT,
    BUNDLE_INSTALL_PROMPT,
    RE_ENABLE_PROMPT,
    PERMISSIONS_PROMPT,
    NUM_PROMPT_TYPES
  };

  // Extra information needed to display an installation or uninstallation
  // prompt. Gets populated with raw data and exposes getters for formatted
  // strings so that the GTK/views/Cocoa install dialogs don't have to repeat
  // that logic.
  class Prompt {
   public:
    explicit Prompt(PromptType type);
    ~Prompt();

    void SetPermissions(const std::vector<string16>& permissions);
    void SetInlineInstallWebstoreData(const std::string& localized_user_count,
                                      double average_rating,
                                      int rating_count);

    PromptType type() const { return type_; }
    void set_type(PromptType type) { type_ = type; }

    // Getters for UI element labels.
    string16 GetDialogTitle() const;
    string16 GetHeading() const;
    string16 GetAcceptButtonLabel() const;
    bool HasAbortButtonLabel() const;
    string16 GetAbortButtonLabel() const;
    string16 GetPermissionsHeading() const;

    // Getters for webstore metadata. Only populated when the type is
    // INLINE_INSTALL_PROMPT.

    // The star display logic replicates the one used by the webstore (from
    // components.ratingutils.setFractionalYellowStars). Callers pass in an
    // "appender", which will be repeatedly called back with the star images
    // that they append to the star display area.
    typedef void(*StarAppender)(const gfx::ImageSkia*, void*);
    void AppendRatingStars(StarAppender appender, void* data) const;
    string16 GetRatingCount() const;
    string16 GetUserCount() const;
    size_t GetPermissionCount() const;
    string16 GetPermission(size_t index) const;

    // Populated for BUNDLE_INSTALL_PROMPT.
    const extensions::BundleInstaller* bundle() const { return bundle_; }
    void set_bundle(const extensions::BundleInstaller* bundle) {
      bundle_ = bundle;
    }

    // Populated for all other types.
    const extensions::Extension* extension() const { return extension_; }
    void set_extension(const extensions::Extension* extension) {
      extension_ = extension;
    }

    const gfx::Image& icon() const { return icon_; }
    void set_icon(const gfx::Image& icon) { icon_ = icon; }

   private:
    PromptType type_;
    // Permissions that are being requested (may not be all of an extension's
    // permissions if only additional ones are being requested)
    std::vector<string16> permissions_;

    // The extension or bundle being installed.
    const extensions::Extension* extension_;
    const extensions::BundleInstaller* bundle_;

    // The icon to be displayed.
    gfx::Image icon_;

    // These fields are populated only when the prompt type is
    // INLINE_INSTALL_PROMPT
    // Already formatted to be locale-specific.
    std::string localized_user_count_;
    // Range is kMinExtensionRating to kMaxExtensionRating
    double average_rating_;
    int rating_count_;
  };

  static const int kMinExtensionRating = 0;
  static const int kMaxExtensionRating = 5;

  class Delegate {
   public:
    // We call this method to signal that the installation should continue.
    virtual void InstallUIProceed() = 0;

    // We call this method to signal that the installation should stop, with
    // |user_initiated| true if the installation was stopped by the user.
    virtual void InstallUIAbort(bool user_initiated) = 0;

   protected:
    virtual ~Delegate() {}
  };

  // Creates a dummy extension from the |manifest|, replacing the name and
  // description with the localizations if provided.
  static scoped_refptr<extensions::Extension> GetLocalizedExtensionForDisplay(
      const base::DictionaryValue* manifest,
      const std::string& id,
      const std::string& localized_name,
      const std::string& localized_description,
      std::string* error);

  explicit ExtensionInstallPrompt(Browser* browser);
  virtual ~ExtensionInstallPrompt();

  ExtensionInstallUI* install_ui() const { return install_ui_.get(); }

  // This is called by the bundle installer to verify whether the bundle
  // should be installed.
  //
  // We *MUST* eventually call either Proceed() or Abort() on |delegate|.
  virtual void ConfirmBundleInstall(extensions::BundleInstaller* bundle,
                                    const ExtensionPermissionSet* permissions);

  // This is called by the inline installer to verify whether the inline
  // install from the webstore should proceed.
  //
  // We *MUST* eventually call either Proceed() or Abort() on |delegate|.
  virtual void ConfirmInlineInstall(Delegate* delegate,
                                    const extensions::Extension* extension,
                                    SkBitmap* icon,
                                    const Prompt& prompt);

  // This is called by the installer to verify whether the installation from
  // the webstore should proceed.
  //
  // We *MUST* eventually call either Proceed() or Abort() on |delegate|.
  virtual void ConfirmWebstoreInstall(Delegate* delegate,
                                      const extensions::Extension* extension,
                                      const SkBitmap* icon);

  // This is called by the installer to verify whether the installation should
  // proceed. This is declared virtual for testing.
  //
  // We *MUST* eventually call either Proceed() or Abort() on |delegate|.
  virtual void ConfirmInstall(Delegate* delegate,
                              const extensions::Extension* extension);

  // This is called by the app handler launcher to verify whether the app
  // should be re-enabled. This is declared virtual for testing.
  //
  // We *MUST* eventually call either Proceed() or Abort() on |delegate|.
  virtual void ConfirmReEnable(Delegate* delegate,
                               const extensions::Extension* extension);

  // This is called by the extension permissions API to verify whether an
  // extension may be granted additional permissions.
  //
  // We *MUST* eventually call either Proceed() or Abort() on |delegate|.
  virtual void ConfirmPermissions(Delegate* delegate,
                                  const extensions::Extension* extension,
                                  const ExtensionPermissionSet* permissions);

  // Installation was successful. This is declared virtual for testing.
  virtual void OnInstallSuccess(const extensions::Extension* extension,
                                SkBitmap* icon);

  // Installation failed. This is declared virtual for testing.
  virtual void OnInstallFailure(const CrxInstallerError& error);

  // ImageLoadingTracker::Observer:
  virtual void OnImageLoaded(const gfx::Image& image,
                             const std::string& extension_id,
                             int index) OVERRIDE;

 protected:
  friend class extensions::ExtensionWebstorePrivateApiTest;
  friend class WebstoreInlineInstallUnpackFailureTest;

 private:
  friend class GalleryInstallApiTestObserver;

  // Sets the icon that will be used in any UI. If |icon| is NULL, or contains
  // an empty bitmap, then a default icon will be used instead.
  void SetIcon(const SkBitmap* icon);

  // Starts the process of showing a confirmation UI, which is split into two.
  // 1) Set off a 'load icon' task.
  // 2) Handle the load icon response and show the UI (OnImageLoaded).
  void LoadImageIfNeeded();

  // Shows the actual UI (the icon should already be loaded).
  void ShowConfirmation();

  Browser* browser_;
  MessageLoop* ui_loop_;

  // The extensions installation icon.
  SkBitmap icon_;

  // The extension we are showing the UI for.
  const extensions::Extension* extension_;

  // The bundle we are showing the UI for, if type BUNDLE_INSTALL_PROMPT.
  const extensions::BundleInstaller* bundle_;

  // The permissions being prompted for.
  scoped_refptr<const ExtensionPermissionSet> permissions_;

  // The object responsible for doing the UI specific actions.
  scoped_ptr<ExtensionInstallUI> install_ui_;

  // The delegate we will call Proceed/Abort on after confirmation UI.
  Delegate* delegate_;

  // A pre-filled prompt.
  Prompt prompt_;

  // The type of prompt we are going to show.
  PromptType prompt_type_;

  // Keeps track of extension images being loaded on the File thread for the
  // purpose of showing the install UI.
  ImageLoadingTracker tracker_;
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_PROMPT_H_
