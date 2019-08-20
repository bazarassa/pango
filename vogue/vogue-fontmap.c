/* Vogue
 * vogue-fontmap.c: Font handling
 *
 * Copyright (C) 2000 Red Hat Software
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
#include "vogue-fontmap-private.h"
#include "vogue-fontset-private.h"
#include "vogue-impl-utils.h"
#include <stdlib.h>

static VogueFontset *vogue_font_map_real_load_fontset (VogueFontMap               *fontmap,
						       VogueContext               *context,
						       const VogueFontDescription *desc,
						       VogueLanguage              *language);


G_DEFINE_ABSTRACT_TYPE (VogueFontMap, vogue_font_map, G_TYPE_OBJECT)

static void
vogue_font_map_class_init (VogueFontMapClass *class)
{
  class->load_fontset = vogue_font_map_real_load_fontset;
}

static void
vogue_font_map_init (VogueFontMap *fontmap G_GNUC_UNUSED)
{
}

/**
 * vogue_font_map_create_context:
 * @fontmap: a #VogueFontMap
 *
 * Creates a #VogueContext connected to @fontmap.  This is equivalent
 * to vogue_context_new() followed by vogue_context_set_font_map().
 *
 * If you are using Vogue as part of a higher-level system,
 * that system may have it's own way of create a #VogueContext.
 * For instance, the GTK+ toolkit has, among others,
 * gdk_vogue_context_get_for_screen(), and
 * gtk_widget_get_vogue_context().  Use those instead.
 *
 * Return value: (transfer full): the newly allocated #VogueContext,
 *               which should be freed with g_object_unref().
 *
 * Since: 1.22
 **/
VogueContext *
vogue_font_map_create_context (VogueFontMap *fontmap)
{
  VogueContext *context;

  g_return_val_if_fail (fontmap != NULL, NULL);

  context = vogue_context_new ();
  vogue_context_set_font_map (context, fontmap);

  return context;
}

/**
 * vogue_font_map_load_font:
 * @fontmap: a #VogueFontMap
 * @context: the #VogueContext the font will be used with
 * @desc: a #VogueFontDescription describing the font to load
 *
 * Load the font in the fontmap that is the closest match for @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated #VogueFont
 *          loaded, or %NULL if no font matched.
 **/
