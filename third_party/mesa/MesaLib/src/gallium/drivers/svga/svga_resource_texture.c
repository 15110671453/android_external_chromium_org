/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "svga_cmd.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "os/os_thread.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "svga_screen.h"
#include "svga_context.h"
#include "svga_resource_texture.h"
#include "svga_resource_buffer.h"
#include "svga_sampler_view.h"
#include "svga_winsys.h"
#include "svga_debug.h"


/* XXX: This isn't a real hardware flag, but just a hack for kernel to
 * know about primary surfaces. Find a better way to accomplish this.
 */
#define SVGA3D_SURFACE_HINT_SCANOUT (1 << 9)


static unsigned int
svga_texture_is_referenced( struct pipe_context *pipe,
			    struct pipe_resource *texture,
			    unsigned face, unsigned level)
{
   struct svga_texture *tex = svga_texture(texture);
   struct svga_screen *ss = svga_screen(pipe->screen);

   /**
    * The screen does not cache texture writes.
    */

   if (!tex->handle || ss->sws->surface_is_flushed(ss->sws, tex->handle))
      return PIPE_UNREFERENCED;

   /**
    * sws->surface_is_flushed() does not distinguish between read references
    * and write references. So assume a reference is both.
    */

   return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
}



/*
 * Helper function and arrays
 */

SVGA3dSurfaceFormat
svga_translate_format(enum pipe_format format)
{
   switch(format) {
   
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return SVGA3D_A8R8G8B8;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return SVGA3D_X8R8G8B8;

      /* Required for GL2.1:
       */
   case PIPE_FORMAT_B8G8R8A8_SRGB:
      return SVGA3D_A8R8G8B8;

   case PIPE_FORMAT_B5G6R5_UNORM:
      return SVGA3D_R5G6B5;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return SVGA3D_A1R5G5B5;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return SVGA3D_A4R4G4B4;

      
   /* XXX: Doesn't seem to work properly.
   case PIPE_FORMAT_Z32_UNORM:
      return SVGA3D_Z_D32;
    */
   case PIPE_FORMAT_Z16_UNORM:
      return SVGA3D_Z_D16;
   case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
      return SVGA3D_Z_D24S8;
   case PIPE_FORMAT_X8Z24_UNORM:
      return SVGA3D_Z_D24X8;

   case PIPE_FORMAT_A8_UNORM:
      return SVGA3D_ALPHA8;
   case PIPE_FORMAT_L8_UNORM:
      return SVGA3D_LUMINANCE8;

   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
      return SVGA3D_DXT1;
   case PIPE_FORMAT_DXT3_RGBA:
      return SVGA3D_DXT3;
   case PIPE_FORMAT_DXT5_RGBA:
      return SVGA3D_DXT5;

   default:
      return SVGA3D_FORMAT_INVALID;
   }
}


SVGA3dSurfaceFormat
svga_translate_format_render(enum pipe_format format)
{
   switch(format) { 
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B5G5R5A1_UNORM:
   case PIPE_FORMAT_B4G4R4A4_UNORM:
   case PIPE_FORMAT_B5G6R5_UNORM:
   case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_Z32_UNORM:
   case PIPE_FORMAT_Z16_UNORM:
   case PIPE_FORMAT_L8_UNORM:
      return svga_translate_format(format);

#if 1
   /* For on host conversion */
   case PIPE_FORMAT_DXT1_RGB:
      return SVGA3D_X8R8G8B8;
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT5_RGBA:
      return SVGA3D_A8R8G8B8;
#endif

   default:
      return SVGA3D_FORMAT_INVALID;
   }
}


static INLINE void
svga_transfer_dma_band(struct svga_context *svga,
                       struct svga_transfer *st,
                       SVGA3dTransferType transfer,
                       unsigned y, unsigned h, unsigned srcy)
{
   struct svga_texture *texture = svga_texture(st->base.resource); 
   SVGA3dCopyBox box;
   enum pipe_error ret;
   
   SVGA_DBG(DEBUG_DMA, "dma %s sid %p, face %u, (%u, %u, %u) - (%u, %u, %u), %ubpp\n",
                transfer == SVGA3D_WRITE_HOST_VRAM ? "to" : "from", 
                texture->handle,
                st->base.sr.face,
                st->base.box.x,
                y,
                st->base.box.z,
                st->base.box.x + st->base.box.width,
                y + h,
                st->base.box.z + 1,
                util_format_get_blocksize(texture->b.b.format) * 8 /
                (util_format_get_blockwidth(texture->b.b.format)*util_format_get_blockheight(texture->b.b.format)));
   
   box.x = st->base.box.x;
   box.y = y;
   box.z = st->base.box.z;
   box.w = st->base.box.width;
   box.h = h;
   box.d = 1;
   box.srcx = 0;
   box.srcy = srcy;
   box.srcz = 0;

   ret = SVGA3D_SurfaceDMA(svga->swc, st, transfer, &box, 1);
   if(ret != PIPE_OK) {
      svga->swc->flush(svga->swc, NULL);
      ret = SVGA3D_SurfaceDMA(svga->swc, st, transfer, &box, 1);
      assert(ret == PIPE_OK);
   }
}


