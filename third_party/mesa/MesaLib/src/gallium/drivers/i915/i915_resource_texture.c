/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "i915_context.h"
#include "i915_resource.h"
#include "i915_screen.h"
#include "i915_winsys.h"
#include "i915_debug.h"


#define DEBUG_TEXTURES 0

/*
 * Helper function and arrays
 */


/**
 * Initial offset for Cube map.
 */
static const int initial_offsets[6][2] = {
   [PIPE_TEX_FACE_POS_X] = {0, 0},
   [PIPE_TEX_FACE_POS_Y] = {1, 0},
   [PIPE_TEX_FACE_POS_Z] = {1, 1},
   [PIPE_TEX_FACE_NEG_X] = {0, 2},
   [PIPE_TEX_FACE_NEG_Y] = {1, 2},
   [PIPE_TEX_FACE_NEG_Z] = {1, 3},
};

/**
 * Step offsets for Cube map.
 */
static const int step_offsets[6][2] = {
   [PIPE_TEX_FACE_POS_X] = { 0, 2},
   [PIPE_TEX_FACE_POS_Y] = {-1, 2},
   [PIPE_TEX_FACE_POS_Z] = {-1, 1},
   [PIPE_TEX_FACE_NEG_X] = { 0, 2},
   [PIPE_TEX_FACE_NEG_Y] = {-1, 2},
   [PIPE_TEX_FACE_NEG_Z] = {-1, 1},
};

/**
 * For compressed level 2
 */
static const int bottom_offsets[6] = {
   [PIPE_TEX_FACE_POS_X] = 16 + 0 * 8,
   [PIPE_TEX_FACE_POS_Y] = 16 + 1 * 8,
   [PIPE_TEX_FACE_POS_Z] = 16 + 2 * 8,
   [PIPE_TEX_FACE_NEG_X] = 16 + 3 * 8,
   [PIPE_TEX_FACE_NEG_Y] = 16 + 4 * 8,
   [PIPE_TEX_FACE_NEG_Z] = 16 + 5 * 8,
};

static INLINE unsigned
align_nblocksx(enum pipe_format format, unsigned width, unsigned align_to)
{
   return align(util_format_get_nblocksx(format, width), align_to);
}

static INLINE unsigned
align_nblocksy(enum pipe_format format, unsigned width, unsigned align_to)
{
   return align(util_format_get_nblocksy(format, width), align_to);
}

static INLINE unsigned
get_pot_stride(enum pipe_format format, unsigned width)
{
   return util_next_power_of_two(util_format_get_stride(format, width));
}

/*
 * More advanced helper funcs
 */


static void
i915_texture_set_level_info(struct i915_texture *tex,
                            unsigned level, unsigned nr_images)
{
   assert(level < Elements(tex->nr_images));
   assert(nr_images);
   assert(!tex->image_offset[level]);

   tex->nr_images[level] = nr_images;
   tex->image_offset[level] = (unsigned *) MALLOC(nr_images * sizeof(unsigned));
   tex->image_offset[level][0] = 0;
}

static void
i915_texture_set_image_offset(struct i915_texture *tex,
                              unsigned level, unsigned img,
                              unsigned x, unsigned y)
{
   /* for the first image and level make sure offset is zero */
   assert(!(img == 0 && level == 0) || (x == 0 && y == 0));
   assert(img < tex->nr_images[level]);

   tex->image_offset[level][img] = y * tex->stride + x * util_format_get_blocksize(tex->b.b.format);

#if DEBUG_TEXTURES
   debug_printf("%s: %p level %u, img %u (%u, %u) %p\n", __FUNCTION__,
                tex, level, img, x, y,
                (void*)(uintptr_t)tex->image_offset[level][img]);
#endif
}


/*
 * Shared layout functions
 */


/**
 * Special case to deal with scanout textures.
 */
