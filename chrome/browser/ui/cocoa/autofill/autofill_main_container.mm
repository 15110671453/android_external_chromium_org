// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/autofill_main_container.h"

#include <algorithm>
#include <cmath>

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/autofill/autofill_dialog_controller.h"
#include "chrome/browser/ui/cocoa/autofill/autofill_dialog_constants.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_details_container.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_notification_container.h"
#import "chrome/browser/ui/cocoa/hyperlink_text_view.h"
#import "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "grit/generated_resources.h"
#import "third_party/GTM/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/range/range.h"

@interface AutofillMainContainer (Private)
- (void)buildWindowButtonsForFrame:(NSRect)frame;
- (void)layoutButtons;
- (NSSize)preferredLegalDocumentSizeForWidth:(CGFloat)width;
@end


@implementation AutofillMainContainer

@synthesize target = target_;

- (id)initWithController:(autofill::AutofillDialogController*)controller {
  if (self = [super init]) {
    controller_ = controller;
  }
  return self;
}

- (void)loadView {
  [self buildWindowButtonsForFrame:NSZeroRect];

  base::scoped_nsobject<NSView> view([[NSView alloc] initWithFrame:NSZeroRect]);
  [view setAutoresizesSubviews:YES];
  [view setSubviews:@[buttonContainer_]];
  [self setView:view];

  [self layoutButtons];

  detailsContainer_.reset(
      [[AutofillDetailsContainer alloc] initWithController:controller_]);
  NSSize frameSize = [[detailsContainer_ view] frame].size;
  [[detailsContainer_ view] setFrameOrigin:
      NSMakePoint(0, NSHeight([buttonContainer_ frame]))];
  frameSize.height += NSHeight([buttonContainer_ frame]);
  [[self view] setFrameSize:frameSize];
  [[self view] addSubview:[detailsContainer_ view]];

  legalDocumentsView_.reset(
      [[HyperlinkTextView alloc] initWithFrame:NSZeroRect]);
  [legalDocumentsView_ setEditable:NO];
  [legalDocumentsView_ setBackgroundColor:
      [NSColor colorWithCalibratedRed:0.96
                                green:0.96
                                 blue:0.96
                                alpha:1.0]];
  [legalDocumentsView_ setDrawsBackground:YES];
  [legalDocumentsView_ setHidden:YES];
  [legalDocumentsView_ setDelegate:self];
  legalDocumentsSizeDirty_ = YES;
  [[self view] addSubview:legalDocumentsView_];

  notificationContainer_.reset(
      [[AutofillNotificationContainer alloc] initWithController:controller_]);
  [[self view] addSubview:[notificationContainer_ view]];
}

// Called when embedded links are clicked.
- (BOOL)textView:(NSTextView*)textView
    clickedOnLink:(id)link
          atIndex:(NSUInteger)charIndex {
  int index = [base::mac::ObjCCastStrict<NSNumber>(link) intValue];
  controller_->LegalDocumentLinkClicked(
      controller_->LegalDocumentLinks()[index]);
  return YES;
}

- (NSSize)preferredSize {
  // Overall width is determined by |detailsContainer_|.
  NSSize buttonSize = [buttonContainer_ frame].size;
  NSSize detailsSize = [detailsContainer_ preferredSize];

  NSSize size = NSMakeSize(std::max(buttonSize.width, detailsSize.width),
                           buttonSize.height + detailsSize.height);

  if (![legalDocumentsView_ isHidden]) {
    NSSize legalDocumentSize =
        [self preferredLegalDocumentSizeForWidth:detailsSize.width];
    size.height += legalDocumentSize.height + kVerticalSpacing;
  }

  NSSize notificationSize =
      [notificationContainer_ preferredSizeForWidth:detailsSize.width];
  size.height += notificationSize.height;
  return size;
}

- (void)performLayout {
  NSSize preferredContainerSize = [self preferredSize];

  CGFloat currentY = 0.0;
  if (![legalDocumentsView_ isHidden]) {
    [legalDocumentsView_ setFrameSize:
        [self preferredLegalDocumentSizeForWidth:preferredContainerSize.width]];
    currentY = NSMaxY([legalDocumentsView_ frame]) + kVerticalSpacing;
  }

  NSRect buttonFrame = [buttonContainer_ frame];
  buttonFrame.origin.y = currentY;
  [buttonContainer_ setFrameOrigin:buttonFrame.origin];

  [detailsContainer_ performLayout];
  NSRect containerFrame = [[detailsContainer_ view] frame];
  containerFrame.origin.y = NSMaxY(buttonFrame);
  [[detailsContainer_ view] setFrameOrigin:containerFrame.origin];

  NSRect notificationFrame;
  notificationFrame.origin = NSMakePoint(0, NSMaxY(containerFrame));
  notificationFrame.size = [notificationContainer_ preferredSizeForWidth:
      preferredContainerSize.width];
  [[notificationContainer_ view] setFrame:notificationFrame];
  [notificationContainer_ performLayout];
}

