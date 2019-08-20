/* Vogue
 * voguecairo.h:
 *
 * Copyright (C) 1999, 2004 Red Hat, Inc.
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

#ifndef __PANGOCAIRO_H__
#define __PANGOCAIRO_H__

#include <vogue/vogue.h>
#include <cairo.h>

G_BEGIN_DECLS

/**
 * VogueCairoFont:
 *
 * #VogueCairoFont is an interface exported by fonts for
 * use with Cairo. The actual type of the font will depend
 * on the particular font technology Cairo was compiled to use.
 *
 * Since: 1.18
 **/
typedef struct _VogueCairoFont      VogueCairoFont;
#define PANGO_TYPE_CAIRO_FONT       (vogue_cairo_font_get_type ())
#define PANGO_CAIRO_FONT(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_FONT, VogueCairoFont))
#define PANGO_IS_CAIRO_FONT(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_FONT))

/**
 * VogueCairoFontMap:
 *
 * #VogueCairoFontMap is an interface exported by font maps for
 * use with Cairo. The actual type of the font map will depend
 * on the particular font technology Cairo was compiled to use.
 *
 * Since: 1.10
 **/
typedef struct _VogueCairoFontMap        VogueCairoFontMap;
#define PANGO_TYPE_CAIRO_FONT_MAP       (vogue_cairo_font_map_get_type ())
#define PANGO_CAIRO_FONT_MAP(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_FONT_MAP, VogueCairoFontMap))
#define PANGO_IS_CAIRO_FONT_MAP(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_FONT_MAP))

/**
 * VogueCairoShapeRendererFunc:
 * @cr: a Cairo context with current point set to where the shape should
 * be rendered
 * @attr: the %PANGO_ATTR_SHAPE to render
 * @do_path: whether only the shape path should be appended to current
 * path of @cr and no filling/stroking done.  This will be set
 * to %TRUE when called from vogue_cairo_layout_path() and
 * vogue_cairo_layout_line_path() rendering functions.
 * @data: user data passed to vogue_cairo_context_set_shape_renderer()
 *
 * Function type for rendering attributes of type %PANGO_ATTR_SHAPE
 * with Vogue's Cairo renderer.
 */
typedef void (* VogueCairoShapeRendererFunc) (cairo_t        *cr,
					      VogueAttrShape *attr,
					      gboolean        do_path,
					      gpointer        data);

/*
 * VogueCairoFontMap
 */
PANGO_AVAILABLE_IN_1_10
GType         vogue_cairo_font_map_get_type          (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_10
VogueFontMap *vogue_cairo_font_map_new               (void);
PANGO_AVAILABLE_IN_1_18
VogueFontMap *vogue_cairo_font_map_new_for_font_type (cairo_font_type_t fonttype);
PANGO_AVAILABLE_IN_1_10
VogueFontMap *vogue_cairo_font_map_get_default       (void);
PANGO_AVAILABLE_IN_1_22
void          vogue_cairo_font_map_set_default       (VogueCairoFontMap *fontmap);
PANGO_AVAILABLE_IN_1_18
cairo_font_type_t vogue_cairo_font_map_get_font_type (VogueCairoFontMap *fontmap);

PANGO_AVAILABLE_IN_1_10
void          vogue_cairo_font_map_set_resolution (VogueCairoFontMap *fontmap,
						   double             dpi);
PANGO_AVAILABLE_IN_1_10
double        vogue_cairo_font_map_get_resolution (VogueCairoFontMap *fontmap);
#ifndef PANGO_DISABLE_DEPRECATED
PANGO_DEPRECATED_IN_1_22_FOR(vogue_font_map_create_context)
VogueContext *vogue_cairo_font_map_create_context (VogueCairoFontMap *fontmap);
#endif

/*
 * VogueCairoFont
 */
PANGO_AVAILABLE_IN_1_18
GType         vogue_cairo_font_get_type               (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_18
cairo_scaled_font_t *vogue_cairo_font_get_scaled_font (VogueCairoFont *font);

/* Update a Vogue context for the current state of a cairo context
 */
PANGO_AVAILABLE_IN_1_10
void         vogue_cairo_update_context (cairo_t      *cr,
					 VogueContext *context);

PANGO_AVAILABLE_IN_1_10
void                        vogue_cairo_context_set_font_options (VogueContext               *context,
								  const cairo_font_options_t *options);
PANGO_AVAILABLE_IN_1_10
const cairo_font_options_t *vogue_cairo_context_get_font_options (VogueContext               *context);

PANGO_AVAILABLE_IN_1_10
void               vogue_cairo_context_set_resolution     (VogueContext       *context,
							   double              dpi);
PANGO_AVAILABLE_IN_1_10
double             vogue_cairo_context_get_resolution     (VogueContext       *context);

PANGO_AVAILABLE_IN_1_18
void                        vogue_cairo_context_set_shape_renderer (VogueContext                *context,
								    VogueCairoShapeRendererFunc  func,
								    gpointer                     data,
								    GDestroyNotify               dnotify);
PANGO_AVAILABLE_IN_1_18
VogueCairoShapeRendererFunc vogue_cairo_context_get_shape_renderer (VogueContext                *context,
								    gpointer                    *data);

/* Convenience
 */
PANGO_AVAILABLE_IN_1_22
VogueContext *vogue_cairo_create_context (cairo_t   *cr);
PANGO_AVAILABLE_IN_ALL
VogueLayout *vogue_cairo_create_layout (cairo_t     *cr);
PANGO_AVAILABLE_IN_1_10
void         vogue_cairo_update_layout (cairo_t     *cr,
					VogueLayout *layout);

/*
 * Rendering
 */
PANGO_AVAILABLE_IN_1_10
void vogue_cairo_show_glyph_string (cairo_t          *cr,
				    VogueFont        *font,
				    VogueGlyphString *glyphs);
PANGO_AVAILABLE_IN_1_22
void vogue_cairo_show_glyph_item   (cairo_t          *cr,
				    const char       *text,
				    VogueGlyphItem   *glyph_item);
PANGO_AVAILABLE_IN_1_10
void vogue_cairo_show_layout_line  (cairo_t          *cr,
				    VogueLayoutLine  *line);
PANGO_AVAILABLE_IN_1_10
void vogue_cairo_show_layout       (cairo_t          *cr,
				    VogueLayout      *layout);

PANGO_AVAILABLE_IN_1_14
void vogue_cairo_show_error_underline (cairo_t       *cr,
				       double         x,
				       double         y,
				       double         width,
				       double         height);

/*
 * Rendering to a path
 */
PANGO_AVAILABLE_IN_1_10
void vogue_cairo_glyph_string_path (cairo_t          *cr,
				    VogueFont        *font,
				    VogueGlyphString *glyphs);
PANGO_AVAILABLE_IN_1_10
void vogue_cairo_layout_line_path  (cairo_t          *cr,
				    VogueLayoutLine  *line);
PANGO_AVAILABLE_IN_1_10
void vogue_cairo_layout_path       (cairo_t          *cr,
				    VogueLayout      *layout);

PANGO_AVAILABLE_IN_1_14
void vogue_cairo_error_underline_path (cairo_t       *cr,
				       double         x,
				       double         y,
				       double         width,
				       double         height);

G_END_DECLS

#endif /* __PANGOCAIRO_H__ */
