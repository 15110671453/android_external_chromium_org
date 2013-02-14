// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_TOUCHUI_TOUCH_EDITING_MENU_H_
#define UI_VIEWS_TOUCHUI_TOUCH_EDITING_MENU_H_

#include "ui/gfx/point.h"
#include "ui/views/bubble/bubble_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/views_export.h"

namespace gfx {
class Canvas;
}

namespace views {
class TouchEditingMenuView;
class Widget;

class VIEWS_EXPORT TouchEditingMenuController {
 public:
  // Checks if the specified menu command is supported.
  virtual bool IsCommandIdEnabled(int command_id) const = 0;

  // Send a context menu command to the controller.
  virtual void ExecuteCommand(int command_id) = 0;

  // Tell the controller that user has selected the context menu button.
  virtual void OpenContextMenu() = 0;

  // Called when the menu is closed.
  virtual void OnMenuClosed(TouchEditingMenuView* menu) = 0;

 protected:
  virtual ~TouchEditingMenuController() {}
};

// A View that displays the touch context menu.
class VIEWS_EXPORT TouchEditingMenuView : public BubbleDelegateView,
                                          public ButtonListener {
 public:
  TouchEditingMenuView(TouchEditingMenuController* controller,
                       gfx::Point anchor_point,
                       gfx::NativeView context);
  virtual ~TouchEditingMenuView();

  void Close();

 private:
  // views::WidgetDelegate overrides:
  void WindowClosing() OVERRIDE;

  // Overridden from ButtonListener.
  void ButtonPressed(Button* sender, const ui::Event& event) OVERRIDE;

  // Overridden from BubbleDelegateView.
  void OnPaint(gfx::Canvas* canvas) OVERRIDE;

  // Queries the |controller_| for what elements to show in the menu and sizes
  // the menu appropriately.
  void CreateButtons();

  // Helper method to create a single button.
  Button* CreateButton(const string16& title, int tag);

  TouchEditingMenuController* controller_;

  DISALLOW_COPY_AND_ASSIGN(TouchEditingMenuView);
};

}  // namespace views

#endif  // UI_VIEWS_TOUCHUI_TOUCH_SELECTION_CONTROLLER_IMPL_H_
