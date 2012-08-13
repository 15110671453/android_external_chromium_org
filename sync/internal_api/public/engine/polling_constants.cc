// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "sync/internal_api/public/engine/polling_constants.h"

namespace syncer {

// Server can overwrite these values via client commands.
// Standard short poll. This is used when XMPP is off.
// We use high values here to ensure that failure to receive poll updates from
// the server doesn't result in rapid-fire polling from the client due to low
// local limits.
const int64 kDefaultShortPollIntervalSeconds = 3600 * 8;
// Long poll is used when XMPP is on.
const int64 kDefaultLongPollIntervalSeconds = 3600 * 12;

// Maximum interval for exponential backoff.
const int64 kMaxBackoffSeconds = 60 * 60 * 4;  // 4 hours.

// Backoff interval randomization factor.
const int kBackoffRandomizationFactor = 2;

// After a failure contacting sync servers, specifies how long to wait before
// reattempting and entering exponential backoff if consecutive failures
// occur.
const int kInitialBackoffRetrySeconds = 60 * 5;  // 5 minutes.

// Similar to kInitialBackoffRetrySeconds above, but only to be used in
// certain exceptional error cases, such as MIGRATION_DONE.
const int kInitialBackoffShortRetrySeconds = 1;

}  // namespace syncer
