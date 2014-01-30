// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CHILD_FRAME_COMPOSITING_HELPER_H_
#define CONTENT_RENDERER_CHILD_FRAME_COMPOSITING_HELPER_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "cc/layers/delegated_frame_resource_collection.h"
#include "content/common/content_export.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "ui/gfx/size.h"

namespace base {
class SharedMemory;
}

namespace cc {
class CompositorFrame;
class CopyOutputResult;
class Layer;
class SolidColorLayer;
class TextureLayer;
class DelegatedFrameProvider;
class DelegatedFrameResourceCollection;
class DelegatedRendererLayer;
}

namespace blink {
class WebFrame;
class WebPluginContainer;
class WebLayer;
}

namespace gfx {
class Rect;
class Size;
}

struct FrameHostMsg_CompositorFrameSwappedACK_Params;
struct FrameHostMsg_BuffersSwappedACK_Params;
struct FrameHostMsg_ReclaimCompositorResources_Params;

namespace content {

class BrowserPluginManager;
class RenderFrameImpl;

class CONTENT_EXPORT ChildFrameCompositingHelper
    : public base::RefCounted<ChildFrameCompositingHelper>,
      public cc::DelegatedFrameResourceCollectionClient {
 public:
  static ChildFrameCompositingHelper* CreateCompositingHelperForBrowserPlugin(
      blink::WebPluginContainer* container,
      BrowserPluginManager* manager,
      int instance_id,
      int host_routing_id);
  static ChildFrameCompositingHelper* CreateCompositingHelperForRenderFrame(
      blink::WebFrame* frame,
      RenderFrameImpl* render_frame,
      int host_routing_id);

  void CopyFromCompositingSurface(int request_id,
                                  gfx::Rect source_rect,
                                  gfx::Size dest_size);
  void DidCommitCompositorFrame();
  void EnableCompositing(bool);
  void OnContainerDestroy();
  void OnBuffersSwapped(const gfx::Size& size,
                        const std::string& mailbox_name,
                        int gpu_route_id,
                        int gpu_host_id,
                        float device_scale_factor);
  void OnCompositorFrameSwapped(scoped_ptr<cc::CompositorFrame> frame,
                                int route_id,
                                uint32 output_surface_id,
                                int host_id);
  void UpdateVisibility(bool);
  void ChildFrameGone();

  // cc::DelegatedFrameProviderClient implementation.
  virtual void UnusedResourcesAreAvailable() OVERRIDE;
  void SetContentsOpaque(bool);

 protected:
  // Friend RefCounted so that the dtor can be non-public.
  friend class base::RefCounted<ChildFrameCompositingHelper>;

 private:
  ChildFrameCompositingHelper(blink::WebPluginContainer* container,
                              blink::WebFrame* frame,
                              BrowserPluginManager* manager,
                              RenderFrameImpl* render_frame,
                              int instance_id,
                              int host_routing_id);

  enum SwapBuffersType {
    TEXTURE_IMAGE_TRANSPORT,
    GL_COMPOSITOR_FRAME,
    SOFTWARE_COMPOSITOR_FRAME,
  };
  struct SwapBuffersInfo {
    SwapBuffersInfo();

    gpu::Mailbox name;
    SwapBuffersType type;
    gfx::Size size;
    int route_id;
    uint32 output_surface_id;
    int host_id;
    unsigned software_frame_id;
    base::SharedMemory* shared_memory;
  };
  virtual ~ChildFrameCompositingHelper();
  void SendCompositorFrameSwappedACKToBrowser(
      FrameHostMsg_CompositorFrameSwappedACK_Params& params);
  void SendBuffersSwappedACKToBrowser(
      FrameHostMsg_BuffersSwappedACK_Params& params);
  void SendReclaimCompositorResourcesToBrowser(
      FrameHostMsg_ReclaimCompositorResources_Params& params);
  void CheckSizeAndAdjustLayerProperties(const gfx::Size& new_size,
                                         float device_scale_factor,
                                         cc::Layer* layer);
  void OnBuffersSwappedPrivate(const SwapBuffersInfo& mailbox,
                               unsigned sync_point,
                               float device_scale_factor);
  void MailboxReleased(SwapBuffersInfo mailbox,
                       unsigned sync_point,
                       bool lost_resource);
  void SendReturnedDelegatedResources();
  void CopyFromCompositingSurfaceHasResult(
      int request_id,
      gfx::Size dest_size,
      scoped_ptr<cc::CopyOutputResult> result);

  int instance_id_;
  int host_routing_id_;
  int last_route_id_;
  uint32 last_output_surface_id_;
  int last_host_id_;
  bool last_mailbox_valid_;
  bool ack_pending_;
  bool software_ack_pending_;
  bool opaque_;
  std::vector<unsigned> unacked_software_frames_;

  gfx::Size buffer_size_;

  scoped_refptr<cc::DelegatedFrameResourceCollection> resource_collection_;
  scoped_refptr<cc::DelegatedFrameProvider> frame_provider_;

  scoped_refptr<cc::SolidColorLayer> background_layer_;
  scoped_refptr<cc::TextureLayer> texture_layer_;
  scoped_refptr<cc::DelegatedRendererLayer> delegated_layer_;
  scoped_ptr<blink::WebLayer> web_layer_;
  blink::WebPluginContainer* container_;
  blink::WebFrame* frame_;

  scoped_refptr<BrowserPluginManager> browser_plugin_manager_;
  RenderFrameImpl* render_frame_;

  DISALLOW_COPY_AND_ASSIGN(ChildFrameCompositingHelper);
};

}  // namespace content

#endif  // CONTENT_RENDERER_CHILD_FRAME_COMPOSITING_HELPER_H_
