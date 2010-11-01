// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/automation/automation_provider.h"

#include "app/keyboard_codes.h"
#include "base/debug/trace_event.h"
#include "base/json/json_reader.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/automation/automation_browser_tracker.h"
#include "chrome/browser/automation/automation_extension_function.h"
#include "chrome/browser/automation/automation_tab_tracker.h"
#include "chrome/browser/automation/automation_window_tracker.h"
#include "chrome/browser/automation/extension_automation_constants.h"
#include "chrome/browser/automation/extension_port_container.h"
#include "chrome/browser/automation/ui_controls.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/extensions/extension_event_router.h"
#include "chrome/browser/external_tab_container_win.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/views/bookmark_bar_view.h"
#include "chrome/common/page_zoom.h"
#include "chrome/test/automation/automation_messages.h"
#include "views/focus/accelerator_handler.h"
#include "views/widget/root_view.h"
#include "views/widget/widget_win.h"
#include "views/window/window.h"

// This task just adds another task to the event queue.  This is useful if
// you want to ensure that any tasks added to the event queue after this one
// have already been processed by the time |task| is run.
class InvokeTaskLaterTask : public Task {
 public:
  explicit InvokeTaskLaterTask(Task* task) : task_(task) {}
  virtual ~InvokeTaskLaterTask() {}

  virtual void Run() {
    MessageLoop::current()->PostTask(FROM_HERE, task_);
  }

 private:
  Task* task_;

  DISALLOW_COPY_AND_ASSIGN(InvokeTaskLaterTask);
};

static void MoveMouse(const POINT& point) {
  SetCursorPos(point.x, point.y);

  // Now, make sure that GetMessagePos returns the values we just set by
  // simulating a mouse move.  The value returned by GetMessagePos is updated
  // when a mouse move event is removed from the event queue.
  PostMessage(NULL, WM_MOUSEMOVE, 0, MAKELPARAM(point.x, point.y));
  MSG msg;
  while (PeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE)) {
  }

  // Verify
#ifndef NDEBUG
  DWORD pos = GetMessagePos();
  gfx::Point cursor_point(pos);
  DCHECK_EQ(point.x, cursor_point.x());
  DCHECK_EQ(point.y, cursor_point.y());
#endif
}

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM l_param) {
  if (hwnd == reinterpret_cast<HWND>(l_param)) {
    return FALSE;
  }
  return TRUE;
}

// This task enqueues a mouse event on the event loop, so that the view
// that it's being sent to can do the requisite post-processing.
class MouseEventTask : public Task {
 public:
  MouseEventTask(views::View* view,
                 views::Event::EventType type,
                 const gfx::Point& point,
                 int flags)
      : view_(view), type_(type), point_(point), flags_(flags) {}
  virtual ~MouseEventTask() {}

  virtual void Run() {
    views::MouseEvent event(type_, point_.x(), point_.y(), flags_);
    // We need to set the cursor position before we process the event because
    // some code (tab dragging, for instance) queries the actual cursor location
    // rather than the location of the mouse event. Note that the reason why
    // the drag code moved away from using mouse event locations was because
    // our conversion to screen location doesn't work well with multiple
    // monitors, so this only works reliably in a single monitor setup.
    gfx::Point screen_location(point_.x(), point_.y());
    view_->ConvertPointToScreen(view_, &screen_location);
    MoveMouse(screen_location.ToPOINT());
    switch (type_) {
      case views::Event::ET_MOUSE_PRESSED:
        view_->OnMousePressed(event);
        break;

      case views::Event::ET_MOUSE_DRAGGED:
        view_->OnMouseDragged(event);
        break;

      case views::Event::ET_MOUSE_RELEASED:
        view_->OnMouseReleased(event, false);
        break;

      default:
        NOTREACHED();
    }
  }

 private:
  views::View* view_;
  views::Event::EventType type_;
  gfx::Point point_;
  int flags_;

  DISALLOW_COPY_AND_ASSIGN(MouseEventTask);
};