- (void)buildWindowButtonsForFrame:(NSRect)frame {
  if (buttonContainer_.get())
    return;

  buttonContainer_.reset([[GTMWidthBasedTweaker alloc] initWithFrame:
      ui::kWindowSizeDeterminedLater]);
  [buttonContainer_
      setAutoresizingMask:(NSViewMinXMargin | NSViewMaxYMargin)];

  base::scoped_nsobject<NSButton> button(
      [[ConstrainedWindowButton alloc] initWithFrame:NSZeroRect]);
  [button setTitle:l10n_util::GetNSStringWithFixup(IDS_CANCEL)];
  [button setKeyEquivalent:kKeyEquivalentEscape];
  [button setTarget:target_];
  [button setAction:@selector(cancel:)];
  [button sizeToFit];
  [buttonContainer_ addSubview:button];

  CGFloat nextX = NSMaxX([button frame]) + kButtonGap;
  button.reset([[ConstrainedWindowButton alloc] initWithFrame:NSZeroRect]);
  [button setFrameOrigin:NSMakePoint(nextX, 0)];
  [button  setTitle:l10n_util::GetNSStringWithFixup(
       IDS_AUTOFILL_DIALOG_SUBMIT_BUTTON)];
  [button setKeyEquivalent:kKeyEquivalentReturn];
  [button setTarget:target_];
  [button setAction:@selector(accept:)];
  [button sizeToFit];
  [buttonContainer_ addSubview:button];

  frame = NSMakeRect(
      NSWidth(frame) - NSMaxX([button frame]), 0,
      NSMaxX([button frame]), NSHeight([button frame]));

  [buttonContainer_ setFrame:frame];
}

- (void)layoutButtons {
  base::scoped_nsobject<GTMUILocalizerAndLayoutTweaker> layoutTweaker(
      [[GTMUILocalizerAndLayoutTweaker alloc] init]);
  [layoutTweaker tweakUI:buttonContainer_];
}

// Compute the preferred size for the legal documents text, given a width.
- (NSSize)preferredLegalDocumentSizeForWidth:(CGFloat)width {
  // Only recompute if necessary (On text or frame width change).
  if (!legalDocumentsSizeDirty_ && abs(legalDocumentsSize_.width-width) < 1.0)
    return legalDocumentsSize_;

  // There's no direct API to compute desired sizes - use layouting instead.
  // Layout in a rect with fixed width and "infinite" height.
  NSRect currentFrame = [legalDocumentsView_ frame];
  [legalDocumentsView_ setFrame:NSMakeRect(0, 0, width, CGFLOAT_MAX)];

  // Now use the layout manager to compute layout.
  NSLayoutManager* layoutManager = [legalDocumentsView_ layoutManager];
  NSTextContainer* textContainer = [legalDocumentsView_ textContainer];
  [layoutManager ensureLayoutForTextContainer:textContainer];
  NSRect newFrame = [layoutManager usedRectForTextContainer:textContainer];

  // And finally, restore old frame.
  [legalDocumentsView_ setFrame:currentFrame];
  newFrame.size.width = width;

  legalDocumentsSizeDirty_ = NO;
  legalDocumentsSize_ = newFrame.size;
  return legalDocumentsSize_;
}

- (AutofillSectionContainer*)sectionForId:(autofill::DialogSection)section {
  return [detailsContainer_ sectionForId:section];
}

- (void)modelChanged {
  [detailsContainer_ modelChanged];
}

- (void)updateLegalDocuments {
  NSString* text = base::SysUTF16ToNSString(controller_->LegalDocumentsText());

  if ([text length]) {
    NSFont* font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    [legalDocumentsView_ setMessage:text
                           withFont:font
                       messageColor:[NSColor blackColor]];

    const std::vector<ui::Range>& link_ranges =
        controller_->LegalDocumentLinks();
    for (size_t i = 0; i < link_ranges.size(); ++i) {
      NSRange range = link_ranges[i].ToNSRange();
      [legalDocumentsView_ addLinkRange:range
                               withName:@(i)
                              linkColor:[NSColor blueColor]];
    }
    legalDocumentsSizeDirty_ = YES;
  }
  [legalDocumentsView_ setHidden:[text length] == 0];

  // Always request re-layout on state change.
  id controller = [[[self view] window] windowController];
  if ([controller respondsToSelector:@selector(requestRelayout)])
    [controller performSelector:@selector(requestRelayout)];
}

- (void)updateNotificationArea {
  [notificationContainer_ setNotifications:controller_->CurrentNotifications()];
  id controller = [[[self view] window] windowController];
  if ([controller respondsToSelector:@selector(requestRelayout)])
    [controller performSelector:@selector(requestRelayout)];
}

- (void)setAnchorView:(NSView*)anchorView {
  [notificationContainer_ setAnchorView:anchorView];
}

@end