static INLINE void
svga_transfer_dma(struct svga_context *svga,
                  struct svga_transfer *st,
                  SVGA3dTransferType transfer)
{
   struct svga_texture *texture = svga_texture(st->base.resource); 
   struct svga_screen *screen = svga_screen(texture->b.b.screen);
   struct svga_winsys_screen *sws = screen->sws;
   struct pipe_fence_handle *fence = NULL;
   
   if (transfer == SVGA3D_READ_HOST_VRAM) {
      SVGA_DBG(DEBUG_PERF, "%s: readback transfer\n", __FUNCTION__);
   }


   if(!st->swbuf) {
      /* Do the DMA transfer in a single go */
      
      svga_transfer_dma_band(svga, st, transfer, st->base.box.y, st->base.box.height, 0);

      if(transfer == SVGA3D_READ_HOST_VRAM) {
         svga_context_flush(svga, &fence);
         sws->fence_finish(sws, fence, 0);
         sws->fence_reference(sws, &fence, NULL);
      }
   }
   else {
      unsigned y, h, srcy;
      unsigned blockheight = util_format_get_blockheight(st->base.resource->format);
      h = st->hw_nblocksy * blockheight;
      srcy = 0;
      for(y = 0; y < st->base.box.height; y += h) {
         unsigned offset, length;
         void *hw, *sw;

         if (y + h > st->base.box.height)
            h = st->base.box.height - y;

         /* Transfer band must be aligned to pixel block boundaries */
         assert(y % blockheight == 0);
         assert(h % blockheight == 0);
         
         offset = y * st->base.stride / blockheight;
         length = h * st->base.stride / blockheight;

         sw = (uint8_t *)st->swbuf + offset;
         
         if(transfer == SVGA3D_WRITE_HOST_VRAM) {
            /* Wait for the previous DMAs to complete */
            /* TODO: keep one DMA (at half the size) in the background */
            if(y) {
               svga_context_flush(svga, &fence);
               sws->fence_finish(sws, fence, 0);
               sws->fence_reference(sws, &fence, NULL);
            }

            hw = sws->buffer_map(sws, st->hwbuf, PIPE_TRANSFER_WRITE);
            assert(hw);
            if(hw) {
               memcpy(hw, sw, length);
               sws->buffer_unmap(sws, st->hwbuf);
            }
         }
         
         svga_transfer_dma_band(svga, st, transfer, y, h, srcy);
         
         if(transfer == SVGA3D_READ_HOST_VRAM) {
            svga_context_flush(svga, &fence);
            sws->fence_finish(sws, fence, 0);

            hw = sws->buffer_map(sws, st->hwbuf, PIPE_TRANSFER_READ);
            assert(hw);
            if(hw) {
               memcpy(sw, hw, length);
               sws->buffer_unmap(sws, st->hwbuf);
            }
         }
      }
   }
}





static boolean 
svga_texture_get_handle(struct pipe_screen *screen,
                               struct pipe_resource *texture,
                               struct winsys_handle *whandle)
{
   struct svga_winsys_screen *sws = svga_winsys_screen(texture->screen);
   unsigned stride;

   assert(svga_texture(texture)->key.cachable == 0);
   svga_texture(texture)->key.cachable = 0;
   stride = util_format_get_nblocksx(texture->format, texture->width0) *
            util_format_get_blocksize(texture->format);
   return sws->surface_get_handle(sws, svga_texture(texture)->handle, stride, whandle);
}


static void
svga_texture_destroy(struct pipe_screen *screen,
		     struct pipe_resource *pt)
{
   struct svga_screen *ss = svga_screen(screen);
   struct svga_texture *tex = (struct svga_texture *)pt;

   ss->texture_timestamp++;

   svga_sampler_view_reference(&tex->cached_view, NULL);

   /*
     DBG("%s deleting %p\n", __FUNCTION__, (void *) tex);
   */
   SVGA_DBG(DEBUG_DMA, "unref sid %p (texture)\n", tex->handle);
   svga_screen_surface_destroy(ss, &tex->key, &tex->handle);

   FREE(tex);
}







