// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/view_event_test_base.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/message_loop.h"
#include "base/string_number_conversions.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/ime/text_input_test_support.h"
#include "ui/compositor/test/compositor_test_support.h"
#include "ui/ui_controls/ui_controls.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#if defined(USE_ASH)
#include "ash/shell.h"
#endif
#if defined(USE_AURA)
#include "ui/aura/client/event_client.h"
#include "ui/aura/env.h"
#include "ui/aura/root_window.h"
#endif

namespace {

// View subclass that allows you to specify the preferred size.
class TestView : public views::View {
 public:
  TestView() {}

  void SetPreferredSize(const gfx::Size& size) {
    preferred_size_ = size;
    PreferredSizeChanged();
  }

  gfx::Size GetPreferredSize() {
    if (!preferred_size_.IsEmpty())
      return preferred_size_;
    return View::GetPreferredSize();
  }

  virtual void Layout() {
    View* child_view = child_at(0);
    child_view->SetBounds(0, 0, width(), height());
  }

 private:
  gfx::Size preferred_size_;

  DISALLOW_COPY_AND_ASSIGN(TestView);
};

// Delay in background thread before posting mouse move.
const int kMouseMoveDelayMS = 200;

}  // namespace

ViewEventTestBase::ViewEventTestBase()
  : window_(NULL),
    content_view_(NULL),
    ui_thread_(content::BrowserThread::UI, &message_loop_) {
}

void ViewEventTestBase::Done() {
  MessageLoop::current()->Quit();

#if defined(OS_WIN) && !defined(USE_AURA)
  // We need to post a message to tickle the Dispatcher getting called and
  // exiting out of the nested loop. Without this the quit never runs.
  if (window_)
    PostMessage(window_->GetNativeWindow(), WM_USER, 0, 0);
#endif

  // If we're in a nested message loop, as is the case with menus, we
  // need to quit twice. The second quit does that for us. Finish all
  // pending UI events before posting closure because events it may be
  // executed before UI events are executed.
  ui_controls::RunClosureAfterAllPendingUIEvents(MessageLoop::QuitClosure());
}

void ViewEventTestBase::SetUp() {
  ui::TextInputTestSupport::Initilaize();
  ui::CompositorTestSupport::Initialize();
#if defined(USE_ASH)
  ash::Shell::CreateInstance(NULL);
  // The shell runs with a locked screen in tests, so we must clear the event
  // client so it doesn't interfere with event propagation.
  aura::client::SetEventClient(ash::Shell::GetPrimaryRootWindow(),
                               NULL);
#endif
  window_ = views::Widget::CreateWindow(this);
}

void ViewEventTestBase::TearDown() {
  if (window_) {
#if defined(OS_WIN) && !defined(USE_AURA)
    DestroyWindow(window_->GetNativeWindow());
#else
    window_->Close();
    content::RunAllPendingInMessageLoop();
#endif
    window_ = NULL;
  }
#if defined(USE_ASH)
  ash::Shell::DeleteInstance();
#endif
#if defined(USE_AURA)
  aura::Env::DeleteInstance();
#endif
  ui::CompositorTestSupport::Terminate();
  ui::TextInputTestSupport::Shutdown();
}

bool ViewEventTestBase::CanResize() const {
  return true;
}

views::View* ViewEventTestBase::GetContentsView() {
  if (!content_view_) {
    // Wrap the real view (as returned by CreateContentsView) in a View so
    // that we can customize the preferred size.
    TestView* test_view = new TestView();
    test_view->SetPreferredSize(GetPreferredSize());
    test_view->AddChildView(CreateContentsView());
    content_view_ = test_view;
  }
  return content_view_;
}

const views::Widget* ViewEventTestBase::GetWidget() const {
  return content_view_->GetWidget();
}

views::Widget* ViewEventTestBase::GetWidget() {
  return content_view_->GetWidget();
}

ViewEventTestBase::~ViewEventTestBase() {
}

void ViewEventTestBase::StartMessageLoopAndRunTest() {
  window_->Show();
  // Make sure the window is the foreground window, otherwise none of the
  // mouse events are going to be targeted correctly.
#if defined(OS_WIN) && !defined(USE_AURA)
  SetForegroundWindow(window_->GetNativeWindow());
#endif

  // Flush any pending events to make sure we start with a clean slate.
  content::RunAllPendingInMessageLoop();

  // Schedule a task that starts the test. Need to do this as we're going to
  // run the message loop.
  MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&ViewEventTestBase::DoTestOnMessageLoop, this));

  content::RunMessageLoop();
}

gfx::Size ViewEventTestBase::GetPreferredSize() {
  return gfx::Size();
}

void ViewEventTestBase::ScheduleMouseMoveInBackground(int x, int y) {
  if (!dnd_thread_.get()) {
    dnd_thread_.reset(new base::Thread("mouse-move-thread"));
    dnd_thread_->Start();
  }
  dnd_thread_->message_loop()->PostDelayedTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(&ui_controls::SendMouseMove), x, y),
      base::TimeDelta::FromMilliseconds(kMouseMoveDelayMS));
}

void ViewEventTestBase::StopBackgroundThread() {
  dnd_thread_.reset(NULL);
}

void ViewEventTestBase::RunTestMethod(const base::Closure& task) {
  StopBackgroundThread();

  task.Run();
  if (HasFatalFailure())
    Done();
}
