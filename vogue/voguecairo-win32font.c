/* Vogue
 * voguecairowin32-font.c: Cairo font handling, Win32 backend
 *
 * Copyright (C) 2000-2005 Red Hat Software
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
#include <stdlib.h>
#include <string.h>

#include "vogue-fontmap.h"
#include "vogue-impl-utils.h"
#include "voguecairo-private.h"
#include "voguecairo-win32.h"

#include <cairo-win32.h>

#define PANGO_TYPE_CAIRO_WIN32_FONT           (vogue_cairo_win32_font_get_type ())
#define PANGO_CAIRO_WIN32_FONT(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_WIN32_FONT, VogueCairoWin32Font))
#define PANGO_CAIRO_WIN32_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CAIRO_WIN32_FONT, VogueCairoWin32FontClass))
#define PANGO_CAIRO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_CAIRO_WIN32_FONT))
#define PANGO_CAIRO_WIN32_FONT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CAIRO_WIN32_FONT, VogueCairoWin32FontClass))

typedef struct _VogueCairoWin32Font      VogueCairoWin32Font;
typedef struct _VogueCairoWin32FontClass VogueCairoWin32FontClass;

struct _VogueCairoWin32Font
{
  VogueWin32Font font;
  VogueCairoFontPrivate cf_priv;
};

struct _VogueCairoWin32FontClass
{
  VogueWin32FontClass  parent_class;
};

GType vogue_cairo_win32_font_get_type (void);

static cairo_font_face_t *vogue_cairo_win32_font_create_font_face                (VogueCairoFont *font);
static VogueFontMetrics  *vogue_cairo_win32_font_create_base_metrics_for_context (VogueCairoFont *font,
										  VogueContext    *context);


static void
cairo_font_iface_init (VogueCairoFontIface *iface)
{
  iface->create_font_face = vogue_cairo_win32_font_create_font_face;
  iface->create_base_metrics_for_context = vogue_cairo_win32_font_create_base_metrics_for_context;
  iface->cf_priv_offset = G_STRUCT_OFFSET (VogueCairoWin32Font, cf_priv);
}

G_DEFINE_TYPE_WITH_CODE (VogueCairoWin32Font, vogue_cairo_win32_font, PANGO_TYPE_WIN32_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

static cairo_font_face_t *
vogue_cairo_win32_font_create_font_face (VogueCairoFont *font)
{
  VogueCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);
  VogueWin32Font *win32font =  &cwfont->font;

  return cairo_win32_font_face_create_for_logfontw (&win32font->logfontw);
}

static VogueFontMetrics *
vogue_cairo_win32_font_create_base_metrics_for_context (VogueCairoFont *font,
							VogueContext   *context)
{
  VogueFontMetrics *metrics;
  cairo_scaled_font_t *scaled_font;
  cairo_font_extents_t font_extents;

  metrics = vogue_font_metrics_new ();

  scaled_font = vogue_cairo_font_get_scaled_font (font);

  cairo_scaled_font_extents (scaled_font, &font_extents);
  cairo_win32_scaled_font_done_font (scaled_font);

  metrics->ascent = font_extents.ascent * PANGO_SCALE;
  metrics->descent = font_extents.descent * PANGO_SCALE;

  /* FIXME: Should get the real settings for these from the TrueType
   * font file.
   */
  metrics->height = metrics->ascent + metrics->descent;
  metrics->underline_thickness = metrics->height / 14;
  metrics->underline_position = - metrics->underline_thickness;
  metrics->strikethrough_thickness = metrics->underline_thickness;
  metrics->strikethrough_position = metrics->height / 4;

  vogue_quantize_line_geometry (&metrics->underline_thickness,
				&metrics->underline_position);
  vogue_quantize_line_geometry (&metrics->strikethrough_thickness,
				&metrics->strikethrough_position);
  /* Quantizing may have pushed underline_position to 0.  Not good */
  if (metrics->underline_position == 0)
    metrics->underline_position = - metrics->underline_thickness;

  return metrics;
}

static void
vogue_cairo_win32_font_finalize (GObject *object)
{
  VogueCairoWin32Font *cwfont = (VogueCairoWin32Font *) object;

  _vogue_cairo_font_private_finalize (&cwfont->cf_priv);

  G_OBJECT_CLASS (vogue_cairo_win32_font_parent_class)->finalize (object);
}

static void
vogue_cairo_win32_font_get_glyph_extents (VogueFont        *font,
					  VogueGlyph        glyph,
					  VogueRectangle   *ink_rect,
					  VogueRectangle   *logical_rect)
{
  VogueCairoWin32Font *cwfont = (VogueCairoWin32Font *) font;

  _vogue_cairo_font_private_get_glyph_extents (&cwfont->cf_priv,
					       glyph,
					       ink_rect,
					       logical_rect);
}

