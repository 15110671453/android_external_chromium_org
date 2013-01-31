// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/location_bar/keyword_hint_decoration.h"

#include "base/utf_string_conversions.h"
#import "chrome/browser/ui/cocoa/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class KeywordHintDecorationTest : public CocoaTest {
 public:
  KeywordHintDecorationTest()
      : decoration_(NULL) {
  }

  KeywordHintDecoration decoration_;
};

TEST_F(KeywordHintDecorationTest, GetWidthForSpace) {
  decoration_.SetVisible(true);
  decoration_.SetKeyword(ASCIIToUTF16("google"), false);

  const CGFloat kVeryWide = 1000.0;
  const CGFloat kFairlyWide = 100.0;  // Estimate for full hint space.
  const CGFloat kEditingSpace = 50.0;

  // Wider than the [tab] image when we have lots of space.
  EXPECT_NE(decoration_.GetWidthForSpace(kVeryWide, 0),
            LocationBarDecoration::kOmittedWidth);
  EXPECT_GE(decoration_.GetWidthForSpace(kVeryWide, 0), kFairlyWide);

  // When there's not enough space for the text, trims to something
  // narrower.
  const CGFloat full_width = decoration_.GetWidthForSpace(kVeryWide, 0);
  const CGFloat not_wide_enough = full_width - 10.0;
  EXPECT_NE(decoration_.GetWidthForSpace(not_wide_enough, 0),
            LocationBarDecoration::kOmittedWidth);
  EXPECT_LT(decoration_.GetWidthForSpace(not_wide_enough, 0), full_width);

  // Even trims when there's enough space for everything, but it would
  // eat "too much".
  EXPECT_NE(decoration_.GetWidthForSpace(full_width + kEditingSpace, 0),
            LocationBarDecoration::kOmittedWidth);
  EXPECT_LT(decoration_.GetWidthForSpace(full_width + kEditingSpace, 0),
            full_width);

  // Omitted when not wide enough to fit even the image.
  const CGFloat image_width = decoration_.GetWidthForSpace(not_wide_enough, 0);
  EXPECT_EQ(decoration_.GetWidthForSpace(image_width - 1.0, 0),
            LocationBarDecoration::kOmittedWidth);
}

}  // namespace
