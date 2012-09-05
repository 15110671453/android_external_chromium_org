// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.graphics.RectF;

/**
 * Java equivalent to the C++ FindMatchRectsDetails class
 * defined in chrome/browser/ui/find_bar/find_match_rects_details.h
 */
public class FindMatchRectsDetails {
    /** Version of the the rects in this result. */
    public final int version;

    /** Rects of the find matches in find-in-page coordinates. */
    public final RectF[] rects;

    /** Rect of the active match in find-in-page coordinates. */
    public final RectF activeRect;

    public FindMatchRectsDetails(int version, RectF[] rects, RectF activeRect) {
        this.version = version;
        this.rects = rects;
        this.activeRect = activeRect;
    }
}
