// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/partial_screenshot_event_filter.h"

#include "ash/wm/partial_screenshot_view.h"

namespace ash {
namespace internal {

PartialScreenshotEventFilter::PartialScreenshotEventFilter()
    : view_(NULL) {
}

PartialScreenshotEventFilter::~PartialScreenshotEventFilter() {
  view_ = NULL;
}

bool PartialScreenshotEventFilter::PreHandleKeyEvent(
    aura::Window* target, aura::KeyEvent* event) {
  if (!view_)
    return false;

  // Do not consume a translated key event which is generated by an IME (e.g.,
  // ui::VKEY_PROCESSKEY) since the key event is generated in response to a key
  // press or release before showing the screenshot view. This is important not
  // to confuse key event handling JavaScript code in a page.
  if (event->type() == ui::ET_TRANSLATED_KEY_PRESS ||
      event->type() == ui::ET_TRANSLATED_KEY_RELEASE) {
    return false;
  }

  if (event->key_code() == ui::VKEY_ESCAPE)
    Cancel();

  // Always handled: other windows shouldn't receive input while we're
  // taking a screenshot.
  return true;
}

bool PartialScreenshotEventFilter::PreHandleMouseEvent(
    aura::Window* target, aura::MouseEvent* event) {
  return false;  // Not handled.
}

ui::TouchStatus PartialScreenshotEventFilter::PreHandleTouchEvent(
    aura::Window* target, aura::TouchEvent* event) {
  return ui::TOUCH_STATUS_UNKNOWN;  // Not handled.
}

ui::GestureStatus PartialScreenshotEventFilter::PreHandleGestureEvent(
    aura::Window* target, aura::GestureEvent* event) {
  return ui::GESTURE_STATUS_UNKNOWN;  // Not handled.
}

void PartialScreenshotEventFilter::OnLoginStateChanged(
    user::LoginStatus status) {
  Cancel();
}

void PartialScreenshotEventFilter::OnAppTerminating() {
  Cancel();
}

void PartialScreenshotEventFilter::OnLockStateChanged(bool locked) {
  Cancel();
}

void PartialScreenshotEventFilter::Activate(PartialScreenshotView* view) {
  view_ = view;
}

void PartialScreenshotEventFilter::Deactivate() {
  view_ = NULL;
}

void PartialScreenshotEventFilter::Cancel() {
  if (view_)
    view_->Cancel();
}
}  // namespace internal
}  // namespace ash
