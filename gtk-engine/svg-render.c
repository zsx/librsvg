/* GTK+ Rsvg Engine
 * Copyright (C) 1998-2000 Red Hat, Inc.
 * Copyright (C) 2002 Dom Lachowicz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by Owen Taylor <otaylor@redhat.com>, based on code by
 * Carsten Haitzler <raster@rasterman.com>
 */

#include <stdio.h>
#include <string.h>

#include "svg.h"

static GCache *pixbuf_cache = NULL;

static GdkPixbuf *
bilinear_gradient (GdkPixbuf    *src,
		   gint          src_x,
		   gint          src_y,
		   gint          width,
		   gint          height)
{
  guint n_channels = gdk_pixbuf_get_n_channels (src);
  guint src_rowstride = gdk_pixbuf_get_rowstride (src);
  guchar *src_pixels = gdk_pixbuf_get_pixels (src);
  guchar *p1, *p2, *p3, *p4;
  guint dest_rowstride;
  guchar *dest_pixels;
  GdkPixbuf *result;
  int i, j;
  guint k;

  p1 = src_pixels + (src_y - 1) * src_rowstride + (src_x - 1) * n_channels;
  p2 = p1 + n_channels;
  p3 = src_pixels + src_y * src_rowstride + (src_x - 1) * n_channels;
  p4 = p3 + n_channels;

  result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
			   width, height);
  dest_rowstride = gdk_pixbuf_get_rowstride (result);
  dest_pixels = gdk_pixbuf_get_pixels (result);

  for (i = 0; i < height; i++)
    {
      guchar *p = dest_pixels + dest_rowstride *i;
      guint v[4];
      gint dv[4];

      for (k = 0; k < n_channels; k++)
	{
	  guint start = ((height - i) * p1[k] + (1 + i) * p3[k]) / (height + 1);
	  guint end = ((height -  i) * p2[k] + (1 + i) * p4[k]) / (height + 1);

	  dv[k] = (((gint)end - (gint)start) << 16) / (width + 1);
	  v[k] = (start << 16) + dv[k] + 0x8000;
	}

      for (j = width; j; j--)
	{
	  for (k = 0; k < n_channels; k++)
	    {
	      *(p++) = v[k] >> 16;
	      v[k] += dv[k];
	    }
	}
    }

  return result;
}

static GdkPixbuf *
horizontal_gradient (GdkPixbuf    *src,
		     gint          src_x,
		     gint          src_y,
		     gint          width,
		     gint          height)
{
  guint n_channels = gdk_pixbuf_get_n_channels (src);
  guint src_rowstride = gdk_pixbuf_get_rowstride (src);
  guchar *src_pixels = gdk_pixbuf_get_pixels (src);
  guint dest_rowstride;
  guchar *dest_pixels;
  GdkPixbuf *result;
  int i, j;
  guint k;

  result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
			   width, height);
  dest_rowstride = gdk_pixbuf_get_rowstride (result);
  dest_pixels = gdk_pixbuf_get_pixels (result);

  for (i = 0; i < height; i++)
    {
      guchar *p = dest_pixels + dest_rowstride *i;
      guchar *p1 = src_pixels + (src_y + i) * src_rowstride + (src_x - 1) * n_channels;
      guchar *p2 = p1 + n_channels;

      guint v[4];
      gint dv[4];

      for (k = 0; k < n_channels; k++)
	{
	  dv[k] = (((gint)p2[k] - (gint)p1[k]) << 16) / (width + 1);
	  v[k] = (p1[k] << 16) + dv[k] + 0x8000;
	}
      
      for (j = width; j; j--)
	{
	  for (k = 0; k < n_channels; k++)
	    {
	      *(p++) = v[k] >> 16;
	      v[k] += dv[k];
	    }
	}
    }

  return result;
}

