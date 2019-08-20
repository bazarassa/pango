/* Vogue
 * voguexft-render.c: Rendering routines for the Xft library
 *
 * Copyright (C) 2004 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <math.h>

#include "voguexft-render.h"
#include "voguexft-private.h"

enum {
  PROP_0,
  PROP_DISPLAY,
  PROP_SCREEN
};

struct _VogueXftRendererPrivate
{
  VogueColor default_color;
  guint16 alpha;

  Picture src_picture;
  Picture dest_picture;

  XRenderPictFormat *mask_format;

  GArray *trapezoids;
  VogueRenderPart trapezoid_part;

  GArray *glyphs;
  VogueFont *glyph_font;
};

static void vogue_xft_renderer_finalize     (GObject      *object);
static void vogue_xft_renderer_set_property (GObject      *object,
					     guint         prop_id,
					     const GValue *value,
					     GParamSpec   *pspec);

static void vogue_xft_renderer_real_composite_trapezoids (VogueXftRenderer *xftrenderer,
							  VogueRenderPart   part,
							  XTrapezoid       *trapezoids,
							  int               n_trapezoids);
static void vogue_xft_renderer_real_composite_glyphs     (VogueXftRenderer *xftrenderer,
							  XftFont          *xft_font,
							  XftGlyphSpec     *glyphs,
							  int               n_glyphs);

static void vogue_xft_renderer_draw_glyphs    (VogueRenderer    *renderer,
					       VogueFont        *font,
					       VogueGlyphString *glyphs,
					       int               x,
					       int               y);
static void vogue_xft_renderer_draw_trapezoid (VogueRenderer    *renderer,
					       VogueRenderPart   part,
					       double            y1,
					       double            x11,
					       double            x21,
					       double            y2,
					       double            x12,
					       double            x22);
static void vogue_xft_renderer_part_changed   (VogueRenderer    *renderer,
					       VogueRenderPart   part);
static void vogue_xft_renderer_end            (VogueRenderer    *renderer);

static void flush_trapezoids (VogueXftRenderer *xftrenderer);
static void flush_glyphs (VogueXftRenderer *xftrenderer);

G_DEFINE_TYPE_WITH_PRIVATE (VogueXftRenderer, vogue_xft_renderer, PANGO_TYPE_RENDERER)

static void
vogue_xft_renderer_init (VogueXftRenderer *xftrenderer)
{
  xftrenderer->priv = vogue_xft_renderer_get_instance_private (xftrenderer);
  xftrenderer->priv->alpha = 0xffff;
}

static void
vogue_xft_renderer_class_init (VogueXftRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  VogueRendererClass *renderer_class = PANGO_RENDERER_CLASS (klass);

  klass->composite_glyphs = vogue_xft_renderer_real_composite_glyphs;
  klass->composite_trapezoids = vogue_xft_renderer_real_composite_trapezoids;

  renderer_class->draw_glyphs = vogue_xft_renderer_draw_glyphs;
  renderer_class->draw_trapezoid = vogue_xft_renderer_draw_trapezoid;
  renderer_class->part_changed = vogue_xft_renderer_part_changed;
  renderer_class->end = vogue_xft_renderer_end;

  object_class->finalize = vogue_xft_renderer_finalize;
  object_class->set_property = vogue_xft_renderer_set_property;

  g_object_class_install_property (object_class, PROP_DISPLAY,
				   g_param_spec_pointer ("display",
							 "Display",
							 "The display being rendered to",
							 G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_SCREEN,
				   g_param_spec_int ("screen",
						     "Screen",
						     "The screen being rendered to",
						     0, G_MAXINT, 0,
						     G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
vogue_xft_renderer_finalize (GObject *object)
{
  VogueXftRenderer *renderer = PANGO_XFT_RENDERER (object);

  if (renderer->priv->glyphs)
    g_array_free (renderer->priv->glyphs, TRUE);
  if (renderer->priv->trapezoids)
    g_array_free (renderer->priv->trapezoids, TRUE);

  G_OBJECT_CLASS (vogue_xft_renderer_parent_class)->finalize (object);
}

static void
vogue_xft_renderer_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  VogueXftRenderer *xftrenderer = PANGO_XFT_RENDERER (object);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      xftrenderer->display = g_value_get_pointer (value);
      /* We possibly should use ARGB format when subpixel-AA is turned
       * on for the fontmap; we could discover that using the technique
       * for FC_DPI in vogue_fc_face_list_sizes.
       */
      xftrenderer->priv->mask_format = XRenderFindStandardFormat (xftrenderer->display,
								  PictStandardA8);
      break;
    case PROP_SCREEN:
      xftrenderer->screen = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
