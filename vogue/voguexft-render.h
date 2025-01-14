/* Vogue
 * voguexft-render.h: Rendering routines for the Xft library
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

#ifndef __PANGOXFT_RENDER_H__
#define __PANGOXFT_RENDER_H__

#include <vogue/vogue-renderer.h>

G_BEGIN_DECLS

#define _XFT_NO_COMPAT_
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#if defined(XftVersion) && XftVersion >= 20000
#else
#error "must have Xft version 2 or newer"
#endif

typedef struct _VogueXftRenderer        VogueXftRenderer;
typedef struct _VogueXftRendererClass   VogueXftRendererClass;
typedef struct _VogueXftRendererPrivate VogueXftRendererPrivate;

#define PANGO_TYPE_XFT_RENDERER            (vogue_xft_renderer_get_type())
#define PANGO_XFT_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_RENDERER, VogueXftRenderer))
#define PANGO_IS_XFT_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_RENDERER))
#define PANGO_XFT_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_XFT_RENDERER, VogueXftRendererClass))
#define PANGO_IS_XFT_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_XFT_RENDERER))
#define PANGO_XFT_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_XFT_RENDERER, VogueXftRendererClass))

/**
 * VogueXftRenderer:
 *
 * #VogueXftRenderer is a subclass of #VogueRenderer used for rendering
 * with Vogue's Xft backend. It can be used directly, or it can be
 * further subclassed to modify exactly how drawing of individual
 * elements occurs.
 *
 * Since: 1.8
 */
struct _VogueXftRenderer
{
  /*< private >*/
  VogueRenderer parent_instance;

  Display *display;
  int screen;
  XftDraw *draw;

  VogueXftRendererPrivate *priv;
};

/**
 * VogueXftRendererClass:
 * @composite_trapezoids: draw the specified trapezoids using
 *   the current color and other attributes for @part
 * @composite_glyphs: draw the specified glyphs using
 *   the current foreground color and other foreground
 *   attributes
 *
 * The class structure for #VogueXftRenderer
 *
 * Since: 1.8
 */
struct _VogueXftRendererClass
{
  /*< private >*/
  VogueRendererClass parent_class;

  /*< public >*/
  void (*composite_trapezoids) (VogueXftRenderer *xftrenderer,
				VogueRenderPart   part,
				XTrapezoid       *trapezoids,
				int               n_trapezoids);
  void (*composite_glyphs)     (VogueXftRenderer *xftrenderer,
				XftFont          *xft_font,
				XftGlyphSpec     *glyphs,
				int               n_glyphs);
};

PANGO_AVAILABLE_IN_1_8
GType vogue_xft_renderer_get_type    (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_8
VogueRenderer *vogue_xft_renderer_new                 (Display          *display,
						       int               screen);
PANGO_AVAILABLE_IN_1_8
void           vogue_xft_renderer_set_draw            (VogueXftRenderer *xftrenderer,
						       XftDraw          *draw);
PANGO_AVAILABLE_IN_1_8
void           vogue_xft_renderer_set_default_color   (VogueXftRenderer *xftrenderer,
						       VogueColor       *default_color);

PANGO_AVAILABLE_IN_ALL
void vogue_xft_render             (XftDraw          *draw,
				   XftColor         *color,
				   VogueFont        *font,
				   VogueGlyphString *glyphs,
				   gint              x,
				   gint              y);
PANGO_AVAILABLE_IN_ALL
void vogue_xft_picture_render     (Display          *display,
				   Picture           src_picture,
				   Picture           dest_picture,
				   VogueFont        *font,
				   VogueGlyphString *glyphs,
				   gint              x,
				   gint              y);
PANGO_AVAILABLE_IN_1_8
void vogue_xft_render_transformed (XftDraw          *draw,
				   XftColor         *color,
				   VogueMatrix      *matrix,
				   VogueFont        *font,
				   VogueGlyphString *glyphs,
				   int               x,
				   int               y);
PANGO_AVAILABLE_IN_1_8
void vogue_xft_render_layout_line (XftDraw          *draw,
				   XftColor         *color,
				   VogueLayoutLine  *line,
				   int               x,
				   int               y);
PANGO_AVAILABLE_IN_1_8
void vogue_xft_render_layout      (XftDraw          *draw,
				   XftColor         *color,
				   VogueLayout      *layout,
				   int               x,
				   int               y);

G_END_DECLS

#endif /* __PANGOXFT_RENDER_H__ */

