// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/test_suite.h"

int main(int argc, char** argv) {
  return aura_shell::test::AuraShellTestSuite(argc, argv).Run();
}
