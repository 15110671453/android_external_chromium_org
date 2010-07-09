// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GFX_CANVAS_DIRECT2D_H_
#define GFX_CANVAS_DIRECT2D_H_

#include <d2d1.h>

#include <stack>

#include "base/scoped_comptr_win.h"
#include "gfx/canvas.h"

namespace gfx {

class CanvasDirect2D : public Canvas {
 public:
  // Creates an empty Canvas.
  explicit CanvasDirect2D(ID2D1RenderTarget* rt);
  virtual ~CanvasDirect2D();

  // Retrieves the application's D2D1 Factory.
  static ID2D1Factory* GetD2D1Factory();

  // Overridden from Canvas:
  virtual void Save();
  virtual void SaveLayerAlpha(uint8 alpha);
  virtual void SaveLayerAlpha(uint8 alpha, const gfx::Rect& layer_bounds);
  virtual void Restore();
  virtual bool GetClipRect(gfx::Rect* clip_rect);
  virtual bool ClipRectInt(int x, int y, int w, int h);
  virtual bool IntersectsClipRectInt(int x, int y, int w, int h);
  virtual void TranslateInt(int x, int y);
  virtual void ScaleInt(int x, int y);
  virtual void FillRectInt(int x, int y, int w, int h,
                           const SkPaint& paint);
  virtual void FillRectInt(const SkColor& color, int x, int y, int w,
                           int h);
  virtual void DrawRectInt(const SkColor& color, int x, int y, int w,
                           int h);
  virtual void DrawRectInt(const SkColor& color, int x, int y, int w, int h,
                           SkXfermode::Mode mode);
  virtual void DrawLineInt(const SkColor& color, int x1, int y1, int x2,
                           int y2);
  virtual void DrawBitmapInt(const SkBitmap& bitmap, int x, int y);
  virtual void DrawBitmapInt(const SkBitmap& bitmap, int x, int y,
                             const SkPaint& paint);
  virtual void DrawBitmapInt(const SkBitmap& bitmap, int src_x, int src_y,
                             int src_w, int src_h, int dest_x, int dest_y,
                             int dest_w, int dest_h, bool filter);
  virtual void DrawBitmapInt(const SkBitmap& bitmap, int src_x, int src_y,
                             int src_w, int src_h, int dest_x, int dest_y,
                             int dest_w, int dest_h, bool filter,
                             const SkPaint& paint);
  virtual void DrawStringInt(const std::wstring& text, const gfx::Font& font,
                             const SkColor& color, int x, int y, int w,
                             int h);
  virtual void DrawStringInt(const std::wstring& text, const gfx::Font& font,
                             const SkColor& color,
                             const gfx::Rect& display_rect);
  virtual void DrawStringInt(const std::wstring& text, const gfx::Font& font,
                             const SkColor& color, int x, int y, int w, int h,
                             int flags);
  virtual void DrawFocusRect(int x, int y, int width, int height);
  virtual void TileImageInt(const SkBitmap& bitmap, int x, int y, int w, int h);
  virtual void TileImageInt(const SkBitmap& bitmap, int src_x, int src_y,
                            int dest_x, int dest_y, int w, int h);
  virtual gfx::NativeDrawingContext BeginPlatformPaint();
  virtual void EndPlatformPaint();
  virtual CanvasSkia* AsCanvasSkia();
  virtual const CanvasSkia* AsCanvasSkia() const;

 private:
  ID2D1RenderTarget* rt_;
  ScopedComPtr<ID2D1GdiInteropRenderTarget> interop_rt_;
  ScopedComPtr<ID2D1DrawingStateBlock> drawing_state_block_;
  static ID2D1Factory* d2d1_factory_;
  std::stack<ID2D1Layer*> layers_;

  DISALLOW_COPY_AND_ASSIGN(CanvasDirect2D);
};

}  // namespace gfx;

#endif  // GFX_CANVAS_DIRECT2D_H_