static GdkPixbuf *
vertical_gradient (GdkPixbuf    *src,
		   gint          src_x,
		   gint          src_y,
		   gint          width,
		   gint          height)
{
  guint n_channels = gdk_pixbuf_get_n_channels (src);
  guint src_rowstride = gdk_pixbuf_get_rowstride (src);
  guchar *src_pixels = gdk_pixbuf_get_pixels (src);
  guchar *top_pixels, *bottom_pixels;
  guint dest_rowstride;
  guchar *dest_pixels;
  GdkPixbuf *result;
  int i, j;

  top_pixels = src_pixels + (src_y - 1) * src_rowstride + (src_x) * n_channels;
  bottom_pixels = top_pixels + src_rowstride;

  result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
			   width, height);
  dest_rowstride = gdk_pixbuf_get_rowstride (result);
  dest_pixels = gdk_pixbuf_get_pixels (result);

  for (i = 0; i < height; i++)
    {
      guchar *p = dest_pixels + dest_rowstride *i;
      guchar *p1 = top_pixels;
      guchar *p2 = bottom_pixels;

      for (j = width * n_channels; j; j--)
	*(p++) = ((height - i) * *(p1++) + (1 + i) * *(p2++)) / (height + 1);
    }

  return result;
}

static GdkPixbuf *
replicate_single (GdkPixbuf    *src,
		  gint          src_x,
		  gint          src_y,
		  gint          width,
		  gint          height)
{
  guint n_channels = gdk_pixbuf_get_n_channels (src);
  guchar *pixels = (gdk_pixbuf_get_pixels (src) +
		    src_y * gdk_pixbuf_get_rowstride (src) +
		    src_x * n_channels);
  guchar r = *(pixels++);
  guchar g = *(pixels++);
  guchar b = *(pixels++);
  guint dest_rowstride;
  guchar *dest_pixels;
  guchar a = 0;
  GdkPixbuf *result;
  int i, j;

  if (n_channels == 4)
    a = *(pixels++);

  result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
			   width, height);
  dest_rowstride = gdk_pixbuf_get_rowstride (result);
  dest_pixels = gdk_pixbuf_get_pixels (result);
  
  for (i = 0; i < height; i++)
    {
      guchar *p = dest_pixels + dest_rowstride *i;

      for (j = 0; j < width; j++)
	{
	  *(p++) = r;
	  *(p++) = g;
	  *(p++) = b;

	  if (n_channels == 4)
	    *(p++) = a;
	}
    }

  return result;
}

static GdkPixbuf *
replicate_rows (GdkPixbuf    *src,
		gint          src_x,
		gint          src_y,
		gint          width,
		gint          height)
{
  guint n_channels = gdk_pixbuf_get_n_channels (src);
  guint src_rowstride = gdk_pixbuf_get_rowstride (src);
  guchar *pixels = (gdk_pixbuf_get_pixels (src) + src_y * src_rowstride + src_x * n_channels);
  guchar *dest_pixels;
  GdkPixbuf *result;
  guint dest_rowstride;
  int i;

  result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
			   width, height);
  dest_rowstride = gdk_pixbuf_get_rowstride (result);
  dest_pixels = gdk_pixbuf_get_pixels (result);

  for (i = 0; i < height; i++)
    memcpy (dest_pixels + dest_rowstride * i, pixels, n_channels * width);

  return result;
}

static GdkPixbuf *
replicate_cols (GdkPixbuf    *src,
		gint          src_x,
		gint          src_y,
		gint          width,
		gint          height)
{
  guint n_channels = gdk_pixbuf_get_n_channels (src);
  guint src_rowstride = gdk_pixbuf_get_rowstride (src);
  guchar *pixels = (gdk_pixbuf_get_pixels (src) + src_y * src_rowstride + src_x * n_channels);
  guchar *dest_pixels;
  GdkPixbuf *result;
  guint dest_rowstride;
  int i, j;

  result = gdk_pixbuf_new (GDK_COLORSPACE_RGB, n_channels == 4, 8,
			   width, height);
  dest_rowstride = gdk_pixbuf_get_rowstride (result);
  dest_pixels = gdk_pixbuf_get_pixels (result);

  for (i = 0; i < height; i++)
    {
      guchar *p = dest_pixels + dest_rowstride * i;
      guchar *q = pixels + src_rowstride * i;

      guchar r = *(q++);
      guchar g = *(q++);
      guchar b = *(q++);
      guchar a = 0;
      
      if (n_channels == 4)
	a = *(q++);

      for (j = 0; j < width; j++)
	{
	  *(p++) = r;
	  *(p++) = g;
	  *(p++) = b;

	  if (n_channels == 4)
	    *(p++) = a;
	}
    }

  return result;
}

/* Scale the rectangle (src_x, src_y, src_width, src_height)
 * onto the rectangle (dest_x, dest_y, dest_width, dest_height)
 * of the destination, clip by clip_rect and render
 */
