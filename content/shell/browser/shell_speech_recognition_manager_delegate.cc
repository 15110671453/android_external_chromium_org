// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_speech_recognition_manager_delegate.h"

#include "content/public/browser/browser_thread.h"

using base::Callback;

namespace content {

void ShellSpeechRecognitionManagerDelegate::CheckRecognitionIsAllowed(
    int session_id, Callback<void(bool ask_user, bool is_allowed)> callback) {
  // In content_shell, we expect speech recognition to happen when requested.
  // Therefore we simply authorize it by calling back with is_allowed=true. The
  // first parameter, ask_user, is set to false because we don't want to prompt
  // the user for permission with an infobar.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE, base::Bind(callback, false, true));
}

SpeechRecognitionEventListener*
    ShellSpeechRecognitionManagerDelegate::GetEventListener() {
  return NULL;
}

bool ShellSpeechRecognitionManagerDelegate::FilterProfanities(
    int render_process_id) {
  return false;
}

}  // namespace content
