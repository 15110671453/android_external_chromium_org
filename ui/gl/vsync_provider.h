// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_VSYNC_PROVIDER_H_
#define UI_GL_VSYNC_PROVIDER_H_

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/time.h"

namespace gfx {

class VSyncProvider {
 public:
  VSyncProvider();
  virtual ~VSyncProvider();

  typedef base::Callback<void(const base::TimeTicks timebase,
                              const base::TimeDelta interval)>
      UpdateVSyncCallback;

  // Get the time of the most recent screen refresh, along with the time
  // between consecutive refreshes. The callback is called as soon as
  // the data is available: it could be immediately from this method,
  // later via a PostTask to the current MessageLoop, or never (if we have
  // no data source). We provide the strong guarantee that the callback will
  // not be called once the instance of this class is destroyed.
  virtual void GetVSyncParameters(const UpdateVSyncCallback& callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(VSyncProvider);
};

// Base class for providers based on extensions like GLX_OML_sync_control and
// EGL_CHROMIUM_sync_control.
class SyncControlVSyncProvider : public VSyncProvider {
 public:
  SyncControlVSyncProvider();
  virtual ~SyncControlVSyncProvider();

  virtual void GetVSyncParameters(
      const UpdateVSyncCallback& callback) OVERRIDE;

 protected:
  virtual bool GetSyncValues(int64* system_time,
                             int64* media_stream_counter,
                             int64* swap_buffer_counter) = 0;

  virtual bool GetMscRate(int32* numerator, int32* denominator) = 0;

 private:
  base::TimeTicks last_timebase_;
  uint64 last_media_stream_counter_;
  base::TimeDelta last_good_interval_;

  DISALLOW_COPY_AND_ASSIGN(SyncControlVSyncProvider);
};

}  // namespace gfx

#endif  // UI_GL_VSYNC_PROVIDER_H_
