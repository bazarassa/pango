/* Vogue
 * voguecairo-win32fontmap.c: Cairo font handling, Win32 backend
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

#include "voguewin32-private.h"
#include "voguecairo.h"
#include "voguecairo-private.h"
#include "voguecairo-win32.h"

typedef struct _VogueCairoWin32FontMapClass VogueCairoWin32FontMapClass;

struct _VogueCairoWin32FontMapClass
{
  VogueWin32FontMapClass parent_class;
};

static guint
vogue_cairo_win32_font_map_get_serial (VogueFontMap *fontmap)
{
  VogueCairoWin32FontMap *cwfontmap = PANGO_CAIRO_WIN32_FONT_MAP (fontmap);

  return cwfontmap->serial;
}

static void
vogue_cairo_win32_font_map_changed (VogueFontMap *fontmap)
{
  VogueCairoWin32FontMap *cwfontmap = PANGO_CAIRO_WIN32_FONT_MAP (fontmap);

  cwfontmap->serial++;
  if (cwfontmap->serial == 0)
    cwfontmap->serial++;
}

static void
vogue_cairo_win32_font_map_set_resolution (VogueCairoFontMap *cfontmap,
					   double             dpi)
{
  VogueCairoWin32FontMap *cwfontmap = PANGO_CAIRO_WIN32_FONT_MAP (cfontmap);

  cwfontmap->serial++;
  if (cwfontmap->serial == 0)
    cwfontmap->serial++;
  cwfontmap->dpi = dpi;
}

static double
vogue_cairo_win32_font_map_get_resolution (VogueCairoFontMap *cfontmap)
{
  VogueCairoWin32FontMap *cwfontmap = PANGO_CAIRO_WIN32_FONT_MAP (cfontmap);

  return cwfontmap->dpi;
}

static cairo_font_type_t
vogue_cairo_win32_font_map_get_font_type (VogueCairoFontMap *cfontmap)
{
  return CAIRO_FONT_TYPE_WIN32;
}

static void
cairo_font_map_iface_init (VogueCairoFontMapIface *iface)
{
  iface->set_resolution = vogue_cairo_win32_font_map_set_resolution;
  iface->get_resolution = vogue_cairo_win32_font_map_get_resolution;
  iface->get_font_type  = vogue_cairo_win32_font_map_get_font_type;
}

G_DEFINE_TYPE_WITH_CODE (VogueCairoWin32FontMap, vogue_cairo_win32_font_map, PANGO_TYPE_WIN32_FONT_MAP,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT_MAP, cairo_font_map_iface_init) });

static void
vogue_cairo_win32_font_map_finalize (GObject *object)
{
  G_OBJECT_CLASS (vogue_cairo_win32_font_map_parent_class)->finalize (object);
}

static VogueFont *
vogue_cairo_win32_font_map_find_font (VogueWin32FontMap          *fontmap,
				      VogueContext               *context,
				      VogueWin32Face             *face,
				      const VogueFontDescription *desc)
{
  return _vogue_cairo_win32_font_new (PANGO_CAIRO_WIN32_FONT_MAP (fontmap),
				      context, face, desc);
}

static void
vogue_cairo_win32_font_map_class_init (VogueCairoWin32FontMapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  VogueFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  VogueWin32FontMapClass *win32fontmap_class = PANGO_WIN32_FONT_MAP_CLASS (class);

  gobject_class->finalize  = vogue_cairo_win32_font_map_finalize;
  fontmap_class->get_serial = vogue_cairo_win32_font_map_get_serial;
  fontmap_class->changed = vogue_cairo_win32_font_map_changed;
  win32fontmap_class->find_font = vogue_cairo_win32_font_map_find_font;
}

static void
vogue_cairo_win32_font_map_init (VogueCairoWin32FontMap *cwfontmap)
{
  cwfontmap->serial = 1;
  cwfontmap->dpi = GetDeviceCaps (vogue_win32_get_dc (), LOGPIXELSY);
}
