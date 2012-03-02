// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_TEXT_UTF16_INDEXING_H_
#define UI_BASE_TEXT_UTF16_INDEXING_H_
#pragma once

#include "base/string16.h"
#include "ui/base/ui_export.h"

namespace ui {

// Returns false if s[index-1] is a high surrogate and s[index] is a low
// surrogate, true otherwise.
UI_EXPORT bool IsValidCodePointIndex(const string16& s, size_t index);

// |UTF16IndexToOffset| returns the number of code points between |base| and
// |pos| in the given string. |UTF16OffsetToIndex| returns the index that is
// |offset| code points away from the given |base| index. These functions are
// named after glib's |g_utf8_pointer_to_offset| and |g_utf8_offset_to_pointer|,
// which perform the same function for UTF-8. As in glib, it is an error to
// pass an |offset| that walks off the edge of the string.
//
// These functions attempt to deal with invalid use of UTF-16 surrogates in a
// way that makes as much sense as possible: unpaired surrogates are treated as
// single characters, and if an argument index points to the middle of a valid
// surrogate pair, it is treated as though it pointed to the end of that pair.
// The index returned by |UTF16OffsetToIndex| never points to the middle of a
// surrogate pair.
//
// The following identities hold:
//   If |s| contains no surrogate pairs, then
//     UTF16IndexToOffset(s, base, pos) == pos - base
//     UTF16OffsetToIndex(s, base, offset) == base + offset
//   If |pos| does not point to the middle of a surrogate pair, then
//     UTF16OffsetToIndex(s, base, UTF16IndexToOffset(s, base, pos)) == pos
//   Always,
//     UTF16IndexToOffset(s, base, UTF16OffsetToIndex(s, base, ofs)) == ofs
//     UTF16IndexToOffset(s, i, j) == -UTF16IndexToOffset(s, j, i)
UI_EXPORT ptrdiff_t UTF16IndexToOffset(const string16& s,
                                       size_t base,
                                       size_t pos);
UI_EXPORT size_t UTF16OffsetToIndex(const string16& s,
                                    size_t base,
                                    ptrdiff_t offset);

}  // namespace ui

#endif  // UI_BASE_TEXT_UTF16_INDEXING_H_