// This task sends a WindowDragResponse message with the appropriate
// routing ID to the automation proxy.  This is implemented as a task so that
// we know that the mouse events (and any tasks that they spawn on the message
// loop) have been processed by the time this is sent.
class WindowDragResponseTask : public Task {
 public:
  WindowDragResponseTask(AutomationProvider* provider,
                         IPC::Message* reply_message)
      : provider_(provider), reply_message_(reply_message) {}
  virtual ~WindowDragResponseTask() {}

  virtual void Run() {
    DCHECK(reply_message_ != NULL);
    AutomationMsg_WindowDrag::WriteReplyParams(reply_message_, true);
    provider_->Send(reply_message_);
  }

 private:
  AutomationProvider* provider_;
  IPC::Message* reply_message_;

  DISALLOW_COPY_AND_ASSIGN(WindowDragResponseTask);
};

void AutomationProvider::WindowSimulateDrag(int handle,
                                            std::vector<gfx::Point> drag_path,
                                            int flags,
                                            bool press_escape_en_route,
                                            IPC::Message* reply_message) {
  if (browser_tracker_->ContainsHandle(handle) && (drag_path.size() > 1)) {
    gfx::NativeWindow window =
        browser_tracker_->GetResource(handle)->window()->GetNativeHandle();

    UINT down_message = 0;
    UINT up_message = 0;
    WPARAM wparam_flags = 0;
    if (flags & views::Event::EF_SHIFT_DOWN)
      wparam_flags |= MK_SHIFT;
    if (flags & views::Event::EF_CONTROL_DOWN)
      wparam_flags |= MK_CONTROL;
    if (flags & views::Event::EF_LEFT_BUTTON_DOWN) {
      wparam_flags |= MK_LBUTTON;
      down_message = WM_LBUTTONDOWN;
      up_message = WM_LBUTTONUP;
    }
    if (flags & views::Event::EF_MIDDLE_BUTTON_DOWN) {
      wparam_flags |= MK_MBUTTON;
      down_message = WM_MBUTTONDOWN;
      up_message = WM_MBUTTONUP;
    }
    if (flags & views::Event::EF_RIGHT_BUTTON_DOWN) {
      wparam_flags |= MK_RBUTTON;
      down_message = WM_LBUTTONDOWN;
      up_message = WM_LBUTTONUP;
    }

    Browser* browser = browser_tracker_->GetResource(handle);
    DCHECK(browser);
    HWND top_level_hwnd =
        reinterpret_cast<HWND>(browser->window()->GetNativeHandle());
    POINT temp = drag_path[0].ToPOINT();
    MapWindowPoints(top_level_hwnd, HWND_DESKTOP, &temp, 1);
    MoveMouse(temp);
    SendMessage(top_level_hwnd, down_message, wparam_flags,
                MAKELPARAM(drag_path[0].x(), drag_path[0].y()));
    for (int i = 1; i < static_cast<int>(drag_path.size()); ++i) {
      temp = drag_path[i].ToPOINT();
      MapWindowPoints(top_level_hwnd, HWND_DESKTOP, &temp, 1);
      MoveMouse(temp);
      SendMessage(top_level_hwnd, WM_MOUSEMOVE, wparam_flags,
                  MAKELPARAM(drag_path[i].x(), drag_path[i].y()));
    }
    POINT end = drag_path[drag_path.size() - 1].ToPOINT();
    MapWindowPoints(top_level_hwnd, HWND_DESKTOP, &end, 1);
    MoveMouse(end);

    if (press_escape_en_route) {
      // Press Escape, making sure we wait until chrome processes the escape.
      // TODO(phajdan.jr): make this use ui_test_utils::SendKeyPressSync.
      ui_controls::SendKeyPressNotifyWhenDone(
          window, app::VKEY_ESCAPE,
          ((flags & views::Event::EF_CONTROL_DOWN) ==
           views::Event::EF_CONTROL_DOWN),
          ((flags & views::Event::EF_SHIFT_DOWN) ==
           views::Event::EF_SHIFT_DOWN),
          ((flags & views::Event::EF_ALT_DOWN) == views::Event::EF_ALT_DOWN),
          false,
          new MessageLoop::QuitTask());
      MessageLoopForUI* loop = MessageLoopForUI::current();
      bool did_allow_task_nesting = loop->NestableTasksAllowed();
      loop->SetNestableTasksAllowed(true);
      views::AcceleratorHandler handler;
      loop->Run(&handler);
      loop->SetNestableTasksAllowed(did_allow_task_nesting);
    }
    SendMessage(top_level_hwnd, up_message, wparam_flags,
                MAKELPARAM(end.x, end.y));

    MessageLoop::current()->PostTask(FROM_HERE, new InvokeTaskLaterTask(
        new WindowDragResponseTask(this, reply_message)));
  } else {
    AutomationMsg_WindowDrag::WriteReplyParams(reply_message, false);
    Send(reply_message);
  }
}

