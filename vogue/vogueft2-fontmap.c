/* Vogue
 * vogueft2-fontmap.c:
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fontconfig/fontconfig.h>

#include "vogue-impl-utils.h"
#include "vogueft2-private.h"
#include "voguefc-fontmap.h"

typedef struct _VogueFT2Family       VogueFT2Family;
typedef struct _VogueFT2FontMapClass VogueFT2FontMapClass;

/**
 * VogueFT2FontMap:
 *
 * The #VogueFT2FontMap is the #VogueFontMap implementation for FreeType fonts.
 */
struct _VogueFT2FontMap
{
  VogueFcFontMap parent_instance;

  FT_Library library;

  guint serial;
  double dpi_x;
  double dpi_y;

  /* Function to call on prepared patterns to do final
   * config tweaking.
   */
  VogueFT2SubstituteFunc substitute_func;
  gpointer substitute_data;
  GDestroyNotify substitute_destroy;

  VogueRenderer *renderer;
};

struct _VogueFT2FontMapClass
{
  VogueFcFontMapClass parent_class;
};

static void          vogue_ft2_font_map_finalize            (GObject              *object);
static VogueFcFont * vogue_ft2_font_map_new_font            (VogueFcFontMap       *fcfontmap,
							     FcPattern            *pattern);
static double        vogue_ft2_font_map_get_resolution      (VogueFcFontMap       *fcfontmap,
							     VogueContext         *context);
static guint         vogue_ft2_font_map_get_serial          (VogueFontMap         *fontmap);
static void          vogue_ft2_font_map_changed             (VogueFontMap         *fontmap);

static VogueFT2FontMap *vogue_ft2_global_fontmap = NULL; /* MT-safe */

G_DEFINE_TYPE (VogueFT2FontMap, vogue_ft2_font_map, PANGO_TYPE_FC_FONT_MAP)

static void
vogue_ft2_font_map_class_init (VogueFT2FontMapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  VogueFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  VogueFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (class);

  gobject_class->finalize = vogue_ft2_font_map_finalize;
  fontmap_class->get_serial = vogue_ft2_font_map_get_serial;
  fontmap_class->changed = vogue_ft2_font_map_changed;
  fcfontmap_class->default_substitute = _vogue_ft2_font_map_default_substitute;
  fcfontmap_class->new_font = vogue_ft2_font_map_new_font;
  fcfontmap_class->get_resolution = vogue_ft2_font_map_get_resolution;
}

static void
vogue_ft2_font_map_init (VogueFT2FontMap *fontmap)
{
  FT_Error error;

  fontmap->serial = 1;
  fontmap->library = NULL;
  fontmap->dpi_x   = 72.0;
  fontmap->dpi_y   = 72.0;

  error = FT_Init_FreeType (&fontmap->library);
  if (error != FT_Err_Ok)
    g_critical ("vogue_ft2_font_map_init: Could not initialize freetype");
}

static void
vogue_ft2_font_map_finalize (GObject *object)
{
  VogueFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (object);

  if (ft2fontmap->renderer)
    g_object_unref (ft2fontmap->renderer);

  if (ft2fontmap->substitute_destroy)
    ft2fontmap->substitute_destroy (ft2fontmap->substitute_data);

  G_OBJECT_CLASS (vogue_ft2_font_map_parent_class)->finalize (object);

  FT_Done_FreeType (ft2fontmap->library);
}

/**
 * vogue_ft2_font_map_new:
 *
 * Create a new #VogueFT2FontMap object; a fontmap is used
 * to cache information about available fonts, and holds
 * certain global parameters such as the resolution and
 * the default substitute function (see
 * vogue_ft2_font_map_set_default_substitute()).
 *
 * Return value: the newly created fontmap object. Unref
 * with g_object_unref() when you are finished with it.
 *
 * Since: 1.2
 **/
VogueFontMap *
vogue_ft2_font_map_new (void)
{
  return (VogueFontMap *) g_object_new (PANGO_TYPE_FT2_FONT_MAP, NULL);
}

