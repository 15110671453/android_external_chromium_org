// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/document_custom_bindings.h"

#include <string>

#include "base/bind.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "v8/include/v8.h"

namespace extensions {

DocumentCustomBindings::DocumentCustomBindings(
    Dispatcher* dispatcher, ChromeV8Context* context)
    : ChromeV8Extension(dispatcher, context) {
  RouteFunction("RegisterElement",
      base::Bind(&DocumentCustomBindings::RegisterElement,
                 base::Unretained(this)));
}

// Attach an event name to an object.
void DocumentCustomBindings::RegisterElement(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  content::RenderView* render_view = GetRenderView();
  if (!render_view) {
    return;
  }

  WebKit::WebView* web_view = render_view->GetWebView();
  if (!web_view) {
    return;
  }

  if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsObject()) {
    NOTREACHED();
    return;
  }

  std::string element_name(*v8::String::AsciiValue(args[0]));
  v8::Local<v8::Object> options = args[1]->ToObject();

  WebKit::WebExceptionCode ec = 0;
  WebKit::WebDocument document = web_view->mainFrame()->document();
  v8::Handle<v8::Value> constructor =
      document.registerEmbedderCustomElement(
          WebKit::WebString::fromUTF8(element_name), options, ec);
  args.GetReturnValue().Set(constructor);
}

}  // namespace extensions