static boolean
i9x5_scanout_layout(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;

   if (pt->last_level > 0 || util_format_get_blocksize(pt->format) != 4)
      return FALSE;

   i915_texture_set_level_info(tex, 0, 1);
   i915_texture_set_image_offset(tex, 0, 0, 0, 0);

   if (pt->width0 >= 240) {
      tex->stride = get_pot_stride(pt->format, pt->width0);
      tex->total_nblocksy = align_nblocksy(pt->format, pt->height0, 8);
      tex->hw_tiled = I915_TILE_X;
   } else if (pt->width0 == 64 && pt->height0 == 64) {
      tex->stride = get_pot_stride(pt->format, pt->width0);
      tex->total_nblocksy = align_nblocksy(pt->format, pt->height0, 8);
   } else {
      return FALSE;
   }

#if DEBUG_TEXTURE
   debug_printf("%s size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
      pt->width0, pt->height0, util_format_get_blocksize(pt->format),
      tex->stride, tex->total_nblocksy, tex->stride * tex->total_nblocksy);
#endif

   return TRUE;
}

/**
 * Special case to deal with shared textures.
 */
static boolean
i9x5_display_target_layout(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;

   if (pt->last_level > 0 || util_format_get_blocksize(pt->format) != 4)
      return FALSE;

   /* fallback to normal textures for small textures */
   if (pt->width0 < 240)
      return FALSE;

   i915_texture_set_level_info(tex, 0, 1);
   i915_texture_set_image_offset(tex, 0, 0, 0, 0);

   tex->stride = get_pot_stride(pt->format, pt->width0);
   tex->total_nblocksy = align_nblocksy(pt->format, pt->height0, 8);
   tex->hw_tiled = I915_TILE_X;

#if DEBUG_TEXTURE
   debug_printf("%s size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
      pt->width0, pt->height0, util_format_get_blocksize(pt->format),
      tex->stride, tex->total_nblocksy, tex->stride * tex->total_nblocksy);
#endif

   return TRUE;
}

/**
 * Helper function for special layouts
 */
static boolean
i9x5_special_layout(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;

   /* Scanouts needs special care */
   if (pt->bind & PIPE_BIND_SCANOUT)
      if (i9x5_scanout_layout(tex))
         return TRUE;

   /* Shared buffers needs to be compatible with X servers
    *
    * XXX: need a better name than shared for this if it is to be part
    * of core gallium, and probably move the flag to resource.flags,
    * rather than bindings.
    */
   if (pt->bind & (PIPE_BIND_SHARED | PIPE_BIND_DISPLAY_TARGET))
      if (i9x5_display_target_layout(tex))
         return TRUE;

   return FALSE;
}

/**
 * Cube layout used on i915 and for non-compressed textures on i945.
 */
static void
i9x5_texture_layout_cube(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   const unsigned nblocks = util_format_get_nblocksx(pt->format, pt->width0);
   unsigned level;
   unsigned face;

   assert(pt->width0 == pt->height0); /* cubemap images are square */

   /* double pitch for cube layouts */
   tex->stride = align(nblocks * util_format_get_blocksize(pt->format) * 2, 4);
   tex->total_nblocksy = nblocks * 4;

   for (level = 0; level <= pt->last_level; level++)
      i915_texture_set_level_info(tex, level, 6);

   for (face = 0; face < 6; face++) {
      unsigned x = initial_offsets[face][0] * nblocks;
      unsigned y = initial_offsets[face][1] * nblocks;
      unsigned d = nblocks;

      for (level = 0; level <= pt->last_level; level++) {
         i915_texture_set_image_offset(tex, level, face, x, y);
         d >>= 1;
         x += step_offsets[face][0] * d;
         y += step_offsets[face][1] * d;
      }
   }
}


/*
 * i915 layout functions
 */


static void
i915_texture_layout_2d(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned nblocksy = util_format_get_nblocksy(pt->format, pt->width0);
   unsigned align_y = 2;

   if (util_format_is_s3tc(pt->format))
      align_y = 1;

   tex->stride = align(util_format_get_stride(pt->format, pt->width0), 4);
   tex->total_nblocksy = 0;

   for (level = 0; level <= pt->last_level; level++) {
      i915_texture_set_level_info(tex, level, 1);
      i915_texture_set_image_offset(tex, level, 0, 0, tex->total_nblocksy);

      tex->total_nblocksy += nblocksy;

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      nblocksy = align_nblocksy(pt->format, height, align_y);
   }
}

