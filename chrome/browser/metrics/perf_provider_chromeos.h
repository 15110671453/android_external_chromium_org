// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_PERF_PROVIDER_CHROMEOS_H_
#define CHROME_BROWSER_METRICS_PERF_PROVIDER_CHROMEOS_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/threading/non_thread_safe.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chromeos/login/login_state.h"
#include "components/metrics/proto/sampled_profile.pb.h"

namespace metrics {

class WindowedIncognitoObserver;

// Provides access to ChromeOS perf data. perf aka "perf events" is a
// performance profiling infrastructure built into the linux kernel. For more
// information, see: https://perf.wiki.kernel.org/index.php/Main_Page.
class PerfProvider : public base::NonThreadSafe {
 public:
  PerfProvider();
  ~PerfProvider();

  // Stores collected perf data protobufs in |sampled_profiles|. Clears all the
  // stored profile data. Returns true if it wrote to |sampled_profiles|.
  bool GetSampledProfiles(std::vector<SampledProfile>* sampled_profiles);

 private:
  // Class that listens for changes to the login state. When a normal user logs
  // in, it updates PerfProvider to start collecting data.
  class LoginObserver : public chromeos::LoginState::Observer {
   public:
    explicit LoginObserver(PerfProvider* perf_provider);

    // Called when either the login state or the logged in user type changes.
    // Activates |perf_provider_| to start collecting.
    virtual void LoggedInStateChanged() OVERRIDE;

   private:
    // Points to a PerfProvider instance that can be turned on or off based on
    // the login state.
    PerfProvider* perf_provider_;
  };

  // Turns on perf collection. Starts the timer that's used to schedule
  // collections.
  void OnUserLoggedIn();

  // Turns off perf collection. Does not delete any data that was already
  // collected and stored in |cached_perf_data_|.
  void Deactivate();

  // Selects a random time in the upcoming profiling interval that begins at
  // |next_profiling_interval_start_|. Schedules |timer_| to invoke
  // DoPeriodicCollection() when that time comes.
  void ScheduleCollection();

  // Collects perf data for a given |trigger_event|. Calls perf via the ChromeOS
  // debug daemon's dbus interface.
  void CollectIfNecessary(SampledProfile::TriggerEvent trigger_event);

  // Collects perf data on a repeating basis by calling CollectIfNecessary() and
  // reschedules it to be collected again.
  void DoPeriodicCollection();

  // Parses a perf data protobuf from the |data| passed in only if the
  // |incognito_observer| indicates that no incognito window had been opened
  // during the profile collection period.
  // |trigger_event| is the cause of the perf data collection.
  void ParseProtoIfValid(
      scoped_ptr<WindowedIncognitoObserver> incognito_observer,
      SampledProfile::TriggerEvent trigger_event,
      const std::vector<uint8>& data);

  // Vector of SampledProfile protobufs containing perf profiles.
  std::vector<SampledProfile> cached_perf_data_;

  // For scheduling collection of perf data.
  base::OneShotTimer<PerfProvider> timer_;

  // For detecting when changes to the login state.
  LoginObserver login_observer_;

  // Record of the last login time.
  base::TimeTicks login_time_;

  // Record of the start of the upcoming profiling interval.
  base::TimeTicks next_profiling_interval_start_;

  // To pass around the "this" pointer across threads safely.
  base::WeakPtrFactory<PerfProvider> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PerfProvider);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_PERF_PROVIDER_CHROMEOS_H_
