// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/histogram_controller.h"

#include "base/bind.h"
#include "base/metrics/histogram.h"
#include "content/browser/histogram_subscriber.h"
#include "content/common/child_process_messages.h"
#include "content/public/browser/browser_child_process_host_iterator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/render_process_host.h"

namespace content {

HistogramController* HistogramController::GetInstance() {
  return Singleton<HistogramController>::get();
}

HistogramController::HistogramController() : subscriber_(NULL) {
}

HistogramController::~HistogramController() {
}

void HistogramController::OnPendingProcesses(int sequence_number,
                                             int pending_processes,
                                             bool end) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (subscriber_)
    subscriber_->OnPendingProcesses(sequence_number, pending_processes, end);
}

void HistogramController::OnHistogramDataCollected(
    int sequence_number,
    const std::vector<std::string>& pickled_histograms) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&HistogramController::OnHistogramDataCollected,
                   base::Unretained(this),
                   sequence_number,
                   pickled_histograms));
    return;
  }

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (subscriber_) {
    subscriber_->OnHistogramDataCollected(sequence_number,
                                          pickled_histograms);
  }
}

void HistogramController::Register(HistogramSubscriber* subscriber) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!subscriber_);
  subscriber_ = subscriber;
}

void HistogramController::Unregister(
    const HistogramSubscriber* subscriber) {
  DCHECK_EQ(subscriber_, subscriber);
  subscriber_ = NULL;
}

void HistogramController::GetHistogramDataFromChildProcesses(
    int sequence_number) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  int pending_processes = 0;
  /* TODO(rtenneti): Enable collecting histogram from child processes.
  for (BrowserChildProcessHostIterator iter; !iter.Done(); ++iter) {
    ++pending_processes;
    if (!iter.Send(new ChildProcessMsg_GetChildHistogramData(sequence_number)))
      --pending_processes;
  }
  */

  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(
          &HistogramController::OnPendingProcesses,
          base::Unretained(this),
          sequence_number,
          pending_processes,
          true));
}

void HistogramController::GetHistogramData(int sequence_number) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  int pending_processes = 0;
  for (RenderProcessHost::iterator it(RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd(); it.Advance()) {
    ++pending_processes;
    if (!it.GetCurrentValue()->Send(
            new ChildProcessMsg_GetChildHistogramData(sequence_number))) {
      --pending_processes;
    }
  }
  OnPendingProcesses(sequence_number, pending_processes, false);

  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&HistogramController::GetHistogramDataFromChildProcesses,
                 base::Unretained(this),
                 sequence_number));
}

}  // namespace content
