// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/renderer/aw_content_renderer_client.h"

#include "android_webview/common/url_constants.h"
#include "android_webview/renderer/aw_render_view_ext.h"
#include "base/utf_string_conversions.h"
#include "content/public/renderer/render_thread.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebString.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebSecurityPolicy.h"

namespace android_webview {

AwContentRendererClient::AwContentRendererClient() {
}

AwContentRendererClient::~AwContentRendererClient() {
}

void AwContentRendererClient::RenderThreadStarted() {
  WebKit::WebString content_scheme(
      ASCIIToUTF16(android_webview::kContentScheme));
  WebKit::WebSecurityPolicy::registerURLSchemeAsLocal(content_scheme);

  aw_render_process_observer_.reset(new AwRenderProcessObserver);
  content::RenderThread::Get()->AddObserver(
      aw_render_process_observer_.get());
}

void AwContentRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  AwRenderViewExt::RenderViewCreated(render_view);
}

std::string AwContentRendererClient::GetDefaultEncoding() {
  // TODO(boliu): query android system locale.
  return std::string("ISO-8859-1");
}

WebKit::WebPlugin* AwContentRendererClient::CreatePluginReplacement(
    content::RenderView* render_view,
    const FilePath& plugin_path) {
  // TODO(boliu): Call WebViewPlugin::Create with appropriate html for missing
  // plugin placeholder.
  return NULL;
}

bool AwContentRendererClient::HasErrorPage(int http_status_code,
                          std::string* error_domain) {
  // TODO(boliu): Implement our own error pages.
  return false;
}

void AwContentRendererClient::GetNavigationErrorStrings(
    const WebKit::WebURLRequest& failed_request,
    const WebKit::WebURLError& error,
    std::string* error_html,
    string16* error_description) {
  // TODO(boliu): Implement our own error pages.
}

unsigned long long AwContentRendererClient::VisitedLinkHash(
    const char* canonical_url,
    size_t length) {
  // TODO(boliu): Implement a visited link solution for Android WebView.
  // Perhaps componentize chrome implementation or move to content/?
  return 0LL;
}

bool AwContentRendererClient::IsLinkVisited(unsigned long long link_hash) {
  // TODO(boliu): Implement a visited link solution for Android WebView.
  // Perhaps componentize chrome implementation or move to content/?
  return false;
}

void AwContentRendererClient::PrefetchHostName(const char* hostname,
                                               size_t length) {
  // TODO(boliu): Implement hostname prefetch for Android WebView.
  // Perhaps componentize chrome implementation or move to content/?
}

}  // namespace android_webview