vogue_xft_renderer_set_pictures (VogueXftRenderer *renderer,
				 Picture           src_picture,
				 Picture           dest_picture)
{
  renderer->priv->src_picture = src_picture;
  renderer->priv->dest_picture = dest_picture;
}

static void
flush_glyphs (VogueXftRenderer *xftrenderer)
{
  XftFont *xft_font;

  if (!xftrenderer->priv->glyphs ||
      xftrenderer->priv->glyphs->len == 0)
    return;

  xft_font = vogue_xft_font_get_font (xftrenderer->priv->glyph_font);

  PANGO_XFT_RENDERER_GET_CLASS (xftrenderer)->composite_glyphs (xftrenderer,
								xft_font,
								(XftGlyphSpec *)(void*)xftrenderer->priv->glyphs->data,
								xftrenderer->priv->glyphs->len);

  g_array_set_size (xftrenderer->priv->glyphs, 0);
  g_object_unref (xftrenderer->priv->glyph_font);
  xftrenderer->priv->glyph_font = NULL;
}

#define MAX_GLYPHS	1024

static void
draw_glyph (VogueRenderer *renderer,
	    VogueFont     *font,
	    FT_UInt        glyph,
	    int            x,
	    int            y)
{
  VogueXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);
  XftGlyphSpec gs;
  int pixel_x, pixel_y;

  if (renderer->matrix)
    {
      pixel_x = floor (0.5 + (x * renderer->matrix->xx + y * renderer->matrix->xy) / PANGO_SCALE + renderer->matrix->x0);
      pixel_y = floor (0.5 + (x * renderer->matrix->yx + y * renderer->matrix->yy) / PANGO_SCALE + renderer->matrix->y0);
    }
  else
    {
      pixel_x = PANGO_PIXELS (x);
      pixel_y = PANGO_PIXELS (y);
    }

  /* Clip glyphs into the X coordinate range; we really
   * want to clip glyphs with an ink rect outside the
   * [0,32767] x [0,32767] rectangle but looking up
   * the ink rect here would be a noticeable speed hit.
   * This is close enough.
   */
  if (pixel_x < -32768 || pixel_x > 32767 ||
      pixel_y < -32768 || pixel_y > 32767)
    return;

  flush_trapezoids (xftrenderer);

  if (!xftrenderer->priv->glyphs)
    xftrenderer->priv->glyphs = g_array_new (FALSE, FALSE,
					     sizeof (XftGlyphSpec));

  if (xftrenderer->priv->glyph_font != font ||
      xftrenderer->priv->glyphs->len == MAX_GLYPHS)
    {
      flush_glyphs (xftrenderer);

      xftrenderer->priv->glyph_font = g_object_ref (font);
    }

  gs.x = pixel_x;
  gs.y = pixel_y;
  gs.glyph = glyph;

  g_array_append_val (xftrenderer->priv->glyphs, gs);
}

static gboolean
point_in_bounds (VogueRenderer *renderer,
		 gint           x,
		 gint           y)
{
  gdouble pixel_x = (x * renderer->matrix->xx + y * renderer->matrix->xy) / PANGO_SCALE + renderer->matrix->x0;
  gdouble pixel_y = (x * renderer->matrix->yx + y * renderer->matrix->yy) / PANGO_SCALE + renderer->matrix->y0;

  return (pixel_x >= -32768. && pixel_x < 32768. &&
	  pixel_y >= -32768. && pixel_y < 32768.);
}

static gboolean
box_in_bounds (VogueRenderer *renderer,
	       gint           x,
	       gint           y,
	       gint           width,
	       gint           height)
{
  if (!renderer->matrix)
    {
#define COORD_MIN (PANGO_SCALE * -16384 - PANGO_SCALE / 2)
#define COORD_MAX (PANGO_SCALE * 32767 + PANGO_SCALE / 2 - 1)
      return (x >= COORD_MIN && x + width <= COORD_MAX &&
	      y >= COORD_MIN && y + width <= COORD_MAX);
#undef COORD_MIN
#undef COORD_MAX
    }
  else
    {
      return (point_in_bounds (renderer, x, y) &&
	      point_in_bounds (renderer, x + width, y) &&
	      point_in_bounds (renderer, x + width, y + height) &&
	      point_in_bounds (renderer, x, y + height));
    }
}

