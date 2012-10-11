// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A template for a simple rectangle class.  The containment semantics
// are array-like; that is, the coordinate (x, y) is considered to be
// contained by the rectangle, but the coordinate (x + width, y) is not.
// The class will happily let you create malformed rectangles (that is,
// rectangles with negative width and/or height), but there will be assertions
// in the operations (such as Contains()) to complain in this case.

#ifndef UI_GFX_RECT_BASE_H_
#define UI_GFX_RECT_BASE_H_

#include <string>

#include "base/compiler_specific.h"

namespace gfx {

template<typename Class,
         typename PointClass,
         typename SizeClass,
         typename InsetsClass,
         typename Type>
class UI_EXPORT RectBase {
 public:
  Type x() const { return origin_.x(); }
  void set_x(Type x) { origin_.set_x(x); }

  Type y() const { return origin_.y(); }
  void set_y(Type y) { origin_.set_y(y); }

  Type width() const { return size_.width(); }
  void set_width(Type width) { size_.set_width(width); }

  Type height() const { return size_.height(); }
  void set_height(Type height) { size_.set_height(height); }

  const PointClass& origin() const { return origin_; }
  void set_origin(const PointClass& origin) { origin_ = origin; }

  const SizeClass& size() const { return size_; }
  void set_size(const SizeClass& size) { size_ = size; }

  Type right() const { return x() + width(); }
  Type bottom() const { return y() + height(); }

  void SetRect(Type x, Type y, Type width, Type height);

  // Shrink the rectangle by a horizontal and vertical distance on all sides.
  void Inset(Type horizontal, Type vertical) {
    Inset(horizontal, vertical, horizontal, vertical);
  }

  // Shrink the rectangle by the given insets.
  void Inset(const InsetsClass& insets);

  // Shrink the rectangle by the specified amount on each side.
  void Inset(Type left, Type top, Type right, Type bottom);

  // Move the rectangle by a horizontal and vertical distance.
  void Offset(Type horizontal, Type vertical);
  void Offset(const PointClass& point) {
    Offset(point.x(), point.y());
  }

  InsetsClass InsetsFrom(const Class& inner) const {
    return InsetsClass(inner.y() - y(),
                       inner.x() - x(),
                       bottom() - inner.bottom(),
                       right() - inner.right());
  }

  // Returns true if the area of the rectangle is zero.
  bool IsEmpty() const { return size_.IsEmpty(); }

  // A rect is less than another rect if its origin is less than
  // the other rect's origin. If the origins are equal, then the
  // shortest rect is less than the other. If the origin and the
  // height are equal, then the narrowest rect is less than.
  // This comparison is required to use Rects in sets, or sorted
  // vectors.
  bool operator<(const Class& other) const;

  // Returns true if the point identified by point_x and point_y falls inside
  // this rectangle.  The point (x, y) is inside the rectangle, but the
  // point (x + width, y + height) is not.
  bool Contains(Type point_x, Type point_y) const;

  // Returns true if the specified point is contained by this rectangle.
  bool Contains(const PointClass& point) const {
    return Contains(point.x(), point.y());
  }

  // Returns true if this rectangle contains the specified rectangle.
  bool Contains(const Class& rect) const;

  // Returns true if this rectangle intersects the specified rectangle.
  bool Intersects(const Class& rect) const;

  // Computes the intersection of this rectangle with the given rectangle.
  Class Intersect(const Class& rect) const WARN_UNUSED_RESULT;

  // Computes the union of this rectangle with the given rectangle.  The union
  // is the smallest rectangle containing both rectangles.
  Class Union(const Class& rect) const WARN_UNUSED_RESULT;

  // Computes the rectangle resulting from subtracting |rect| from |this|.  If
  // |rect| does not intersect completely in either the x- or y-direction, then
  // |*this| is returned.  If |rect| contains |this|, then an empty Rect is
  // returned.
  Class Subtract(const Class& rect) const WARN_UNUSED_RESULT;

  // Fits as much of the receiving rectangle into the supplied rectangle as
  // possible, returning the result. For example, if the receiver had
  // a x-location of 2 and a width of 4, and the supplied rectangle had
  // an x-location of 0 with a width of 5, the returned rectangle would have
  // an x-location of 1 with a width of 4.
  Class AdjustToFit(const Class& rect) const WARN_UNUSED_RESULT;

  // Returns the center of this rectangle.
  PointClass CenterPoint() const;

  // Return a rectangle that has the same center point but with a size capped
  // at given |size|.
  Class Center(const SizeClass& size) const WARN_UNUSED_RESULT;

  // Splits |this| in two halves, |left_half| and |right_half|.
  void SplitVertically(Class* left_half, Class* right_half) const;

  // Returns true if this rectangle shares an entire edge (i.e., same width or
  // same height) with the given rectangle, and the rectangles do not overlap.
  bool SharesEdgeWith(const Class& rect) const;

 protected:
  RectBase(const PointClass& origin, const SizeClass& size);
  explicit RectBase(const SizeClass& size);
  explicit RectBase(const PointClass& origin);
  // Destructor is intentionally made non virtual and protected.
  // Do not make this public.
  ~RectBase();

 private:
  PointClass origin_;
  SizeClass size_;
};

}  // namespace gfx

#endif  // UI_GFX_RECT_BASE_H_
