/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef LIBRARIES_NACL_MOUNTS_OSDIRENT_H_
#define LIBRARIES_NACL_MOUNTS_OSDIRENT_H_

#if defined(WIN32)

#include <sys/types.h>
#include "utils/macros.h"

struct dirent {
  _ino_t d_ino;
  _off_t d_off;
  unsigned short int d_reclen;
  char d_name[256];
};

#else

#include <dirent.h>

#endif

#endif  // LIBRARIES_NACL_MOUNTS_OSDIRENT_H_