static void
get_total_matrix (VogueMatrix       *total,
		  const VogueMatrix *global,
		  double             x,
		  double             y,
		  double             width,
		  double             height)
{
  VogueMatrix local = PANGO_MATRIX_INIT;
  gdouble angle = atan2 (height, width);

  vogue_matrix_translate (&local, x, y);
  vogue_matrix_rotate (&local, -angle * (180. / G_PI));

  *total = *global;
  vogue_matrix_concat (total, &local);
}

static void
draw_box (VogueRenderer *renderer,
	  gint           line_width,
	  gint           x,
	  gint           y,
	  gint           width,
	  gint           height,
	  gboolean       invalid)
{
  vogue_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				 x, y, width, line_width);
  vogue_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				 x, y + line_width, line_width, height - line_width * 2);
  vogue_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				 x + width - line_width, y + line_width, line_width, height - line_width * 2);
  vogue_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				 x, y + height - line_width, width, line_width);

  if (invalid)
    {
      int length;
      double in_width, in_height;
      VogueMatrix orig_matrix = PANGO_MATRIX_INIT, new_matrix;
      const VogueMatrix *porig_matrix;

      in_width  = vogue_units_to_double (width  - 2 * line_width);
      in_height = vogue_units_to_double (height - 2 * line_width);
      length = PANGO_SCALE * sqrt (in_width*in_width + in_height*in_height);

      porig_matrix = vogue_renderer_get_matrix (renderer);
      if (porig_matrix)
        {
	  orig_matrix = *porig_matrix;
	  porig_matrix = &orig_matrix;
	}

      get_total_matrix (&new_matrix, &orig_matrix,
			vogue_units_to_double (x + line_width), vogue_units_to_double (y + line_width),
			in_width, in_height);
      vogue_renderer_set_matrix (renderer, &new_matrix);
      vogue_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				     0, -line_width / 2, length, line_width);

      get_total_matrix (&new_matrix, &orig_matrix,
			vogue_units_to_double (x + line_width), vogue_units_to_double (y + height - line_width),
			in_width, -in_height);
      vogue_renderer_set_matrix (renderer, &new_matrix);
      vogue_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				     0, -line_width / 2, length, line_width);

      vogue_renderer_set_matrix (renderer, porig_matrix);
    }
}

static void
_vogue_xft_renderer_draw_box_glyph (VogueRenderer    *renderer,
				    VogueGlyphInfo   *gi,
				    int               glyph_x,
				    int               glyph_y,
				    gboolean          invalid)
{
  int x = glyph_x + PANGO_SCALE;
  int y = glyph_y - PANGO_SCALE * (PANGO_UNKNOWN_GLYPH_HEIGHT - 1);
  int width = gi->geometry.width - PANGO_SCALE * 2;
  int height = PANGO_SCALE * (PANGO_UNKNOWN_GLYPH_HEIGHT - 2);

  if (box_in_bounds (renderer, x, y, width, height))
    draw_box (renderer, PANGO_SCALE, x, y, width, height, invalid);
}

