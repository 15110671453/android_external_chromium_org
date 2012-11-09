// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_FINDER_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_FINDER_H_

#include <vector>

#include "base/callback_forward.h"

class FilePath;

// Gets the path to the default Chrome executable. Returns true on success.
bool FindChrome(FilePath* browser_exe);

namespace internal {

bool FindExe(
    const base::Callback<bool(const FilePath&)>& exists_func,
    const std::vector<FilePath>& rel_paths,
    const std::vector<FilePath>& locations,
    FilePath* out_path);

}  // namespace internal

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_FINDER_H_
