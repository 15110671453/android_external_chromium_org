// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/base_file.h"

#include "content/browser/download/file_metadata_linux.h"
#include "content/public/browser/browser_thread.h"

namespace content {

void BaseFile::AnnotateWithSourceInformation() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DCHECK(!detached_);

  AddOriginMetadataToFile(full_path_, source_url_, referrer_url_);
}

}  // namespace content
