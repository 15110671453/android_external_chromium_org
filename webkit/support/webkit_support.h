// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_SUPPORT_WEBKIT_SUPPORT_H_
#define WEBKIT_SUPPORT_WEBKIT_SUPPORT_H_

// This package provides functions used by webkit_unit_tests.
namespace webkit_support {

// Initializes or terminates a test environment for unit tests.
void SetUpTestEnvironmentForUnitTests();
void TearDownTestEnvironment();

}  // namespace webkit_support

#endif  // WEBKIT_SUPPORT_WEBKIT_SUPPORT_H_