static void
pixbuf_render (GdkPixbuf    *src,
	       guint         hints,
	       GdkWindow    *window,
	       GdkBitmap    *mask,
	       GdkRectangle *clip_rect,
	       gint          src_x,
	       gint          src_y,
	       gint          src_width,
	       gint          src_height,
	       gint          dest_x,
	       gint          dest_y,
	       gint          dest_width,
	       gint          dest_height)
{
  GdkPixbuf *tmp_pixbuf;
  GdkRectangle rect;
  int x_offset, y_offset;
  gboolean has_alpha = gdk_pixbuf_get_has_alpha (src);
  gint src_rowstride = gdk_pixbuf_get_rowstride (src);
  gint src_n_channels = gdk_pixbuf_get_n_channels (src);

  if (dest_width <= 0 || dest_height <= 0)
    return;

  rect.x = dest_x;
  rect.y = dest_y;
  rect.width = dest_width;
  rect.height = dest_height;

  if (hints & THEME_MISSING)
    return;

  /* FIXME: Because we use the mask to shape windows, we don't use
   * clip_rect to clip what we draw to the mask, only to clip
   * what we actually draw. But this leads to the horrible ineffiency
   * of scale the whole image to get a little bit of it.
   */
  if (!mask && clip_rect)
    {
      if (!gdk_rectangle_intersect (clip_rect, &rect, &rect))
	return;
    }

  if (dest_width == src_width && dest_height == src_height)
    {
      tmp_pixbuf = g_object_ref (src);

      x_offset = src_x + rect.x - dest_x;
      y_offset = src_y + rect.y - dest_y;
    }
  else if (src_width == 0 && src_height == 0)
    {
      tmp_pixbuf = bilinear_gradient (src, src_x, src_y, dest_width, dest_height);      
      
      x_offset = rect.x - dest_x;
      y_offset = rect.y - dest_y;
    }
  else if (src_width == 0 && dest_height == src_height)
    {
      tmp_pixbuf = horizontal_gradient (src, src_x, src_y, dest_width, dest_height);      
      
      x_offset = rect.x - dest_x;
      y_offset = rect.y - dest_y;
    }
  else if (src_height == 0 && dest_width == src_width)
    {
      tmp_pixbuf = vertical_gradient (src, src_x, src_y, dest_width, dest_height);
      
      x_offset = rect.x - dest_x;
      y_offset = rect.y - dest_y;
    }
  else if ((hints & THEME_CONSTANT_COLS) && (hints & THEME_CONSTANT_ROWS))
    {
      tmp_pixbuf = replicate_single (src, src_x, src_y, dest_width, dest_height);

      x_offset = rect.x - dest_x;
      y_offset = rect.y - dest_y;
    }
  else if (dest_width == src_width && (hints & THEME_CONSTANT_COLS))
    {
      tmp_pixbuf = replicate_rows (src, src_x, src_y, dest_width, dest_height);

      x_offset = rect.x - dest_x;
      y_offset = rect.y - dest_y;
    }
  else if (dest_height == src_height && (hints & THEME_CONSTANT_ROWS))
    {
      tmp_pixbuf = replicate_cols (src, src_x, src_y, dest_width, dest_height);

      x_offset = rect.x - dest_x;
      y_offset = rect.y - dest_y;
    }
  else 
    {
      double x_scale = (double)dest_width / src_width;
      double y_scale = (double)dest_height / src_height;
      guchar *pixels;
      GdkPixbuf *partial_src;
      
      pixels = (gdk_pixbuf_get_pixels (src)
		+ src_y * src_rowstride
		+ src_x * src_n_channels);

      partial_src = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
					      has_alpha,
					      8, src_width, src_height,
					      src_rowstride,
					      NULL, NULL);
						  
      tmp_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
				   has_alpha, 8,
				   rect.width, rect.height);

      gdk_pixbuf_scale (partial_src, tmp_pixbuf,
			0, 0, rect.width, rect.height,
			dest_x - rect.x, dest_y - rect.y, 
			x_scale, y_scale,
			GDK_INTERP_BILINEAR);

      g_object_unref (partial_src);

      x_offset = 0;
      y_offset = 0;
    }

  if (mask)
    {
      gdk_pixbuf_render_threshold_alpha (tmp_pixbuf, mask,
					 x_offset, y_offset,
					 rect.x, rect.y,
					 rect.width, rect.height,
					 128);
    }

  gdk_draw_pixbuf (window, NULL, tmp_pixbuf, 
		   x_offset, y_offset,
		   rect.x, rect.y,
		   rect.width, rect.height,
		   GDK_RGB_DITHER_NORMAL,
		   0, 0);
  g_object_unref (tmp_pixbuf);
}