static guint
vogue_ft2_font_map_get_serial (VogueFontMap *fontmap)
{
  VogueFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  return ft2fontmap->serial;
}

static void
vogue_ft2_font_map_changed (VogueFontMap *fontmap)
{
  VogueFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  ft2fontmap->serial++;
  if (ft2fontmap->serial == 0)
    ft2fontmap->serial++;
}

/**
 * vogue_ft2_font_map_set_default_substitute:
 * @fontmap: a #VogueFT2FontMap
 * @func: function to call to to do final config tweaking
 *        on #FcPattern objects.
 * @data: data to pass to @func
 * @notify: function to call when @data is no longer used.
 *
 * Sets a function that will be called to do final configuration
 * substitution on a #FcPattern before it is used to load
 * the font. This function can be used to do things like set
 * hinting and antialiasing options.
 *
 * Since: 1.2
 **/
void
vogue_ft2_font_map_set_default_substitute (VogueFT2FontMap        *fontmap,
					   VogueFT2SubstituteFunc  func,
					   gpointer                data,
					   GDestroyNotify          notify)
{
  fontmap->serial++;
  if (fontmap->serial == 0)
    fontmap->serial++;

  if (fontmap->substitute_destroy)
    fontmap->substitute_destroy (fontmap->substitute_data);

  fontmap->substitute_func = func;
  fontmap->substitute_data = data;
  fontmap->substitute_destroy = notify;

  vogue_fc_font_map_cache_clear (PANGO_FC_FONT_MAP (fontmap));
}

/**
 * vogue_ft2_font_map_substitute_changed:
 * @fontmap: a #VogueFT2FontMap
 *
 * Call this function any time the results of the
 * default substitution function set with
 * vogue_ft2_font_map_set_default_substitute() change.
 * That is, if your substitution function will return different
 * results for the same input pattern, you must call this function.
 *
 * Since: 1.2
 **/
void
vogue_ft2_font_map_substitute_changed (VogueFT2FontMap *fontmap)
{
  fontmap->serial++;
  if (fontmap->serial == 0)
    fontmap->serial++;
  vogue_fc_font_map_cache_clear (PANGO_FC_FONT_MAP (fontmap));
}

/**
 * vogue_ft2_font_map_set_resolution:
 * @fontmap: a #VogueFT2FontMap
 * @dpi_x: dots per inch in the X direction
 * @dpi_y: dots per inch in the Y direction
 *
 * Sets the horizontal and vertical resolutions for the fontmap.
 *
 * Since: 1.2
 **/
void
vogue_ft2_font_map_set_resolution (VogueFT2FontMap *fontmap,
				   double           dpi_x,
				   double           dpi_y)
{
  g_return_if_fail (PANGO_FT2_IS_FONT_MAP (fontmap));

  fontmap->dpi_x = dpi_x;
  fontmap->dpi_y = dpi_y;

  vogue_ft2_font_map_substitute_changed (fontmap);
}

/**
 * vogue_ft2_font_map_create_context: (skip)
 * @fontmap: a #VogueFT2FontMap
 *
 * Create a #VogueContext for the given fontmap.
 *
 * Return value: (transfer full): the newly created context; free with
 *     g_object_unref().
 *
 * Since: 1.2
 *
 * Deprecated: 1.22: Use vogue_font_map_create_context() instead.
 **/
VogueContext *
vogue_ft2_font_map_create_context (VogueFT2FontMap *fontmap)
{
  g_return_val_if_fail (PANGO_FT2_IS_FONT_MAP (fontmap), NULL);

  return vogue_font_map_create_context (PANGO_FONT_MAP (fontmap));
}

