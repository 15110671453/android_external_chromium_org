// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_UI_H_
#pragma once

#include "base/memory/weak_ptr.h"
#include "content/browser/webui/web_ui.h"
#include "content/public/browser/web_ui_controller.h"

class PortalFrameLoadObserver;
// A custom WebUI that defines datasource for mobile setup registration page
// that is used in Chrome OS activate modem and perform plan subscription tasks.
class MobileSetupUI : public WebUI,
                      public content::WebUIController,
                      public base::SupportsWeakPtr<MobileSetupUI> {
 public:
  explicit MobileSetupUI(content::WebContents* contents);

  void OnObserverDeleted();

 private:
  // WebUIController overrides.
  virtual void RenderViewCreated(RenderViewHost* render_view_host) OVERRIDE;

  PortalFrameLoadObserver* frame_load_observer_;
  DISALLOW_COPY_AND_ASSIGN(MobileSetupUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_UI_H_
