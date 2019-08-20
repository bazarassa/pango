/* Vogue
 * voguexft-fontmap.c: Xft font handling
 *
 * Copyright (C) 2000-2003 Red Hat Software
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
#include <stdlib.h>
#include <string.h>

#include "voguefc-fontmap-private.h"
#include "voguexft.h"
#include "voguexft-private.h"

/* For XExtSetCloseDisplay */
#include <X11/Xlibint.h>

typedef struct _VogueXftFamily       VogueXftFamily;
typedef struct _VogueXftFontMapClass VogueXftFontMapClass;

#define PANGO_TYPE_XFT_FONT_MAP              (vogue_xft_font_map_get_type ())
#define PANGO_XFT_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT_MAP, VogueXftFontMap))
#define PANGO_XFT_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT_MAP))

struct _VogueXftFontMap
{
  VogueFcFontMap parent_instance;

  guint serial;

  Display *display;
  int screen;

  /* Function to call on prepared patterns to do final
   * config tweaking.
   */
  VogueXftSubstituteFunc substitute_func;
  gpointer substitute_data;
  GDestroyNotify substitute_destroy;

  VogueRenderer *renderer;
};

struct _VogueXftFontMapClass
{
  VogueFcFontMapClass parent_class;
};

static guint         vogue_xft_font_map_get_serial         (VogueFontMap         *fontmap);
static void          vogue_xft_font_map_changed            (VogueFontMap         *fontmap);
static void          vogue_xft_font_map_default_substitute (VogueFcFontMap       *fcfontmap,
							    FcPattern            *pattern);
static VogueFcFont * vogue_xft_font_map_new_font           (VogueFcFontMap       *fcfontmap,
							    FcPattern            *pattern);
static void          vogue_xft_font_map_finalize           (GObject              *object);

G_LOCK_DEFINE_STATIC (fontmaps);
static GSList *fontmaps = NULL; /* MT-safe */

G_DEFINE_TYPE (VogueXftFontMap, vogue_xft_font_map, PANGO_TYPE_FC_FONT_MAP)

static void
vogue_xft_font_map_class_init (VogueXftFontMapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  VogueFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  VogueFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (class);

  gobject_class->finalize  = vogue_xft_font_map_finalize;

  fontmap_class->get_serial = vogue_xft_font_map_get_serial;
  fontmap_class->changed = vogue_xft_font_map_changed;

  fcfontmap_class->default_substitute = vogue_xft_font_map_default_substitute;
  fcfontmap_class->new_font = vogue_xft_font_map_new_font;
}

static void
vogue_xft_font_map_init (VogueXftFontMap *xftfontmap)
{
  xftfontmap->serial = 1;
}

static void
vogue_xft_font_map_finalize (GObject *object)
{
  VogueXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (object);

  if (xftfontmap->renderer)
    g_object_unref (xftfontmap->renderer);

  G_LOCK (fontmaps);
  fontmaps = g_slist_remove (fontmaps, object);
  G_UNLOCK (fontmaps);

  if (xftfontmap->substitute_destroy)
    xftfontmap->substitute_destroy (xftfontmap->substitute_data);

  G_OBJECT_CLASS (vogue_xft_font_map_parent_class)->finalize (object);
}


static guint
vogue_xft_font_map_get_serial (VogueFontMap *fontmap)
{
  VogueXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fontmap);

  return xftfontmap->serial;
}

static void
vogue_xft_font_map_changed (VogueFontMap *fontmap)
{
  VogueXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fontmap);

  xftfontmap->serial++;
  if (xftfontmap->serial == 0)
    xftfontmap->serial++;
}

static VogueFontMap *
vogue_xft_find_font_map (Display *display,
			 int      screen)
{
  GSList *tmp_list;

  G_LOCK (fontmaps);
  tmp_list = fontmaps;
  while (tmp_list)
    {
      VogueXftFontMap *xftfontmap = tmp_list->data;

      if (xftfontmap->display == display &&
	  xftfontmap->screen == screen)
        {
          G_UNLOCK (fontmaps);
	  return PANGO_FONT_MAP (xftfontmap);
        }

      tmp_list = tmp_list->next;
    }
  G_UNLOCK (fontmaps);

  return NULL;
}

