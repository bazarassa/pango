/* Vogue
 * vogueft2.c: Routines for handling FreeType2 fonts
 *
 * Copyright (C) 1999 Red Hat Software
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

/**
 * SECTION:freetype-fonts
 * @short_description:Font handling and rendering with FreeType
 * @title:FreeType Fonts and Rendering
 *
 * The macros and functions in this section are used to access fonts and render
 * text to bitmaps using the FreeType 2 library.
 */
#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "vogueft2.h"
#include "vogueft2-private.h"
#include "voguefc-fontmap-private.h"
#include "voguefc-private.h"

/* for compatibility with older freetype versions */
#ifndef FT_LOAD_TARGET_MONO
#define FT_LOAD_TARGET_MONO  FT_LOAD_MONOCHROME
#endif

#define PANGO_FT2_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FT2_FONT, VogueFT2FontClass))
#define PANGO_FT2_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FT2_FONT))
#define PANGO_FT2_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FT2_FONT, VogueFT2FontClass))

typedef struct _VogueFT2FontClass   VogueFT2FontClass;

struct _VogueFT2FontClass
{
  VogueFcFontClass parent_class;
};

static void     vogue_ft2_font_finalize          (GObject        *object);

static void     vogue_ft2_font_get_glyph_extents (VogueFont      *font,
                                                  VogueGlyph      glyph,
                                                  VogueRectangle *ink_rect,
                                                  VogueRectangle *logical_rect);

static FT_Face  vogue_ft2_font_real_lock_face    (VogueFcFont    *font);
static void     vogue_ft2_font_real_unlock_face  (VogueFcFont    *font);


VogueFT2Font *
_vogue_ft2_font_new (VogueFT2FontMap *ft2fontmap,
		     FcPattern       *pattern)
{
  VogueFontMap *fontmap = PANGO_FONT_MAP (ft2fontmap);
  VogueFT2Font *ft2font;
  double d;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  ft2font = (VogueFT2Font *)g_object_new (PANGO_TYPE_FT2_FONT,
					  "pattern", pattern,
					  "fontmap", fontmap,
					  NULL);

  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch)
    ft2font->size = d*PANGO_SCALE;

  return ft2font;
}

static void
load_fallback_face (VogueFT2Font *ft2font,
		    const char   *original_file)
{
  VogueFcFont *fcfont = PANGO_FC_FONT (ft2font);
  FcPattern *sans;
  FcPattern *matched;
  FcResult result;
  FT_Error error;
  FcChar8 *filename2 = NULL;
  gchar *name;
  int id;

  sans = FcPatternBuild (NULL,
			 FC_FAMILY,     FcTypeString, "sans",
			 FC_PIXEL_SIZE, FcTypeDouble, (double)ft2font->size / PANGO_SCALE,
			 NULL);

  _vogue_ft2_font_map_default_substitute ((VogueFcFontMap *)fcfont->fontmap, sans);

  matched = FcFontMatch (vogue_fc_font_map_get_config ((VogueFcFontMap *)fcfont->fontmap), sans, &result);

  if (FcPatternGetString (matched, FC_FILE, 0, &filename2) != FcResultMatch)
    goto bail1;

  if (FcPatternGetInteger (matched, FC_INDEX, 0, &id) != FcResultMatch)
    goto bail1;

  error = FT_New_Face (_vogue_ft2_font_map_get_library (fcfont->fontmap),
		       (char *) filename2, id, &ft2font->face);


  if (error)
    {
    bail1:
      name = vogue_font_description_to_string (fcfont->description);
      g_error ("Unable to open font file %s for font %s, exiting\n", filename2, name);
    }
  else
    {
      name = vogue_font_description_to_string (fcfont->description);
      g_warning ("Unable to open font file %s for font %s, falling back to %s\n", original_file, name, filename2);
      g_free (name);
    }

  FcPatternDestroy (sans);
  FcPatternDestroy (matched);
}

