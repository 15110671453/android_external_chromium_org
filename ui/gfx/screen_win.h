// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SCREEN_WIN_H_
#define UI_GFX_SCREEN_WIN_H_

#include "base/compiler_specific.h"
#include "ui/base/ui_export.h"
#include "ui/gfx/screen.h"

namespace gfx {

class UI_EXPORT ScreenWin : public gfx::Screen {
 public:
  ScreenWin();
  virtual ~ScreenWin();

 protected:
  // Overridden from gfx::Screen:
  virtual bool IsDIPEnabled() OVERRIDE;
  virtual gfx::Point GetCursorScreenPoint() OVERRIDE;
  virtual gfx::NativeWindow GetWindowAtCursorScreenPoint() OVERRIDE;
  virtual int GetNumDisplays() OVERRIDE;
  virtual gfx::Display GetDisplayNearestWindow(
      gfx::NativeView window) const OVERRIDE;
  virtual gfx::Display GetDisplayNearestPoint(
      const gfx::Point& point) const OVERRIDE;
  virtual gfx::Display GetDisplayMatching(
      const gfx::Rect& match_rect) const OVERRIDE;
  virtual gfx::Display GetPrimaryDisplay() const OVERRIDE;

  // Returns the HWND associated with the NativeView.
  virtual HWND GetHWNDFromNativeView(NativeView window) const;

  // Returns the NativeView associated with the HWND.
  virtual NativeWindow GetNativeWindowFromHWND(HWND hwnd) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ScreenWin);
};

}  // namespace gfx

#endif  // UI_GFX_SCREEN_WIN_H_
