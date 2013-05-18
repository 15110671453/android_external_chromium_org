// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/win/worker_process_launcher.h"

#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/time.h"
#include "base/win/windows_version.h"
#include "ipc/ipc_message.h"
#include "remoting/host/chromoting_messages.h"
#include "remoting/host/host_exit_codes.h"
#include "remoting/host/worker_process_ipc_delegate.h"

using base::TimeDelta;
using base::win::ScopedHandle;

const net::BackoffEntry::Policy kDefaultBackoffPolicy = {
  // Number of initial errors (in sequence) to ignore before applying
  // exponential back-off rules.
  0,

  // Initial delay for exponential back-off in ms.
  100,

  // Factor by which the waiting time will be multiplied.
  2,

  // Fuzzing percentage. ex: 10% will spread requests randomly
  // between 90%-100% of the calculated time.
  0,

  // Maximum amount of time we are willing to delay our request in ms.
  60000,

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  -1,

  // Don't use initial delay unless the last request was an error.
  false,
};

const int kKillProcessTimeoutSeconds = 5;
const int kLaunchResultTimeoutSeconds = 5;

namespace remoting {

WorkerProcessLauncher::Delegate::~Delegate() {
}

WorkerProcessLauncher::WorkerProcessLauncher(
    scoped_ptr<WorkerProcessLauncher::Delegate> launcher_delegate,
    WorkerProcessIpcDelegate* ipc_handler)
    : ipc_handler_(ipc_handler),
      launcher_delegate_(launcher_delegate.Pass()),
      exit_code_(CONTROL_C_EXIT),
      ipc_enabled_(false),
      kill_process_timeout_(
          base::TimeDelta::FromSeconds(kKillProcessTimeoutSeconds)),
      launch_backoff_(&kDefaultBackoffPolicy) {
  DCHECK(ipc_handler_ != NULL);

  LaunchWorker();
}

WorkerProcessLauncher::~WorkerProcessLauncher() {
  DCHECK(CalledOnValidThread());

  ipc_handler_ = NULL;
  StopWorker();
}

void WorkerProcessLauncher::Crash(
    const tracked_objects::Location& location) {
  DCHECK(CalledOnValidThread());

  // Ask the worker process to crash voluntarily if it is still connected.
  if (ipc_enabled_) {
    Send(new ChromotingDaemonMsg_Crash(location.function_name(),
                                       location.file_name(),
                                       location.line_number()));
  }

  // Close the channel and ignore any not yet processed messages.
  launcher_delegate_->CloseChannel();
  ipc_enabled_ = false;

  // Give the worker process some time to crash.
  if (!kill_process_timer_.IsRunning()) {
    kill_process_timer_.Start(FROM_HERE, kill_process_timeout_, this,
                              &WorkerProcessLauncher::StopWorker);
  }
}

void WorkerProcessLauncher::Send(IPC::Message* message) {
  DCHECK(CalledOnValidThread());

  if (ipc_enabled_) {
    launcher_delegate_->Send(message);
  } else {
    delete message;
  }
}

void WorkerProcessLauncher::OnProcessLaunched(
    base::win::ScopedHandle worker_process) {
  DCHECK(CalledOnValidThread());
  DCHECK(!ipc_enabled_);
  DCHECK(!launch_timer_.IsRunning());
  DCHECK(!process_watcher_.GetWatchedObject());
  DCHECK(!worker_process_.IsValid());

  if (!process_watcher_.StartWatching(worker_process, this)) {
    StopWorker();
    return;
  }

  ipc_enabled_ = true;
  worker_process_ = worker_process.Pass();
}

void WorkerProcessLauncher::OnFatalError() {
  DCHECK(CalledOnValidThread());

  StopWorker();
}

bool WorkerProcessLauncher::OnMessageReceived(
  const IPC::Message& message) {
  DCHECK(CalledOnValidThread());

  if (!ipc_enabled_)
    return false;

  return ipc_handler_->OnMessageReceived(message);
}

void WorkerProcessLauncher::OnChannelConnected(int32 peer_pid) {
  DCHECK(CalledOnValidThread());

  if (!ipc_enabled_)
    return;

  // This can result in |this| being deleted, so this call must be the last in
  // this method.
  ipc_handler_->OnChannelConnected(peer_pid);
}

void WorkerProcessLauncher::OnChannelError() {
  DCHECK(CalledOnValidThread());

  // Schedule a delayed termination of the worker process. Usually, the pipe is
  // disconnected when the worker process is about to exit. Waiting a little bit
  // here allows the worker to exit completely and so, notify
  // |process_watcher_|. As the result KillProcess() will not be called and
  // the original exit code reported by the worker process will be retrieved.
  if (!kill_process_timer_.IsRunning()) {
    kill_process_timer_.Start(FROM_HERE, kill_process_timeout_, this,
                              &WorkerProcessLauncher::StopWorker);
  }
}

void WorkerProcessLauncher::OnObjectSignaled(HANDLE object) {
  DCHECK(CalledOnValidThread());
  DCHECK(!process_watcher_.GetWatchedObject());
  DCHECK_EQ(exit_code_, CONTROL_C_EXIT);
  DCHECK_EQ(worker_process_, object);

  // Get exit code of the worker process if it is available.
  if (!::GetExitCodeProcess(worker_process_, &exit_code_)) {
    LOG_GETLASTERROR(INFO)
        << "Failed to query the exit code of the worker process";
    exit_code_ = CONTROL_C_EXIT;
  }

  worker_process_.Close();
  StopWorker();
}

void WorkerProcessLauncher::LaunchWorker() {
  DCHECK(CalledOnValidThread());
  DCHECK(!ipc_enabled_);
  DCHECK(!kill_process_timer_.IsRunning());
  DCHECK(!launch_timer_.IsRunning());
  DCHECK(!process_watcher_.GetWatchedObject());
  DCHECK(!launch_result_timer_.IsRunning());

  exit_code_ = CONTROL_C_EXIT;

  // Make sure launching a process will not take forever.
  launch_result_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kLaunchResultTimeoutSeconds),
      this, &WorkerProcessLauncher::RecordLaunchResult);

  launcher_delegate_->LaunchProcess(this);
}