static void
i915_texture_layout_3d(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   unsigned level;

   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   unsigned nblocksy = util_format_get_nblocksy(pt->format, pt->height0);
   unsigned stack_nblocksy = 0;

   /* Calculate the size of a single slice. 
    */
   tex->stride = align(util_format_get_stride(pt->format, pt->width0), 4);

   /* XXX: hardware expects/requires 9 levels at minimum.
    */
   for (level = 0; level <= MAX2(8, pt->last_level); level++) {
      i915_texture_set_level_info(tex, level, depth);

      stack_nblocksy += MAX2(2, nblocksy);

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      nblocksy = util_format_get_nblocksy(pt->format, height);
   }

   /* Fixup depth image_offsets: 
    */
   for (level = 0; level <= pt->last_level; level++) {
      unsigned i;
      for (i = 0; i < depth; i++) 
         i915_texture_set_image_offset(tex, level, i, 0, i * stack_nblocksy);

      depth = u_minify(depth, 1);
   }

   /* Multiply slice size by texture depth for total size.  It's
    * remarkable how wasteful of memory the i915 texture layouts
    * are.  They are largely fixed in the i945.
    */
   tex->total_nblocksy = stack_nblocksy * pt->depth0;
}

static boolean
i915_texture_layout(struct i915_texture * tex)
{
   struct pipe_resource *pt = &tex->b.b;

   switch (pt->target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      if (!i9x5_special_layout(tex))
         i915_texture_layout_2d(tex);
      break;
   case PIPE_TEXTURE_3D:
      i915_texture_layout_3d(tex);
      break;
   case PIPE_TEXTURE_CUBE:
      i9x5_texture_layout_cube(tex);
      break;
   default:
      assert(0);
      return FALSE;
   }

   return TRUE;
}


/*
 * i945 layout functions
 */


static void
i945_texture_layout_2d(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   int align_x = 4, align_y = 2;
   unsigned level;
   unsigned x = 0;
   unsigned y = 0;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned nblocksx = util_format_get_nblocksx(pt->format, pt->width0);
   unsigned nblocksy = util_format_get_nblocksy(pt->format, pt->height0);

   if (util_format_is_s3tc(pt->format)) {
      align_x = 1;
      align_y = 1;
   }

   tex->stride = align(util_format_get_stride(pt->format, pt->width0), 4);

   /* May need to adjust pitch to accomodate the placement of
    * the 2nd mipmap level.  This occurs when the alignment
    * constraints of mipmap placement push the right edge of the
    * 2nd mipmap level out past the width of its parent.
    */
   if (pt->last_level > 0) {
      unsigned mip1_nblocksx =
         align_nblocksx(pt->format, u_minify(pt->width0, 1), align_x) +
         util_format_get_nblocksx(pt->format, u_minify(pt->width0, 2));

      if (mip1_nblocksx > nblocksx)
         tex->stride = mip1_nblocksx * util_format_get_blocksize(pt->format);
   }

   /* Pitch must be a whole number of dwords
    */
   tex->stride = align(tex->stride, 64);
   tex->total_nblocksy = 0;

   for (level = 0; level <= pt->last_level; level++) {
      i915_texture_set_level_info(tex, level, 1);
      i915_texture_set_image_offset(tex, level, 0, x, y);

      /* Because the images are packed better, the final offset
       * might not be the maximal one:
       */
      tex->total_nblocksy = MAX2(tex->total_nblocksy, y + nblocksy);

      /* Layout_below: step right after second mipmap level.
       */
      if (level == 1) {
         x += nblocksx;
      } else {
         y += nblocksy;
      }

      width  = u_minify(width, 1);
      height = u_minify(height, 1);
      nblocksx = align_nblocksx(pt->format, width, align_x);
      nblocksy = align_nblocksy(pt->format, height, align_y);
   }
}

