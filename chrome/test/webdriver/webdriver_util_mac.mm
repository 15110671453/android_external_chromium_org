// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/webdriver/webdriver_util.h"

#import <Foundation/Foundation.h>

#include "base/files/file_path.h"
#include "base/mac/foundation_util.h"

namespace webdriver {

void GetApplicationDirs(std::vector<base::FilePath>* app_dirs) {
  base::FilePath user_app_dir;
  if (base::mac::GetUserDirectory(NSApplicationDirectory, &user_app_dir))
    app_dirs->push_back(user_app_dir);
  base::FilePath local_app_dir;
  if (base::mac::GetLocalDirectory(NSApplicationDirectory, &local_app_dir))
    app_dirs->push_back(local_app_dir);
}

}  // namespace webdriver
