// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/scheduler/scheduler.h"

#include "base/auto_reset.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "cc/base/thread.h"

namespace cc {

Scheduler::Scheduler(SchedulerClient* client,
                     const SchedulerSettings& scheduler_settings)
    : settings_(scheduler_settings),
      client_(client),
      weak_factory_(this),
      last_set_needs_begin_frame_(false),
      has_pending_begin_frame_(false),
      last_begin_frame_time_(base::TimeTicks()),
      // TODO(brianderson): Pass with BeginFrame in the near future.
      interval_(base::TimeDelta::FromMicroseconds(16666)),
      state_machine_(scheduler_settings),
      inside_process_scheduled_actions_(false) {
  DCHECK(client_);
  DCHECK(!state_machine_.BeginFrameNeededByImplThread());
}

Scheduler::~Scheduler() {
  client_->SetNeedsBeginFrameOnImplThread(false);
}

void Scheduler::SetCanStart() {
  state_machine_.SetCanStart();
  ProcessScheduledActions();
}

void Scheduler::SetVisible(bool visible) {
  state_machine_.SetVisible(visible);
  ProcessScheduledActions();
}

void Scheduler::SetCanDraw(bool can_draw) {
  state_machine_.SetCanDraw(can_draw);
  ProcessScheduledActions();
}

void Scheduler::SetHasPendingTree(bool has_pending_tree) {
  state_machine_.SetHasPendingTree(has_pending_tree);
  ProcessScheduledActions();
}

void Scheduler::SetNeedsCommit() {
  state_machine_.SetNeedsCommit();
  ProcessScheduledActions();
}

void Scheduler::SetNeedsForcedCommit() {
  state_machine_.SetNeedsCommit();
  state_machine_.SetNeedsForcedCommit();
  ProcessScheduledActions();
}

void Scheduler::SetNeedsRedraw() {
  state_machine_.SetNeedsRedraw();
  ProcessScheduledActions();
}

void Scheduler::DidSwapUseIncompleteTile() {
  state_machine_.DidSwapUseIncompleteTile();
  ProcessScheduledActions();
}

void Scheduler::SetNeedsForcedRedraw() {
  state_machine_.SetNeedsForcedRedraw();
  ProcessScheduledActions();
}

void Scheduler::SetMainThreadNeedsLayerTextures() {
  state_machine_.SetMainThreadNeedsLayerTextures();
  ProcessScheduledActions();
}

void Scheduler::FinishCommit() {
  TRACE_EVENT0("cc", "Scheduler::FinishCommit");
  state_machine_.FinishCommit();
  ProcessScheduledActions();
}

void Scheduler::BeginFrameAbortedByMainThread() {
  TRACE_EVENT0("cc", "Scheduler::BeginFrameAbortedByMainThread");
  state_machine_.BeginFrameAbortedByMainThread();
  ProcessScheduledActions();
}

void Scheduler::DidLoseOutputSurface() {
  TRACE_EVENT0("cc", "Scheduler::DidLoseOutputSurface");
  state_machine_.DidLoseOutputSurface();
  ProcessScheduledActions();
}

void Scheduler::DidCreateAndInitializeOutputSurface() {
  TRACE_EVENT0("cc", "Scheduler::DidCreateAndInitializeOutputSurface");
  state_machine_.DidCreateAndInitializeOutputSurface();
  has_pending_begin_frame_ = false;
  last_set_needs_begin_frame_ = false;
  ProcessScheduledActions();
}

base::TimeTicks Scheduler::AnticipatedDrawTime() {
  TRACE_EVENT0("cc", "Scheduler::AnticipatedDrawTime");
  base::TimeTicks now = base::TimeTicks::Now();
  int64 intervals = ((now - last_begin_frame_time_) / interval_) + 1;
  return last_begin_frame_time_ + (interval_ * intervals);
}

base::TimeTicks Scheduler::LastBeginFrameOnImplThreadTime() {
  return last_begin_frame_time_;
}

void Scheduler::SetupNextBeginFrameIfNeeded() {
  // Determine if we need BeginFrame notifications.
  // If we do, always request the BeginFrame immediately.
  // If not, only disable on the next BeginFrame to avoid unnecessary toggles.
  // The synchronous renderer compositor requires immediate disables though.
  bool needs_begin_frame = state_machine_.BeginFrameNeededByImplThread();
  if ((needs_begin_frame ||
       state_machine_.inside_begin_frame() ||
       settings_.using_synchronous_renderer_compositor) &&
      (needs_begin_frame != last_set_needs_begin_frame_)) {
    client_->SetNeedsBeginFrameOnImplThread(needs_begin_frame);
    last_set_needs_begin_frame_ = needs_begin_frame;
  }

  // Request another BeginFrame if we haven't drawn for now until we have
  // deadlines implemented.
  if (state_machine_.inside_begin_frame() && has_pending_begin_frame_) {
    has_pending_begin_frame_ = false;
    client_->SetNeedsBeginFrameOnImplThread(true);
  }
}

void Scheduler::BeginFrame(base::TimeTicks frame_time) {
  TRACE_EVENT0("cc", "Scheduler::BeginFrame");
  DCHECK(!has_pending_begin_frame_);
  has_pending_begin_frame_ = true;
  last_begin_frame_time_ = frame_time;
  state_machine_.DidEnterBeginFrame();
  state_machine_.SetFrameTime(frame_time);
  ProcessScheduledActions();
  state_machine_.DidLeaveBeginFrame();
}

void Scheduler::DrawAndSwapIfPossible() {
  ScheduledActionDrawAndSwapResult result =
      client_->ScheduledActionDrawAndSwapIfPossible();
  state_machine_.DidDrawIfPossibleCompleted(result.did_draw);
  if (result.did_swap)
    has_pending_begin_frame_ = false;
}

void Scheduler::DrawAndSwapForced() {
  ScheduledActionDrawAndSwapResult result =
      client_->ScheduledActionDrawAndSwapForced();
  if (result.did_swap)
    has_pending_begin_frame_ = false;
}

void Scheduler::ProcessScheduledActions() {
  // We do not allow ProcessScheduledActions to be recursive.
  // The top-level call will iteratively execute the next action for us anyway.
  if (inside_process_scheduled_actions_)
    return;

  base::AutoReset<bool> mark_inside(&inside_process_scheduled_actions_, true);

  SchedulerStateMachine::Action action = state_machine_.NextAction();
  while (action != SchedulerStateMachine::ACTION_NONE) {
    state_machine_.UpdateState(action);
    switch (action) {
      case SchedulerStateMachine::ACTION_NONE:
        break;
      case SchedulerStateMachine::ACTION_SEND_BEGIN_FRAME_TO_MAIN_THREAD:
        client_->ScheduledActionSendBeginFrameToMainThread();
        break;
      case SchedulerStateMachine::ACTION_COMMIT:
        client_->ScheduledActionCommit();
        break;
      case SchedulerStateMachine::ACTION_CHECK_FOR_COMPLETED_TILE_UPLOADS:
        client_->ScheduledActionCheckForCompletedTileUploads();
        break;
      case SchedulerStateMachine::ACTION_ACTIVATE_PENDING_TREE_IF_NEEDED:
        client_->ScheduledActionActivatePendingTreeIfNeeded();
        break;
      case SchedulerStateMachine::ACTION_DRAW_IF_POSSIBLE:
        DrawAndSwapIfPossible();
        break;
      case SchedulerStateMachine::ACTION_DRAW_FORCED:
        DrawAndSwapForced();
        break;
      case SchedulerStateMachine::ACTION_BEGIN_OUTPUT_SURFACE_CREATION:
        client_->ScheduledActionBeginOutputSurfaceCreation();
        break;
      case SchedulerStateMachine::ACTION_ACQUIRE_LAYER_TEXTURES_FOR_MAIN_THREAD:
        client_->ScheduledActionAcquireLayerTexturesForMainThread();
        break;
    }
    action = state_machine_.NextAction();
  }

  SetupNextBeginFrameIfNeeded();
  client_->DidAnticipatedDrawTimeChange(AnticipatedDrawTime());
}

bool Scheduler::WillDrawIfNeeded() const {
  return !state_machine_.DrawSuspendedUntilCommit();
}

}  // namespace cc