static void
set_transform (VogueFT2Font *ft2font)
{
  VogueFcFont *fcfont = (VogueFcFont *)ft2font;
  FcMatrix *fc_matrix;

  if (FcPatternGetMatrix (fcfont->font_pattern, FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    {
      FT_Matrix ft_matrix;

      ft_matrix.xx = 0x10000L * fc_matrix->xx;
      ft_matrix.yy = 0x10000L * fc_matrix->yy;
      ft_matrix.xy = 0x10000L * fc_matrix->xy;
      ft_matrix.yx = 0x10000L * fc_matrix->yx;

      FT_Set_Transform (ft2font->face, &ft_matrix, NULL);
    }
}

/**
 * vogue_ft2_font_get_face:
 * @font: a #VogueFont
 *
 * Returns the native FreeType2 <type>FT_Face</type> structure used for this #VogueFont.
 * This may be useful if you want to use FreeType2 functions directly.
 *
 * Use vogue_fc_font_lock_face() instead; when you are done with a
 * face from vogue_fc_font_lock_face() you must call
 * vogue_fc_font_unlock_face().
 *
 * Return value: (nullable): a pointer to a <type>FT_Face</type>
 *               structure, with the size set correctly, or %NULL if
 *               @font is %NULL.
 **/
FT_Face
vogue_ft2_font_get_face (VogueFont *font)
{
  VogueFT2Font *ft2font = (VogueFT2Font *)font;
  VogueFcFont *fcfont = (VogueFcFont *)font;
  FT_Error error;
  FcPattern *pattern;
  FcChar8 *filename;
  FcBool antialias, hinting, autohint;
  int hintstyle;
  int id;

  if (G_UNLIKELY (!font))
    return NULL;

  pattern = fcfont->font_pattern;

  if (!ft2font->face)
    {
      ft2font->load_flags = 0;

      /* disable antialiasing if requested */
      if (FcPatternGetBool (pattern,
			    FC_ANTIALIAS, 0, &antialias) != FcResultMatch)
	antialias = FcTrue;

      if (antialias)
	ft2font->load_flags |= FT_LOAD_NO_BITMAP;
      else
	ft2font->load_flags |= FT_LOAD_TARGET_MONO;

      /* disable hinting if requested */
      if (FcPatternGetBool (pattern,
			    FC_HINTING, 0, &hinting) != FcResultMatch)
	hinting = FcTrue;

#ifdef FC_HINT_STYLE
      if (FcPatternGetInteger (pattern, FC_HINT_STYLE, 0, &hintstyle) != FcResultMatch)
	hintstyle = FC_HINT_FULL;

      if (!hinting || hintstyle == FC_HINT_NONE)
          ft2font->load_flags |= FT_LOAD_NO_HINTING;
      
      switch (hintstyle) {
      case FC_HINT_SLIGHT:
      case FC_HINT_MEDIUM:
	ft2font->load_flags |= FT_LOAD_TARGET_LIGHT;
	break;
      default:
	ft2font->load_flags |= FT_LOAD_TARGET_NORMAL;
	break;
      }
#else
      if (!hinting)
          ft2font->load_flags |= FT_LOAD_NO_HINTING;
#endif

      /* force autohinting if requested */
      if (FcPatternGetBool (pattern,
			    FC_AUTOHINT, 0, &autohint) != FcResultMatch)
	autohint = FcFalse;

      if (autohint)
	ft2font->load_flags |= FT_LOAD_FORCE_AUTOHINT;

      if (FcPatternGetString (pattern, FC_FILE, 0, &filename) != FcResultMatch)
	      goto bail0;

      if (FcPatternGetInteger (pattern, FC_INDEX, 0, &id) != FcResultMatch)
	      goto bail0;

      error = FT_New_Face (_vogue_ft2_font_map_get_library (fcfont->fontmap),
			   (char *) filename, id, &ft2font->face);
      if (error != FT_Err_Ok)
	{
	bail0:
	  load_fallback_face (ft2font, (char *) filename);
	}

      g_assert (ft2font->face);

      set_transform (ft2font);

      error = FT_Set_Char_Size (ft2font->face,
				PANGO_PIXELS_26_6 (ft2font->size),
				PANGO_PIXELS_26_6 (ft2font->size),
				0, 0);
      if (error)
	g_warning ("Error in FT_Set_Char_Size: %d", error);
    }

  return ft2font->face;
}

G_DEFINE_TYPE (VogueFT2Font, vogue_ft2_font, PANGO_TYPE_FC_FONT)

static void
vogue_ft2_font_init (VogueFT2Font *ft2font)
{
  ft2font->face = NULL;

  ft2font->size = 0;

  ft2font->glyph_info = g_hash_table_new (NULL, NULL);
}

static void
vogue_ft2_font_class_init (VogueFT2FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontClass *font_class = PANGO_FONT_CLASS (class);
  VogueFcFontClass *fc_font_class = PANGO_FC_FONT_CLASS (class);

  object_class->finalize = vogue_ft2_font_finalize;

  font_class->get_glyph_extents = vogue_ft2_font_get_glyph_extents;

  fc_font_class->lock_face = vogue_ft2_font_real_lock_face;
  fc_font_class->unlock_face = vogue_ft2_font_real_unlock_face;
}

static VogueFT2GlyphInfo *
vogue_ft2_font_get_glyph_info (VogueFont   *font,
			       VogueGlyph   glyph,
			       gboolean     create)
{
  VogueFT2Font *ft2font = (VogueFT2Font *)font;
  VogueFcFont *fcfont = (VogueFcFont *)font;
  VogueFT2GlyphInfo *info;

  info = g_hash_table_lookup (ft2font->glyph_info, GUINT_TO_POINTER (glyph));

  if ((info == NULL) && create)
    {
      info = g_slice_new0 (VogueFT2GlyphInfo);

      vogue_fc_font_get_raw_extents (fcfont,
				     glyph,
				     &info->ink_rect,
				     &info->logical_rect);

      g_hash_table_insert (ft2font->glyph_info, GUINT_TO_POINTER(glyph), info);
    }

  return info;
}

static void
vogue_ft2_font_get_glyph_extents (VogueFont      *font,
				  VogueGlyph      glyph,
				  VogueRectangle *ink_rect,
				  VogueRectangle *logical_rect)
{
  VogueFT2GlyphInfo *info;
  gboolean empty = FALSE;

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      glyph = vogue_fc_font_get_glyph ((VogueFcFont *) font, ' ');
      empty = TRUE;
    }

  if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
      VogueFontMetrics *metrics = vogue_font_get_metrics (font, NULL);

      if (metrics)
	{
	  if (ink_rect)
	    {
	      ink_rect->x = PANGO_SCALE;
	      ink_rect->width = metrics->approximate_char_width - 2 * PANGO_SCALE;
	      ink_rect->y = - (metrics->ascent - PANGO_SCALE);
	      ink_rect->height = metrics->ascent + metrics->descent - 2 * PANGO_SCALE;
	    }
	  if (logical_rect)
	    {
	      logical_rect->x = 0;
	      logical_rect->width = metrics->approximate_char_width;
	      logical_rect->y = -metrics->ascent;
	      logical_rect->height = metrics->ascent + metrics->descent;
	    }

	  vogue_font_metrics_unref (metrics);
	}
      else
	{
	  if (ink_rect)
	    ink_rect->x = ink_rect->y = ink_rect->height = ink_rect->width = 0;
	  if (logical_rect)
	    logical_rect->x = logical_rect->y = logical_rect->height = logical_rect->width = 0;
	}
      return;
    }

  info = vogue_ft2_font_get_glyph_info (font, glyph, TRUE);

  if (ink_rect)
    *ink_rect = info->ink_rect;
  if (logical_rect)
    *logical_rect = info->logical_rect;

  if (empty)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->y = ink_rect->height = ink_rect->width = 0;
      if (logical_rect)
	logical_rect->x = logical_rect->width = 0;
      return;
    }
}