/*
 * Hackery to set up notification when a Display is closed
 */
static GSList *registered_displays; /* MT-safe, protected by fontmaps lock */

static int
close_display_cb (Display   *display,
		  XExtCodes *extcodes G_GNUC_UNUSED)
{
  GSList *tmp_list;

  G_LOCK (fontmaps);
  tmp_list = g_slist_copy (fontmaps);
  G_UNLOCK (fontmaps);

  while (tmp_list)
    {
      VogueXftFontMap *xftfontmap = tmp_list->data;
      tmp_list = tmp_list->next;

      if (xftfontmap->display == display)
	vogue_xft_shutdown_display (display, xftfontmap->screen);
    }

  g_slist_free (tmp_list);

  registered_displays = g_slist_remove (registered_displays, display);

  return 0;
}

static void
register_display (Display *display)
{
  XExtCodes *extcodes;
  GSList *tmp_list;

  for (tmp_list = registered_displays; tmp_list; tmp_list = tmp_list->next)
    {
      if (tmp_list->data == display)
	return;
    }

  registered_displays = g_slist_prepend (registered_displays, display);

  extcodes = XAddExtension (display);
  XESetCloseDisplay (display, extcodes->extension, close_display_cb);
}

/**
 * vogue_xft_get_font_map:
 * @display: an X display
 * @screen: the screen number of a screen within @display
 *
 * Returns the #VogueXftFontMap for the given display and screen.
 * The fontmap is owned by Vogue and will be valid until
 * the display is closed.
 *
 * Return value: (transfer none): a #VogueFontMap object, owned by Vogue.
 *
 * Since: 1.2
 **/
VogueFontMap *
vogue_xft_get_font_map (Display *display,
			int      screen)
{
  VogueFontMap *fontmap;
  VogueXftFontMap *xftfontmap;

  g_return_val_if_fail (display != NULL, NULL);

  fontmap = vogue_xft_find_font_map (display, screen);
  if (fontmap)
    return fontmap;

  xftfontmap = (VogueXftFontMap *)g_object_new (PANGO_TYPE_XFT_FONT_MAP, NULL);

  xftfontmap->display = display;
  xftfontmap->screen = screen;

  G_LOCK (fontmaps);

  register_display (display);

  fontmaps = g_slist_prepend (fontmaps, xftfontmap);

  G_UNLOCK (fontmaps);

  return PANGO_FONT_MAP (xftfontmap);
}

/**
 * vogue_xft_shutdown_display:
 * @display: an X display
 * @screen: the screen number of a screen within @display
 *
 * Release any resources that have been cached for the
 * combination of @display and @screen. Note that when the
 * X display is closed, resources are released automatically,
 * without needing to call this function.
 *
 * Since: 1.2
 **/
void
vogue_xft_shutdown_display (Display *display,
			    int      screen)
{
  VogueFontMap *fontmap;

  fontmap = vogue_xft_find_font_map (display, screen);
  if (fontmap)
    {
      VogueXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fontmap);

      G_LOCK (fontmaps);
      fontmaps = g_slist_remove (fontmaps, fontmap);
      G_UNLOCK (fontmaps);
      vogue_fc_font_map_shutdown (PANGO_FC_FONT_MAP (fontmap));

      xftfontmap->display = NULL;
      g_object_unref (fontmap);
    }
}