/**
 * vogue_ft2_get_context: (skip)
 * @dpi_x:  the horizontal DPI of the target device
 * @dpi_y:  the vertical DPI of the target device
 *
 * Retrieves a #VogueContext for the default VogueFT2 fontmap
 * (see vogue_ft2_font_map_for_display()) and sets the resolution
 * for the default fontmap to @dpi_x by @dpi_y.
 *
 * Return value: (transfer full): the new #VogueContext
 *
 * Deprecated: 1.22: Use vogue_font_map_create_context() instead.
 **/
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
VogueContext *
vogue_ft2_get_context (double dpi_x, double dpi_y)
{
  VogueFontMap *fontmap;

  fontmap = vogue_ft2_font_map_for_display ();
  vogue_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (fontmap), dpi_x, dpi_y);

  return vogue_font_map_create_context (fontmap);
}
G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * vogue_ft2_font_map_for_display: (skip)
 *
 * Returns a #VogueFT2FontMap. This font map is cached and should
 * not be freed. If the font map is no longer needed, it can
 * be released with vogue_ft2_shutdown_display(). Use of the
 * global VogueFT2 fontmap is deprecated; use vogue_ft2_font_map_new()
 * instead.
 *
 * Return value: (transfer none): a #VogueFT2FontMap.
 **/
VogueFontMap *
vogue_ft2_font_map_for_display (void)
{
  if (g_once_init_enter (&vogue_ft2_global_fontmap))
    g_once_init_leave (&vogue_ft2_global_fontmap, PANGO_FT2_FONT_MAP (vogue_ft2_font_map_new ()));

  return PANGO_FONT_MAP (vogue_ft2_global_fontmap);
}

/**
 * vogue_ft2_shutdown_display:
 *
 * Free the global fontmap. (See vogue_ft2_font_map_for_display())
 * Use of the global VogueFT2 fontmap is deprecated.
 **/
void
vogue_ft2_shutdown_display (void)
{
  if (vogue_ft2_global_fontmap)
    {
      vogue_fc_font_map_cache_clear (PANGO_FC_FONT_MAP (vogue_ft2_global_fontmap));

      g_object_unref (vogue_ft2_global_fontmap);

      vogue_ft2_global_fontmap = NULL;
    }
}

FT_Library
_vogue_ft2_font_map_get_library (VogueFontMap *fontmap)
{
  VogueFT2FontMap *ft2fontmap = (VogueFT2FontMap *)fontmap;

  return ft2fontmap->library;
}


/**
 * _vogue_ft2_font_map_get_renderer:
 * @fontmap: a #VogueFT2FontMap
 *
 * Gets the singleton VogueFT2Renderer for this fontmap.
 *
 * Return value: the renderer.
 **/
VogueRenderer *
_vogue_ft2_font_map_get_renderer (VogueFT2FontMap *ft2fontmap)
{
  if (!ft2fontmap->renderer)
    ft2fontmap->renderer = g_object_new (PANGO_TYPE_FT2_RENDERER, NULL);

  return ft2fontmap->renderer;
}

void
_vogue_ft2_font_map_default_substitute (VogueFcFontMap *fcfontmap,
				       FcPattern      *pattern)
{
  VogueFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fcfontmap);
  FcValue v;

  FcConfigSubstitute (NULL, pattern, FcMatchPattern);

  if (ft2fontmap->substitute_func)
    ft2fontmap->substitute_func (pattern, ft2fontmap->substitute_data);

  if (FcPatternGet (pattern, FC_DPI, 0, &v) == FcResultNoMatch)
    FcPatternAddDouble (pattern, FC_DPI, ft2fontmap->dpi_y);
  FcDefaultSubstitute (pattern);
}

static double
vogue_ft2_font_map_get_resolution (VogueFcFontMap       *fcfontmap,
				   VogueContext         *context G_GNUC_UNUSED)
{
  return ((VogueFT2FontMap *)fcfontmap)->dpi_y;
}

static VogueFcFont *
vogue_ft2_font_map_new_font (VogueFcFontMap  *fcfontmap,
			     FcPattern       *pattern)
{
  return (VogueFcFont *)_vogue_ft2_font_new (PANGO_FT2_FONT_MAP (fcfontmap), pattern);
}
