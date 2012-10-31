// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines a simple float vector class.  This class is used to indicate a
// distance in two dimensions between two points. Subtracting two points should
// produce a vector, and adding a vector to a point produces the point at the
// vector's distance from the original point.

#ifndef UI_GFX_VECTOR2D_F_H_
#define UI_GFX_VECTOR2D_F_H_

#include <string>

#include "ui/base/ui_export.h"

namespace gfx {

class UI_EXPORT Vector2dF {
 public:
  Vector2dF();
  Vector2dF(float x, float y);

  float x() const { return x_; }
  void set_x(float x) { x_ = x; }

  float y() const { return y_; }
  void set_y(float y) { y_ = y; }

  // True if both components of the vector are 0.
  bool IsZero() const;

  // Add the components of the |other| vector to the current vector.
  void Add(const Vector2dF& other);
  // Subtract the components of the |other| vector from the current vector.
  void Subtract(const Vector2dF& other);

  void operator+=(const Vector2dF& other) { Add(other); }
  void operator-=(const Vector2dF& other) { Subtract(other); }

  // Gives the square of the diagonal length of the vector.
  double LengthSquared() const;
  // Gives the diagonal length of the vector.
  float Length() const;

  // Scale the x and y components of the vector by |scale|.
  void Scale(float scale) { Scale(scale, scale); }
  // Scale the x and y components of the vector by |x_scale| and |y_scale|
  // respectively.
  void Scale(float x_scale, float y_scale);

  std::string ToString() const;

 private:
  float x_;
  float y_;
};

inline bool operator==(const Vector2dF& lhs, const Vector2dF& rhs) {
  return lhs.x() == rhs.x() && lhs.y() == rhs.y();
}

inline Vector2dF operator-(const Vector2dF& v) {
  return Vector2dF(-v.x(), -v.y());
}

inline Vector2dF operator+(const Vector2dF& lhs, const Vector2dF& rhs) {
  Vector2dF result = lhs;
  result.Add(rhs);
  return result;
}

inline Vector2dF operator-(const Vector2dF& lhs, const Vector2dF& rhs) {
  Vector2dF result = lhs;
  result.Add(-rhs);
  return result;
}

}  // namespace gfx

#endif // UI_GFX_VECTOR2D_F_H_
