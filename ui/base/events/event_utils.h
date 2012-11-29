// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_EVENTS_EVENT_UTILS_H_
#define UI_BASE_EVENTS_EVENT_UTILS_H_

#include "base/event_types.h"
#include "ui/base/events/event_constants.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "ui/gfx/native_widget_types.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace gfx {
class Point;
}

namespace base {
class TimeDelta;
}

namespace ui {

class Event;

// Updates the list of devices for cached properties.
UI_EXPORT void UpdateDeviceList();

// Get the EventType from a native event.
UI_EXPORT EventType EventTypeFromNative(const base::NativeEvent& native_event);

// Get the EventFlags from a native event.
UI_EXPORT int EventFlagsFromNative(const base::NativeEvent& native_event);

UI_EXPORT base::TimeDelta EventTimeFromNative(
    const base::NativeEvent& native_event);

// Get the location from a native event.  The coordinate system of the resultant
// |Point| has the origin at top-left of the "root window".  The nature of
// this "root window" and how it maps to platform-specific drawing surfaces is
// defined in ui/aura/root_window.* and ui/aura/root_window_host*.
UI_EXPORT gfx::Point EventLocationFromNative(
    const base::NativeEvent& native_event);

// Gets the location in native system coordinate space.
UI_EXPORT gfx::Point EventSystemLocationFromNative(
    const base::NativeEvent& native_event);

#if defined(USE_X11)
// Returns the 'real' button for an event. The button reported in slave events
// does not take into account any remapping (e.g. using xmodmap), while the
// button reported in master events do. This is a utility function to always
// return the mapped button.
UI_EXPORT int EventButtonFromNative(const base::NativeEvent& native_event);
#endif

// Returns the KeyboardCode from a native event.
UI_EXPORT KeyboardCode KeyboardCodeFromNative(
    const base::NativeEvent& native_event);

// Returns true if the message is a mouse event.
UI_EXPORT bool IsMouseEvent(const base::NativeEvent& native_event);

// Returns the flags of the button that changed during a press/release.
UI_EXPORT int GetChangedMouseButtonFlagsFromNative(
    const base::NativeEvent& native_event);

// Gets the mouse wheel offset from a native event.
UI_EXPORT int GetMouseWheelOffset(const base::NativeEvent& native_event);

// Gets the touch id from a native event.
UI_EXPORT int GetTouchId(const base::NativeEvent& native_event);

// Gets the radius along the X/Y axis from a native event. Default is 1.0.
UI_EXPORT float GetTouchRadiusX(const base::NativeEvent& native_event);
UI_EXPORT float GetTouchRadiusY(const base::NativeEvent& native_event);

// Gets the angle of the major axis away from the X axis. Default is 0.0.
UI_EXPORT float GetTouchAngle(const base::NativeEvent& native_event);

// Gets the force from a native_event. Normalized to be [0, 1]. Default is 0.0.
UI_EXPORT float GetTouchForce(const base::NativeEvent& native_event);

// Gets the fling velocity from a native event. is_cancel is set to true if
// this was a tap down, intended to stop an ongoing fling.
UI_EXPORT bool GetFlingData(const base::NativeEvent& native_event,
                            float* vx,
                            float* vy,
                            bool* is_cancel);

// Returns whether this is a scroll event and optionally gets the amount to be
// scrolled. |x_offset|, |y_offset| and |finger_count| can be NULL.
UI_EXPORT bool GetScrollOffsets(const base::NativeEvent& native_event,
                                float* x_offset,
                                float* y_offset,
                                int* finger_count);

UI_EXPORT bool GetGestureTimes(const base::NativeEvent& native_event,
                               double* start_time,
                               double* end_time);

// Enable/disable natural scrolling for touchpads.
UI_EXPORT void SetNaturalScroll(bool enabled);

// In natural scrolling enabled for touchpads?
UI_EXPORT bool IsNaturalScrollEnabled();

// Was this event generated by a touchpad device?
// The caller is responsible for ensuring that this is a mouse/touchpad event
// before calling this function.
UI_EXPORT bool IsTouchpadEvent(const base::NativeEvent& event);

// Returns true if event is noop.
UI_EXPORT bool IsNoopEvent(const base::NativeEvent& event);

// Creates and returns no-op event.
UI_EXPORT base::NativeEvent CreateNoopEvent();

#if defined(OS_WIN)
UI_EXPORT int GetModifiersFromACCEL(const ACCEL& accel);
UI_EXPORT int GetModifiersFromKeyState();

// Returns true if |message| identifies a mouse event that was generated as the
// result of a touch event.
UI_EXPORT bool IsMouseEventFromTouch(UINT message);
#endif

// Returns true if default post-target handling was canceled for |event| after
// its dispatch to its target.
UI_EXPORT bool EventCanceledDefaultHandling(const Event& event);

// Registers a custom event type.
UI_EXPORT int RegisterCustomEventType();

}  // namespace ui

#endif  // UI_BASE_EVENTS_EVENT_UTILS_H_
