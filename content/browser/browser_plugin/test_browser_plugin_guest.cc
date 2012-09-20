// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/browser_plugin/test_browser_plugin_guest.h"

#include "base/test/test_timeouts.h"
#include "content/browser/browser_plugin/browser_plugin_guest.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/browser_plugin_messages.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/test_utils.h"
#include "ui/gfx/size.h"

namespace content {

class BrowserPluginGuest;

TestBrowserPluginGuest::TestBrowserPluginGuest(
    int instance_id,
    WebContentsImpl* web_contents,
    RenderViewHost* render_view_host)
    : BrowserPluginGuest(instance_id, web_contents, render_view_host),
      update_rect_count_(0),
      crash_observed_(false),
      focus_observed_(false),
      advance_focus_observed_(false),
      was_hidden_observed_(false),
      waiting_for_update_rect_msg_with_size_(false),
      last_update_rect_width_(-1),
      last_update_rect_height_(-1) {
  // Listen to visibility changes so that a test can wait for these changes.
  registrar_.Add(this,
                 NOTIFICATION_WEB_CONTENTS_VISIBILITY_CHANGED,
                 Source<WebContents>(web_contents));
}

TestBrowserPluginGuest::~TestBrowserPluginGuest() {
}

void TestBrowserPluginGuest::Observe(int type,
                                     const NotificationSource& source,
                                     const NotificationDetails& details) {
  switch (type) {
    case NOTIFICATION_WEB_CONTENTS_VISIBILITY_CHANGED: {
      bool visible = *Details<bool>(details).ptr();
      if (!visible) {
        was_hidden_observed_ = true;
        if (was_hidden_message_loop_runner_)
          was_hidden_message_loop_runner_->Quit();
      }
      break;
    }
    default:
      NOTREACHED() << "Unexpected notification type: " << type;
  }
}

void TestBrowserPluginGuest::SendMessageToEmbedder(IPC::Message* msg) {
  if (msg->type() == BrowserPluginMsg_UpdateRect::ID) {
    PickleIterator iter(*msg);

    int instance_id;
    int message_id;
    BrowserPluginMsg_UpdateRect_Params update_rect_params;

    if (!IPC::ReadParam(msg, &iter, &instance_id) ||
        !IPC::ReadParam(msg, &iter, &message_id) ||
        !IPC::ReadParam(msg, &iter, &update_rect_params)) {
      NOTREACHED() <<
          "Cannot read BrowserPluginMsg_UpdateRect params from ipc message";
    }
    last_update_rect_width_ = update_rect_params.view_size.width();
    last_update_rect_height_ = update_rect_params.view_size.height();
    update_rect_count_++;
    if (waiting_for_update_rect_msg_with_size_ &&
        expected_width_ == last_update_rect_width_ &&
        expected_height_ == last_update_rect_height_) {
      waiting_for_update_rect_msg_with_size_ = false;
      if (send_message_loop_runner_)
        send_message_loop_runner_->Quit();
    } else if (!waiting_for_update_rect_msg_with_size_) {
      if (send_message_loop_runner_)
        send_message_loop_runner_->Quit();
    }
  }
  BrowserPluginGuest::SendMessageToEmbedder(msg);
}

void TestBrowserPluginGuest::WaitForUpdateRectMsg() {
  // Check if we already got any UpdateRect message.
  if (update_rect_count_ > 0)
    return;
  send_message_loop_runner_ = new MessageLoopRunner();
  send_message_loop_runner_->Run();
}

void TestBrowserPluginGuest::WaitForUpdateRectMsgWithSize(int width,
                                                          int height) {
  if (update_rect_count_ > 0 &&
      last_update_rect_width_ == width &&
      last_update_rect_height_ == height) {
    // We already saw this message.
    return;
  }
  waiting_for_update_rect_msg_with_size_ = true;
  expected_width_ = width;
  expected_height_ = height;

  send_message_loop_runner_ = new MessageLoopRunner();
  send_message_loop_runner_->Run();
}

void TestBrowserPluginGuest::RenderViewGone(base::TerminationStatus status) {
  crash_observed_ = true;
  LOG(INFO) << "Guest crashed";
  if (crash_message_loop_runner_)
    crash_message_loop_runner_->Quit();
  BrowserPluginGuest::RenderViewGone(status);
}

void TestBrowserPluginGuest::WaitForCrashed() {
  // Check if we already observed a guest crash, return immediately if so.
  if (crash_observed_)
    return;

  crash_message_loop_runner_ = new MessageLoopRunner();
  crash_message_loop_runner_->Run();
}

void TestBrowserPluginGuest::WaitForFocus() {
  if (focus_observed_)
    return;
  focus_message_loop_runner_ = new MessageLoopRunner();
  focus_message_loop_runner_->Run();
}

void TestBrowserPluginGuest::WaitForAdvanceFocus() {
  if (advance_focus_observed_)
    return;
  advance_focus_message_loop_runner_ = new MessageLoopRunner();
  advance_focus_message_loop_runner_->Run();
}

void TestBrowserPluginGuest::WaitUntilHidden() {
  if (was_hidden_observed_) {
    was_hidden_observed_ = false;
    return;
  }
  was_hidden_message_loop_runner_ = new MessageLoopRunner();
  was_hidden_message_loop_runner_->Run();
  was_hidden_observed_ = false;
}

void TestBrowserPluginGuest::SetFocus(bool focused) {
  focus_observed_ = true;
  if (focus_message_loop_runner_)
    focus_message_loop_runner_->Quit();
  BrowserPluginGuest::SetFocus(focused);
}

bool TestBrowserPluginGuest::ViewTakeFocus(bool reverse) {
  advance_focus_observed_ = true;
  if (advance_focus_message_loop_runner_)
    advance_focus_message_loop_runner_->Quit();
  return BrowserPluginGuest::ViewTakeFocus(reverse);
}

}  // namespace content
