// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_WEB_INPUT_EVENT_AURA_H_
#define CONTENT_BROWSER_RENDERER_HOST_WEB_INPUT_EVENT_AURA_H_

#include "content/common/content_export.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"

namespace ui {
class GestureEvent;
class KeyEvent;
class MouseEvent;
class MouseWheelEvent;
class ScrollEvent;
class TouchEvent;
}

namespace content {

// Used for scrolling. This matches Firefox behavior.
const int kPixelsPerTick = 53;

CONTENT_EXPORT WebKit::WebMouseEvent MakeWebMouseEvent(
    ui::MouseEvent* event);
CONTENT_EXPORT WebKit::WebMouseWheelEvent MakeWebMouseWheelEvent(
    ui::MouseWheelEvent* event);
CONTENT_EXPORT WebKit::WebMouseWheelEvent MakeWebMouseWheelEvent(
    ui::ScrollEvent* event);
CONTENT_EXPORT WebKit::WebKeyboardEvent MakeWebKeyboardEvent(
    ui::KeyEvent* event);
CONTENT_EXPORT WebKit::WebGestureEvent MakeWebGestureEvent(
    ui::GestureEvent* event);
CONTENT_EXPORT WebKit::WebGestureEvent MakeWebGestureEvent(
    ui::ScrollEvent* event);
CONTENT_EXPORT WebKit::WebGestureEvent MakeWebGestureEventFlingCancel();

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_WEB_INPUT_EVENT_AURA_H_
