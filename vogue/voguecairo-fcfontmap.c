/* Vogue
 * voguecairo-fontmap.c: Cairo font handling, fontconfig backend
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

/* Freetype has undefined macros in its headers */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include <cairo-ft.h>
#pragma GCC diagnostic pop

#include "voguefc-fontmap-private.h"
#include "voguecairo.h"
#include "voguecairo-private.h"
#include "voguecairo-fc-private.h"

typedef struct _VogueCairoFcFontMapClass VogueCairoFcFontMapClass;

struct _VogueCairoFcFontMapClass
{
  VogueFcFontMapClass parent_class;
};

static guint
vogue_cairo_fc_font_map_get_serial (VogueFontMap *fontmap)
{
  VogueCairoFcFontMap *cffontmap = (VogueCairoFcFontMap *) (fontmap);

  return cffontmap->serial;
}

static void
vogue_cairo_fc_font_map_changed (VogueFontMap *fontmap)
{
  VogueCairoFcFontMap *cffontmap = (VogueCairoFcFontMap *) (fontmap);

  cffontmap->serial++;
  if (cffontmap->serial == 0)
    cffontmap->serial++;
}

static void
vogue_cairo_fc_font_map_set_resolution (VogueCairoFontMap *cfontmap,
					double             dpi)
{
  VogueCairoFcFontMap *cffontmap = (VogueCairoFcFontMap *) (cfontmap);

  if (dpi != cffontmap->dpi)
    {
      cffontmap->serial++;
      if (cffontmap->serial == 0)
	cffontmap->serial++;
      cffontmap->dpi = dpi;

      vogue_fc_font_map_cache_clear ((VogueFcFontMap *) (cfontmap));
    }
}

static double
vogue_cairo_fc_font_map_get_resolution_cairo (VogueCairoFontMap *cfontmap)
{
  VogueCairoFcFontMap *cffontmap = (VogueCairoFcFontMap *) (cfontmap);

  return cffontmap->dpi;
}

static cairo_font_type_t
vogue_cairo_fc_font_map_get_font_type (VogueCairoFontMap *cfontmap G_GNUC_UNUSED)
{
  return CAIRO_FONT_TYPE_FT;
}

static void
cairo_font_map_iface_init (VogueCairoFontMapIface *iface)
{
  iface->set_resolution = vogue_cairo_fc_font_map_set_resolution;
  iface->get_resolution = vogue_cairo_fc_font_map_get_resolution_cairo;
  iface->get_font_type  = vogue_cairo_fc_font_map_get_font_type;
}

G_DEFINE_TYPE_WITH_CODE (VogueCairoFcFontMap, vogue_cairo_fc_font_map, PANGO_TYPE_FC_FONT_MAP,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT_MAP, cairo_font_map_iface_init) })

static void
vogue_cairo_fc_font_map_fontset_key_substitute (VogueFcFontMap    *fcfontmap G_GNUC_UNUSED,
						VogueFcFontsetKey *fontkey,
						FcPattern         *pattern)
{
  FcConfigSubstitute (NULL, pattern, FcMatchPattern);

  if (fontkey)
    cairo_ft_font_options_substitute (vogue_fc_fontset_key_get_context_key (fontkey),
				      pattern);

  FcDefaultSubstitute (pattern);
}

static double
vogue_cairo_fc_font_map_get_resolution_fc (VogueFcFontMap *fcfontmap,
					   VogueContext   *context)
{
  VogueCairoFcFontMap *cffontmap = (VogueCairoFcFontMap *) (fcfontmap);
  double dpi;

  if (context)
    {
      dpi = vogue_cairo_context_get_resolution (context);

      if (dpi <= 0)
	dpi = cffontmap->dpi;
    }
  else
    dpi = cffontmap->dpi;

  return dpi;
}

static gconstpointer
vogue_cairo_fc_font_map_context_key_get (VogueFcFontMap *fcfontmap G_GNUC_UNUSED,
					 VogueContext   *context)
{
  return _vogue_cairo_context_get_merged_font_options (context);
}

static gpointer
vogue_cairo_fc_font_map_context_key_copy (VogueFcFontMap *fcfontmap G_GNUC_UNUSED,
					  gconstpointer   key)
{
  return cairo_font_options_copy (key);
}

static void
vogue_cairo_fc_font_map_context_key_free (VogueFcFontMap *fcfontmap G_GNUC_UNUSED,
					  gpointer        key)
{
  cairo_font_options_destroy (key);
}


static guint32
vogue_cairo_fc_font_map_context_key_hash (VogueFcFontMap *fcfontmap G_GNUC_UNUSED,
					  gconstpointer        key)
{
  return (guint32)cairo_font_options_hash (key);
}

static gboolean
vogue_cairo_fc_font_map_context_key_equal (VogueFcFontMap *fcfontmap G_GNUC_UNUSED,
					   gconstpointer   key_a,
					   gconstpointer   key_b)
{
  return cairo_font_options_equal (key_a, key_b);
}

static VogueFcFont *
vogue_cairo_fc_font_map_create_font (VogueFcFontMap *fcfontmap,
				     VogueFcFontKey *key)
{
  return _vogue_cairo_fc_font_new ((VogueCairoFcFontMap *) (fcfontmap),
				   key);
}

static void
vogue_cairo_fc_font_map_class_init (VogueCairoFcFontMapClass *class)
{
  VogueFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  VogueFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (class);

  fontmap_class->get_serial = vogue_cairo_fc_font_map_get_serial;
  fontmap_class->changed = vogue_cairo_fc_font_map_changed;

  fcfontmap_class->fontset_key_substitute = vogue_cairo_fc_font_map_fontset_key_substitute;
  fcfontmap_class->get_resolution = vogue_cairo_fc_font_map_get_resolution_fc;

  fcfontmap_class->context_key_get = vogue_cairo_fc_font_map_context_key_get;
  fcfontmap_class->context_key_copy = vogue_cairo_fc_font_map_context_key_copy;
  fcfontmap_class->context_key_free = vogue_cairo_fc_font_map_context_key_free;
  fcfontmap_class->context_key_hash = vogue_cairo_fc_font_map_context_key_hash;
  fcfontmap_class->context_key_equal = vogue_cairo_fc_font_map_context_key_equal;

  fcfontmap_class->create_font = vogue_cairo_fc_font_map_create_font;
}

static void
vogue_cairo_fc_font_map_init (VogueCairoFcFontMap *cffontmap)
{
  cffontmap->serial = 1;
  cffontmap->dpi   = 96.0;
}
