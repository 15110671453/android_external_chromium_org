// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/toplevel_window_event_filter.h"

#include "ash/shell.h"
#include "ash/wm/property_util.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_util.h"
#include "base/message_loop.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/cursor.h"
#include "ui/aura/env.h"
#include "ui/aura/event.h"
#include "ui/aura/root_window.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/hit_test.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/screen.h"

namespace ash {

ToplevelWindowEventFilter::ToplevelWindowEventFilter(aura::Window* owner)
    : in_move_loop_(false),
      in_gesture_resize_(false),
      grid_size_(0) {
  aura::client::SetWindowMoveClient(owner, this);
}

ToplevelWindowEventFilter::~ToplevelWindowEventFilter() {
}

bool ToplevelWindowEventFilter::PreHandleKeyEvent(aura::Window* target,
                                                  aura::KeyEvent* event) {
  return false;
}

bool ToplevelWindowEventFilter::PreHandleMouseEvent(aura::Window* target,
                                                    aura::MouseEvent* event) {
  switch (event->type()) {
    case ui::ET_MOUSE_PRESSED: {
      // We also update the current window component here because for the
      // mouse-drag-release-press case, where the mouse is released and
      // pressed without mouse move event.
      int component =
          target->delegate()->GetNonClientComponent(event->location());
      if (WindowResizer::GetBoundsChangeForWindowComponent(component)) {
        window_resizer_.reset(
            CreateWindowResizer(target, event->location(), component));
        if (!window_resizer_->is_resizable())
          window_resizer_.reset();
      } else {
        window_resizer_.reset();
      }
      if (component == HTCAPTION && event->flags() & ui::EF_IS_DOUBLE_CLICK)
        ToggleMaximizedState(target);
      return WindowResizer::GetBoundsChangeForWindowComponent(component) != 0;
    }
    case ui::ET_MOUSE_DRAGGED:
      return HandleDrag(target, event);
    case ui::ET_MOUSE_CAPTURE_CHANGED:
    case ui::ET_MOUSE_RELEASED:
      CompleteDrag(target);
      if (in_move_loop_) {
        MessageLoop::current()->Quit();
        in_move_loop_ = false;
      }
      break;
    default:
      break;
  }
  return false;
}

ui::TouchStatus ToplevelWindowEventFilter::PreHandleTouchEvent(
    aura::Window* target,
    aura::TouchEvent* event) {
  return ui::TOUCH_STATUS_UNKNOWN;
}

ui::GestureStatus ToplevelWindowEventFilter::PreHandleGestureEvent(
    aura::Window* target, aura::GestureEvent* event) {
  switch (event->type()) {
    case ui::ET_GESTURE_SCROLL_BEGIN: {
      int component =
          target->delegate()->GetNonClientComponent(event->location());
      if (WindowResizer::GetBoundsChangeForWindowComponent(component) == 0) {
        window_resizer_.reset();
        return ui::GESTURE_STATUS_UNKNOWN;
      }
      in_gesture_resize_ = true;
      window_resizer_.reset(
          CreateWindowResizer(target, event->location(), component));
      if (!window_resizer_->is_resizable())
        window_resizer_.reset();
      break;
    }
    case ui::ET_GESTURE_SCROLL_UPDATE: {
      if (!in_gesture_resize_)
        return ui::GESTURE_STATUS_UNKNOWN;
      HandleDrag(target, event);
      break;
    }
    case ui::ET_GESTURE_SCROLL_END: {
      if (!in_gesture_resize_)
        return ui::GESTURE_STATUS_UNKNOWN;
      CompleteDrag(target);
      in_gesture_resize_ = false;
      break;
    }
    default:
      return ui::GESTURE_STATUS_UNKNOWN;
  }

  return ui::GESTURE_STATUS_CONSUMED;
}

void ToplevelWindowEventFilter::RunMoveLoop(aura::Window* source) {
  DCHECK(!in_move_loop_);  // Can only handle one nested loop at a time.
  in_move_loop_ = true;
  gfx::Point source_mouse_location(gfx::Screen::GetCursorScreenPoint());
  aura::Window::ConvertPointToWindow(
      Shell::GetRootWindow(), source, &source_mouse_location);
  window_resizer_.reset(
      CreateWindowResizer(source, source_mouse_location, HTCAPTION));
#if !defined(OS_MACOSX)
  MessageLoopForUI::current()->RunWithDispatcher(
      aura::Env::GetInstance()->GetDispatcher());
#endif  // !defined(OS_MACOSX)
  in_move_loop_ = false;
}

void ToplevelWindowEventFilter::EndMoveLoop() {
  if (!in_move_loop_)
    return;

  in_move_loop_ = false;
  window_resizer_.reset();
  MessageLoopForUI::current()->Quit();
  Shell::GetRootWindow()->PostNativeEvent(ui::CreateNoopEvent());
}

WindowResizer* ToplevelWindowEventFilter::CreateWindowResizer(
    aura::Window* window,
    const gfx::Point& point,
    int window_component) {
  return new WindowResizer(window, point, window_component, grid_size_);
}

void ToplevelWindowEventFilter::CompleteDrag(aura::Window* window) {
  scoped_ptr<WindowResizer> resizer(window_resizer_.release());
  if (resizer.get())
    resizer->CompleteDrag();
}

bool ToplevelWindowEventFilter::HandleDrag(aura::Window* target,
                                           aura::LocatedEvent* event) {
  // This function only be triggered to move window
  // by mouse drag or touch move event.
  DCHECK(event->type() == ui::ET_MOUSE_DRAGGED ||
         event->type() == ui::ET_TOUCH_MOVED ||
         event->type() == ui::ET_GESTURE_SCROLL_UPDATE);

  if (!window_resizer_.get())
    return false;
  window_resizer_->Drag(event->location());
  return true;
}

}  // namespace ash