static void
i945_texture_layout_3d(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   unsigned nblocksy = util_format_get_nblocksy(pt->format, pt->width0);
   unsigned pack_x_pitch, pack_x_nr;
   unsigned pack_y_pitch;
   unsigned level;

   tex->stride = align(util_format_get_stride(pt->format, pt->width0), 4);
   tex->total_nblocksy = 0;

   pack_y_pitch = MAX2(nblocksy, 2);
   pack_x_pitch = tex->stride / util_format_get_blocksize(pt->format);
   pack_x_nr = 1;

   for (level = 0; level <= pt->last_level; level++) {
      int x = 0;
      int y = 0;
      unsigned q, j;

      i915_texture_set_level_info(tex, level, depth);

      for (q = 0; q < depth;) {
         for (j = 0; j < pack_x_nr && q < depth; j++, q++) {
            i915_texture_set_image_offset(tex, level, q, x, y + tex->total_nblocksy);
            x += pack_x_pitch;
         }

         x = 0;
         y += pack_y_pitch;
      }

      tex->total_nblocksy += y;

      if (pack_x_pitch > 4) {
         pack_x_pitch >>= 1;
         pack_x_nr <<= 1;
         assert(pack_x_pitch * pack_x_nr * util_format_get_blocksize(pt->format) <= tex->stride);
      }

      if (pack_y_pitch > 2) {
         pack_y_pitch >>= 1;
      }

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
      nblocksy = util_format_get_nblocksy(pt->format, height);
   }
}

static void
i945_texture_layout_cube(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   const unsigned nblocks = util_format_get_nblocksx(pt->format, pt->width0);
   const unsigned dim = pt->width0;
   unsigned level;
   unsigned face;

   assert(pt->width0 == pt->height0); /* cubemap images are square */
   assert(util_next_power_of_two(pt->width0) == pt->width0); /* npot only */
   assert(util_format_is_s3tc(pt->format)); /* compressed only */

   /*
    * Depending on the size of the largest images, pitch can be
    * determined either by the old-style packing of cubemap faces,
    * or the final row of 4x4, 2x2 and 1x1 faces below this.
    *
    * 64  * 2 / 4 = 32
    * 14 * 2 = 28
    */
   if (pt->width0 >= 64)
      tex->stride = nblocks * 2 * util_format_get_blocksize(pt->format);
   else
      tex->stride = 14 * 2 * util_format_get_blocksize(pt->format);

   /*
    * Something similary apply for height as well.
    */
   if (pt->width0 >= 4)
      tex->total_nblocksy = nblocks * 4 + 1;
   else
      tex->total_nblocksy = 1;

   /* Set all the levels to effectively occupy the whole rectangular region */
   for (level = 0; level <= pt->last_level; level++)
      i915_texture_set_level_info(tex, level, 6);

   for (face = 0; face < 6; face++) {
      /* all calculations in pixels */
      unsigned total_height = tex->total_nblocksy * 4;
      unsigned x = initial_offsets[face][0] * dim;
      unsigned y = initial_offsets[face][1] * dim;
      unsigned d = dim;

      if (dim == 4 && face >= 4) {
         x = (face - 4) * 8;
         y = tex->total_nblocksy * 4 - 4; /* 4 = 1 block */
      } else if (dim < 4 && (face > 0)) {
         x = face * 8;
         y = total_height - 4;
      }

      for (level = 0; level <= pt->last_level; level++) {
         i915_texture_set_image_offset(tex, level, face,
                                       util_format_get_nblocksx(pt->format, x),
                                       util_format_get_nblocksy(pt->format, y));

         d >>= 1;

         switch (d) {
         case 4:
            switch (face) {
            case PIPE_TEX_FACE_POS_X:
            case PIPE_TEX_FACE_NEG_X:
               x += step_offsets[face][0] * d;
               y += step_offsets[face][1] * d;
               break;
            case PIPE_TEX_FACE_POS_Y:
            case PIPE_TEX_FACE_NEG_Y:
               y += 12;
               x -= 8;
               break;
            case PIPE_TEX_FACE_POS_Z:
            case PIPE_TEX_FACE_NEG_Z:
               y = total_height - 4;
               x = (face - 4) * 8;
               break;
            }
            break;
         case 2:
            y = total_height - 4;
            x = bottom_offsets[face];
            break;
         case 1:
            x += 48;
            break;
         default:
            x += step_offsets[face][0] * d;
            y += step_offsets[face][1] * d;
            break;
         }
      }
   }
}

