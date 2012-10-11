// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SIZE_F_H_
#define UI_GFX_SIZE_F_H_

#include <string>

#include "ui/base/ui_export.h"
#include "ui/gfx/size_base.h"

namespace gfx {

// A floating version of gfx::Size.
class UI_EXPORT SizeF : public SizeBase<SizeF, float> {
 public:
  SizeF();
  SizeF(float width, float height);
  ~SizeF();

  SizeF Scale(float scale) const WARN_UNUSED_RESULT {
    return Scale(scale, scale);
  }

  SizeF Scale(float x_scale, float y_scale) const WARN_UNUSED_RESULT {
    return SizeF(width() * x_scale, height() * y_scale);
  }

  std::string ToString() const;
};

inline bool operator==(const SizeF& lhs, const SizeF& rhs) {
  return lhs.width() == rhs.width() && lhs.height() == rhs.height();
}

inline bool operator!=(const SizeF& lhs, const SizeF& rhs) {
  return !(lhs == rhs);
}

#if !defined(COMPILER_MSVC)
extern template class SizeBase<SizeF, float>;
#endif

}  // namespace gfx

#endif  // UI_GFX_SIZE_F_H_
