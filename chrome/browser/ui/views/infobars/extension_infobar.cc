// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/infobars/extension_infobar.h"

#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/browser/extensions/extension_infobar_delegate.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_icon_set.h"
#include "chrome/common/extensions/extension_resource.h"
#include "grit/theme_resources.h"
#include "ui/base/animation/slide_animation.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas_skia.h"
#include "views/controls/button/menu_button.h"
#include "views/controls/menu/menu_item_view.h"
#include "views/controls/menu/menu_model_adapter.h"
#include "views/widget/widget.h"

// ExtensionInfoBarDelegate ----------------------------------------------------

InfoBar* ExtensionInfoBarDelegate::CreateInfoBar(TabContentsWrapper* owner) {
  return new ExtensionInfoBar(owner, this);
}

// ExtensionInfoBar ------------------------------------------------------------

namespace {
// The horizontal margin between the menu and the Extension (HTML) view.
const int kMenuHorizontalMargin = 1;
}  // namespace

ExtensionInfoBar::ExtensionInfoBar(TabContentsWrapper* owner,
                                   ExtensionInfoBarDelegate* delegate)
    : InfoBarView(owner, delegate),
      delegate_(delegate),
      menu_(NULL),
      ALLOW_THIS_IN_INITIALIZER_LIST(tracker_(this)) {
  delegate->set_observer(this);

  int height = delegate->height();
  SetBarTargetHeight((height > 0) ? (height + kSeparatorLineHeight) : 0);
}

ExtensionInfoBar::~ExtensionInfoBar() {
  if (GetDelegate())
    GetDelegate()->set_observer(NULL);
}

void ExtensionInfoBar::Layout() {
  InfoBarView::Layout();

  gfx::Size menu_size = menu_->GetPreferredSize();
  menu_->SetBounds(StartX(), OffsetY(menu_size), menu_size.width(),
                   menu_size.height());

  GetDelegate()->extension_host()->view()->SetBounds(
      menu_->bounds().right() + kMenuHorizontalMargin,
      arrow_height(),
      std::max(0, EndX() - StartX() - ContentMinimumWidth()),
      height() - arrow_height() - 1);
}

void ExtensionInfoBar::ViewHierarchyChanged(bool is_add,
                                            View* parent,
                                            View* child) {
  if (!is_add || (child != this) || (menu_ != NULL)) {
    InfoBarView::ViewHierarchyChanged(is_add, parent, child);
    return;
  }

  menu_ = new views::MenuButton(NULL, std::wstring(), this, false);
  menu_->SetVisible(false);
  AddChildView(menu_);

  ExtensionHost* extension_host = GetDelegate()->extension_host();
  AddChildView(extension_host->view());

  // This must happen after adding all other children so InfoBarView can ensure
  // the close button is the last child.
  InfoBarView::ViewHierarchyChanged(is_add, parent, child);

  // This must happen after adding all children because it can trigger layout,
  // which assumes that particular children (e.g. the close button) have already
  // been added.
  const Extension* extension = extension_host->extension();
  int image_size = Extension::EXTENSION_ICON_BITTY;
  ExtensionResource icon_resource = extension->GetIconResource(
      image_size, ExtensionIconSet::MATCH_EXACTLY);
  if (!icon_resource.relative_path().empty()) {
    tracker_.LoadImage(extension, icon_resource,
        gfx::Size(image_size, image_size), ImageLoadingTracker::DONT_CACHE);
  } else {
    OnImageLoaded(NULL, icon_resource, 0);
  }
}

int ExtensionInfoBar::ContentMinimumWidth() const {
  return menu_->GetPreferredSize().width() + kMenuHorizontalMargin;
}

void ExtensionInfoBar::OnImageLoaded(SkBitmap* image,
                                     const ExtensionResource& resource,
                                     int index) {
  if (!GetDelegate())
    return;  // The delegate can go away while we asynchronously load images.

  SkBitmap* icon = image;
  // Fall back on the default extension icon on failure.
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  if (!image || image->empty())
    icon = rb.GetBitmapNamed(IDR_EXTENSIONS_SECTION);

  SkBitmap* drop_image = rb.GetBitmapNamed(IDR_APP_DROPARROW);

  int image_size = Extension::EXTENSION_ICON_BITTY;
  // The margin between the extension icon and the drop-down arrow bitmap.
  static const int kDropArrowLeftMargin = 3;
  scoped_ptr<gfx::CanvasSkia> canvas(new gfx::CanvasSkia(
      image_size + kDropArrowLeftMargin + drop_image->width(), image_size,
      false));
  canvas->DrawBitmapInt(*icon, 0, 0, icon->width(), icon->height(), 0, 0,
                        image_size, image_size, false);
  canvas->DrawBitmapInt(*drop_image, image_size + kDropArrowLeftMargin,
                        image_size / 2);
  menu_->SetIcon(canvas->ExtractBitmap());
  menu_->SetVisible(true);

  Layout();
}

void ExtensionInfoBar::OnDelegateDeleted() {
  delegate_ = NULL;
}

void ExtensionInfoBar::RunMenu(View* source, const gfx::Point& pt) {
  const Extension* extension = GetDelegate()->extension_host()->extension();
  if (!extension->ShowConfigureContextMenus())
    return;

  Browser* browser = BrowserView::GetBrowserViewForNativeWindow(
      platform_util::GetTopLevel(source->GetWidget()->GetNativeView()))->
      browser();
  scoped_refptr<ExtensionContextMenuModel> options_menu_contents =
      new ExtensionContextMenuModel(extension, browser, NULL);
  views::MenuModelAdapter options_menu_delegate(options_menu_contents.get());
  views::MenuItemView options_menu(&options_menu_delegate);
  options_menu_delegate.BuildMenu(&options_menu);

  gfx::Point screen_point;
  views::View::ConvertPointToScreen(menu_, &screen_point);
  options_menu.RunMenuAt(GetWidget(), menu_,
      gfx::Rect(screen_point, menu_->size()), views::MenuItemView::TOPLEFT,
      true);
}

ExtensionInfoBarDelegate* ExtensionInfoBar::GetDelegate() {
  return delegate_ ? delegate_->AsExtensionInfoBarDelegate() : NULL;
}
