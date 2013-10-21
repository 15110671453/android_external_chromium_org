// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_CHROMEOS_PATHS_H_
#define CHROMEOS_CHROMEOS_PATHS_H_

#include "chromeos/chromeos_export.h"

// This file declares path keys for the chromeos module.  These can be used with
// the PathService to access various special directories and files.

namespace chromeos {

enum {
  PATH_START = 7000,

  FILE_DEFAULT_APP_ORDER,       // Full path to the json file that defines the
                                // default app order.
  DIR_USER_POLICY_KEYS,         // Directory where the session_manager stores
                                // the user policy keys.
  FILE_OWNER_KEY,               // Full path to the owner key file.
  FILE_INSTALL_ATTRIBUTES,      // Full path to the install attributes file.
  FILE_UPTIME,                  // Full path to the file via which the kernel
                                // exposes the current device uptime.
  FILE_UPDATE_REBOOT_NEEDED_UPTIME,  // Full path to a file in which Chrome can
                                     // store the uptime at which an update
                                     // became necessary. The file should be
                                     // cleared on boot.

  PATH_END
};

// Call once to register the provider for the path keys defined above.
CHROMEOS_EXPORT void RegisterPathProvider();

}  // namespace chromeos

#endif  // CHROMEOS_CHROMEOS_PATHS_H_