void AutomationProvider::GetTabHWND(int handle, HWND* tab_hwnd) {
  *tab_hwnd = NULL;

  if (tab_tracker_->ContainsHandle(handle)) {
    NavigationController* tab = tab_tracker_->GetResource(handle);
    *tab_hwnd = tab->tab_contents()->GetNativeView();
  }
}

void AutomationProvider::CreateExternalTab(
    const IPC::ExternalTabSettings& settings,
    gfx::NativeWindow* tab_container_window, gfx::NativeWindow* tab_window,
    int* tab_handle) {
  TRACE_EVENT_BEGIN("AutomationProvider::CreateExternalTab", 0, "");

  *tab_handle = 0;
  *tab_container_window = NULL;
  *tab_window = NULL;
  scoped_refptr<ExternalTabContainer> external_tab_container =
      new ExternalTabContainer(this, automation_resource_message_filter_);

  Profile* profile = settings.is_off_the_record ?
      profile_->GetOffTheRecordProfile() : profile_;

  // When the ExternalTabContainer window is created we grab a reference on it
  // which is released when the window is destroyed.
  external_tab_container->Init(profile, settings.parent, settings.dimensions,
      settings.style, settings.load_requests_via_automation,
      settings.handle_top_level_requests, NULL, settings.initial_url,
      settings.referrer, settings.infobars_enabled,
      settings.route_all_top_level_navigations);

  if (AddExternalTab(external_tab_container)) {
    TabContents* tab_contents = external_tab_container->tab_contents();
    *tab_handle = external_tab_container->tab_handle();
    *tab_container_window = external_tab_container->GetNativeView();
    *tab_window = tab_contents->GetNativeView();
  } else {
    external_tab_container->Uninitialize();
  }

  TRACE_EVENT_END("AutomationProvider::CreateExternalTab", 0, "");
}

bool AutomationProvider::AddExternalTab(ExternalTabContainer* external_tab) {
  DCHECK(external_tab != NULL);

  TabContents* tab_contents = external_tab->tab_contents();
  if (tab_contents) {
    int tab_handle = tab_tracker_->Add(&tab_contents->controller());
    external_tab->SetTabHandle(tab_handle);
    return true;
  }

  return false;
}

void AutomationProvider::ProcessUnhandledAccelerator(
    const IPC::Message& message, int handle, const MSG& msg) {
  ExternalTabContainer* external_tab = GetExternalTabForHandle(handle);
  if (external_tab) {
    external_tab->ProcessUnhandledAccelerator(msg);
  }
  // This message expects no response.
}

void AutomationProvider::SetInitialFocus(const IPC::Message& message,
                                         int handle, bool reverse,
                                         bool restore_focus_to_view) {
  ExternalTabContainer* external_tab = GetExternalTabForHandle(handle);
  if (external_tab) {
    external_tab->FocusThroughTabTraversal(reverse, restore_focus_to_view);
  }
  // This message expects no response.
}

void AutomationProvider::PrintAsync(int tab_handle) {
  NavigationController* tab = NULL;
  TabContents* tab_contents = GetTabContentsForHandle(tab_handle, &tab);
  if (tab_contents) {
    if (tab_contents->PrintNow())
      return;
  }
}