static gboolean
vogue_cairo_win32_font_select_font (VogueFont *font,
				    HDC        hdc)
{
  cairo_scaled_font_t *scaled_font = _vogue_cairo_font_private_get_scaled_font (&PANGO_CAIRO_WIN32_FONT (font)->cf_priv);

  return cairo_win32_scaled_font_select_font (scaled_font, hdc) == CAIRO_STATUS_SUCCESS;
}

static void
vogue_cairo_win32_font_done_font (VogueFont *font)
{
  cairo_scaled_font_t *scaled_font = _vogue_cairo_font_private_get_scaled_font (&PANGO_CAIRO_WIN32_FONT (font)->cf_priv);

  cairo_win32_scaled_font_done_font (scaled_font);
}

static double
vogue_cairo_win32_font_get_metrics_factor (VogueFont *font)
{
  VogueWin32Font *win32font = PANGO_WIN32_FONT (font);
  cairo_scaled_font_t *scaled_font = _vogue_cairo_font_private_get_scaled_font (&PANGO_CAIRO_WIN32_FONT (font)->cf_priv);

  return cairo_win32_scaled_font_get_metrics_factor (scaled_font) * win32font->size;
}

static void
vogue_cairo_win32_font_class_init (VogueCairoWin32FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontClass *font_class = PANGO_FONT_CLASS (class);
  VogueWin32FontClass *win32_font_class = PANGO_WIN32_FONT_CLASS (class);

  object_class->finalize = vogue_cairo_win32_font_finalize;

  font_class->get_glyph_extents = vogue_cairo_win32_font_get_glyph_extents;
  font_class->get_metrics = _vogue_cairo_font_get_metrics;

  win32_font_class->select_font = vogue_cairo_win32_font_select_font;
  win32_font_class->done_font = vogue_cairo_win32_font_done_font;
  win32_font_class->get_metrics_factor = vogue_cairo_win32_font_get_metrics_factor;
}

static void
vogue_cairo_win32_font_init (VogueCairoWin32Font *cwfont G_GNUC_UNUSED)
{
}

/********************
 *    Private API   *
 ********************/

VogueFont *
_vogue_cairo_win32_font_new (VogueCairoWin32FontMap     *cwfontmap,
			     VogueContext               *context,
			     VogueWin32Face             *face,
			     const VogueFontDescription *desc)
{
  VogueCairoWin32Font *cwfont;
  VogueWin32Font *win32font;
  double size;
  double dpi;
#define USE_FACE_CACHED_FONTS
#ifdef USE_FACE_CACHED_FONTS
  VogueWin32FontMap *win32fontmap;
  GSList *tmp_list;
#endif
  cairo_matrix_t font_matrix;

  g_return_val_if_fail (PANGO_IS_CAIRO_WIN32_FONT_MAP (cwfontmap), NULL);

  size = (double) vogue_font_description_get_size (desc) / PANGO_SCALE;

  if (context)
    {
      dpi = vogue_cairo_context_get_resolution (context);

      if (dpi <= 0)
	dpi = cwfontmap->dpi;
    }
  else
    dpi = cwfontmap->dpi;

  if (!vogue_font_description_get_size_is_absolute (desc))
    size *= dpi / 72.;

#ifdef USE_FACE_CACHED_FONTS
  win32fontmap = PANGO_WIN32_FONT_MAP (cwfontmap);

  tmp_list = face->cached_fonts;
  while (tmp_list)
    {
      win32font = tmp_list->data;
      if (ABS (win32font->size - size * PANGO_SCALE) < 2)
	{
	  g_object_ref (win32font);
	  if (win32font->in_cache)
	    _vogue_win32_fontmap_cache_remove (PANGO_FONT_MAP (win32fontmap), win32font);

	  return PANGO_FONT (win32font);
	}
      tmp_list = tmp_list->next;
    }
#endif
  cwfont = g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT, NULL);
  win32font = PANGO_WIN32_FONT (cwfont);

  g_assert (win32font->fontmap == NULL);
  win32font->fontmap = (VogueFontMap *) cwfontmap;
  g_object_add_weak_pointer (G_OBJECT (win32font->fontmap), (gpointer *) (gpointer) &win32font->fontmap);

  win32font->win32face = face;

#ifdef USE_FACE_CACHED_FONTS
  face->cached_fonts = g_slist_prepend (face->cached_fonts, win32font);
#endif

  /* FIXME: This is a pixel size, so not really what we want for describe(),
   * but it's what we need when computing the scale factor.
   */
  win32font->size = size * PANGO_SCALE;

  _vogue_win32_make_matching_logfontw (win32font->fontmap,
				       &face->logfontw,
				       win32font->size,
				       &win32font->logfontw);

  cairo_matrix_init_identity (&font_matrix);

  cairo_matrix_scale (&font_matrix, size, size);

  _vogue_cairo_font_private_initialize (&cwfont->cf_priv,
					(VogueCairoFont *) cwfont,
					vogue_font_description_get_gravity (desc),
					_vogue_cairo_context_get_merged_font_options (context),
					vogue_context_get_matrix (context),
					&font_matrix);

  return PANGO_FONT (cwfont);
}
