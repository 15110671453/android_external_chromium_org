// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_utils.h"

#include <stdio.h>
#include <stdlib.h>
#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"

const int kActionTimeoutMs = 10000;

const PPB_Testing_Dev* GetTestingInterface() {
  static const PPB_Testing_Dev* g_testing_interface =
      static_cast<const PPB_Testing_Dev*>(
          pp::Module::Get()->GetBrowserInterface(PPB_TESTING_DEV_INTERFACE));
  return g_testing_interface;
}

std::string ReportError(const char* method, int32_t error) {
  char error_as_string[12];
  sprintf(error_as_string, "%d", static_cast<int>(error));
  std::string result = method + std::string(" failed with error: ") +
      error_as_string;
  return result;
}

void PlatformSleep(int duration_ms) {
#if defined(_MSC_VER)
  ::Sleep(duration_ms);
#else
  usleep(duration_ms * 1000);
#endif
}

bool GetLocalHostPort(PP_Instance instance, std::string* host, uint16_t* port) {
  if (!host || !port)
    return false;

  const PPB_Testing_Dev* testing = GetTestingInterface();
  if (!testing)
    return false;

  PP_URLComponents_Dev components;
  pp::Var pp_url(pp::PASS_REF,
                 testing->GetDocumentURL(instance, &components));
  if (!pp_url.is_string())
    return false;
  std::string url = pp_url.AsString();

  if (components.host.len < 0)
    return false;
  host->assign(url.substr(components.host.begin, components.host.len));

  if (components.port.len <= 0)
    return false;

  int i = atoi(url.substr(components.port.begin, components.port.len).c_str());
  if (i < 0 || i > 65535)
    return false;
  *port = static_cast<uint16_t>(i);

  return true;
}

void NestedEvent::Wait() {
  PP_DCHECK(pp::Module::Get()->core()->IsMainThread());
  // Don't allow nesting more than once; it doesn't work with the code as-is,
  // and probably is a bad idea most of the time anyway.
  PP_DCHECK(!waiting_);
  if (signalled_)
    return;
  waiting_ = true;
  while (!signalled_)
    GetTestingInterface()->RunMessageLoop(instance_);
  waiting_ = false;
}

void NestedEvent::Signal() {
  if (pp::Module::Get()->core()->IsMainThread())
    SignalOnMainThread();
  else
    PostSignal(0);
}

void NestedEvent::PostSignal(int32_t wait_ms) {
  pp::Module::Get()->core()->CallOnMainThread(
      wait_ms,
      pp::CompletionCallback(&SignalThunk, this),
      0);
}

void NestedEvent::Reset() {
  PP_DCHECK(pp::Module::Get()->core()->IsMainThread());
  // It doesn't make sense to reset when we're still waiting.
  PP_DCHECK(!waiting_);
  signalled_ = false;
}

void NestedEvent::SignalOnMainThread() {
  PP_DCHECK(pp::Module::Get()->core()->IsMainThread());
  signalled_ = true;
  if (waiting_)
    GetTestingInterface()->QuitMessageLoop(instance_);
}

void NestedEvent::SignalThunk(void* event, int32_t /* result */) {
  static_cast<NestedEvent*>(event)->SignalOnMainThread();
}

TestCompletionCallback::TestCompletionCallback(PP_Instance instance)
    : wait_for_result_called_(false),
      have_result_(false),
      result_(PP_OK_COMPLETIONPENDING),
      // TODO(dmichael): The default should probably be PP_REQUIRED, but this is
      //                 what the tests currently expect.
      callback_type_(PP_OPTIONAL),
      post_quit_task_(false),
      instance_(instance),
      delegate_(NULL) {
}

TestCompletionCallback::TestCompletionCallback(PP_Instance instance,
                                               bool force_async)
    : wait_for_result_called_(false),
      have_result_(false),
      result_(PP_OK_COMPLETIONPENDING),
      callback_type_(force_async ? PP_REQUIRED : PP_OPTIONAL),
      post_quit_task_(false),
      instance_(instance),
      delegate_(NULL) {
}

TestCompletionCallback::TestCompletionCallback(PP_Instance instance,
                                               CallbackType callback_type)
    : wait_for_result_called_(false),
      have_result_(false),
      result_(PP_OK_COMPLETIONPENDING),
      callback_type_(callback_type),
      post_quit_task_(false),
      instance_(instance),
      delegate_(NULL) {
}

