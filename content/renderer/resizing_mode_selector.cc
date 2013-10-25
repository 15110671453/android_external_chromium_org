// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/resizing_mode_selector.h"

#include "content/common/view_messages.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/render_widget.h"
#include "third_party/WebKit/public/web/WebScreenInfo.h"

namespace content {

ResizingModeSelector::ResizingModeSelector()
    : is_synchronous_mode_(false) {
  // TODO(dglazkov): Remove this re-initialization once the transition to
  // useUnfortunateAsynchronousResizeMode is complete.
  // See http://crbug.com/309760 for details.
  is_synchronous_mode_ = RenderThreadImpl::current() &&
      RenderThreadImpl::current()->layout_test_mode();
}

bool ResizingModeSelector::ShouldAbortOnResize(
    RenderWidget* widget,
    const ViewMsg_Resize_Params& params) {
  return is_synchronous_mode_ &&
      params.is_fullscreen == widget->is_fullscreen() &&
      params.screen_info.deviceScaleFactor ==
        widget->screenInfo().deviceScaleFactor;
}

void ResizingModeSelector::set_is_synchronous_mode(bool mode) {
  // TODO(dglazkov): Actually set is_synchronous_mode_ once the transition to
  // useUnfortunateAsynchronousResizeMode is complete.
}

bool ResizingModeSelector::is_synchronous_mode() const {
  return is_synchronous_mode_;
}

}  // namespace content
