// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/accelerator_dispatcher.h"

#if defined(USE_X11)
#include <X11/Xlib.h>

// Xlib defines RootWindow
#ifdef RootWindow
#undef RootWindow
#endif
#endif  // defined(USE_X11)

#include "ash/accelerators/accelerator_controller.h"
#include "ash/shell.h"
#include "ash/wm/event_rewriter_event_filter.h"
#include "ui/aura/env.h"
#include "ui/aura/root_window.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_utils.h"
#include "ui/views/controls/menu/menu_controller.h"

namespace ash {
namespace {

const int kModifierMask = (ui::EF_SHIFT_DOWN |
                           ui::EF_CONTROL_DOWN |
                           ui::EF_ALT_DOWN);
#if defined(OS_WIN)
bool IsKeyEvent(const MSG& msg) {
  return
      msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN ||
      msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP;
}
#elif defined(USE_X11)
bool IsKeyEvent(const XEvent* xev) {
  return xev->type == KeyPress || xev->type == KeyRelease;
}
#elif defined(USE_OZONE)
bool IsKeyEvent(const base::NativeEvent& native_event) {
  const ui::KeyEvent* event = static_cast<const ui::KeyEvent*>(native_event);
  return event->IsKeyEvent();
}
#endif

bool IsPossibleAcceleratorNotForMenu(const ui::KeyEvent& key_event) {
  // For shortcuts generated by Ctrl or Alt plus a letter, number or
  // the tab key, we want to exit the context menu first and then
  // repost the event. That allows for the shortcut execution after
  // the context menu has exited.
  if (key_event.type() == ui::ET_KEY_PRESSED &&
      (key_event.flags() & (ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN))) {
    const ui::KeyboardCode key_code = key_event.key_code();
    if ((key_code >= ui::VKEY_A && key_code <= ui::VKEY_Z) ||
        (key_code >= ui::VKEY_0 && key_code <= ui::VKEY_9) ||
        (key_code == ui::VKEY_TAB)) {
      return true;
    }
  }
  return false;
}

}  // namespace

AcceleratorDispatcher::AcceleratorDispatcher(
    base::MessagePumpDispatcher* nested_dispatcher,
    aura::Window* associated_window)
    : nested_dispatcher_(nested_dispatcher),
      associated_window_(associated_window) {
  DCHECK(nested_dispatcher_);
  associated_window_->AddObserver(this);
}

AcceleratorDispatcher::~AcceleratorDispatcher() {
  if (associated_window_)
    associated_window_->RemoveObserver(this);
}

void AcceleratorDispatcher::OnWindowDestroying(aura::Window* window) {
  if (associated_window_ == window)
    associated_window_ = NULL;
}

uint32_t AcceleratorDispatcher::Dispatch(const base::NativeEvent& event) {
  if (!associated_window_)
    return POST_DISPATCH_QUIT_LOOP;

  if (!ui::IsNoopEvent(event) && !associated_window_->CanReceiveEvents())
    return POST_DISPATCH_PERFORM_DEFAULT;

  if (IsKeyEvent(event)) {
    // Modifiers can be changed by the user preference, so we need to rewrite
    // the event explicitly.
    ui::KeyEvent key_event(event, false);
    ui::EventHandler* event_rewriter =
        ash::Shell::GetInstance()->event_rewriter_filter();
    DCHECK(event_rewriter);
    event_rewriter->OnKeyEvent(&key_event);
    if (key_event.stopped_propagation())
      return POST_DISPATCH_NONE;

    if (IsPossibleAcceleratorNotForMenu(key_event)) {
      if (views::MenuController* menu_controller =
          views::MenuController::GetActiveInstance()) {
        menu_controller->CancelAll();
#if defined(USE_X11)
        XPutBackEvent(event->xany.display, event);
#else
        NOTIMPLEMENTED() << " Repost NativeEvent here.";
#endif
        return POST_DISPATCH_QUIT_LOOP;
      }
    }

    ash::AcceleratorController* accelerator_controller =
        ash::Shell::GetInstance()->accelerator_controller();
    if (accelerator_controller) {
      ui::Accelerator accelerator(key_event.key_code(),
                                  key_event.flags() & kModifierMask);
      if (key_event.type() == ui::ET_KEY_RELEASED)
        accelerator.set_type(ui::ET_KEY_RELEASED);
      // Fill out context object so AcceleratorController will know what
      // was the previous accelerator or if the current accelerator is repeated.
      Shell::GetInstance()->accelerator_controller()->context()->
          UpdateContext(accelerator);
      if (accelerator_controller->Process(accelerator))
        return POST_DISPATCH_NONE;
    }

    return nested_dispatcher_->Dispatch(key_event.native_event());
  }

  return nested_dispatcher_->Dispatch(event);
}

}  // namespace ash