VogueFont *
vogue_font_map_load_font  (VogueFontMap               *fontmap,
			   VogueContext               *context,
			   const VogueFontDescription *desc)
{
  g_return_val_if_fail (fontmap != NULL, NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->load_font (fontmap, context, desc);
}

/**
 * vogue_font_map_list_families:
 * @fontmap: a #VogueFontMap
 * @families: (out) (array length=n_families) (transfer container): location to store a pointer to an array of #VogueFontFamily *.
 *            This array should be freed with g_free().
 * @n_families: (out): location to store the number of elements in @families
 *
 * List all families for a fontmap.
 **/
void
vogue_font_map_list_families (VogueFontMap      *fontmap,
			      VogueFontFamily ***families,
			      int               *n_families)
{
  g_return_if_fail (fontmap != NULL);

  PANGO_FONT_MAP_GET_CLASS (fontmap)->list_families (fontmap, families, n_families);
}

/**
 * vogue_font_map_load_fontset:
 * @fontmap: a #VogueFontMap
 * @context: the #VogueContext the font will be used with
 * @desc: a #VogueFontDescription describing the font to load
 * @language: a #VogueLanguage the fonts will be used for
 *
 * Load a set of fonts in the fontmap that can be used to render
 * a font matching @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated
 *          #VogueFontset loaded, or %NULL if no font matched.
 **/
VogueFontset *
vogue_font_map_load_fontset (VogueFontMap                 *fontmap,
			     VogueContext                 *context,
			     const VogueFontDescription   *desc,
			     VogueLanguage                *language)
{
  g_return_val_if_fail (fontmap != NULL, NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->load_fontset (fontmap, context, desc, language);
}

static void
vogue_font_map_fontset_add_fonts (VogueFontMap          *fontmap,
				  VogueContext          *context,
				  VogueFontsetSimple    *fonts,
				  VogueFontDescription  *desc,
				  const char            *family)
{
  VogueFont *font;

  vogue_font_description_set_family_static (desc, family);
  font = vogue_font_map_load_font (fontmap, context, desc);
  if (font)
    vogue_fontset_simple_append (fonts, font);
}

static VogueFontset *
vogue_font_map_real_load_fontset (VogueFontMap               *fontmap,
				  VogueContext               *context,
				  const VogueFontDescription *desc,
				  VogueLanguage              *language)
{
  VogueFontDescription *tmp_desc = vogue_font_description_copy_static (desc);
  const char *family;
  char **families;
  int i;
  VogueFontsetSimple *fonts;
  static GHashTable *warned_fonts = NULL; /* MT-safe */
  G_LOCK_DEFINE_STATIC (warned_fonts);

  family = vogue_font_description_get_family (desc);
  families = g_strsplit (family ? family : "", ",", -1);

  fonts = vogue_fontset_simple_new (language);

  for (i = 0; families[i]; i++)
    vogue_font_map_fontset_add_fonts (fontmap,
				      context,
				      fonts,
				      tmp_desc,
				      families[i]);

  g_strfreev (families);

  /* The font description was completely unloadable, try with
   * family == "Sans"
   */
  if (vogue_fontset_simple_size (fonts) == 0)
    {
      char *ctmp1, *ctmp2;

      vogue_font_description_set_family_static (tmp_desc,
						vogue_font_description_get_family (desc));

      ctmp1 = vogue_font_description_to_string (desc);
      vogue_font_description_set_family_static (tmp_desc, "Sans");

      G_LOCK (warned_fonts);
      if (!warned_fonts || !g_hash_table_lookup (warned_fonts, ctmp1))
	{
	  if (!warned_fonts)
	    warned_fonts = g_hash_table_new (g_str_hash, g_str_equal);

	  g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

	  ctmp2 = vogue_font_description_to_string (tmp_desc);
	  g_warning ("couldn't load font \"%s\", falling back to \"%s\", "
		     "expect ugly output.", ctmp1, ctmp2);
	  g_free (ctmp2);
	}
      G_UNLOCK (warned_fonts);
      g_free (ctmp1);

      vogue_font_map_fontset_add_fonts (fontmap,
					context,
					fonts,
					tmp_desc,
					"Sans");
    }

  /* We couldn't try with Sans and the specified style. Try Sans Normal
   */
  if (vogue_fontset_simple_size (fonts) == 0)
    {
      char *ctmp1, *ctmp2;

      vogue_font_description_set_family_static (tmp_desc, "Sans");
      ctmp1 = vogue_font_description_to_string (tmp_desc);
      vogue_font_description_set_style (tmp_desc, PANGO_STYLE_NORMAL);
      vogue_font_description_set_weight (tmp_desc, PANGO_WEIGHT_NORMAL);
      vogue_font_description_set_variant (tmp_desc, PANGO_VARIANT_NORMAL);
      vogue_font_description_set_stretch (tmp_desc, PANGO_STRETCH_NORMAL);

      G_LOCK (warned_fonts);
      if (!warned_fonts || !g_hash_table_lookup (warned_fonts, ctmp1))
	{
	  g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

	  ctmp2 = vogue_font_description_to_string (tmp_desc);

	  g_warning ("couldn't load font \"%s\", falling back to \"%s\", "
		     "expect ugly output.", ctmp1, ctmp2);
	  g_free (ctmp2);
	}
      G_UNLOCK (warned_fonts);
      g_free (ctmp1);

      vogue_font_map_fontset_add_fonts (fontmap,
					context,
					fonts,
					tmp_desc,
					"Sans");
    }

  vogue_font_description_free (tmp_desc);

  /* Everything failed, we are screwed, there is no way to continue,
   * but lets just not crash here.
   */
  if (vogue_fontset_simple_size (fonts) == 0)
      g_warning ("All font fallbacks failed!!!!");

  return PANGO_FONTSET (fonts);
}

/**
 * vogue_font_map_get_shape_engine_type:
 * @fontmap: a #VogueFontMap
 *
 * Returns the render ID for shape engines for this fontmap.
 * See the <structfield>render_type</structfield> field of
 * #VogueEngineInfo.
  *
 * Return value: the ID string for shape engines for
 *  this fontmap. Owned by Vogue, should not be modified
 *  or freed.
 *
 * Since: 1.4
 * Deprecated: 1.38
 **/
const char *
vogue_font_map_get_shape_engine_type (VogueFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (fontmap), NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->shape_engine_type;
}

/**
 * vogue_font_map_get_serial:
 * @fontmap: a #VogueFontMap
 *
 * Returns the current serial number of @fontmap.  The serial number is
 * initialized to an small number larger than zero when a new fontmap
 * is created and is increased whenever the fontmap is changed. It may
 * wrap, but will never have the value 0. Since it can wrap, never compare
 * it with "less than", always use "not equals".
 *
 * The fontmap can only be changed using backend-specific API, like changing
 * fontmap resolution.
 *
 * This can be used to automatically detect changes to a #VogueFontMap, like
 * in #VogueContext.
 *
 * Return value: The current serial number of @fontmap.
 *
 * Since: 1.32.4
 **/
guint
vogue_font_map_get_serial (VogueFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (fontmap), 0);

  if (PANGO_FONT_MAP_GET_CLASS (fontmap)->get_serial)
    return PANGO_FONT_MAP_GET_CLASS (fontmap)->get_serial (fontmap);
  else
    return 1;
}

/**
 * vogue_font_map_changed:
 * @fontmap: a #VogueFontMap
 *
 * Forces a change in the context, which will cause any #VogueContext
 * using this fontmap to change.
 *
 * This function is only useful when implementing a new backend
 * for Vogue, something applications won't do. Backends should
 * call this function if they have attached extra data to the context
 * and such data is changed.
 *
 * Since: 1.34
 **/
void
vogue_font_map_changed (VogueFontMap *fontmap)
{
  g_return_if_fail (PANGO_IS_FONT_MAP (fontmap));

  if (PANGO_FONT_MAP_GET_CLASS (fontmap)->changed)
    PANGO_FONT_MAP_GET_CLASS (fontmap)->changed (fontmap);
}