static void
_vogue_xft_renderer_draw_unknown_glyph (VogueRenderer    *renderer,
					VogueXftFont     *xfont,
					XftFont          *xft_font,
					VogueGlyphInfo   *gi,
					int               glyph_x,
					int               glyph_y)
{
  char buf[7];
  int ys[3];
  int xs[4];
  int row, col;
  int cols;
  gunichar ch;
  gboolean invalid_input;

  VogueFont *mini_font;
  XftFont *mini_xft_font;

  ch = gi->glyph & ~PANGO_GLYPH_UNKNOWN_FLAG;
  if (G_UNLIKELY (gi->glyph == PANGO_GLYPH_INVALID_INPUT || ch > 0x10FFFF))
    {
      invalid_input = TRUE;
      cols = 1;
    }
  else
    {
      invalid_input = FALSE;
      cols = ch > 0xffff ? 3 : 2;
      g_snprintf (buf, sizeof(buf), (ch > 0xffff) ? "%06X" : "%04X", ch);
    }

  mini_font = _vogue_xft_font_get_mini_font (xfont);
  mini_xft_font = vogue_xft_font_get_font (mini_font);
  if (!mini_xft_font)
    {
      _vogue_xft_renderer_draw_box_glyph (renderer, gi, glyph_x, glyph_y, invalid_input);
      return;
    }


  ys[0] = glyph_y - PANGO_SCALE * xft_font->ascent + PANGO_SCALE * (((xft_font->ascent + xft_font->descent) - (xfont->mini_height * 2 + xfont->mini_pad * 5 + PANGO_SCALE / 2) / PANGO_SCALE) / 2);
  ys[1] = ys[0] + 2 * xfont->mini_pad + xfont->mini_height;
  ys[2] = ys[1] + xfont->mini_height + xfont->mini_pad;

  xs[0] = glyph_x;
  xs[1] = xs[0] + 2 * xfont->mini_pad;
  xs[2] = xs[1] + xfont->mini_width + xfont->mini_pad;
  xs[3] = xs[2] + xfont->mini_width + xfont->mini_pad;

  if (box_in_bounds (renderer,
		     xs[0], ys[0],
		     xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 1),
		     xfont->mini_height * 2 + xfont->mini_pad * 5))
    {
      if (xfont->mini_pad)
	draw_box (renderer, xfont->mini_pad,
		  xs[0], ys[0],
		  xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 1),
		  xfont->mini_height * 2 + xfont->mini_pad * 5,
		  invalid_input);

      if (invalid_input)
        return;

      for (row = 0; row < 2; row++)
	for (col = 0; col < cols; col++)
	  {
	    draw_glyph (renderer, mini_font,
			XftCharIndex (NULL, mini_xft_font,
				      buf[row * cols + col] & 0xff),
			xs[col+1],
			ys[row+1]);
	  }
    }
}

static void
vogue_xft_renderer_draw_glyphs (VogueRenderer    *renderer,
				VogueFont        *font,
				VogueGlyphString *glyphs,
				int               x,
				int               y)
{
  VogueXftFont *xfont = PANGO_XFT_FONT (font);
  VogueFcFont *fcfont = PANGO_FC_FONT (font);
  XftFont *xft_font = vogue_xft_font_get_font (font);
  int i;
  int x_off = 0;

  if (!fcfont)
    {
      for (i=0; i<glyphs->num_glyphs; i++)
	{
	  VogueGlyphInfo *gi = &glyphs->glyphs[i];

	  if (gi->glyph != PANGO_GLYPH_EMPTY)
	    {
	      int glyph_x = x + x_off + gi->geometry.x_offset;
	      int glyph_y = y + gi->geometry.y_offset;

	      _vogue_xft_renderer_draw_unknown_glyph (renderer,
						      xfont,
						      xft_font,
						      gi,
						      glyph_x,
						      glyph_y);
	    }

	  x_off += gi->geometry.width;
	}
      return;
    }

  if (!fcfont->fontmap)	/* Display closed */
    return;

  for (i=0; i<glyphs->num_glyphs; i++)
    {
      VogueGlyphInfo *gi = &glyphs->glyphs[i];

      if (gi->glyph != PANGO_GLYPH_EMPTY)
	{
	  int glyph_x = x + x_off + gi->geometry.x_offset;
	  int glyph_y = y + gi->geometry.y_offset;

	  if (gi->glyph & PANGO_GLYPH_UNKNOWN_FLAG)
	    {
	      _vogue_xft_renderer_draw_unknown_glyph (renderer,
						      xfont,
						      xft_font,
						      gi,
						      glyph_x,
						      glyph_y);
	    }
	  else
	    {
	      draw_glyph (renderer, font, gi->glyph, glyph_x, glyph_y);
	    }
	}

      x_off += gi->geometry.width;
    }
}

static void
flush_trapezoids (VogueXftRenderer *xftrenderer)
{
  if (!xftrenderer->priv->trapezoids ||
      xftrenderer->priv->trapezoids->len == 0)
    return;

  PANGO_XFT_RENDERER_GET_CLASS (xftrenderer)->composite_trapezoids (xftrenderer,
								    xftrenderer->priv->trapezoid_part,
								    (XTrapezoid *)(void *)xftrenderer->priv->trapezoids->data,
								    xftrenderer->priv->trapezoids->len);

  g_array_set_size (xftrenderer->priv->trapezoids, 0);
}