ThemePixbuf *
theme_pixbuf_new (void)
{
  ThemePixbuf *result = g_new0 (ThemePixbuf, 1);
  result->filename = NULL;
  result->handle = NULL;

  result->stretch = TRUE;
  result->border_left = 0;
  result->border_right = 0;
  result->border_bottom = 0;
  result->border_top = 0;

  return result;
}

void
theme_pixbuf_destroy (ThemePixbuf *theme_pb)
{
  theme_pixbuf_set_filename (theme_pb, NULL);
  g_free (theme_pb);
}

void         
theme_pixbuf_set_filename (ThemePixbuf *theme_pb,
			   const char  *filename)
{
  if (theme_pb->handle)
    {
      g_cache_remove (pixbuf_cache, theme_pb->handle);
      theme_pb->handle = NULL;
    }

  if (theme_pb->filename)
    g_free (theme_pb->filename);

  if (filename)
    theme_pb->filename = g_strdup (filename);
  else
    theme_pb->filename = NULL;
}

static guint
compute_hint (GdkPixbuf *pixbuf,
	      gint       x0,
	      gint       x1,
	      gint       y0,
	      gint       y1)
{
  int i, j;
  int hints = THEME_CONSTANT_ROWS | THEME_CONSTANT_COLS | THEME_MISSING;
  int n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  
  guchar *data = gdk_pixbuf_get_pixels (pixbuf);
  int rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  if (x0 == x1 || y0 == y1)
    return 0;

  for (i = y0; i < y1; i++)
    {
      guchar *p = data + i * rowstride + x0 * n_channels;
      guchar r = p[0];
      guchar g = p[1];
      guchar b = p[2];
      guchar a = 0;
      
      if (n_channels == 4)
	a = p[3];

      for (j = x0; j < x1 ; j++)
	{
	  if (n_channels != 4 || p[3] != 0)
	    {
	      hints &= ~THEME_MISSING;
	      if (!(hints & THEME_CONSTANT_ROWS))
		goto cols;
	    }
	  
	  if (r != *(p++) ||
	      g != *(p++) ||
	      b != *(p++) ||
	      (n_channels != 4 && a != *(p++)))
	    {
	      hints &= ~THEME_CONSTANT_ROWS;
	      if (!(hints & THEME_MISSING))
		goto cols;
	    }
	}
    }

 cols:
  for (i = y0 + 1; i < y1; i++)
    {
      guchar *base = data + y0 * rowstride + x0 * n_channels;
      guchar *p = data + i * rowstride + x0 * n_channels;

      if (memcmp (p, base, n_channels * (x1 - x0)) != 0)
	{
	  hints &= ~THEME_CONSTANT_COLS;
	  return hints;
	}
    }

  return hints;
}

static void
theme_pixbuf_compute_hints (ThemePixbuf *theme_pb, GdkPixbuf *pixbuf)
{
  int i, j;
  gint width = gdk_pixbuf_get_width (pixbuf);
  gint height = gdk_pixbuf_get_height (pixbuf);

  if (theme_pb->border_left + theme_pb->border_right > width ||
      theme_pb->border_top + theme_pb->border_bottom > height)
    {
      g_warning ("Invalid borders specified for theme pixmap:\n"
		 "        %s,\n"
		 "borders don't fit within the image", theme_pb->filename);
      if (theme_pb->border_left + theme_pb->border_right > width)
	{
	  theme_pb->border_left = width / 2;
	  theme_pb->border_right = (width + 1) / 2;
	}
      if (theme_pb->border_bottom + theme_pb->border_top > height)
	{
	  theme_pb->border_top = height / 2;
	  theme_pb->border_bottom = (height + 1) / 2;
	}
    }
  
  for (i = 0; i < 3; i++)
    {
      gint y0, y1;

      switch (i)
	{
	case 0:
	  y0 = 0;
	  y1 = theme_pb->border_top;
	  break;
	case 1:
	  y0 = theme_pb->border_top;
	  y1 = height - theme_pb->border_bottom;
	  break;
	default:
	  y0 = height - theme_pb->border_bottom;
	  y1 = height;
	  break;
	}
      
      for (j = 0; j < 3; j++)
	{
	  gint x0, x1;
	  
	  switch (j)
	    {
	    case 0:
	      x0 = 0;
	      x1 = theme_pb->border_left;
	      break;
	    case 1:
	      x0 = theme_pb->border_left;
	      x1 = width - theme_pb->border_right;
	      break;
	    default:
	      x0 = width - theme_pb->border_right;
	      x1 = width;
	      break;
	    }

	  theme_pb->hints[i][j] = compute_hint (pixbuf, x0, x1, y0, y1);
	}
    }
  
}

