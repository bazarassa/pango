/* Vogue
 * voguecairo-coretextfont.c
 *
 * Copyright (C) 2000-2005 Red Hat Software
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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

#include <Carbon/Carbon.h>

#include "vogue-impl-utils.h"
#include "voguecoretext-private.h"
#include "voguecairo.h"
#include "voguecairo-private.h"
#include "voguecairo-coretext.h"
#include "voguecairo-coretextfont.h"

struct _VogueCairoCoreTextFont
{
  VogueCoreTextFont font;
  VogueCairoFontPrivate cf_priv;
  int abs_size;
};

struct _VogueCairoCoreTextFontClass
{
  VogueCoreTextFontClass parent_class;
};



static cairo_font_face_t *vogue_cairo_core_text_font_create_font_face           (VogueCairoFont *font);
static VogueFontMetrics  *vogue_cairo_core_text_font_create_base_metrics_for_context (VogueCairoFont *font,
                                                                                      VogueContext    *context);

static void
cairo_font_iface_init (VogueCairoFontIface *iface)
{
  iface->create_font_face = vogue_cairo_core_text_font_create_font_face;
  iface->create_base_metrics_for_context = vogue_cairo_core_text_font_create_base_metrics_for_context;
  iface->cf_priv_offset = G_STRUCT_OFFSET (VogueCairoCoreTextFont, cf_priv);
}

G_DEFINE_TYPE_WITH_CODE (VogueCairoCoreTextFont, vogue_cairo_core_text_font, PANGO_TYPE_CORE_TEXT_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

/* we want get_glyph_extents extremely fast, so we use a small wrapper here
 * to avoid having to lookup the interface data like we do for get_metrics
 * in _vogue_cairo_font_get_metrics(). */
static void
vogue_cairo_core_text_font_get_glyph_extents (VogueFont      *font,
                                              VogueGlyph      glyph,
                                              VogueRectangle *ink_rect,
                                              VogueRectangle *logical_rect)
{
  VogueCairoCoreTextFont *cafont = (VogueCairoCoreTextFont *) (font);

  _vogue_cairo_font_private_get_glyph_extents (&cafont->cf_priv,
					       glyph,
					       ink_rect,
					       logical_rect);
}

static cairo_font_face_t *
vogue_cairo_core_text_font_create_font_face (VogueCairoFont *font)
{
  VogueCoreTextFont *ctfont = (VogueCoreTextFont *) (font);
  CTFontRef font_id;
  CGFontRef cgfont;
  cairo_font_face_t *cairo_face;

  font_id = vogue_core_text_font_get_ctfont (ctfont);
  cgfont = CTFontCopyGraphicsFont (font_id, NULL);

  cairo_face = cairo_quartz_font_face_create_for_cgfont (cgfont);

  CFRelease (cgfont);

  return cairo_face;
}

static VogueFontMetrics *
vogue_cairo_core_text_font_create_base_metrics_for_context (VogueCairoFont *font,
                                                            VogueContext   *context)
{
  VogueCoreTextFont *cfont = (VogueCoreTextFont *) font;
  VogueFontMetrics *metrics;
  VogueFontDescription *font_desc;
  VogueLayout *layout;
  VogueRectangle extents;
  VogueLanguage *language = vogue_context_get_language (context);
  const char *sample_str = vogue_language_get_sample_string (language);
  CTFontRef ctfont;

  metrics = vogue_font_metrics_new ();

  ctfont = vogue_core_text_font_get_ctfont (cfont);

  metrics->ascent = CTFontGetAscent (ctfont) * PANGO_SCALE;
  metrics->descent = CTFontGetDescent (ctfont) * PANGO_SCALE;
  metrics->height = (CTFontGetAscent (ctfont) + CTFontGetDescent (ctfont) + CTFontGetLeading (ctfont)) * PANGO_SCALE;

  metrics->underline_position = CTFontGetUnderlinePosition (ctfont) * PANGO_SCALE;
  metrics->underline_thickness = CTFontGetUnderlineThickness (ctfont) * PANGO_SCALE;

  metrics->strikethrough_position = metrics->ascent / 3;
  metrics->strikethrough_thickness = CTFontGetUnderlineThickness (ctfont) * PANGO_SCALE;

  return metrics;
}

static VogueFontDescription *
vogue_cairo_core_text_font_describe_absolute (VogueFont *font)
{
  VogueCairoCoreTextFont *cafont = (VogueCairoCoreTextFont *)font;
  VogueFontDescription *desc = vogue_font_describe (font);
  
  vogue_font_description_set_absolute_size (desc, cafont->abs_size);

  return desc;
}

static void
vogue_cairo_core_text_font_finalize (GObject *object)
{
  VogueCairoCoreTextFont *cafont = (VogueCairoCoreTextFont *) object;

  _vogue_cairo_font_private_finalize (&cafont->cf_priv);

  G_OBJECT_CLASS (vogue_cairo_core_text_font_parent_class)->finalize (object);
}

static void
vogue_cairo_core_text_font_class_init (VogueCairoCoreTextFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = vogue_cairo_core_text_font_finalize;
  /* font_class->describe defined by parent class VogueCoreTextFont. */
  font_class->get_glyph_extents = vogue_cairo_core_text_font_get_glyph_extents;
  font_class->get_metrics = _vogue_cairo_font_get_metrics;
  font_class->describe_absolute = vogue_cairo_core_text_font_describe_absolute;
}

static void
vogue_cairo_core_text_font_init (VogueCairoCoreTextFont *cafont G_GNUC_UNUSED)
{
}

VogueCoreTextFont *
_vogue_cairo_core_text_font_new (VogueCairoCoreTextFontMap  *cafontmap,
                                 VogueCoreTextFontKey       *key)
{
  gboolean synthesize_italic = FALSE;
  VogueCairoCoreTextFont *cafont;
  VogueCoreTextFont *cfont;
  CTFontRef font_ref;
  CTFontDescriptorRef ctdescriptor;
  CGFontRef font_id;
  double size;
  cairo_matrix_t font_matrix;

  size = vogue_units_to_double (vogue_core_text_font_key_get_size (key));

  size /= vogue_matrix_get_font_scale_factor (vogue_core_text_font_key_get_matrix (key));

  ctdescriptor = vogue_core_text_font_key_get_ctfontdescriptor (key);
  font_ref = CTFontCreateWithFontDescriptor (ctdescriptor, size, NULL);

  if (vogue_core_text_font_key_get_synthetic_italic (key))
    synthesize_italic = TRUE;

  font_id = CTFontCopyGraphicsFont (font_ref, NULL);
  if (!font_id)
    return NULL;

  cafont = g_object_new (PANGO_TYPE_CAIRO_CORE_TEXT_FONT, NULL);
  cfont = PANGO_CORE_TEXT_FONT (cafont);

  cafont->abs_size = vogue_core_text_font_key_get_size (key);

  _vogue_core_text_font_set_ctfont (cfont, font_ref);

  if (synthesize_italic)
    cairo_matrix_init (&font_matrix,
                       1, 0,
                       -0.25, 1,
                       0, 0);
  else
    cairo_matrix_init_identity (&font_matrix);
 
  cairo_matrix_scale (&font_matrix, size, size);

  _vogue_cairo_font_private_initialize (&cafont->cf_priv,
					(VogueCairoFont *) cafont,
                                        vogue_core_text_font_key_get_gravity (key),
                                        vogue_core_text_font_key_get_context_key (key),
                                        vogue_core_text_font_key_get_matrix (key),
					&font_matrix);

  return cfont;
}
