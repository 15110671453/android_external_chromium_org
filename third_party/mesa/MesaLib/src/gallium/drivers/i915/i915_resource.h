/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef I915_RESOURCE_H
#define I915_RESOURCE_H

struct i915_screen;

#include "util/u_transfer.h"
#include "util/u_debug.h"


struct i915_context;
struct i915_screen;


struct i915_buffer {
   struct u_resource b;
   uint8_t *data;
   boolean free_on_destroy;
};

#define I915_MAX_TEXTURE_2D_LEVELS 11  /* max 1024x1024 */
#define I915_MAX_TEXTURE_3D_LEVELS  8  /* max 128x128x128 */



struct i915_texture {
   struct u_resource b;

   unsigned stride;
   unsigned depth_stride;          /* per-image on i945? */
   unsigned total_nblocksy;

   unsigned sw_tiled; /**< tiled with software flags */
   unsigned hw_tiled; /**< tiled with hardware fences */

   unsigned nr_images[I915_MAX_TEXTURE_2D_LEVELS];

   /* Explicitly store the offset of each image for each cube face or
    * depth value.
    */
   unsigned *image_offset[I915_MAX_TEXTURE_2D_LEVELS];   /**< array [depth] of offsets */

   /* The data is held here:
    */
   struct i915_winsys_buffer *buffer;
};

void i915_init_screen_resource_functions(struct i915_screen *is);
void i915_init_resource_functions(struct i915_context *i915);

extern struct u_resource_vtbl i915_buffer_vtbl;
extern struct u_resource_vtbl i915_texture_vtbl;

static INLINE struct i915_texture *i915_texture(struct pipe_resource *resource)
{
   struct i915_texture *tex = (struct i915_texture *)resource;
   assert(tex->b.vtbl == &i915_texture_vtbl);
   return tex;
}

static INLINE struct i915_buffer *i915_buffer(struct pipe_resource *resource)
{
   struct i915_buffer *tex = (struct i915_buffer *)resource;
   assert(tex->b.vtbl == &i915_buffer_vtbl);
   return tex;
}

struct pipe_resource *
i915_texture_create(struct pipe_screen *screen,
                    const struct pipe_resource *template);

struct pipe_resource *
i915_texture_from_handle(struct pipe_screen * screen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle);


struct pipe_resource *
i915_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes,
			unsigned usage);

struct pipe_resource *
i915_buffer_create(struct pipe_screen *screen,
		   const struct pipe_resource *template);

#endif /* I915_RESOURCE_H */