/* XXX: Still implementing this as if it was a screen function, but
 * can now modify it to queue transfers on the context.
 */
static struct pipe_transfer *
svga_texture_get_transfer(struct pipe_context *pipe,
			  struct pipe_resource *texture,
			  struct pipe_subresource sr,
			  unsigned usage,
			  const struct pipe_box *box)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st;
   unsigned nblocksx = util_format_get_nblocksx(texture->format, box->width);
   unsigned nblocksy = util_format_get_nblocksy(texture->format, box->height);

   /* We can't map texture storage directly */
   if (usage & PIPE_TRANSFER_MAP_DIRECTLY)
      return NULL;

   st = CALLOC_STRUCT(svga_transfer);
   if (!st)
      return NULL;
   
   pipe_resource_reference(&st->base.resource, texture);
   st->base.sr = sr;
   st->base.usage = usage;
   st->base.box = *box;
   st->base.stride = nblocksx*util_format_get_blocksize(texture->format);
   st->base.slice_stride = 0;

   st->hw_nblocksy = nblocksy;
   
   st->hwbuf = svga_winsys_buffer_create(svga,
                                         1, 
                                         0,
                                         st->hw_nblocksy*st->base.stride);
   while(!st->hwbuf && (st->hw_nblocksy /= 2)) {
      st->hwbuf = svga_winsys_buffer_create(svga,
                                            1, 
                                            0,
                                            st->hw_nblocksy*st->base.stride);
   }

   if(!st->hwbuf)
      goto no_hwbuf;

   if(st->hw_nblocksy < nblocksy) {
      /* We couldn't allocate a hardware buffer big enough for the transfer, 
       * so allocate regular malloc memory instead */
      debug_printf("%s: failed to allocate %u KB of DMA, splitting into %u x %u KB DMA transfers\n",
                   __FUNCTION__,
                   (nblocksy*st->base.stride + 1023)/1024,
                   (nblocksy + st->hw_nblocksy - 1)/st->hw_nblocksy,
                   (st->hw_nblocksy*st->base.stride + 1023)/1024);
      st->swbuf = MALLOC(nblocksy*st->base.stride);
      if(!st->swbuf)
         goto no_swbuf;
   }
   
   if (usage & PIPE_TRANSFER_READ)
      svga_transfer_dma(svga, st, SVGA3D_READ_HOST_VRAM);

   return &st->base;

no_swbuf:
   sws->buffer_destroy(sws, st->hwbuf);
no_hwbuf:
   FREE(st);
   return NULL;
}


/* XXX: Still implementing this as if it was a screen function, but
 * can now modify it to queue transfers on the context.
 */
static void *
svga_texture_transfer_map( struct pipe_context *pipe,
			   struct pipe_transfer *transfer )
{
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st = svga_transfer(transfer);

   if(st->swbuf)
      return st->swbuf;
   else
      /* The wait for read transfers already happened when svga_transfer_dma
       * was called. */
      return sws->buffer_map(sws, st->hwbuf, transfer->usage);
}


/* XXX: Still implementing this as if it was a screen function, but
 * can now modify it to queue transfers on the context.
 */
static void
svga_texture_transfer_unmap(struct pipe_context *pipe,
			    struct pipe_transfer *transfer)
{
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st = svga_transfer(transfer);
   
   if(!st->swbuf)
      sws->buffer_unmap(sws, st->hwbuf);
}


static void
svga_texture_transfer_destroy(struct pipe_context *pipe,
			      struct pipe_transfer *transfer)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_texture *tex = svga_texture(transfer->resource);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st = svga_transfer(transfer);

   if (st->base.usage & PIPE_TRANSFER_WRITE) {
      svga_transfer_dma(svga, st, SVGA3D_WRITE_HOST_VRAM);
      ss->texture_timestamp++;
      tex->view_age[transfer->sr.level] = ++(tex->age);
      tex->defined[transfer->sr.face][transfer->sr.level] = TRUE;
   }

   pipe_resource_reference(&st->base.resource, NULL);
   FREE(st->swbuf);
   sws->buffer_destroy(sws, st->hwbuf);
   FREE(st);
}





struct u_resource_vtbl svga_texture_vtbl = 
{
   svga_texture_get_handle,	      /* get_handle */
   svga_texture_destroy,	      /* resource_destroy */
   svga_texture_is_referenced,	      /* is_resource_referenced */
   svga_texture_get_transfer,	      /* get_transfer */
   svga_texture_transfer_destroy,     /* transfer_destroy */
   svga_texture_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   svga_texture_transfer_unmap,	      /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};