int32_t TestCompletionCallback::WaitForResult() {
  PP_DCHECK(!wait_for_result_called_);
  wait_for_result_called_ = true;
  errors_.clear();
  if (!have_result_) {
    post_quit_task_ = true;
    RunMessageLoop();
  }
  return result_;
}

void TestCompletionCallback::WaitForResult(int32_t result) {
  PP_DCHECK(!wait_for_result_called_);
  wait_for_result_called_ = true;
  errors_.clear();
  if (result == PP_OK_COMPLETIONPENDING) {
    if (!have_result_) {
      post_quit_task_ = true;
      RunMessageLoop();
    }
    if (callback_type_ == PP_BLOCKING) {
      errors_.assign(
          ReportError("TestCompletionCallback: Call did not run synchronously "
                      "when passed a blocking completion callback!",
                      result_));
      return;
    }
  } else {
    result_ = result;
    have_result_ = true;
    if (callback_type_ == PP_REQUIRED) {
      errors_.assign(
          ReportError("TestCompletionCallback: Call ran synchronously when "
                      "passed a required completion callback!",
                      result_));
      return;
    }
  }
  PP_DCHECK(have_result_ == true);
}

void TestCompletionCallback::WaitForAbortResult(int32_t result) {
  WaitForResult(result);
  int32_t final_result = result_;
  if (result == PP_OK_COMPLETIONPENDING) {
    if (final_result != PP_ERROR_ABORTED) {
      errors_.assign(
          ReportError("TestCompletionCallback: Expected PP_ERROR_ABORTED or "
                      "PP_OK. Ran asynchronously.",
                      final_result));
      return;
    }
  } else if (result < PP_OK) {
    errors_.assign(
        ReportError("TestCompletionCallback: Expected PP_ERROR_ABORTED or "
                    "non-error response. Ran synchronously.",
                    result));
    return;
  }
}

pp::CompletionCallback TestCompletionCallback::GetCallback() {
  Reset();
  int32_t flags = 0;
  if (callback_type_ == PP_BLOCKING)
    return pp::CompletionCallback();
  else if (callback_type_ == PP_OPTIONAL)
    flags = PP_COMPLETIONCALLBACK_FLAG_OPTIONAL;
  target_loop_ = pp::MessageLoop::GetCurrent();
  return pp::CompletionCallback(&TestCompletionCallback::Handler,
                                const_cast<TestCompletionCallback*>(this),
                                flags);
}

void TestCompletionCallback::Reset() {
  wait_for_result_called_ = false;
  result_ = PP_OK_COMPLETIONPENDING;
  have_result_ = false;
  post_quit_task_ = false;
  delegate_ = NULL;
  errors_.clear();
}

// static
void TestCompletionCallback::Handler(void* user_data, int32_t result) {
  TestCompletionCallback* callback =
      static_cast<TestCompletionCallback*>(user_data);
  // If this check fails, it means that the callback was invoked twice or that
  // the PPAPI call completed synchronously, but also ran the callback.
  PP_DCHECK(!callback->have_result_);
  callback->result_ = result;
  callback->have_result_ = true;
  if (callback->delegate_)
    callback->delegate_->OnCallback(user_data, result);
  if (callback->post_quit_task_) {
    callback->post_quit_task_ = false;
    callback->QuitMessageLoop();
  }
  if (callback->target_loop_ != pp::MessageLoop::GetCurrent()) {
    // Note, in-process, loop_ and GetCurrent() will both be NULL, so should
    // still be equal.
    callback->errors_.assign(
        ReportError("TestCompletionCallback: Callback ran on the wrong message "
                    "loop!",
                    result));
  }
}

void TestCompletionCallback::RunMessageLoop() {
  pp::MessageLoop loop(pp::MessageLoop::GetCurrent());
  // If we don't have a message loop, we're probably running in process, where
  // PPB_MessageLoop is not supported. Just use the Testing message loop.
  if (loop.is_null() || loop == pp::MessageLoop::GetForMainThread())
    GetTestingInterface()->RunMessageLoop(instance_);
  else
    loop.Run();
}

void TestCompletionCallback::QuitMessageLoop() {
  pp::MessageLoop loop(pp::MessageLoop::GetCurrent());
  // If we don't have a message loop, we're probably running in process, where
  // PPB_MessageLoop is not supported. Just use the Testing message loop.
  if (loop.is_null() || loop == pp::MessageLoop::GetForMainThread()) {
    GetTestingInterface()->QuitMessageLoop(instance_);
  } else {
    const bool should_quit = false;
    loop.PostQuit(should_quit);
  }
}
