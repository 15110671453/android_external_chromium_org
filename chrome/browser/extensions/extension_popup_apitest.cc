// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_switches.h"

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, Popup) {
  CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableExperimentalExtensionApis);
  ASSERT_TRUE(RunExtensionTest("popup_api")) << message_;
}
