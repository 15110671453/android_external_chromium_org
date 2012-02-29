// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_TEST_SHELL_DELEGATE_H_
#define ASH_TEST_TEST_SHELL_DELEGATE_H_
#pragma once

#include "ash/shell_delegate.h"
#include "base/compiler_specific.h"

namespace ash {
namespace test {

class TestShellDelegate : public ShellDelegate {
 public:
  TestShellDelegate();
  virtual ~TestShellDelegate();

  void SetOverrideWindowMode(Shell::WindowMode window_mode);

  // Overridden from ShellDelegate:
  virtual views::Widget* CreateStatusArea() OVERRIDE;
#if defined(OS_CHROMEOS)
  virtual void LockScreen() OVERRIDE;
#endif
  virtual void Exit() OVERRIDE;
  virtual void BuildAppListModel(AppListModel* model) OVERRIDE;
  virtual AppListViewDelegate* CreateAppListViewDelegate() OVERRIDE;
  virtual std::vector<aura::Window*> GetCycleWindowList(
      CycleSource source,
      CycleOrder order) const OVERRIDE;
  virtual void StartPartialScreenshot(
      ScreenshotDelegate* screenshot_delegate) OVERRIDE;
  virtual LauncherDelegate* CreateLauncherDelegate(
      ash::LauncherModel* model) OVERRIDE;
  virtual SystemTrayDelegate* CreateSystemTrayDelegate(SystemTray* t) OVERRIDE;
  virtual bool GetOverrideWindowMode(Shell::WindowMode* window_mode) OVERRIDE;

 private:
  bool override_window_mode_;
  Shell::WindowMode window_mode_;

  DISALLOW_COPY_AND_ASSIGN(TestShellDelegate);
};

}  // namespace test
}  // namespace ash

#endif  // ASH_TEST_TEST_SHELL_DELEGATE_H_
