// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_SOFTWARE_OUTPUT_DEVICE_LINUX_H_
#define CONTENT_BROWSER_RENDERER_HOST_SOFTWARE_OUTPUT_DEVICE_LINUX_H_

#include "cc/software_output_device.h"
#include "ui/base/x/x11_util.h"

namespace ui {
class Compositor;
}

namespace content {

class SoftwareOutputDeviceLinux : public cc::SoftwareOutputDevice {
 public:
  explicit SoftwareOutputDeviceLinux(ui::Compositor* compositor);

  virtual ~SoftwareOutputDeviceLinux();

  virtual void Resize(const gfx::Size& viewport_size) OVERRIDE;

  virtual void EndPaint(cc::SoftwareFrameData* frame_data) OVERRIDE;

 private:
  void ClearImage();

  ui::Compositor* compositor_;
  Display* display_;
  GC gc_;
  XImage* image_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_SOFTWARE_OUTPUT_DEVICE_LINUX_H_
