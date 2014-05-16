// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "ui/base/cursor/ozone/cursor_factory_ozone.h"
#include "ui/events/ozone/event_factory_ozone.h"
#include "ui/gfx/ozone/surface_factory_ozone.h"
#include "ui/ozone/ime/input_method_context_factory_ozone.h"
#include "ui/ozone/ozone_platform.h"
#include "ui/ozone/ozone_platform_list.h"
#include "ui/ozone/ozone_switches.h"

namespace ui {

namespace {

// Helper to construct an OzonePlatform by name using the platform list.
OzonePlatform* CreatePlatform(const std::string& platform_name) {
  // Search for a matching platform in the list.
  for (int i = 0; i < kOzonePlatformCount; ++i)
    if (platform_name == kOzonePlatforms[i].name)
      return kOzonePlatforms[i].constructor();

  LOG(FATAL) << "Invalid ozone platform: " << platform_name;
  return NULL;  // not reached
}

// Returns the name of the platform to use (value of --ozone-platform flag).
std::string GetPlatformName() {
  // The first platform is the default.
  if (!CommandLine::ForCurrentProcess()->HasSwitch(switches::kOzonePlatform) &&
      kOzonePlatformCount > 0)
    return kOzonePlatforms[0].name;
  return CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
      switches::kOzonePlatform);
}

}  // namespace

OzonePlatform::OzonePlatform() {
  CHECK(!instance_) << "There should only be a single OzonePlatform.";
  instance_ = this;
}

OzonePlatform::~OzonePlatform() {
  CHECK_EQ(instance_, this);
  instance_ = NULL;
}

// static
void OzonePlatform::InitializeForUI() {
  CreateInstance();
  instance_->InitializeUI();
  ui::InputMethodContextFactoryOzone::SetInstance(
      instance_->GetInputMethodContextFactoryOzone());
}

// static
void OzonePlatform::InitializeForGPU() {
  CreateInstance();
  instance_->InitializeGPU();
}

// static
OzonePlatform* OzonePlatform::GetInstance() {
  CHECK(instance_) << "OzonePlatform is not initialized";
  return instance_;
}

// static
void OzonePlatform::CreateInstance() {
  if (!instance_) {
    std::string platform = GetPlatformName();
    TRACE_EVENT1("ozone", "OzonePlatform::Initialize", "platform", platform);
    CreatePlatform(platform);
  }
}

// static
OzonePlatform* OzonePlatform::instance_;

}  // namespace ui