ExternalTabContainer* AutomationProvider::GetExternalTabForHandle(int handle) {
  if (tab_tracker_->ContainsHandle(handle)) {
    NavigationController* tab = tab_tracker_->GetResource(handle);
    return ExternalTabContainer::GetContainerForTab(
        tab->tab_contents()->GetNativeView());
  }

  return NULL;
}

void AutomationProvider::OnTabReposition(
    int tab_handle, const IPC::Reposition_Params& params) {
  if (!tab_tracker_->ContainsHandle(tab_handle))
    return;

  if (!IsWindow(params.window))
    return;

  unsigned long process_id = 0;
  unsigned long thread_id = 0;

  thread_id = GetWindowThreadProcessId(params.window, &process_id);

  if (thread_id != GetCurrentThreadId()) {
    DCHECK_EQ(thread_id, GetCurrentThreadId());
    return;
  }

  SetWindowPos(params.window, params.window_insert_after, params.left,
               params.top, params.width, params.height, params.flags);

  if (params.set_parent) {
    if (IsWindow(params.parent_window)) {
      if (!SetParent(params.window, params.parent_window))
        DLOG(WARNING) << "SetParent failed. Error 0x%x" << GetLastError();
    }
  }
}

void AutomationProvider::OnForwardContextMenuCommandToChrome(int tab_handle,
                                                             int command) {
  if (tab_tracker_->ContainsHandle(tab_handle)) {
    NavigationController* tab = tab_tracker_->GetResource(tab_handle);
    if (!tab) {
      NOTREACHED();
      return;
    }

    TabContents* tab_contents = tab->tab_contents();
    if (!tab_contents || !tab_contents->delegate()) {
      NOTREACHED();
      return;
    }

    tab_contents->delegate()->ExecuteContextMenuCommand(command);
  }
}

void AutomationProvider::ConnectExternalTab(
    uint64 cookie,
    bool allow,
    gfx::NativeWindow parent_window,
    gfx::NativeWindow* tab_container_window,
    gfx::NativeWindow* tab_window,
    int* tab_handle) {
  TRACE_EVENT_BEGIN("AutomationProvider::ConnectExternalTab", 0, "");

  *tab_handle = 0;
  *tab_container_window = NULL;
  *tab_window = NULL;

  scoped_refptr<ExternalTabContainer> external_tab_container =
      ExternalTabContainer::RemovePendingTab(static_cast<uintptr_t>(cookie));
  if (!external_tab_container.get()) {
    NOTREACHED();
    return;
  }

  if (allow && AddExternalTab(external_tab_container)) {
    external_tab_container->Reinitialize(this,
                                         automation_resource_message_filter_,
                                         parent_window);
    TabContents* tab_contents = external_tab_container->tab_contents();
    *tab_handle = external_tab_container->tab_handle();
    *tab_container_window = external_tab_container->GetNativeView();
    *tab_window = tab_contents->GetNativeView();
  } else {
    external_tab_container->Uninitialize();
  }

  TRACE_EVENT_END("AutomationProvider::ConnectExternalTab", 0, "");
}

void AutomationProvider::SetEnableExtensionAutomation(
    int tab_handle,
    const std::vector<std::string>& functions_enabled) {
  ExternalTabContainer* external_tab = GetExternalTabForHandle(tab_handle);
  if (external_tab) {
    external_tab->SetEnableExtensionAutomation(functions_enabled);
  } else {
    // Tab must exist, and must be an external tab so that its
    // delegate has an on-empty
    // implementation of ForwardMessageToExternalHost.
    DLOG(WARNING) <<
      "SetEnableExtensionAutomation called with invalid tab handle.";
  }
}

void AutomationProvider::OnBrowserMoved(int tab_handle) {
  ExternalTabContainer* external_tab = GetExternalTabForHandle(tab_handle);
  if (external_tab) {
    external_tab->WindowMoved();
  } else {
    DLOG(WARNING) <<
      "AutomationProvider::OnBrowserMoved called with invalid tab handle.";
  }
}

