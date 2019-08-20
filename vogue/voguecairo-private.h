/* Vogue
 * voguecairo-private.h: private symbols for the Cairo backend
 *
 * Copyright (C) 2000,2004 Red Hat, Inc.
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

#ifndef __PANGOCAIRO_PRIVATE_H__
#define __PANGOCAIRO_PRIVATE_H__

#include <vogue/voguecairo.h>
#include <vogue/vogue-renderer.h>

G_BEGIN_DECLS


#define PANGO_CAIRO_FONT_MAP_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), PANGO_TYPE_CAIRO_FONT_MAP, VogueCairoFontMapIface))

typedef struct _VogueCairoFontMapIface VogueCairoFontMapIface;

struct _VogueCairoFontMapIface
{
  GTypeInterface g_iface;

  void           (*set_resolution) (VogueCairoFontMap *fontmap,
				    double             dpi);
  double         (*get_resolution) (VogueCairoFontMap *fontmap);

  cairo_font_type_t (*get_font_type) (VogueCairoFontMap *fontmap);
};


#define PANGO_CAIRO_FONT_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), PANGO_TYPE_CAIRO_FONT, VogueCairoFontIface))

typedef struct _VogueCairoFontIface                  VogueCairoFontIface;
typedef struct _VogueCairoFontPrivate                VogueCairoFontPrivate;
typedef struct _VogueCairoFontHexBoxInfo             VogueCairoFontHexBoxInfo;
typedef struct _VogueCairoFontPrivateScaledFontData  VogueCairoFontPrivateScaledFontData;
typedef struct _VogueCairoFontGlyphExtentsCacheEntry VogueCairoFontGlyphExtentsCacheEntry;

struct _VogueCairoFontHexBoxInfo
{
  VogueCairoFont *font;
  int rows;
  double digit_width;
  double digit_height;
  double pad_x;
  double pad_y;
  double line_width;
  double box_descent;
  double box_height;
};

struct _VogueCairoFontPrivateScaledFontData
{
  cairo_matrix_t font_matrix;
  cairo_matrix_t ctm;
  cairo_font_options_t *options;
};

struct _VogueCairoFontPrivate
{
  VogueCairoFont *cfont;

  VogueCairoFontPrivateScaledFontData *data;

  cairo_scaled_font_t *scaled_font;
  VogueCairoFontHexBoxInfo *hbi;

  gboolean is_hinted;
  VogueGravity gravity;

  VogueRectangle font_extents;
  VogueCairoFontGlyphExtentsCacheEntry *glyph_extents_cache;

  GSList *metrics_by_lang;
};

struct _VogueCairoFontIface
{
  GTypeInterface g_iface;

  cairo_font_face_t *(*create_font_face) (VogueCairoFont *cfont);
  VogueFontMetrics *(*create_base_metrics_for_context) (VogueCairoFont *cfont,
							VogueContext   *context);

  gssize cf_priv_offset;
};

gboolean _vogue_cairo_font_install (VogueFont *font,
				    cairo_t   *cr);
VogueFontMetrics * _vogue_cairo_font_get_metrics (VogueFont     *font,
						  VogueLanguage *language);
VogueCairoFontHexBoxInfo *_vogue_cairo_font_get_hex_box_info (VogueCairoFont *cfont);

void _vogue_cairo_font_private_initialize (VogueCairoFontPrivate      *cf_priv,
					   VogueCairoFont             *font,
					   VogueGravity                gravity,
					   const cairo_font_options_t *font_options,
					   const VogueMatrix          *vogue_ctm,
					   const cairo_matrix_t       *font_matrix);
void _vogue_cairo_font_private_finalize (VogueCairoFontPrivate *cf_priv);
cairo_scaled_font_t *_vogue_cairo_font_private_get_scaled_font (VogueCairoFontPrivate *cf_priv);
gboolean _vogue_cairo_font_private_is_metrics_hinted (VogueCairoFontPrivate *cf_priv);
void _vogue_cairo_font_private_get_glyph_extents (VogueCairoFontPrivate *cf_priv,
						  VogueGlyph             glyph,
						  VogueRectangle        *ink_rect,
						  VogueRectangle        *logical_rect);

#define PANGO_TYPE_CAIRO_RENDERER            (vogue_cairo_renderer_get_type())
#define PANGO_CAIRO_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_RENDERER, VogueCairoRenderer))
#define PANGO_IS_CAIRO_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_RENDERER))

typedef struct _VogueCairoRenderer VogueCairoRenderer;

_PANGO_EXTERN
GType vogue_cairo_renderer_get_type    (void) G_GNUC_CONST;


const cairo_font_options_t *_vogue_cairo_context_get_merged_font_options (VogueContext *context);


G_END_DECLS

#endif /* __PANGOCAIRO_PRIVATE_H__ */
