// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MAC_H_
#define CHROME_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MAC_H_
#pragma once

#include <map>
#include <utility>
#include <vector>

#include "base/scoped_nsobject.h"
#include "chrome/browser/accessibility/browser_accessibility.h"

@class BrowserAccessibilityCocoa;

class BrowserAccessibilityMac : public BrowserAccessibility {
 public:
  // Implementation of BrowserAccessibility.
  virtual void Initialize();
  virtual void ReleaseReference();

  // The BrowserAccessibilityCocoa associated with us.
  BrowserAccessibilityCocoa* native_view() const {
    return browser_accessibility_cocoa_;
  }

 private:
  // This gives BrowserAccessibility::Create access to the class constructor.
  friend class BrowserAccessibility;

  BrowserAccessibilityMac();

  // Allows access to the BrowserAccessibilityCocoa which wraps this.
  // BrowserAccessibility.
  // We own this object.
  BrowserAccessibilityCocoa* browser_accessibility_cocoa_;
  DISALLOW_COPY_AND_ASSIGN(BrowserAccessibilityMac);
};

#endif // CHROME_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MAC_H_
