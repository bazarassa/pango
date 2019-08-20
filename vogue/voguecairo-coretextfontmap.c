/* Vogue
 * voguecairo-coretextfontmap.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "voguecoretext-private.h"
#include "voguecairo.h"
#include "voguecairo-private.h"
#include "voguecairo-coretext.h"

typedef struct _VogueCairoCoreTextFontMapClass VogueCairoCoreTextFontMapClass;

struct _VogueCairoCoreTextFontMapClass
{
  VogueCoreTextFontMapClass parent_class;
};

static guint
vogue_cairo_core_text_font_map_get_serial (VogueFontMap *fontmap)
{
  VogueCairoCoreTextFontMap *cafontmap = PANGO_CAIRO_CORE_TEXT_FONT_MAP (fontmap);

  return cafontmap->serial;
}

static void
vogue_cairo_core_text_font_map_changed (VogueFontMap *fontmap)
{
  VogueCairoCoreTextFontMap *cafontmap = PANGO_CAIRO_CORE_TEXT_FONT_MAP (fontmap);

  cafontmap->serial++;
  if (cafontmap->serial == 0)
    cafontmap->serial++;
}

static void
vogue_cairo_core_text_font_map_set_resolution (VogueCairoFontMap *cfontmap,
                                               double             dpi)
{
  VogueCairoCoreTextFontMap *cafontmap = PANGO_CAIRO_CORE_TEXT_FONT_MAP (cfontmap);

  cafontmap->serial++;
  if (cafontmap->serial == 0)
    cafontmap->serial++;
  cafontmap->dpi = dpi;
}

static double
vogue_cairo_core_text_font_map_get_resolution_cairo (VogueCairoFontMap *cfontmap)
{
  VogueCairoCoreTextFontMap *cafontmap = PANGO_CAIRO_CORE_TEXT_FONT_MAP (cfontmap);

  return cafontmap->dpi;
}

static cairo_font_type_t
vogue_cairo_core_text_font_map_get_font_type (VogueCairoFontMap *cfontmap)
{
  return CAIRO_FONT_TYPE_QUARTZ;
}

static void
cairo_font_map_iface_init (VogueCairoFontMapIface *iface)
{
  iface->set_resolution = vogue_cairo_core_text_font_map_set_resolution;
  iface->get_resolution = vogue_cairo_core_text_font_map_get_resolution_cairo;
  iface->get_font_type  = vogue_cairo_core_text_font_map_get_font_type;
}

G_DEFINE_TYPE_WITH_CODE (VogueCairoCoreTextFontMap, vogue_cairo_core_text_font_map, PANGO_TYPE_CORE_TEXT_FONT_MAP,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT_MAP, cairo_font_map_iface_init) });


static VogueCoreTextFont *
vogue_cairo_core_text_font_map_create_font (VogueCoreTextFontMap       *fontmap,
                                            VogueCoreTextFontKey       *key)

{
  return _vogue_cairo_core_text_font_new (PANGO_CAIRO_CORE_TEXT_FONT_MAP (fontmap),
                                          key);
}

static void
vogue_cairo_core_text_font_map_finalize (GObject *object)
{
  G_OBJECT_CLASS (vogue_cairo_core_text_font_map_parent_class)->finalize (object);
}

static double
vogue_cairo_core_text_font_map_get_resolution_core_text (VogueCoreTextFontMap *ctfontmap,
                                                         VogueContext         *context)
{
  VogueCairoCoreTextFontMap *cafontmap = PANGO_CAIRO_CORE_TEXT_FONT_MAP (ctfontmap);
  double dpi;

  if (context)
    {
      dpi = vogue_cairo_context_get_resolution (context);

      if (dpi <= 0)
        dpi = cafontmap->dpi;
    }
  else
    dpi = cafontmap->dpi;

  return dpi;
}

static gconstpointer
vogue_cairo_core_text_font_map_context_key_get (VogueCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                VogueContext         *context)
{
  return _vogue_cairo_context_get_merged_font_options (context);
}

static gpointer
vogue_cairo_core_text_font_map_context_key_copy (VogueCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                 gconstpointer         key)
{
  return cairo_font_options_copy (key);
}

static void
vogue_cairo_core_text_font_map_context_key_free (VogueCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                 gpointer              key)
{
  cairo_font_options_destroy (key);
}

static guint32
vogue_cairo_core_text_font_map_context_key_hash (VogueCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                 gconstpointer         key)
{
  return (guint32)cairo_font_options_hash (key);
}

static gboolean
vogue_cairo_core_text_font_map_context_key_equal (VogueCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                  gconstpointer         key_a,
                                                  gconstpointer         key_b)
{
  return cairo_font_options_equal (key_a, key_b);
}

static void
vogue_cairo_core_text_font_map_class_init (VogueCairoCoreTextFontMapClass *class)
{
  VogueCoreTextFontMapClass *ctfontmapclass = (VogueCoreTextFontMapClass *)class;
  VogueFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  GObjectClass *object_class = (GObjectClass *)class;

  object_class->finalize = vogue_cairo_core_text_font_map_finalize;

  fontmap_class->get_serial = vogue_cairo_core_text_font_map_get_serial;
  fontmap_class->changed = vogue_cairo_core_text_font_map_changed;

  ctfontmapclass->get_resolution = vogue_cairo_core_text_font_map_get_resolution_core_text;
  ctfontmapclass->create_font = vogue_cairo_core_text_font_map_create_font;
  ctfontmapclass->context_key_get = vogue_cairo_core_text_font_map_context_key_get;
  ctfontmapclass->context_key_copy = vogue_cairo_core_text_font_map_context_key_copy;
  ctfontmapclass->context_key_free = vogue_cairo_core_text_font_map_context_key_free;
  ctfontmapclass->context_key_hash = vogue_cairo_core_text_font_map_context_key_hash;
  ctfontmapclass->context_key_equal = vogue_cairo_core_text_font_map_context_key_equal;
}

static void
vogue_cairo_core_text_font_map_init (VogueCairoCoreTextFontMap *cafontmap)
{
  cafontmap->serial = 1;
  cafontmap->dpi = 96.;
}