static boolean
i945_texture_layout(struct i915_texture * tex)
{
   struct pipe_resource *pt = &tex->b.b;

   switch (pt->target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      if (!i9x5_special_layout(tex))
         i945_texture_layout_2d(tex);
      break;
   case PIPE_TEXTURE_3D:
      i945_texture_layout_3d(tex);
      break;
   case PIPE_TEXTURE_CUBE:
      if (!util_format_is_s3tc(pt->format))
         i9x5_texture_layout_cube(tex);
      else
         i945_texture_layout_cube(tex);
      break;
   default:
      assert(0);
      return FALSE;
   }

   return TRUE;
}



/*
 * Screen texture functions
 */



static boolean
i915_texture_get_handle(struct pipe_screen * screen,
                        struct pipe_resource *texture,
                        struct winsys_handle *whandle)
{
   struct i915_screen *is = i915_screen(screen);
   struct i915_texture *tex = i915_texture(texture);
   struct i915_winsys *iws = is->iws;

   return iws->buffer_get_handle(iws, tex->buffer, whandle, tex->stride);
}


static void
i915_texture_destroy(struct pipe_screen *screen,
		     struct pipe_resource *pt)
{
   struct i915_texture *tex = i915_texture(pt);
   struct i915_winsys *iws = i915_screen(screen)->iws;
   uint i;

   iws->buffer_destroy(iws, tex->buffer);

   for (i = 0; i < Elements(tex->image_offset); i++)
      if (tex->image_offset[i])
         FREE(tex->image_offset[i]);

   FREE(tex);
}

static struct pipe_transfer * 
i915_texture_get_transfer(struct pipe_context *context,
			  struct pipe_resource *resource,
			  struct pipe_subresource sr,
			  unsigned usage,
			  const struct pipe_box *box)
{
   struct i915_texture *tex = i915_texture(resource);
   struct pipe_transfer *transfer = CALLOC_STRUCT(pipe_transfer);
   if (transfer == NULL)
      return NULL;

   transfer->resource = resource;
   transfer->sr = sr;
   transfer->usage = usage;
   transfer->box = *box;
   transfer->stride = tex->stride;

   return transfer;
}


static void *
i915_texture_transfer_map(struct pipe_context *pipe,
			  struct pipe_transfer *transfer)
{
   struct pipe_resource *resource = transfer->resource;
   struct i915_texture *tex = i915_texture(resource);
   struct i915_winsys *iws = i915_screen(pipe->screen)->iws;
   struct pipe_subresource sr = transfer->sr;
   struct pipe_box *box = &transfer->box;
   enum pipe_format format = resource->format;
   unsigned offset;
   char *map;

   if (resource->target == PIPE_TEXTURE_CUBE) {
      offset = tex->image_offset[sr.level][sr.face];
   } else if (resource->target == PIPE_TEXTURE_3D) {
      offset = tex->image_offset[sr.level][box->z];
   } else {
      offset = tex->image_offset[sr.level][0];
      assert(sr.face == 0);
      assert(box->z == 0);
   }

   map = iws->buffer_map(iws, tex->buffer,
                         (transfer->usage & PIPE_TRANSFER_WRITE) ? TRUE : FALSE);
   if (map == NULL)
      return NULL;

   return map + offset +
      box->y / util_format_get_blockheight(format) * transfer->stride +
      box->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
}

static void
i915_texture_transfer_unmap(struct pipe_context *pipe,
			    struct pipe_transfer *transfer)
{
   struct i915_texture *tex = i915_texture(transfer->resource);
   struct i915_winsys *iws = i915_screen(tex->b.b.screen)->iws;
   iws->buffer_unmap(iws, tex->buffer);
}