static void
vogue_xft_renderer_draw_trapezoid (VogueRenderer   *renderer,
				   VogueRenderPart  part,
				   double           y1,
				   double           x11,
				   double           x21,
				   double           y2,
				   double           x12,
				   double           x22)
{
  VogueXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);
  XTrapezoid trap;

  flush_glyphs (xftrenderer);

  if (!xftrenderer->priv->trapezoids)
    xftrenderer->priv->trapezoids = g_array_new (FALSE, FALSE,
						 sizeof (XTrapezoid));

  if (xftrenderer->draw)
    {
      if (xftrenderer->priv->trapezoids->len > 0 &&
	  xftrenderer->priv->trapezoid_part != part)
	flush_trapezoids (xftrenderer);

      xftrenderer->priv->trapezoid_part = part;
    }

  trap.top = XDoubleToFixed (y1);
  trap.bottom = XDoubleToFixed (y2);
  trap.left.p1.x = XDoubleToFixed (x11);
  trap.left.p1.y = XDoubleToFixed (y1);
  trap.left.p2.x = XDoubleToFixed (x12);
  trap.left.p2.y = XDoubleToFixed (y2);
  trap.right.p1.x = XDoubleToFixed (x21);
  trap.right.p1.y = XDoubleToFixed (y1);
  trap.right.p2.x = XDoubleToFixed (x22);
  trap.right.p2.y = XDoubleToFixed (y2);

  g_array_append_val (xftrenderer->priv->trapezoids, trap);
}

static void
vogue_xft_renderer_part_changed (VogueRenderer   *renderer,
				 VogueRenderPart  part)
{
  VogueXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);

  if (part == PANGO_RENDER_PART_FOREGROUND)
    flush_glyphs (xftrenderer);

  if (part == xftrenderer->priv->trapezoid_part)
    flush_trapezoids (xftrenderer);
}

static void
vogue_xft_renderer_end (VogueRenderer *renderer)
{
  VogueXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);

  flush_glyphs (xftrenderer);
  flush_trapezoids (xftrenderer);
}

static void
vogue_xft_renderer_real_composite_trapezoids (VogueXftRenderer *xftrenderer,
					      VogueRenderPart   part,
					      XTrapezoid       *trapezoids,
					      int               n_trapezoids)
{
  Picture src_picture;
  Picture dest_picture;

  if (!XftDefaultHasRender (xftrenderer->display))
      return;

  if (xftrenderer->priv->src_picture != None)
    {
      src_picture = xftrenderer->priv->src_picture;
      dest_picture = xftrenderer->priv->dest_picture;
    }
  else
    {
      XftColor xft_color;
      VogueColor *color = vogue_renderer_get_color (PANGO_RENDERER (xftrenderer),
						    part);
      if (!color)
	color = &xftrenderer->priv->default_color;

      xft_color.color.red = color->red;
      xft_color.color.green = color->green;
      xft_color.color.blue = color->blue;
      xft_color.color.alpha = xftrenderer->priv->alpha;

      src_picture = XftDrawSrcPicture (xftrenderer->draw, &xft_color);
      dest_picture = XftDrawPicture (xftrenderer->draw);
    }

  XRenderCompositeTrapezoids (xftrenderer->display,
			      PictOpOver,
			      src_picture, dest_picture,
			      xftrenderer->priv->mask_format,
			      0, 0,
			      trapezoids, n_trapezoids);
}

static void
vogue_xft_renderer_real_composite_glyphs (VogueXftRenderer *xftrenderer,
					  XftFont          *xft_font,
					  XftGlyphSpec     *glyphs,
					  int               n_glyphs)
{
  if (xftrenderer->priv->src_picture != None)
    {
      XftGlyphSpecRender (xftrenderer->display, PictOpOver,
			  xftrenderer->priv->src_picture,
			  xft_font,
			  xftrenderer->priv->dest_picture, 0, 0,
			  glyphs, n_glyphs);
    }
  else
    {
      XftColor xft_color;
      VogueColor *color = vogue_renderer_get_color (PANGO_RENDERER (xftrenderer),
						    PANGO_RENDER_PART_FOREGROUND);
      if (!color)
	color = &xftrenderer->priv->default_color;

      xft_color.color.red = color->red;
      xft_color.color.green = color->green;
      xft_color.color.blue = color->blue;
      xft_color.color.alpha = xftrenderer->priv->alpha;

      XftDrawGlyphSpec (xftrenderer->draw, &xft_color,
			xft_font,
			glyphs, n_glyphs);
    }
}

