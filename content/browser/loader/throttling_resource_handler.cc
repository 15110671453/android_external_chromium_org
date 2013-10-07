// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/throttling_resource_handler.h"

#include "content/browser/loader/resource_request_info_impl.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/resource_response.h"

namespace content {

ThrottlingResourceHandler::ThrottlingResourceHandler(
    scoped_ptr<ResourceHandler> next_handler,
    net::URLRequest* request,
    ScopedVector<ResourceThrottle> throttles)
    : LayeredResourceHandler(request, next_handler.Pass()),
      deferred_stage_(DEFERRED_NONE),
      throttles_(throttles.Pass()),
      index_(0),
      cancelled_by_resource_throttle_(false) {
  for (size_t i = 0; i < throttles_.size(); ++i)
    throttles_[i]->set_controller(this);
}

ThrottlingResourceHandler::~ThrottlingResourceHandler() {
}

bool ThrottlingResourceHandler::OnRequestRedirected(int request_id,
                                                    const GURL& new_url,
                                                    ResourceResponse* response,
                                                    bool* defer) {
  DCHECK(!cancelled_by_resource_throttle_);

  *defer = false;
  while (index_ < throttles_.size()) {
    throttles_[index_]->WillRedirectRequest(new_url, defer);
    index_++;
    if (cancelled_by_resource_throttle_)
      return false;
    if (*defer) {
      deferred_stage_ = DEFERRED_REDIRECT;
      deferred_url_ = new_url;
      deferred_response_ = response;
      return true;  // Do not cancel.
    }
  }

  index_ = 0;  // Reset for next time.

  return next_handler_->OnRequestRedirected(request_id, new_url, response,
                                            defer);
}

bool ThrottlingResourceHandler::OnWillStart(int request_id,
                                            const GURL& url,
                                            bool* defer) {
  DCHECK(!cancelled_by_resource_throttle_);

  *defer = false;
  while (index_ < throttles_.size()) {
    throttles_[index_]->WillStartRequest(defer);
    index_++;
    if (cancelled_by_resource_throttle_)
      return false;
    if (*defer) {
      deferred_stage_ = DEFERRED_START;
      deferred_url_ = url;
      return true;  // Do not cancel.
    }
  }

  index_ = 0;  // Reset for next time.

  return next_handler_->OnWillStart(request_id, url, defer);
}

bool ThrottlingResourceHandler::OnResponseStarted(int request_id,
                                                  ResourceResponse* response,
                                                  bool* defer) {
  DCHECK(!cancelled_by_resource_throttle_);

  while (index_ < throttles_.size()) {
    throttles_[index_]->WillProcessResponse(defer);
    index_++;
    if (cancelled_by_resource_throttle_)
      return false;
    if (*defer) {
      deferred_stage_ = DEFERRED_RESPONSE;
      deferred_response_ = response;
      return true;  // Do not cancel.
    }
  }

  index_ = 0;  // Reset for next time.

  return next_handler_->OnResponseStarted(request_id, response, defer);
}

void ThrottlingResourceHandler::Cancel() {
  cancelled_by_resource_throttle_ = true;
  controller()->Cancel();
}

void ThrottlingResourceHandler::CancelAndIgnore() {
  cancelled_by_resource_throttle_ = true;
  controller()->CancelAndIgnore();
}

void ThrottlingResourceHandler::CancelWithError(int error_code) {
  cancelled_by_resource_throttle_ = true;
  controller()->CancelWithError(error_code);
}

void ThrottlingResourceHandler::Resume() {
  DCHECK(!cancelled_by_resource_throttle_);

  DeferredStage last_deferred_stage = deferred_stage_;
  deferred_stage_ = DEFERRED_NONE;
  switch (last_deferred_stage) {
    case DEFERRED_NONE:
      NOTREACHED();
      break;
    case DEFERRED_START:
      ResumeStart();
      break;
    case DEFERRED_REDIRECT:
      ResumeRedirect();
      break;
    case DEFERRED_RESPONSE:
      ResumeResponse();
      break;
  }
}

void ThrottlingResourceHandler::ResumeStart() {
  DCHECK(!cancelled_by_resource_throttle_);

  GURL url = deferred_url_;
  deferred_url_ = GURL();

  bool defer = false;
  if (!OnWillStart(GetRequestID(), url, &defer)) {
    controller()->Cancel();
  } else if (!defer) {
    controller()->Resume();
  }
}

void ThrottlingResourceHandler::ResumeRedirect() {
  DCHECK(!cancelled_by_resource_throttle_);

  GURL new_url = deferred_url_;
  deferred_url_ = GURL();
  scoped_refptr<ResourceResponse> response;
  deferred_response_.swap(response);

  bool defer = false;
  if (!OnRequestRedirected(GetRequestID(), new_url, response.get(), &defer)) {
    controller()->Cancel();
  } else if (!defer) {
    controller()->Resume();
  }
}

void ThrottlingResourceHandler::ResumeResponse() {
  DCHECK(!cancelled_by_resource_throttle_);

  scoped_refptr<ResourceResponse> response;
  deferred_response_.swap(response);

  bool defer = false;
  if (!OnResponseStarted(GetRequestID(), response.get(), &defer)) {
    controller()->Cancel();
  } else if (!defer) {
    controller()->Resume();
  }
}

}  // namespace content
