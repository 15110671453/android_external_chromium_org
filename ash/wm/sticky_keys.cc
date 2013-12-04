// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/sticky_keys.h"

#if defined(USE_X11)
#include <X11/extensions/XInput2.h>
#include <X11/Xlib.h>
#undef RootWindow
#endif

#include "base/basictypes.h"
#include "base/debug/stack_trace.h"
#include "ui/aura/root_window.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tracker.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

namespace ash {

namespace {

// Returns true if the type of mouse event should be modified by sticky keys.
bool ShouldModifyMouseEvent(ui::MouseEvent* event) {
  ui::EventType type = event->type();
  return type == ui::ET_MOUSE_PRESSED || type == ui::ET_MOUSE_RELEASED ||
         type == ui::ET_MOUSEWHEEL;
}

// An implementation of StickyKeysHandler::StickyKeysHandlerDelegate.
class StickyKeysHandlerDelegateImpl :
    public StickyKeysHandler::StickyKeysHandlerDelegate {
 public:
  StickyKeysHandlerDelegateImpl();
  virtual ~StickyKeysHandlerDelegateImpl();

  // StickyKeysHandlerDelegate overrides.
  virtual void DispatchKeyEvent(ui::KeyEvent* event,
                                aura::Window* target) OVERRIDE;

  virtual void DispatchMouseEvent(ui::MouseEvent* event,
                                  aura::Window* target) OVERRIDE;

  virtual void DispatchScrollEvent(ui::ScrollEvent* event,
                                   aura::Window* target) OVERRIDE;
 private:
  DISALLOW_COPY_AND_ASSIGN(StickyKeysHandlerDelegateImpl);
};

StickyKeysHandlerDelegateImpl::StickyKeysHandlerDelegateImpl() {
}

StickyKeysHandlerDelegateImpl::~StickyKeysHandlerDelegateImpl() {
}

void StickyKeysHandlerDelegateImpl::DispatchKeyEvent(ui::KeyEvent* event,
                                                     aura::Window* target) {
  DCHECK(target);
  target->GetDispatcher()->AsRootWindowHostDelegate()->OnHostKeyEvent(event);
}

void StickyKeysHandlerDelegateImpl::DispatchMouseEvent(ui::MouseEvent* event,
                                                       aura::Window* target) {
  DCHECK(target);
  // We need to send a new, untransformed mouse event to the host.
  if (event->IsMouseWheelEvent()) {
    ui::MouseWheelEvent new_event(*static_cast<ui::MouseWheelEvent*>(event));
    target->GetDispatcher()->AsRootWindowHostDelegate()
        ->OnHostMouseEvent(&new_event);
  } else {
    ui::MouseEvent new_event(*event, target, target->GetRootWindow());
    target->GetDispatcher()->AsRootWindowHostDelegate()
        ->OnHostMouseEvent(&new_event);
  }
}

void StickyKeysHandlerDelegateImpl::DispatchScrollEvent(
    ui::ScrollEvent* event,
    aura::Window* target)  {
  DCHECK(target);
  target->GetDispatcher()->AsRootWindowHostDelegate()
      ->OnHostScrollEvent(event);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//  StickyKeys
StickyKeys::StickyKeys()
    : enabled_(false),
      shift_sticky_key_(
          new StickyKeysHandler(ui::EF_SHIFT_DOWN,
                              new StickyKeysHandlerDelegateImpl())),
      alt_sticky_key_(
          new StickyKeysHandler(ui::EF_ALT_DOWN,
                                new StickyKeysHandlerDelegateImpl())),
      ctrl_sticky_key_(
          new StickyKeysHandler(ui::EF_CONTROL_DOWN,
                                new StickyKeysHandlerDelegateImpl())) {
}

StickyKeys::~StickyKeys() {
}

void StickyKeys::Enable(bool enabled) {
  if (enabled_ != enabled) {
    enabled_ = enabled;

    // Reset key handlers when activating sticky keys to ensure all
    // the handlers' states are reset.
    if (enabled_) {
      shift_sticky_key_.reset(
          new StickyKeysHandler(ui::EF_SHIFT_DOWN,
                              new StickyKeysHandlerDelegateImpl()));
      alt_sticky_key_.reset(
          new StickyKeysHandler(ui::EF_ALT_DOWN,
                                new StickyKeysHandlerDelegateImpl()));
      ctrl_sticky_key_.reset(
          new StickyKeysHandler(ui::EF_CONTROL_DOWN,
                                new StickyKeysHandlerDelegateImpl()));
    }
  }
}

bool StickyKeys::HandleKeyEvent(ui::KeyEvent* event) {
  return shift_sticky_key_->HandleKeyEvent(event) ||
      alt_sticky_key_->HandleKeyEvent(event) ||
      ctrl_sticky_key_->HandleKeyEvent(event);
  return ctrl_sticky_key_->HandleKeyEvent(event);
}

bool StickyKeys::HandleMouseEvent(ui::MouseEvent* event) {
  return shift_sticky_key_->HandleMouseEvent(event) ||
      alt_sticky_key_->HandleMouseEvent(event) ||
      ctrl_sticky_key_->HandleMouseEvent(event);
}

bool StickyKeys::HandleScrollEvent(ui::ScrollEvent* event) {
  return shift_sticky_key_->HandleScrollEvent(event) ||
      alt_sticky_key_->HandleScrollEvent(event) ||
      ctrl_sticky_key_->HandleScrollEvent(event);
}

void StickyKeys::OnKeyEvent(ui::KeyEvent* event) {
  // Do not consume a translated key event which is generated by an IME.
  if (event->type() == ui::ET_TRANSLATED_KEY_PRESS ||
      event->type() == ui::ET_TRANSLATED_KEY_RELEASE) {
    return;
  }

  if (enabled_ && HandleKeyEvent(event))
    event->StopPropagation();
}

void StickyKeys::OnMouseEvent(ui::MouseEvent* event) {
  if (enabled_ && HandleMouseEvent(event))
    event->StopPropagation();
}

void StickyKeys::OnScrollEvent(ui::ScrollEvent* event) {
  if (enabled_ && HandleScrollEvent(event))
    event->StopPropagation();
}

///////////////////////////////////////////////////////////////////////////////
//  StickyKeysHandler
StickyKeysHandler::StickyKeysHandler(ui::EventFlags target_modifier_flag,
                                     StickyKeysHandlerDelegate* delegate)
    : modifier_flag_(target_modifier_flag),
      current_state_(DISABLED),
      event_from_myself_(false),
      preparing_to_enable_(false),
      scroll_delta_(0),
      delegate_(delegate) {
}

StickyKeysHandler::~StickyKeysHandler() {
}

StickyKeysHandler::StickyKeysHandlerDelegate::StickyKeysHandlerDelegate() {
}

StickyKeysHandler::StickyKeysHandlerDelegate::~StickyKeysHandlerDelegate() {
}

bool StickyKeysHandler::HandleKeyEvent(ui::KeyEvent* event) {
  if (event_from_myself_)
    return false;  // Do not handle self-generated key event.
  switch (current_state_) {
    case DISABLED:
      return HandleDisabledState(event);
    case ENABLED:
      return HandleEnabledState(event);
    case LOCKED:
      return HandleLockedState(event);
  }
  NOTREACHED();
  return false;
}

bool StickyKeysHandler::HandleMouseEvent(ui::MouseEvent* event) {
  if (event_from_myself_ || current_state_ == DISABLED
      || !ShouldModifyMouseEvent(event)) {
    return false;
  }

  DCHECK(current_state_ == ENABLED || current_state_ == LOCKED);

  preparing_to_enable_ = false;
  AppendModifier(event);
  // Only disable on the mouse released event in normal, non-locked mode.
  if (current_state_ == ENABLED && event->type() != ui::ET_MOUSE_PRESSED) {
    current_state_ = DISABLED;
    DispatchEventAndReleaseModifier(event);
    return true;
  }

  return false;
}

bool StickyKeysHandler::HandleScrollEvent(ui::ScrollEvent* event) {
  if (event_from_myself_ || current_state_ == DISABLED)
    return false;
  DCHECK(current_state_ == ENABLED || current_state_ == LOCKED);
  preparing_to_enable_ = false;

  // We detect a direction change if the current |scroll_delta_| is assigned
  // and the offset of the current scroll event has the opposing sign.
  bool direction_changed = false;
  if (current_state_ == ENABLED && event->type() == ui::ET_SCROLL) {
    int offset = event->y_offset();
    if (scroll_delta_)
      direction_changed = offset * scroll_delta_ <= 0;
    scroll_delta_ = offset;
  }

  if (!direction_changed)
    AppendModifier(event);

  // We want to modify all the scroll events in the scroll sequence, which ends
  // with a fling start event. We also stop when the scroll sequence changes
  // direction.
  if (current_state_ == ENABLED &&
      (event->type() == ui::ET_SCROLL_FLING_START || direction_changed)) {
    current_state_ = DISABLED;
    scroll_delta_ = 0;
    DispatchEventAndReleaseModifier(event);
    return true;
  }

  return false;
}

StickyKeysHandler::KeyEventType
    StickyKeysHandler::TranslateKeyEvent(ui::KeyEvent* event) {
  bool is_target_key = false;
  if (event->key_code() == ui::VKEY_SHIFT ||
      event->key_code() == ui::VKEY_LSHIFT ||
      event->key_code() == ui::VKEY_RSHIFT) {
    is_target_key = (modifier_flag_ == ui::EF_SHIFT_DOWN);
  } else if (event->key_code() == ui::VKEY_CONTROL ||
      event->key_code() == ui::VKEY_LCONTROL ||
      event->key_code() == ui::VKEY_RCONTROL) {
    is_target_key = (modifier_flag_ == ui::EF_CONTROL_DOWN);
  } else if (event->key_code() == ui::VKEY_MENU ||
      event->key_code() == ui::VKEY_LMENU ||
      event->key_code() == ui::VKEY_RMENU) {
    is_target_key = (modifier_flag_ == ui::EF_ALT_DOWN);
  } else {
    return event->type() == ui::ET_KEY_PRESSED ?
        NORMAL_KEY_DOWN : NORMAL_KEY_UP;
  }

  if (is_target_key) {
    return event->type() == ui::ET_KEY_PRESSED ?
        TARGET_MODIFIER_DOWN : TARGET_MODIFIER_UP;
  }
  return event->type() == ui::ET_KEY_PRESSED ?
      OTHER_MODIFIER_DOWN : OTHER_MODIFIER_UP;
}

bool StickyKeysHandler::HandleDisabledState(ui::KeyEvent* event) {
  switch (TranslateKeyEvent(event)) {
    case TARGET_MODIFIER_UP:
      if (preparing_to_enable_) {
        preparing_to_enable_ = false;
        scroll_delta_ = 0;
        current_state_ = ENABLED;
        modifier_up_event_.reset(new ui::KeyEvent(*event));
        return true;
      }
      return false;
    case TARGET_MODIFIER_DOWN:
      preparing_to_enable_ = true;
      return false;
    case NORMAL_KEY_DOWN:
      preparing_to_enable_ = false;
      return false;
    case NORMAL_KEY_UP:
    case OTHER_MODIFIER_DOWN:
    case OTHER_MODIFIER_UP:
      return false;
  }
  NOTREACHED();
  return false;
}

bool StickyKeysHandler::HandleEnabledState(ui::KeyEvent* event) {
  switch (TranslateKeyEvent(event)) {
    case NORMAL_KEY_UP:
    case TARGET_MODIFIER_DOWN:
      return true;
    case TARGET_MODIFIER_UP:
      current_state_ = LOCKED;
      modifier_up_event_.reset();
      return true;
    case NORMAL_KEY_DOWN: {
      current_state_ = DISABLED;
      AppendModifier(event);
      DispatchEventAndReleaseModifier(event);
      return true;
    }
    case OTHER_MODIFIER_DOWN:
    case OTHER_MODIFIER_UP:
      return false;
  }
  NOTREACHED();
  return false;
}

bool StickyKeysHandler::HandleLockedState(ui::KeyEvent* event) {
  switch (TranslateKeyEvent(event)) {
    case TARGET_MODIFIER_DOWN:
      return true;
    case TARGET_MODIFIER_UP:
      current_state_ = DISABLED;
      return false;
    case NORMAL_KEY_DOWN:
    case NORMAL_KEY_UP:
      AppendModifier(event);
      return false;
    case OTHER_MODIFIER_DOWN:
    case OTHER_MODIFIER_UP:
      return false;
  }
  NOTREACHED();
  return false;
}

void StickyKeysHandler::DispatchEventAndReleaseModifier(ui::Event* event) {
  DCHECK(event->IsKeyEvent() ||
         event->IsMouseEvent() ||
         event->IsScrollEvent());
  DCHECK(modifier_up_event_.get());
  aura::Window* target = static_cast<aura::Window*>(event->target());
  DCHECK(target);
  aura::Window* root_window = target->GetRootWindow();
  DCHECK(root_window);

  aura::WindowTracker window_tracker;
  window_tracker.Add(target);

  event_from_myself_ = true;
  if (event->IsKeyEvent()) {
    delegate_->DispatchKeyEvent(static_cast<ui::KeyEvent*>(event), target);
  } else if (event->IsMouseEvent()) {
    delegate_->DispatchMouseEvent(static_cast<ui::MouseEvent*>(event), target);
  } else {
    delegate_->DispatchScrollEvent(
        static_cast<ui::ScrollEvent*>(event), target);
  }

  // The action triggered above may have destroyed the event target, in which
  // case we will dispatch the modifier up event to the root window instead.
  aura::Window* modifier_up_target =
      window_tracker.Contains(target) ? target : root_window;
  delegate_->DispatchKeyEvent(modifier_up_event_.get(), modifier_up_target);
  event_from_myself_ = false;
}

void StickyKeysHandler::AppendNativeEventMask(unsigned int* state) {
  unsigned int& state_ref = *state;
  switch (modifier_flag_) {
    case ui::EF_CONTROL_DOWN:
      state_ref |= ControlMask;
      break;
    case ui::EF_ALT_DOWN:
      state_ref |= Mod1Mask;
      break;
    case ui::EF_SHIFT_DOWN:
      state_ref |= ShiftMask;
      break;
    default:
      NOTREACHED();
  }
}

void StickyKeysHandler::AppendModifier(ui::KeyEvent* event) {
#if defined(USE_X11)
  XEvent* xev = event->native_event();
  if (xev) {
    XKeyEvent* xkey = &(xev->xkey);
    AppendNativeEventMask(&xkey->state);
  }
#elif defined(USE_OZONE)
  NOTIMPLEMENTED() << "Modifier key is not handled";
#endif
  event->set_flags(event->flags() | modifier_flag_);
  event->set_character(ui::GetCharacterFromKeyCode(event->key_code(),
                                                   event->flags()));
  event->NormalizeFlags();
}

void StickyKeysHandler::AppendModifier(ui::MouseEvent* event) {
#if defined(USE_X11)
  XEvent* xev = event->native_event();
  if (xev) {
    XButtonEvent* xkey = &(xev->xbutton);
    AppendNativeEventMask(&xkey->state);
  }
#elif defined(USE_OZONE)
  NOTIMPLEMENTED() << "Modifier key is not handled";
#endif
  event->set_flags(event->flags() | modifier_flag_);
}

void StickyKeysHandler::AppendModifier(ui::ScrollEvent* event) {
#if defined(USE_X11)
  XEvent* xev = event->native_event();
  if (xev) {
    XIDeviceEvent* xievent =
        static_cast<XIDeviceEvent*>(xev->xcookie.data);
    if (xievent) {
      AppendNativeEventMask(reinterpret_cast<unsigned int*>(
          &xievent->mods.effective));
    }
  }
#elif defined(USE_OZONE)
  NOTIMPLEMENTED() << "Modifier key is not handled";
#endif
  event->set_flags(event->flags() | modifier_flag_);
}

}  // namespace ash
