// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/webrtc_internals_ui.h"

#include "content/browser/media/webrtc_internals_message_handler.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "grit/content_resources.h"

namespace content {
namespace {

WebUIDataSource* CreateWebRTCInternalsHTMLSource() {
  WebUIDataSource* source =
      WebUIDataSource::Create(kChromeUIWebRTCInternalsHost);

  source->SetJsonPath("strings.js");
  source->AddResourcePath("webrtc_internals.js", IDR_WEBRTC_INTERNALS_JS);
  source->SetDefaultResource(IDR_WEBRTC_INTERNALS_HTML);
  return source;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
//
// WebRTCInternalsUI
//
////////////////////////////////////////////////////////////////////////////////

WebRTCInternalsUI::WebRTCInternalsUI(WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->AddMessageHandler(new WebRTCInternalsMessageHandler());

  BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  WebUIDataSource::Add(browser_context, CreateWebRTCInternalsHTMLSource());
}

}  // namespace content
