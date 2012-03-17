// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/desktop_win.h"

#include <vector>

#include "base/logging.h"

namespace remoting {

DesktopWin::DesktopWin(HDESK desktop, bool own) : desktop_(desktop), own_(own) {
}

DesktopWin::~DesktopWin() {
  if (own_ && desktop_ != NULL) {
    if (!::CloseDesktop(desktop_)) {
      LOG_GETLASTERROR(ERROR)
          << "Failed to close the owned desktop handle";
    }
  }
}

bool DesktopWin::GetName(string16* desktop_name_out) const {
  if (desktop_ == NULL)
    return false;

  DWORD length;
  CHECK(!GetUserObjectInformationW(desktop_, UOI_NAME, NULL, 0, &length));
  CHECK(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

  length /= sizeof(char16);
  std::vector<char16> buffer(length);
  if (!GetUserObjectInformationW(desktop_, UOI_NAME, &buffer[0],
                                 length * sizeof(char16), &length)) {
    LOG_GETLASTERROR(ERROR)
        << "Failed to query the desktop name";
    return false;
  }

  desktop_name_out->assign(&buffer[0], length / sizeof(char16));
  return true;
}

bool DesktopWin::IsSame(const DesktopWin& other) const {
  string16 name;
  if (!GetName(&name))
    return false;

  string16 other_name;
  if (!other.GetName(&other_name))
    return false;

  return name == other_name;
}

bool DesktopWin::SetThreadDesktop() const {
  if (!::SetThreadDesktop(desktop_)) {
    LOG_GETLASTERROR(ERROR)
        << "Failed to assign the desktop to the current thread";
    return false;
  }

  return true;
}

scoped_ptr<DesktopWin> DesktopWin::GetInputDesktop() {
  HDESK desktop = OpenInputDesktop(
                      0, FALSE, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
  if (desktop == NULL) {
    LOG_GETLASTERROR(ERROR)
        << "Failed to open the desktop receiving user input";
    return scoped_ptr<DesktopWin>();
  }

  return scoped_ptr<DesktopWin>(new DesktopWin(desktop, true));
}

scoped_ptr<DesktopWin> DesktopWin::GetThreadDesktop() {
  HDESK desktop = ::GetThreadDesktop(GetCurrentThreadId());
  if (desktop == NULL) {
    LOG_GETLASTERROR(ERROR)
        << "Failed to retrieve the handle of the desktop assigned to "
           "the current thread";
    return scoped_ptr<DesktopWin>();
  }

  return scoped_ptr<DesktopWin>(new DesktopWin(desktop, false));
}

}  // namespace remoting
