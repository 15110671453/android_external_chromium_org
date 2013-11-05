// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEPS_TRAY_STEP_H_
#define CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEPS_TRAY_STEP_H_

#include "base/compiler_specific.h"
#include "chrome/browser/chromeos/first_run/step.h"

namespace chromeos {
namespace first_run {

class TrayStep : public Step {
 public:
  TrayStep(ash::FirstRunHelper* shell_helper, FirstRunActor* actor);

  // Overriden from Step.
  virtual void Show() OVERRIDE;
};

}  // namespace first_run
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FIRST_RUN_STEPS_TRAY_STEP_H_

