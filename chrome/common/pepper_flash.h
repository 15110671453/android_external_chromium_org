// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PEPPER_FLASH_H_
#define CHROME_COMMON_PEPPER_FLASH_H_

#include "base/basictypes.h"

// Whether a field trial for Pepper Flash is going on.
bool ConductingPepperFlashFieldTrial();

// True if Pepper Flash should be enabled by default.
bool IsPepperFlashEnabledByDefault();

// Permission bits for Pepper Flash.
extern int32 kPepperFlashPermissions;

#endif  // CHROME_COMMON_PEPPER_FLASH_H_