/**
 * vogue_xft_set_default_substitute:
 * @display: an X Display
 * @screen: the screen number of a screen within @display
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
vogue_xft_set_default_substitute (Display                *display,
				  int                     screen,
				  VogueXftSubstituteFunc  func,
				  gpointer                data,
				  GDestroyNotify          notify)
{
  VogueXftFontMap *xftfontmap = (VogueXftFontMap *)vogue_xft_get_font_map (display, screen);

  xftfontmap->serial++;
  if (xftfontmap->serial == 0)
    xftfontmap->serial++;

  if (xftfontmap->substitute_destroy)
    xftfontmap->substitute_destroy (xftfontmap->substitute_data);

  xftfontmap->substitute_func = func;
  xftfontmap->substitute_data = data;
  xftfontmap->substitute_destroy = notify;

  vogue_fc_font_map_cache_clear (PANGO_FC_FONT_MAP (xftfontmap));
}

/**
 * vogue_xft_substitute_changed:
 * @display: an X Display
 * @screen: the screen number of a screen within @display
 *
 * Call this function any time the results of the
 * default substitution function set with
 * vogue_xft_set_default_substitute() change.
 * That is, if your substitution function will return different
 * results for the same input pattern, you must call this function.
 *
 * Since: 1.2
 **/
void
vogue_xft_substitute_changed (Display *display,
			      int      screen)
{
  VogueXftFontMap *xftfontmap = (VogueXftFontMap *)vogue_xft_get_font_map (display, screen);

  xftfontmap->serial++;
  if (xftfontmap->serial == 0)
    xftfontmap->serial++;
  vogue_fc_font_map_cache_clear (PANGO_FC_FONT_MAP (xftfontmap));
}

void
_vogue_xft_font_map_get_info (VogueFontMap *fontmap,
			      Display     **display,
			      int          *screen)
{
  VogueXftFontMap *xftfontmap = (VogueXftFontMap *)fontmap;

  if (display)
    *display = xftfontmap->display;
  if (screen)
    *screen = xftfontmap->screen;
}

/**
 * vogue_xft_get_context: (skip)
 * @display: an X display.
 * @screen: an X screen.
 *
 * Retrieves a #VogueContext appropriate for rendering with
 * Xft fonts on the given screen of the given display.
 *
 * Return value: the new #VogueContext.
 *
 * Deprecated: 1.22: Use vogue_xft_get_font_map() followed by
 * vogue_font_map_create_context() instead.
 **/
VogueContext *
vogue_xft_get_context (Display *display,
		       int      screen)
{
  g_return_val_if_fail (display != NULL, NULL);

  return vogue_font_map_create_context (vogue_xft_get_font_map (display, screen));
}

/**
 * _vogue_xft_font_map_get_renderer:
 * @fontmap: a #VogueXftFontMap
 *
 * Gets the singleton #VogueXFTRenderer for this fontmap.
 *
 * Return value: the renderer.
 **/
VogueRenderer *
_vogue_xft_font_map_get_renderer (VogueXftFontMap *xftfontmap)
{
  if (!xftfontmap->renderer)
    xftfontmap->renderer = vogue_xft_renderer_new (xftfontmap->display,
						   xftfontmap->screen);

  return xftfontmap->renderer;
}

static void
vogue_xft_font_map_default_substitute (VogueFcFontMap *fcfontmap,
				       FcPattern      *pattern)
{
  VogueXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fcfontmap);
  double d;

  FcConfigSubstitute (NULL, pattern, FcMatchPattern);
  if (xftfontmap->substitute_func)
    xftfontmap->substitute_func (pattern, xftfontmap->substitute_data);
  XftDefaultSubstitute (xftfontmap->display, xftfontmap->screen, pattern);
  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch && d == 0.0)
    {
      FcValue v;
      v.type = FcTypeDouble;
      v.u.d = 1.0;
      FcPatternAdd (pattern, FC_PIXEL_SIZE, v, FcFalse);
    }
}

static VogueFcFont *
vogue_xft_font_map_new_font (VogueFcFontMap  *fcfontmap,
			     FcPattern       *pattern)
{
  return (VogueFcFont *)_vogue_xft_font_new (PANGO_XFT_FONT_MAP (fcfontmap), pattern);
}