/**
 * vogue_ft2_font_get_kerning:
 * @font: a #VogueFont
 * @left: the left #VogueGlyph
 * @right: the right #VogueGlyph
 *
 * Retrieves kerning information for a combination of two glyphs.
 *
 * Use vogue_fc_font_kern_glyphs() instead.
 *
 * Return value: The amount of kerning (in Vogue units) to apply for
 * the given combination of glyphs.
 **/
int
vogue_ft2_font_get_kerning (VogueFont *font,
			    VogueGlyph left,
			    VogueGlyph right)
{
  VogueFcFont *fc_font = PANGO_FC_FONT (font);

  FT_Face face;
  FT_Error error;
  FT_Vector kerning;

  face = vogue_fc_font_lock_face (fc_font);
  if (!face)
    return 0;

  if (!FT_HAS_KERNING (face))
    {
      vogue_fc_font_unlock_face (fc_font);
      return 0;
    }

  error = FT_Get_Kerning (face, left, right, ft_kerning_default, &kerning);
  if (error != FT_Err_Ok)
    {
      vogue_fc_font_unlock_face (fc_font);
      return 0;
    }

  vogue_fc_font_unlock_face (fc_font);
  return PANGO_UNITS_26_6 (kerning.x);
}

static FT_Face
vogue_ft2_font_real_lock_face (VogueFcFont *font)
{
  return vogue_ft2_font_get_face ((VogueFont *)font);
}