void
theme_pixbuf_set_border (ThemePixbuf *theme_pb,
			 gint         left,
			 gint         right,
			 gint         top,
			 gint         bottom)
{
  theme_pb->border_left = left;
  theme_pb->border_right = right;
  theme_pb->border_top = top;
  theme_pb->border_bottom = bottom;
}

void
theme_pixbuf_set_stretch (ThemePixbuf *theme_pb,
			  gboolean     stretch)
{
  theme_pb->stretch = stretch;
}

static void
svg_cache_value_free(gpointer foo)
{
  RsvgHandle * handle;

  handle = (RsvgHandle *)foo;
  if(handle != NULL)
    g_object_unref(G_OBJECT(handle));
}

#define SVG_BUFFER_SIZE (1024*8)

static RsvgHandle *
svg_cache_value_new (gchar *filename)
{
  RsvgHandle *result = NULL;
  FILE *fp;

  fp = fopen(filename, "rb");
  if(fp)
    {
      size_t nread;
      unsigned char buf[SVG_BUFFER_SIZE];

      result = rsvg_handle_new();
      while((nread = fread(buf, 1, sizeof(buf), fp)) > 0)
	rsvg_handle_write(result, buf, nread, NULL);

      fclose(fp);
      rsvg_handle_close(result, NULL);
    }
  else
    {
      g_warning("Couldn't load theme part: %s\n", filename);
    }

  return result;
}

struct SizeInfo
{
  gint width, height;
};

static void set_size_fn(gint *width, gint *height, gpointer foo)
{
  struct SizeInfo * info = (struct SizeInfo *)foo;

  *width = info->width;
  *height = info->height;
}

static GdkPixbuf *
get_pixbuf(RsvgHandle * handle, gint width, gint height)
{
  GdkPixbuf * result;

  if(!handle)
    return NULL;

  if(width > 0 && height > 0)
    {
      struct SizeInfo info;

      info.width = width;
      info.height = height;
      
      rsvg_handle_set_size_callback(handle, set_size_fn, &info, NULL);
    }

  result = rsvg_handle_get_pixbuf(handle);

  return result;
}

GdkPixbuf *
theme_pixbuf_get_pixbuf (ThemePixbuf *theme_pb, gint width, gint height)
{
  GdkPixbuf *result = NULL;

  if (!theme_pb->handle)
    {
      if (!pixbuf_cache)
	pixbuf_cache = g_cache_new ((GCacheNewFunc)svg_cache_value_new,
				    (GCacheDestroyFunc)svg_cache_value_free,
				    (GCacheDupFunc)g_strdup,
				    (GCacheDestroyFunc)g_free,
				    g_str_hash, g_direct_hash, g_str_equal);
      
      theme_pb->handle = g_cache_insert (pixbuf_cache, theme_pb->filename);
    }
  
  result = get_pixbuf(theme_pb->handle, width, height);
  if(result)
    theme_pixbuf_compute_hints(theme_pb, result);
  
  return result;
}

