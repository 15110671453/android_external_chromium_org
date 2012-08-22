// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/about_protocol_handler.h"

#include "net/url_request/url_request_about_job.h"

namespace net {

AboutProtocolHandler::AboutProtocolHandler() {
}

URLRequestJob* AboutProtocolHandler::MaybeCreateJob(
    URLRequest* request) const {
  DCHECK(false);
  return new URLRequestAboutJob(request);
}

}  // namespace net