static VogueRenderer *
get_renderer (VogueFontMap *fontmap,
	      XftDraw      *draw,
	      XftColor     *color)
{
  VogueRenderer *renderer;
  VogueXftRenderer *xftrenderer;
  VogueColor vogue_color;

  renderer = _vogue_xft_font_map_get_renderer (PANGO_XFT_FONT_MAP (fontmap));
  xftrenderer = PANGO_XFT_RENDERER (renderer);

  vogue_xft_renderer_set_draw (xftrenderer, draw);

  vogue_color.red = color->color.red;
  vogue_color.green = color->color.green;
  vogue_color.blue = color->color.blue;
  vogue_xft_renderer_set_default_color (xftrenderer, &vogue_color);
  xftrenderer->priv->alpha = color->color.alpha;

  return renderer;
}

static void
release_renderer (VogueRenderer *renderer)
{
  VogueXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);

  xftrenderer->priv->alpha = 0xffff;
}

/**
 * vogue_xft_render_layout:
 * @draw:      an #XftDraw
 * @color:     the foreground color in which to draw the layout
 *             (may be overridden by color attributes)
 * @layout:    a #VogueLayout
 * @x:         the X position of the left of the layout (in Vogue units)
 * @y:         the Y position of the top of the layout (in Vogue units)
 *
 * Render a #VogueLayout onto a #XftDraw
 *
 * Since: 1.8
 */
void
vogue_xft_render_layout (XftDraw     *draw,
			 XftColor    *color,
			 VogueLayout *layout,
			 int          x,
			 int          y)
{
  VogueContext *context;
  VogueFontMap *fontmap;
  VogueRenderer *renderer;

  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  context = vogue_layout_get_context (layout);
  fontmap = vogue_context_get_font_map (context);
  renderer = get_renderer (fontmap, draw, color);

  vogue_renderer_draw_layout (renderer, layout, x, y);

  release_renderer (renderer);
}

/**
 * vogue_xft_render_layout_line:
 * @draw:      an #XftDraw
 * @color:     the foreground color in which to draw the layout line
 *             (may be overridden by color attributes)
 * @line:      a #VogueLayoutLine
 * @x:         the x position of start of string (in Vogue units)
 * @y:         the y position of baseline (in Vogue units)
 *
 * Render a #VogueLayoutLine onto a #XftDraw
 *
 * Since: 1.8
 */
void
vogue_xft_render_layout_line (XftDraw         *draw,
			      XftColor        *color,
			      VogueLayoutLine *line,
			      int              x,
			      int              y)
{
  VogueContext *context;
  VogueFontMap *fontmap;
  VogueRenderer *renderer;

  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (line != NULL);

  context = vogue_layout_get_context (line->layout);
  fontmap = vogue_context_get_font_map (context);
  renderer = get_renderer (fontmap, draw, color);

  vogue_renderer_draw_layout_line (renderer, line, x, y);

  release_renderer (renderer);
}

/**
 * vogue_xft_render_transformed:
 * @draw:    an #XftDraw
 * @color:   the color in which to draw the glyphs
 * @font:    the font in which to draw the string
 * @matrix:  (nullable): a #VogueMatrix, or %NULL to use an identity
 *           transformation
 * @glyphs:  the glyph string to draw
 * @x:       the x position of the start of the string (in Vogue
 *           units in user space coordinates)
 * @y:       the y position of the baseline (in Vogue units
 *           in user space coordinates)
 *
 * Renders a #VogueGlyphString onto a #XftDraw, possibly
 * transforming the layed-out coordinates through a transformation
 * matrix. Note that the transformation matrix for @font is not
 * changed, so to produce correct rendering results, the @font
 * must have been loaded using a #VogueContext with an identical
 * transformation matrix to that passed in to this function.
 *
 * Since: 1.8
 **/
void
vogue_xft_render_transformed (XftDraw          *draw,
			      XftColor         *color,
			      VogueMatrix      *matrix,
			      VogueFont        *font,
			      VogueGlyphString *glyphs,
			      int               x,
			      int               y)
{
  VogueFontMap *fontmap;
  VogueRenderer *renderer;

  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);

  fontmap = PANGO_FC_FONT (font)->fontmap;
  renderer = get_renderer (fontmap, draw, color);

  vogue_renderer_set_matrix (renderer, matrix);

  vogue_renderer_draw_glyphs (renderer, font, glyphs, x, y);

  release_renderer (renderer);
}

