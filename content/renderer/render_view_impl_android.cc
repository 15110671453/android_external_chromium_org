// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_view_impl.h"

#include "base/command_line.h"
#include "base/message_loop.h"
#include "cc/trees/layer_tree_host.h"
#include "content/renderer/gpu/render_widget_compositor.h"

namespace content {

// Check content::TopControlsState and cc::TopControlsState are kept in sync.
COMPILE_ASSERT(int(SHOWN) == int(cc::SHOWN), mismatching_enums);
COMPILE_ASSERT(int(HIDDEN) == int(cc::HIDDEN), mismatching_enums);
COMPILE_ASSERT(int(BOTH) == int(cc::BOTH), mismatching_enums);

cc::TopControlsState ContentToCcTopControlsState(
    TopControlsState state) {
  return static_cast<cc::TopControlsState>(state);
}

// TODO(mvanouwerkerk): Stop calling this code path and delete it.
void RenderViewImpl::OnUpdateTopControlsState(bool enable_hiding,
                                              bool enable_showing,
                                              bool animate) {
  // TODO(tedchoc): Investigate why messages are getting here before the
  //                compositor has been initialized.
  LOG_IF(WARNING, !compositor_) << "OnUpdateTopControlsState was unhandled.";
  if (compositor_) {
    cc::TopControlsState constraints = cc::BOTH;
    if (!enable_showing)
      constraints = cc::HIDDEN;
    if (!enable_hiding)
      constraints = cc::SHOWN;
    cc::TopControlsState current = cc::BOTH;
    compositor_->UpdateTopControlsState(constraints, current, animate);
  }
}

void RenderViewImpl::UpdateTopControlsState(TopControlsState constraints,
                                            TopControlsState current,
                                            bool animate) {
  cc::TopControlsState constraints_cc =
      ContentToCcTopControlsState(constraints);
  cc::TopControlsState current_cc = ContentToCcTopControlsState(current);
  if (compositor_)
    compositor_->UpdateTopControlsState(constraints_cc, current_cc, animate);
}

}  // namespace content