struct pipe_resource *
svga_texture_create(struct pipe_screen *screen,
                    const struct pipe_resource *template)
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_texture *tex = CALLOC_STRUCT(svga_texture);
   
   if (!tex)
      goto error1;

   tex->b.b = *template;
   tex->b.vtbl = &svga_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   assert(template->last_level < SVGA_MAX_TEXTURE_LEVELS);
   if(template->last_level >= SVGA_MAX_TEXTURE_LEVELS)
      goto error2;
   
   tex->key.flags = 0;
   tex->key.size.width = template->width0;
   tex->key.size.height = template->height0;
   tex->key.size.depth = template->depth0;
   
   if(template->target == PIPE_TEXTURE_CUBE) {
      tex->key.flags |= SVGA3D_SURFACE_CUBEMAP;
      tex->key.numFaces = 6;
   }
   else {
      tex->key.numFaces = 1;
   }

   tex->key.cachable = 1;

   if (template->bind & PIPE_BIND_SAMPLER_VIEW)
      tex->key.flags |= SVGA3D_SURFACE_HINT_TEXTURE;

   if (template->bind & PIPE_BIND_DISPLAY_TARGET) {
      tex->key.cachable = 0;
   }

   if (template->bind & PIPE_BIND_SHARED) {
      tex->key.cachable = 0;
   }

   if (template->bind & PIPE_BIND_SCANOUT) {
      tex->key.flags |= SVGA3D_SURFACE_HINT_SCANOUT;
      tex->key.cachable = 0;
   }
   
   /* 
    * XXX: Never pass the SVGA3D_SURFACE_HINT_RENDERTARGET hint. Mesa cannot
    * know beforehand whether a texture will be used as a rendertarget or not
    * and it always requests PIPE_BIND_RENDER_TARGET, therefore
    * passing the SVGA3D_SURFACE_HINT_RENDERTARGET here defeats its purpose.
    */
#if 0
   if((template->bind & PIPE_BIND_RENDER_TARGET) &&
      !util_format_is_s3tc(template->format))
      tex->key.flags |= SVGA3D_SURFACE_HINT_RENDERTARGET;
#endif
   
   if(template->bind & PIPE_BIND_DEPTH_STENCIL)
      tex->key.flags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;
   
   tex->key.numMipLevels = template->last_level + 1;
   
   tex->key.format = svga_translate_format(template->format);
   if(tex->key.format == SVGA3D_FORMAT_INVALID)
      goto error2;

   SVGA_DBG(DEBUG_DMA, "surface_create for texture\n", tex->handle);
   tex->handle = svga_screen_surface_create(svgascreen, &tex->key);
   if (tex->handle)
      SVGA_DBG(DEBUG_DMA, "  --> got sid %p (texture)\n", tex->handle);

   return &tex->b.b;

error2:
   FREE(tex);
error1:
   return NULL;
}




struct pipe_resource *
svga_texture_from_handle(struct pipe_screen *screen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle)
{
   struct svga_winsys_screen *sws = svga_winsys_screen(screen);
   struct svga_winsys_surface *srf;
   struct svga_texture *tex;
   enum SVGA3dSurfaceFormat format = 0;
   assert(screen);

   /* Only supports one type */
   if ((template->target != PIPE_TEXTURE_2D &&
       template->target != PIPE_TEXTURE_RECT) ||
       template->last_level != 0 ||
       template->depth0 != 1) {
      return NULL;
   }

   srf = sws->surface_from_handle(sws, whandle, &format);

   if (!srf)
      return NULL;

   if (svga_translate_format(template->format) != format) {
      unsigned f1 = svga_translate_format(template->format);
      unsigned f2 = format;

      /* It's okay for XRGB and ARGB or depth with/out stencil to get mixed up */
      if ( !( (f1 == SVGA3D_X8R8G8B8 && f2 == SVGA3D_A8R8G8B8) ||
              (f1 == SVGA3D_A8R8G8B8 && f2 == SVGA3D_X8R8G8B8) ||
              (f1 == SVGA3D_Z_D24X8 && f2 == SVGA3D_Z_D24S8) ) ) {
         debug_printf("%s wrong format %u != %u\n", __FUNCTION__, f1, f2);
         return NULL;
      }
   }

   tex = CALLOC_STRUCT(svga_texture);
   if (!tex)
      return NULL;

   tex->b.b = *template;
   tex->b.vtbl = &svga_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   if (format == SVGA3D_X8R8G8B8)
      tex->b.b.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   else if (format == SVGA3D_A8R8G8B8)
      tex->b.b.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   else {
      /* ?? */
   }

   SVGA_DBG(DEBUG_DMA, "wrap surface sid %p\n", srf);

   tex->key.cachable = 0;
   tex->handle = srf;

   return &tex->b.b;
}