static void
vogue_ft2_font_real_unlock_face (VogueFcFont *font G_GNUC_UNUSED)
{
}

static gboolean
vogue_ft2_free_glyph_info_callback (gpointer key G_GNUC_UNUSED,
				    gpointer value,
				    gpointer data)
{
  VogueFT2Font *font = PANGO_FT2_FONT (data);
  VogueFT2GlyphInfo *info = value;

  if (font->glyph_cache_destroy && info->cached_glyph)
    (*font->glyph_cache_destroy) (info->cached_glyph);

  g_slice_free (VogueFT2GlyphInfo, info);
  return TRUE;
}

static void
vogue_ft2_font_finalize (GObject *object)
{
  VogueFT2Font *ft2font = (VogueFT2Font *)object;

  if (ft2font->face)
    {
      FT_Done_Face (ft2font->face);
      ft2font->face = NULL;
    }

  g_hash_table_foreach_remove (ft2font->glyph_info,
			       vogue_ft2_free_glyph_info_callback, object);
  g_hash_table_destroy (ft2font->glyph_info);

  G_OBJECT_CLASS (vogue_ft2_font_parent_class)->finalize (object);
}

/**
 * vogue_ft2_font_get_coverage:
 * @font: a <type>VogueFT2Font</type>.
 * @language: a language tag.
 *
 * Gets the #VogueCoverage for a <type>VogueFT2Font</type>. Use
 * vogue_font_get_coverage() instead.
 *
 * Return value: (transfer full): a #VogueCoverage.
 **/
VogueCoverage *
vogue_ft2_font_get_coverage (VogueFont     *font,
			     VogueLanguage *language)
{
  return vogue_font_get_coverage (font, language);
}

/* Utility functions */

/**
 * vogue_ft2_get_unknown_glyph:
 * @font: a #VogueFont
 *
 * Return the index of a glyph suitable for drawing unknown characters with
 * @font, or %PANGO_GLYPH_EMPTY if no suitable glyph found.
 *
 * If you want to draw an unknown-box for a character that is not covered
 * by the font,
 * use PANGO_GET_UNKNOWN_GLYPH() instead.
 *
 * Return value: a glyph index into @font, or %PANGO_GLYPH_EMPTY
 **/
VogueGlyph
vogue_ft2_get_unknown_glyph (VogueFont *font)
{
  FT_Face face = vogue_ft2_font_get_face (font);
  if (face && FT_IS_SFNT (face))
    /* TrueType fonts have an 'unknown glyph' box on glyph index 0 */
    return 0;
  else
    return PANGO_GLYPH_EMPTY;
}

void *
_vogue_ft2_font_get_cache_glyph_data (VogueFont *font,
				     int        glyph_index)
{
  VogueFT2GlyphInfo *info;

  if (!PANGO_FT2_IS_FONT (font))
    return NULL;

  info = vogue_ft2_font_get_glyph_info (font, glyph_index, FALSE);

  if (info == NULL)
    return NULL;

  return info->cached_glyph;
}

void
_vogue_ft2_font_set_cache_glyph_data (VogueFont     *font,
				     int            glyph_index,
				     void          *cached_glyph)
{
  VogueFT2GlyphInfo *info;

  if (!PANGO_FT2_IS_FONT (font))
    return;

  info = vogue_ft2_font_get_glyph_info (font, glyph_index, TRUE);

  info->cached_glyph = cached_glyph;

  /* TODO: Implement limiting of the number of cached glyphs */
}

void
_vogue_ft2_font_set_glyph_cache_destroy (VogueFont      *font,
					 GDestroyNotify  destroy_notify)
{
  if (!PANGO_FT2_IS_FONT (font))
    return;

  PANGO_FT2_FONT (font)->glyph_cache_destroy = destroy_notify;
}
