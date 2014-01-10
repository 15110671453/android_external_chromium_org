// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_commands_chromeos.h"

#include "ash/accelerators/accelerator_controller.h"
#include "ash/screenshot_delegate.h"
#include "ash/shell.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/common/user_metrics_action.h"

using content::UserMetricsAction;

namespace chrome {

void TakeScreenshot() {
  content::RecordAction(UserMetricsAction("Menu_Take_Screenshot"));
  ash::ScreenshotDelegate* screenshot_delegate = ash::Shell::GetInstance()->
      accelerator_controller()->screenshot_delegate();
  if (screenshot_delegate &&
      screenshot_delegate->CanTakeScreenshot()) {
    screenshot_delegate->HandleTakeScreenshotForAllRootWindows();
  }
}

}  // namespace chrome
