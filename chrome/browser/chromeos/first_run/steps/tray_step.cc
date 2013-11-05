// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/first_run/steps/tray_step.h"

#include "ash/first_run/first_run_helper.h"
#include "base/i18n/rtl.h"
#include "chrome/browser/chromeos/first_run/step_names.h"
#include "chrome/browser/ui/webui/chromeos/first_run/first_run_actor.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

namespace chromeos {
namespace first_run {

TrayStep::TrayStep(ash::FirstRunHelper* shell_helper, FirstRunActor* actor)
    : Step(kTrayStep, shell_helper, actor) {
}

void TrayStep::Show() {
  if (!shell_helper()->IsTrayBubbleOpened())
    shell_helper()->OpenTrayBubble();
  gfx::Rect bounds = shell_helper()->GetTrayBubbleBounds();
  actor()->AddRectangularHole(bounds.x(), bounds.y(), bounds.width(),
      bounds.height());
  FirstRunActor::StepPosition position;
  position.SetTop(bounds.y());
  if (!base::i18n::IsRTL())
    position.SetRight(GetOverlaySize().width() - bounds.x());
  else
    position.SetLeft(bounds.right());
  actor()->ShowStepPositioned(name(), position);
}

}  // namespace first_run
}  // namespace chromeos