void
theme_pixbuf_render (ThemePixbuf  *theme_pb,
		     GdkWindow    *window,
		     GdkBitmap    *mask,
		     GdkRectangle *clip_rect,
		     guint         component_mask,
		     gboolean      center,
		     gint          x,
		     gint          y,
		     gint          width,
		     gint          height)
{
  GdkPixbuf *pixbuf = theme_pixbuf_get_pixbuf (theme_pb, width, height);
  gint src_x[4], src_y[4], dest_x[4], dest_y[4];
  gint pixbuf_width = gdk_pixbuf_get_width (pixbuf);
  gint pixbuf_height = gdk_pixbuf_get_height (pixbuf);

  if (!pixbuf)
    return;

  if (theme_pb->stretch)
    {
      src_x[0] = 0;
      src_x[1] = theme_pb->border_left;
      src_x[2] = pixbuf_width - theme_pb->border_right;
      src_x[3] = pixbuf_width;
      
      src_y[0] = 0;
      src_y[1] = theme_pb->border_top;
      src_y[2] = pixbuf_height - theme_pb->border_bottom;
      src_y[3] = pixbuf_height;
      
      dest_x[0] = x;
      dest_x[1] = x + theme_pb->border_left;
      dest_x[2] = x + width - theme_pb->border_right;
      dest_x[3] = x + width;

      dest_y[0] = y;
      dest_y[1] = y + theme_pb->border_top;
      dest_y[2] = y + height - theme_pb->border_bottom;
      dest_y[3] = y + height;

      if (component_mask & COMPONENT_ALL)
	component_mask = (COMPONENT_ALL - 1) & ~component_mask;

#define RENDER_COMPONENT(X1,X2,Y1,Y2)					         \
        pixbuf_render (pixbuf, theme_pb->hints[Y1][X1], window, mask, clip_rect, \
	 	       src_x[X1], src_y[Y1],				         \
		       src_x[X2] - src_x[X1], src_y[Y2] - src_y[Y1],	         \
		       dest_x[X1], dest_y[Y1],				         \
		       dest_x[X2] - dest_x[X1], dest_y[Y2] - dest_y[Y1]);
      
      if (component_mask & COMPONENT_NORTH_WEST)
	RENDER_COMPONENT (0, 1, 0, 1);

      if (component_mask & COMPONENT_NORTH)
	RENDER_COMPONENT (1, 2, 0, 1);

      if (component_mask & COMPONENT_NORTH_EAST)
	RENDER_COMPONENT (2, 3, 0, 1);

      if (component_mask & COMPONENT_WEST)
	RENDER_COMPONENT (0, 1, 1, 2);

      if (component_mask & COMPONENT_CENTER)
	RENDER_COMPONENT (1, 2, 1, 2);

      if (component_mask & COMPONENT_EAST)
	RENDER_COMPONENT (2, 3, 1, 2);

      if (component_mask & COMPONENT_SOUTH_WEST)
	RENDER_COMPONENT (0, 1, 2, 3);

      if (component_mask & COMPONENT_SOUTH)
	RENDER_COMPONENT (1, 2, 2, 3);

      if (component_mask & COMPONENT_SOUTH_EAST)
	RENDER_COMPONENT (2, 3, 2, 3);
    }
  else
    {
      if (center)
	{
	  x += (width - pixbuf_width) / 2;
	  y += (height - pixbuf_height) / 2;
	  
	  pixbuf_render (pixbuf, 0, window, NULL, clip_rect,
			 0, 0,
			 pixbuf_width, pixbuf_height,
			 x, y,
			 pixbuf_width, pixbuf_height);
	}
      else
	{
	  GdkPixmap *tmp_pixmap;
	  GdkGC *tmp_gc;
	  GdkGCValues gc_values;

	  tmp_pixmap = gdk_pixmap_new (window,
				       pixbuf_width,
				       pixbuf_height,
				       -1);
	  tmp_gc = gdk_gc_new (tmp_pixmap);
	  gdk_draw_pixbuf (tmp_pixmap, tmp_gc, pixbuf, 
			   0, 0, 
			   0, 0,
			   pixbuf_width, pixbuf_height,
			   GDK_RGB_DITHER_NORMAL,
			   0, 0);
	  g_object_unref (tmp_gc);

	  gc_values.fill = GDK_TILED;
	  gc_values.tile = tmp_pixmap;
	  tmp_gc = gdk_gc_new_with_values (window,
					   &gc_values, GDK_GC_FILL | GDK_GC_TILE);
	  if (clip_rect)
	    gdk_draw_rectangle (window, tmp_gc, TRUE,
				clip_rect->x, clip_rect->y, clip_rect->width, clip_rect->height);
	  else
	    gdk_draw_rectangle (window, tmp_gc, TRUE, x, y, width, height);
	  
	  g_object_unref (tmp_gc);
	  g_object_unref (tmp_pixmap);
	}
    }

  g_object_unref(G_OBJECT(pixbuf));
}
