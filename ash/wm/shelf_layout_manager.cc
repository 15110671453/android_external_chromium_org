// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/shelf_layout_manager.h"

#include "ash/launcher/launcher.h"
#include "ash/screen_ash.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray.h"
#include "base/auto_reset.h"
#include "ui/aura/root_window.h"
#include "ui/gfx/compositor/layer.h"
#include "ui/gfx/compositor/layer_animation_observer.h"
#include "ui/gfx/compositor/layer_animator.h"
#include "ui/gfx/compositor/scoped_layer_animation_settings.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace internal {

namespace {

// Height of the shelf when auto-hidden.
const int kAutoHideHeight = 2;

ui::Layer* GetLayer(views::Widget* widget) {
  return widget->GetNativeView()->layer();
}

}  // namespace

// static
const int ShelfLayoutManager::kWorkspaceAreaBottomInset = 2;

////////////////////////////////////////////////////////////////////////////////
// ShelfLayoutManager, public:

ShelfLayoutManager::ShelfLayoutManager(views::Widget* status)
    : in_layout_(false),
      shelf_height_(status->GetWindowScreenBounds().height()),
      launcher_(NULL),
      status_(status),
      window_overlaps_shelf_(false),
      root_window_(NULL) {
  root_window_ = status->GetNativeView()->GetRootWindow();
}

ShelfLayoutManager::~ShelfLayoutManager() {
}

gfx::Rect ShelfLayoutManager::GetMaximizedWindowBounds(
    aura::Window* window) const {
  // TODO: needs to be multi-mon aware.
  gfx::Rect bounds(gfx::Screen::GetMonitorAreaNearestWindow(window));
  bounds.set_height(bounds.height() - kAutoHideHeight);
  return bounds;
}

gfx::Rect ShelfLayoutManager::GetUnmaximizedWorkAreaBounds(
    aura::Window* window) const {
  // TODO: needs to be multi-mon aware.
  gfx::Rect bounds(gfx::Screen::GetMonitorAreaNearestWindow(window));
  bounds.set_height(bounds.height() - shelf_height_ -
                    kWorkspaceAreaBottomInset);
  return bounds;
}

void ShelfLayoutManager::SetLauncher(Launcher* launcher) {
  if (launcher == launcher_)
    return;

  launcher_ = launcher;
  shelf_height_ =
      std::max(status_->GetWindowScreenBounds().height(),
               launcher_widget()->GetWindowScreenBounds().height());
  LayoutShelf();
}

void ShelfLayoutManager::LayoutShelf() {
  AutoReset<bool> auto_reset_in_layout(&in_layout_, true);
  StopAnimating();
  TargetBounds target_bounds;
  CalculateTargetBounds(state_, &target_bounds);
  if (launcher_widget()) {
    GetLayer(launcher_widget())->SetOpacity(target_bounds.opacity);
    launcher_widget()->SetBounds(target_bounds.launcher_bounds);
    launcher_->SetStatusWidth(
        target_bounds.status_bounds.width());
  }
  GetLayer(status_)->SetOpacity(target_bounds.opacity);
  status_->SetBounds(target_bounds.status_bounds);
  Shell::GetInstance()->SetMonitorWorkAreaInsets(
      Shell::GetRootWindow(),
      target_bounds.work_area_insets);
}

void ShelfLayoutManager::SetState(VisibilityState visibility_state,
                                  AutoHideState auto_hide_state) {
  State state;
  state.visibility_state = visibility_state;
  state.auto_hide_state = auto_hide_state;

  if (state_.Equals(state))
    return;  // Nothing changed.

  // Animating the background when transitioning from auto-hide & hidden to
  // visibile is janking. Update the background immediately in this case.
  internal::BackgroundAnimator::ChangeType change_type =
      (state_.visibility_state == AUTO_HIDE &&
       state_.auto_hide_state == AUTO_HIDE_HIDDEN &&
       state.visibility_state == VISIBLE) ?
      internal::BackgroundAnimator::CHANGE_IMMEDIATE :
      internal::BackgroundAnimator::CHANGE_ANIMATE;
  StopAnimating();
  state_ = state;
  TargetBounds target_bounds;
  CalculateTargetBounds(state_, &target_bounds);
  if (launcher_widget()) {
    ui::ScopedLayerAnimationSettings launcher_animation_setter(
        GetLayer(launcher_widget())->GetAnimator());
    launcher_animation_setter.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(130));
    launcher_animation_setter.SetTweenType(ui::Tween::EASE_OUT);
    GetLayer(launcher_widget())->SetBounds(target_bounds.launcher_bounds);
    GetLayer(launcher_widget())->SetOpacity(target_bounds.opacity);
  }
  ui::ScopedLayerAnimationSettings status_animation_setter(
      GetLayer(status_)->GetAnimator());
  status_animation_setter.SetTransitionDuration(
      base::TimeDelta::FromMilliseconds(130));
  status_animation_setter.SetTweenType(ui::Tween::EASE_OUT);
  GetLayer(status_)->SetBounds(target_bounds.status_bounds);
  GetLayer(status_)->SetOpacity(target_bounds.opacity);
  Shell::GetInstance()->SetMonitorWorkAreaInsets(
      Shell::GetRootWindow(),
      target_bounds.work_area_insets);
  UpdateShelfBackground(change_type);
}

