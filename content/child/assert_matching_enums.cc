// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Use this file to assert that *_list.h enums that are meant to do the bridge
// from Blink are valid.

#include "base/macros.h"
#include "cc/animation/animation.h"
#include "content/public/common/screen_orientation_values.h"
#include "net/base/mime_util.h"
#include "third_party/WebKit/public/platform/WebCompositorAnimation.h"
#include "third_party/WebKit/public/platform/WebMimeRegistry.h"
#include "third_party/WebKit/public/platform/WebScreenOrientationLockType.h"

namespace content {

#define COMPILE_ASSERT_MATCHING_ENUM(expected, actual) \
  COMPILE_ASSERT(int(expected) == int(actual), mismatching_enums)

// ScreenOrientationValues
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockDefault,
    DEFAULT);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockPortraitPrimary,
    PORTRAIT_PRIMARY);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockPortraitSecondary,
    PORTRAIT_SECONDARY);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockLandscapePrimary,
    LANDSCAPE_PRIMARY);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockLandscapeSecondary,
    LANDSCAPE_SECONDARY);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockAny,
    ANY);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockLandscape,
    LANDSCAPE);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockPortrait,
    PORTRAIT);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebScreenOrientationLockNatural,
    NATURAL);

// SupportsType
COMPILE_ASSERT_MATCHING_ENUM(blink::WebMimeRegistry::IsNotSupported,
    net::IsNotSupported);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebMimeRegistry::IsSupported,
    net::IsSupported);
COMPILE_ASSERT_MATCHING_ENUM(blink::WebMimeRegistry::MayBeSupported,
    net::MayBeSupported);

// TargetProperty
COMPILE_ASSERT_MATCHING_ENUM(
    blink::WebCompositorAnimation::TargetPropertyTransform,
    cc::Animation::Transform);
COMPILE_ASSERT_MATCHING_ENUM(
    blink::WebCompositorAnimation::TargetPropertyOpacity,
    cc::Animation::Opacity);
COMPILE_ASSERT_MATCHING_ENUM(
    blink::WebCompositorAnimation::TargetPropertyFilter,
    cc::Animation::Filter);
COMPILE_ASSERT_MATCHING_ENUM(
    blink::WebCompositorAnimation::TargetPropertyScrollOffset,
    cc::Animation::ScrollOffset);

} // namespace content