void AutomationProvider::OnMessageFromExternalHost(int handle,
                                                   const std::string& message,
                                                   const std::string& origin,
                                                   const std::string& target) {
  RenderViewHost* view_host = GetViewForTab(handle);
  if (!view_host)
    return;

  if (AutomationExtensionFunction::InterceptMessageFromExternalHost(
          view_host, message, origin, target)) {
    // Message was diverted.
    return;
  }

  if (ExtensionPortContainer::InterceptMessageFromExternalHost(
          message, origin, target, this, view_host, handle)) {
    // Message was diverted.
    return;
  }

  if (InterceptBrowserEventMessageFromExternalHost(message, origin, target)) {
    // Message was diverted.
    return;
  }

  view_host->ForwardMessageFromExternalHost(message, origin, target);
}

bool AutomationProvider::InterceptBrowserEventMessageFromExternalHost(
      const std::string& message, const std::string& origin,
      const std::string& target) {
  if (target !=
      extension_automation_constants::kAutomationBrowserEventRequestTarget)
    return false;

  if (origin != extension_automation_constants::kAutomationOrigin) {
    LOG(WARNING) << "Wrong origin on automation browser event " << origin;
    return false;
  }

  // The message is a JSON-encoded array with two elements, both strings. The
  // first is the name of the event to dispatch.  The second is a JSON-encoding
  // of the arguments specific to that event.
  scoped_ptr<Value> message_value(base::JSONReader::Read(message, false));
  if (!message_value.get() || !message_value->IsType(Value::TYPE_LIST)) {
    LOG(WARNING) << "Invalid browser event specified through automation";
    return false;
  }

  const ListValue* args = static_cast<const ListValue*>(message_value.get());

  std::string event_name;
  if (!args->GetString(0, &event_name)) {
    LOG(WARNING) << "No browser event name specified through automation";
    return false;
  }

  std::string json_args;
  if (!args->GetString(1, &json_args)) {
    LOG(WARNING) << "No browser event args specified through automation";
    return false;
  }

  if (profile()->GetExtensionEventRouter()) {
    profile()->GetExtensionEventRouter()->DispatchEventToRenderers(
        event_name, json_args, profile(), GURL());
  }

  return true;
}

void AutomationProvider::NavigateInExternalTab(
    int handle, const GURL& url, const GURL& referrer,
    AutomationMsg_NavigationResponseValues* status) {
  *status = AUTOMATION_MSG_NAVIGATION_ERROR;

  if (tab_tracker_->ContainsHandle(handle)) {
    NavigationController* tab = tab_tracker_->GetResource(handle);
    tab->LoadURL(url, referrer, PageTransition::TYPED);
    *status = AUTOMATION_MSG_NAVIGATION_SUCCESS;
  }
}

void AutomationProvider::NavigateExternalTabAtIndex(
    int handle, int navigation_index,
    AutomationMsg_NavigationResponseValues* status) {
  *status = AUTOMATION_MSG_NAVIGATION_ERROR;

  if (tab_tracker_->ContainsHandle(handle)) {
    NavigationController* tab = tab_tracker_->GetResource(handle);
    tab->GoToIndex(navigation_index);
    *status = AUTOMATION_MSG_NAVIGATION_SUCCESS;
  }
}

void AutomationProvider::OnRunUnloadHandlers(
    int handle, IPC::Message* reply_message) {
  ExternalTabContainer* external_tab = GetExternalTabForHandle(handle);
  if (external_tab) {
    external_tab->RunUnloadHandlers(reply_message);
  }
}

void AutomationProvider::OnSetZoomLevel(int handle, int zoom_level) {
  if (tab_tracker_->ContainsHandle(handle)) {
    NavigationController* tab = tab_tracker_->GetResource(handle);
    if (tab->tab_contents() && tab->tab_contents()->render_view_host()) {
      tab->tab_contents()->render_view_host()->Zoom(
          static_cast<PageZoom::Function>(zoom_level));
    }
  }
}