void WorkerProcessLauncher::RecordLaunchResult() {
  DCHECK(CalledOnValidThread());

  if (!worker_process_.IsValid()) {
    LOG(WARNING) << "A worker process failed to start within "
                 << kLaunchResultTimeoutSeconds << " seconds.";

    launch_backoff_.InformOfRequest(false);
    StopWorker();
    return;
  }

  // Assume success if the worker process has been running for a few seconds.
  launch_backoff_.InformOfRequest(true);
}

void WorkerProcessLauncher::RecordSuccessfulLaunchForTest() {
  DCHECK(CalledOnValidThread());

  if (launch_result_timer_.IsRunning()) {
    launch_result_timer_.Stop();
    RecordLaunchResult();
  }
}

void WorkerProcessLauncher::SetKillProcessTimeoutForTest(
    const base::TimeDelta& timeout) {
  DCHECK(CalledOnValidThread());

  kill_process_timeout_ = timeout;
}

void WorkerProcessLauncher::StopWorker() {
  DCHECK(CalledOnValidThread());

  // Record a launch failure if the process exited too soon.
  if (launch_result_timer_.IsRunning()) {
    launch_backoff_.InformOfRequest(false);
    launch_result_timer_.Stop();
  }

  // Ignore any remaining IPC messages.
  ipc_enabled_ = false;

  // Stop monitoring the worker process.
  process_watcher_.StopWatching();
  worker_process_.Close();

  kill_process_timer_.Stop();
  launcher_delegate_->KillProcess();

  // Do not relaunch the worker process if the caller has asked us to stop.
  if (stopping())
    return;

  // Stop trying to restart the worker process if it exited due to
  // misconfiguration.
  if (kMinPermanentErrorExitCode <= exit_code_ &&
      exit_code_ <= kMaxPermanentErrorExitCode) {
    ipc_handler_->OnPermanentError();
    return;
  }

  // Schedule the next attempt to launch the worker process.
  launch_timer_.Start(FROM_HERE, launch_backoff_.GetTimeUntilRelease(), this,
                      &WorkerProcessLauncher::LaunchWorker);
}

} // namespace remoting
