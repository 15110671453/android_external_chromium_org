// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_FLASH_API_H_
#define PPAPI_THUNK_PPB_FLASH_API_H_

#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/thunk/ppapi_thunk_export.h"

namespace ppapi {
namespace thunk {

// This class collects all of the Flash interface-related APIs into one place.
class PPAPI_THUNK_EXPORT PPB_Flash_API {
 public:
  virtual ~PPB_Flash_API() {}

  // Flash.
  virtual void SetInstanceAlwaysOnTop(PP_Instance instance, PP_Bool on_top) = 0;
  virtual PP_Bool DrawGlyphs(PP_Instance instance,
                             PP_Resource pp_image_data,
                             const PP_FontDescription_Dev* font_desc,
                             uint32_t color,
                             const PP_Point* position,
                             const PP_Rect* clip,
                             const float transformation[3][3],
                             PP_Bool allow_subpixel_aa,
                             uint32_t glyph_count,
                             const uint16_t glyph_indices[],
                             const PP_Point glyph_advances[]) = 0;
  virtual PP_Var GetProxyForURL(PP_Instance instance, const char* url) = 0;
  virtual int32_t Navigate(PP_Instance instance,
                           PP_Resource request_info,
                           const char* target,
                           PP_Bool from_user_action) = 0;
  virtual void RunMessageLoop(PP_Instance instance) = 0;
  virtual void QuitMessageLoop(PP_Instance instance) = 0;
  virtual double GetLocalTimeZoneOffset(PP_Instance instance, PP_Time t) = 0;
  virtual PP_Bool IsRectTopmost(PP_Instance instance, const PP_Rect* rect) = 0;
  virtual int32_t InvokePrinting(PP_Instance instance) = 0;
  virtual void UpdateActivity(PP_Instance instance) = 0;
  virtual PP_Var GetDeviceID(PP_Instance instance) = 0;

  // FlashFullscreen.
  virtual PP_Bool FlashIsFullscreen(PP_Instance instance) = 0;
  virtual PP_Bool FlashSetFullscreen(PP_Instance instance,
                                     PP_Bool fullscreen) = 0;
  virtual PP_Bool FlashGetScreenSize(PP_Instance instance, PP_Size* size) = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif // PPAPI_THUNK_PPB_FLASH_API_H_