void ShelfLayoutManager::SetWindowOverlapsShelf(bool value) {
  window_overlaps_shelf_ = value;
  UpdateShelfBackground(internal::BackgroundAnimator::CHANGE_ANIMATE);
}

////////////////////////////////////////////////////////////////////////////////
// ShelfLayoutManager, aura::LayoutManager implementation:

void ShelfLayoutManager::OnWindowResized() {
  LayoutShelf();
}

void ShelfLayoutManager::OnWindowAddedToLayout(aura::Window* child) {
}

void ShelfLayoutManager::OnWillRemoveWindowFromLayout(aura::Window* child) {
}

void ShelfLayoutManager::OnChildWindowVisibilityChanged(aura::Window* child,
                                                        bool visible) {
}

void ShelfLayoutManager::SetChildBounds(aura::Window* child,
                                        const gfx::Rect& requested_bounds) {
  SetChildBoundsDirect(child, requested_bounds);
  if (!in_layout_)
    LayoutShelf();
}

////////////////////////////////////////////////////////////////////////////////
// ShelfLayoutManager, private:

void ShelfLayoutManager::StopAnimating() {
  if (launcher_widget())
    GetLayer(launcher_widget())->GetAnimator()->StopAnimating();
  GetLayer(status_)->GetAnimator()->StopAnimating();
}

void ShelfLayoutManager::CalculateTargetBounds(
    const State& state,
    TargetBounds* target_bounds) const {
  const gfx::Rect& available_bounds(root_window_->bounds());
  int y = available_bounds.bottom();
  int shelf_height = 0;
  int work_area_delta = 0;
  if (state.visibility_state == VISIBLE ||
      (state.visibility_state == AUTO_HIDE &&
       state.auto_hide_state == AUTO_HIDE_SHOWN)) {
    shelf_height = shelf_height_;
    work_area_delta = kWorkspaceAreaBottomInset;
  } else if (state.visibility_state == AUTO_HIDE &&
             state.auto_hide_state == AUTO_HIDE_HIDDEN) {
    shelf_height = kAutoHideHeight;
  }
  y -= shelf_height;
  gfx::Rect status_bounds(status_->GetWindowScreenBounds());
  // The status widget should extend to the bottom and right edges.
  target_bounds->status_bounds = gfx::Rect(
      available_bounds.right() - status_bounds.width(),
      y + shelf_height_ - status_bounds.height(),
      status_bounds.width(), status_bounds.height());
  if (launcher_widget()) {
    gfx::Rect launcher_bounds(launcher_widget()->GetWindowScreenBounds());
    target_bounds->launcher_bounds = gfx::Rect(
        available_bounds.x(),
        y + (shelf_height_ - launcher_bounds.height()) / 2,
        available_bounds.width(),
        launcher_bounds.height());
  }
  target_bounds->opacity =
      (state.visibility_state == VISIBLE ||
       state.visibility_state == AUTO_HIDE) ? 1.0f : 0.0f;
  target_bounds->work_area_insets =
      gfx::Insets(0, 0, shelf_height + work_area_delta, 0);
}

void ShelfLayoutManager::UpdateShelfBackground(
    BackgroundAnimator::ChangeType type) {
  bool launcher_paints = GetLauncherPaintsBackground();
  if (launcher_)
    launcher_->SetPaintsBackground(launcher_paints, type);
  // SystemTray normally draws a background, but we don't want it to draw a
  // background when the launcher does.
  Shell::GetInstance()->tray()->SetPaintsBackground(!launcher_paints, type);
}

bool ShelfLayoutManager::GetLauncherPaintsBackground() const {
  return window_overlaps_shelf_ || state_.visibility_state == AUTO_HIDE;
}

}  // namespace internal
}  // namespace ash
