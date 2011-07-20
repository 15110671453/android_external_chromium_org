// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prerender/prerender_final_status.h"

#include "chrome/browser/prerender/prerender_manager.h"

namespace prerender {

namespace {

const char* kFinalStatusNames[] = {
  "Used",
  "Timed Out",
  "Evicted",
  "Manager Shutdown",
  "Closed",
  "Create New Window",
  "Profile Destroyed",
  "App Terminating",
  "Javascript Alert",
  "Auth Needed",
  "HTTPS",
  "Download",
  "Memory Limit Exceeded",
  "JS Out Of Memory",
  "Renderer Unresponsive",
  "Too many processes",
  "Rate Limit Exceeded",
  "Pending Skipped",
  "Control Group",
  "HTML5 Media",
  "Source Render View Closed",
  "Renderer Crashed",
  "Unsupported Scheme",
  "Invalid HTTP Method",
  "window.print",
  "Recently Visited",
  "window.opener",
  "Page ID Conflict",
  "Safe Browsing",
  "Fragment Mismatch",
  "SSL Client Certificate Requested",
  "Cache or History Cleared",
  "Cancelled",
  "Max",
};
COMPILE_ASSERT(arraysize(kFinalStatusNames) == FINAL_STATUS_MAX + 1,
               PrerenderFinalStatus_name_count_mismatch);

}

const char* NameFromFinalStatus(FinalStatus final_status) {
  DCHECK(static_cast<int>(final_status) >= 0 &&
         static_cast<int>(final_status) <= FINAL_STATUS_MAX);
  return kFinalStatusNames[final_status];
}

}  // namespace prerender