/**
 * vogue_xft_render:
 * @draw:    the <type>XftDraw</type> object.
 * @color:   the color in which to draw the string
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Renders a #VogueGlyphString onto an <type>XftDraw</type> object wrapping an X drawable.
 */
void
vogue_xft_render (XftDraw          *draw,
		  XftColor         *color,
		  VogueFont        *font,
		  VogueGlyphString *glyphs,
		  gint              x,
		  gint              y)
{
  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);

  vogue_xft_render_transformed (draw, color, NULL, font, glyphs,
				x * PANGO_SCALE, y * PANGO_SCALE);
}

/**
 * vogue_xft_picture_render:
 * @display:      an X display
 * @src_picture:  the source picture to draw the string with
 * @dest_picture: the destination picture to draw the string onto
 * @font:         the font in which to draw the string
 * @glyphs:       the glyph string to draw
 * @x:            the x position of start of string (in pixels)
 * @y:            the y position of baseline (in pixels)
 *
 * Renders a #VogueGlyphString onto an Xrender <type>Picture</type> object.
 */
void
vogue_xft_picture_render (Display          *display,
			  Picture           src_picture,
			  Picture           dest_picture,
			  VogueFont        *font,
			  VogueGlyphString *glyphs,
			  gint              x,
			  gint              y)
{
  VogueFontMap *fontmap;
  VogueRenderer *renderer;

  g_return_if_fail (display != NULL);
  g_return_if_fail (src_picture != None);
  g_return_if_fail (dest_picture != None);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);

  fontmap = PANGO_FC_FONT (font)->fontmap;
  renderer = _vogue_xft_font_map_get_renderer (PANGO_XFT_FONT_MAP (fontmap));

  vogue_xft_renderer_set_pictures (PANGO_XFT_RENDERER (renderer),
				   src_picture, dest_picture);
  vogue_renderer_set_matrix (renderer, NULL);

  vogue_renderer_draw_glyphs (renderer, font, glyphs, x * PANGO_SCALE, y * PANGO_SCALE);

  vogue_xft_renderer_set_pictures (PANGO_XFT_RENDERER (renderer),
				   None, None);
}

/**
 * vogue_xft_renderer_new:
 * @display: an X display
 * @screen:   the index of the screen for @display to which rendering will be done
 *
 * Create a new #VogueXftRenderer to allow rendering Vogue objects
 * with the Xft library. You must call vogue_xft_renderer_set_draw() before
 * using the renderer.
 *
 * Return value: the newly created #VogueXftRenderer, which should
 *               be freed with g_object_unref().
 *
 * Since: 1.8
 **/
VogueRenderer *
vogue_xft_renderer_new (Display *display,
			int      screen)
{
  VogueXftRenderer *xftrenderer;

  xftrenderer = g_object_new (PANGO_TYPE_XFT_RENDERER,
			      "display", display,
			      "screen", screen,
			      NULL);

  return PANGO_RENDERER (xftrenderer);
}

/**
 * vogue_xft_renderer_set_draw:
 * @xftrenderer: a #VogueXftRenderer
 * @draw: a #XftDraw
 *
 * Sets the #XftDraw object that the renderer is drawing to.
 * The renderer must not be currently active.
 *
 * Since: 1.8
 **/
void
vogue_xft_renderer_set_draw (VogueXftRenderer *xftrenderer,
			     XftDraw          *draw)
{
  g_return_if_fail (PANGO_IS_XFT_RENDERER (xftrenderer));

  xftrenderer->draw = draw;
}

/**
 * vogue_xft_renderer_set_default_color:
 * @xftrenderer: a #XftRenderer
 * @default_color: the default foreground color
 *
 * Sets the default foreground color for a #XftRenderer.
 *
 * Since: 1.8
 **/
void
vogue_xft_renderer_set_default_color (VogueXftRenderer *xftrenderer,
				      VogueColor       *default_color)
{
  g_return_if_fail (PANGO_IS_XFT_RENDERER (xftrenderer));

  xftrenderer->priv->default_color = *default_color;
}