struct u_resource_vtbl i915_texture_vtbl = 
{
   i915_texture_get_handle,	      /* get_handle */
   i915_texture_destroy,	      /* resource_destroy */
   NULL,			      /* is_resource_referenced */
   i915_texture_get_transfer,	      /* get_transfer */
   u_default_transfer_destroy,	      /* transfer_destroy */
   i915_texture_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   i915_texture_transfer_unmap,	      /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};




struct pipe_resource *
i915_texture_create(struct pipe_screen *screen,
                    const struct pipe_resource *template)
{
   struct i915_screen *is = i915_screen(screen);
   struct i915_winsys *iws = is->iws;
   struct i915_texture *tex = CALLOC_STRUCT(i915_texture);
   size_t tex_size;
   unsigned buf_usage = 0;

   if (!tex)
      return NULL;

   tex->b.b = *template;
   tex->b.vtbl = &i915_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   if (is->is_i945) {
      if (!i945_texture_layout(tex))
         goto fail;
   } else {
      if (!i915_texture_layout(tex))
         goto fail;
   }

   tex_size = tex->stride * tex->total_nblocksy;

   /* for scanouts and cursors, cursors arn't scanouts */

   /* XXX: use a custom flag for cursors, don't rely on magically
    * guessing that this is Xorg asking for a cursor
    */
   if ((template->bind & PIPE_BIND_SCANOUT) && template->width0 != 64)
      buf_usage = I915_NEW_SCANOUT;
   else
      buf_usage = I915_NEW_TEXTURE;

   tex->buffer = iws->buffer_create(iws, tex_size, 64, buf_usage);
   if (!tex->buffer)
      goto fail;

   /* setup any hw fences */
   if (tex->hw_tiled) {
      assert(tex->sw_tiled == I915_TILE_NONE);
      iws->buffer_set_fence_reg(iws, tex->buffer, tex->stride, tex->hw_tiled);
   }

   
#if 0
   void *ptr = ws->buffer_map(ws, tex->buffer,
      PIPE_BUFFER_USAGE_CPU_WRITE);
   memset(ptr, 0x80, tex_size);
   ws->buffer_unmap(ws, tex->buffer);
#endif

   I915_DBG(DBG_TEXTURE, "%s: %p size %u, stride %u, blocks (%u, %u)\n", __func__,
            tex, (unsigned int)tex_size, tex->stride,
            tex->stride / util_format_get_blocksize(tex->b.b.format),
            tex->total_nblocksy);

   return &tex->b.b;

fail:
   FREE(tex);
   return NULL;
}

struct pipe_resource *
i915_texture_from_handle(struct pipe_screen * screen,
			  const struct pipe_resource *template,
			  struct winsys_handle *whandle)
{
   struct i915_screen *is = i915_screen(screen);
   struct i915_texture *tex;
   struct i915_winsys *iws = is->iws;
   struct i915_winsys_buffer *buffer;
   unsigned stride;

   assert(screen);

   buffer = iws->buffer_from_handle(iws, whandle, &stride);

   /* Only supports one type */
   if ((template->target != PIPE_TEXTURE_2D &&
       template->target != PIPE_TEXTURE_RECT) ||
       template->last_level != 0 ||
       template->depth0 != 1) {
      return NULL;
   }

   tex = CALLOC_STRUCT(i915_texture);
   if (!tex)
      return NULL;

   tex->b.b = *template;
   tex->b.vtbl = &i915_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   tex->stride = stride;
   tex->total_nblocksy = align_nblocksy(tex->b.b.format, tex->b.b.height0, 8);

   i915_texture_set_level_info(tex, 0, 1);
   i915_texture_set_image_offset(tex, 0, 0, 0, 0);

   tex->buffer = buffer;

   I915_DBG(DBG_TEXTURE, "%s: %p stride %u, blocks (%ux%u)\n", __func__,
            tex, tex->stride,
            tex->stride / util_format_get_blocksize(tex->b.b.format),
            tex->total_nblocksy);

   return &tex->b.b;
}

