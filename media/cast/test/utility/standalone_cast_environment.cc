// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/test/utility/standalone_cast_environment.h"

#include "base/time/default_tick_clock.h"

namespace media {
namespace cast {

StandaloneCastEnvironment::StandaloneCastEnvironment(
    const CastLoggingConfig& logging_config)
    : CastEnvironment(
          make_scoped_ptr<base::TickClock>(new base::DefaultTickClock()),
          NULL,
          NULL,
          NULL,
          NULL,
          logging_config),
      main_thread_("StandaloneCastEnvironment Main"),
      audio_thread_("StandaloneCastEnvironment Audio"),
      video_thread_("StandaloneCastEnvironment Video"),
      transport_thread_("StandaloneCastEnvironment Transport") {
#define CREATE_TASK_RUNNER(name, options)   \
  name##_thread_.StartWithOptions(options); \
  CastEnvironment::name##_thread_proxy_ = name##_thread_.message_loop_proxy()

  CREATE_TASK_RUNNER(main,
                     base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  CREATE_TASK_RUNNER(audio, base::Thread::Options());
  CREATE_TASK_RUNNER(video, base::Thread::Options());
  CREATE_TASK_RUNNER(transport, base::Thread::Options());

#undef CREATE_TASK_RUNNER
}

StandaloneCastEnvironment::~StandaloneCastEnvironment() {
  DCHECK(CalledOnValidThread());
}

void StandaloneCastEnvironment::Shutdown() {
  DCHECK(CalledOnValidThread());
  main_thread_.Stop();
  audio_thread_.Stop();
  video_thread_.Stop();
  transport_thread_.Stop();
}

}  // namespace cast
}  // namespace media
