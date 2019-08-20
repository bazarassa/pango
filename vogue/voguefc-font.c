/* Vogue
 * voguefc-font.c: Shared interfaces for fontconfig-based backends
 *
 * Copyright (C) 2003 Red Hat Software
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
 * SECTION:voguefc-font
 * @short_description:Base font class for Fontconfig-based backends
 * @title:VogueFcFont
 * @see_also:
 * <variablelist><varlistentry><term>#VogueFcFontMap</term> <listitem>The base class for font maps; creating a new
 * Fontconfig-based backend involves deriving from both
 * #VogueFcFontMap and #VogueFcFont.</listitem></varlistentry></variablelist>
 *
 * #VogueFcFont is a base class for font implementation using the
 * Fontconfig and FreeType libraries. It is used in the
 * <link linkend="vogue-Xft-Fonts-and-Rendering">Xft</link> and
 * <link linkend="vogue-FreeType-Fonts-and-Rendering">FreeType</link>
 * backends shipped with Vogue, but can also be used when creating
 * new backends. Any backend deriving from this base class will
 * take advantage of the wide range of shapers implemented using
 * FreeType that come with Vogue.
 */
#include "config.h"

#include "voguefc-font-private.h"
#include "voguefc-fontmap.h"
#include "voguefc-private.h"
#include "vogue-layout.h"
#include "vogue-impl-utils.h"

#include <hb-ot.h>

enum {
  PROP_0,
  PROP_PATTERN,
  PROP_FONTMAP
};

typedef struct _VogueFcFontPrivate VogueFcFontPrivate;

struct _VogueFcFontPrivate
{
  VogueFcDecoder *decoder;
  VogueFcFontKey *key;
};

static gboolean vogue_fc_font_real_has_char  (VogueFcFont *font,
					      gunichar     wc);
static guint    vogue_fc_font_real_get_glyph (VogueFcFont *font,
					      gunichar     wc);

static void                  vogue_fc_font_finalize     (GObject          *object);
static void                  vogue_fc_font_set_property (GObject          *object,
							 guint             prop_id,
							 const GValue     *value,
							 GParamSpec       *pspec);
static void                  vogue_fc_font_get_property (GObject          *object,
							 guint             prop_id,
							 GValue           *value,
							 GParamSpec       *pspec);
static VogueCoverage *       vogue_fc_font_get_coverage (VogueFont        *font,
							 VogueLanguage    *language);
static VogueFontMetrics *    vogue_fc_font_get_metrics  (VogueFont        *font,
							 VogueLanguage    *language);
static VogueFontMap *        vogue_fc_font_get_font_map (VogueFont        *font);
static VogueFontDescription *vogue_fc_font_describe     (VogueFont        *font);
static VogueFontDescription *vogue_fc_font_describe_absolute (VogueFont        *font);
static void                  vogue_fc_font_get_features (VogueFont        *font,
                                                         hb_feature_t     *features,
                                                         guint             len,
                                                         guint            *num_features);
static hb_font_t *           vogue_fc_font_create_hb_font (VogueFont        *font);

#define PANGO_FC_FONT_LOCK_FACE(font)	(PANGO_FC_FONT_GET_CLASS (font)->lock_face (font))
#define PANGO_FC_FONT_UNLOCK_FACE(font)	(PANGO_FC_FONT_GET_CLASS (font)->unlock_face (font))

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (VogueFcFont, vogue_fc_font, PANGO_TYPE_FONT,
                                  G_ADD_PRIVATE (VogueFcFont))

static void
vogue_fc_font_class_init (VogueFcFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontClass *font_class = PANGO_FONT_CLASS (class);

  class->has_char = vogue_fc_font_real_has_char;
  class->get_glyph = vogue_fc_font_real_get_glyph;
  class->get_unknown_glyph = NULL;

  object_class->finalize = vogue_fc_font_finalize;
  object_class->set_property = vogue_fc_font_set_property;
  object_class->get_property = vogue_fc_font_get_property;
  font_class->describe = vogue_fc_font_describe;
  font_class->describe_absolute = vogue_fc_font_describe_absolute;
  font_class->get_coverage = vogue_fc_font_get_coverage;
  font_class->get_metrics = vogue_fc_font_get_metrics;
  font_class->get_font_map = vogue_fc_font_get_font_map;
  font_class->get_features = vogue_fc_font_get_features;
  font_class->create_hb_font = vogue_fc_font_create_hb_font;
  font_class->get_features = vogue_fc_font_get_features;

  g_object_class_install_property (object_class, PROP_PATTERN,
				   g_param_spec_pointer ("pattern",
							 "Pattern",
							 "The fontconfig pattern for this font",
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
							 G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (object_class, PROP_FONTMAP,
				   g_param_spec_object ("fontmap",
							"Font Map",
							"The VogueFc font map this font is associated with (Since: 1.26)",
							PANGO_TYPE_FC_FONT_MAP,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));
}

static void
vogue_fc_font_init (VogueFcFont *fcfont)
{
  fcfont->priv = vogue_fc_font_get_instance_private (fcfont);
}

static void
free_metrics_info (VogueFcMetricsInfo *info)
{
  vogue_font_metrics_unref (info->metrics);
  g_slice_free (VogueFcMetricsInfo, info);
}

static void
vogue_fc_font_finalize (GObject *object)
{
  VogueFcFont *fcfont = PANGO_FC_FONT (object);
  VogueFcFontPrivate *priv = fcfont->priv;
  VogueFcFontMap *fontmap;

  g_slist_foreach (fcfont->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (fcfont->metrics_by_lang);

  fontmap = g_weak_ref_get ((GWeakRef *) &fcfont->fontmap);
  if (fontmap)
    {
      _vogue_fc_font_map_remove (PANGO_FC_FONT_MAP (fcfont->fontmap), fcfont);
      g_weak_ref_clear ((GWeakRef *) &fcfont->fontmap);
      g_object_unref (fontmap);
    }

  FcPatternDestroy (fcfont->font_pattern);
  vogue_font_description_free (fcfont->description);

  if (priv->decoder)
    _vogue_fc_font_set_decoder (fcfont, NULL);

  G_OBJECT_CLASS (vogue_fc_font_parent_class)->finalize (object);
}

static gboolean
pattern_is_hinted (FcPattern *pattern)
{
  FcBool hinting;

  if (FcPatternGetBool (pattern,
			FC_HINTING, 0, &hinting) != FcResultMatch)
    hinting = FcTrue;

  return hinting;
}

static gboolean
pattern_is_transformed (FcPattern *pattern)
{
  FcMatrix *fc_matrix;

  if (FcPatternGetMatrix (pattern, FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    {
      return fc_matrix->xx != 1 || fc_matrix->xy != 0 ||
             fc_matrix->yx != 0 || fc_matrix->yy != 1;
    }
  else
    return FALSE;
}

static void
vogue_fc_font_set_property (GObject       *object,
			    guint          prop_id,
			    const GValue  *value,
			    GParamSpec    *pspec)
{
  VogueFcFont *fcfont = PANGO_FC_FONT (object);

  switch (prop_id)
    {
    case PROP_PATTERN:
      {
	FcPattern *pattern = g_value_get_pointer (value);

	g_return_if_fail (pattern != NULL);
	g_return_if_fail (fcfont->font_pattern == NULL);

	FcPatternReference (pattern);
	fcfont->font_pattern = pattern;
	fcfont->description = vogue_fc_font_description_from_pattern (pattern, TRUE);
	fcfont->is_hinted = pattern_is_hinted (pattern);
	fcfont->is_transformed = pattern_is_transformed (pattern);
      }
      goto set_decoder;

    case PROP_FONTMAP:
      {
	VogueFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (g_value_get_object (value));

	g_return_if_fail (fcfont->fontmap == NULL);
	g_weak_ref_set ((GWeakRef *) &fcfont->fontmap, fcfontmap);
      }
      goto set_decoder;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }

set_decoder:
  /* set decoder if both pattern and fontmap are set now */
  if (fcfont->font_pattern && fcfont->fontmap)
    _vogue_fc_font_set_decoder (fcfont,
				vogue_fc_font_map_find_decoder  ((VogueFcFontMap *) fcfont->fontmap,
								 fcfont->font_pattern));
}

static void
vogue_fc_font_get_property (GObject       *object,
			    guint          prop_id,
			    GValue        *value,
			    GParamSpec    *pspec)
{
  switch (prop_id)
    {
    case PROP_PATTERN:
      {
	VogueFcFont *fcfont = PANGO_FC_FONT (object);
	g_value_set_pointer (value, fcfont->font_pattern);
      }
      break;
    case PROP_FONTMAP:
      {
	VogueFcFont *fcfont = PANGO_FC_FONT (object);
	VogueFontMap *fontmap = g_weak_ref_get ((GWeakRef *) &fcfont->fontmap);
	g_value_take_object (value, fontmap);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static VogueFontDescription *
vogue_fc_font_describe (VogueFont *font)
{
  VogueFcFont *fcfont = (VogueFcFont *)font;

  return vogue_font_description_copy (fcfont->description);
}

static VogueFontDescription *
vogue_fc_font_describe_absolute (VogueFont *font)
{
  VogueFcFont *fcfont = (VogueFcFont *)font;
  VogueFontDescription *desc = vogue_font_description_copy (fcfont->description);
  double size;

  if (FcPatternGetDouble (fcfont->font_pattern, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
    vogue_font_description_set_absolute_size (desc, size * PANGO_SCALE);

  return desc;
}

static VogueCoverage *
vogue_fc_font_get_coverage (VogueFont     *font,
			    VogueLanguage *language G_GNUC_UNUSED)
{
  VogueFcFont *fcfont = (VogueFcFont *)font;
  VogueFcFontPrivate *priv = fcfont->priv;
  FcCharSet *charset;
  VogueFcFontMap *fontmap;
  VogueCoverage *coverage;

  if (priv->decoder)
    {
      charset = vogue_fc_decoder_get_charset (priv->decoder, fcfont);
      return _vogue_fc_font_map_fc_to_coverage (charset);
    }

  fontmap = g_weak_ref_get ((GWeakRef *) &fcfont->fontmap);
  if (!fontmap)
    return vogue_coverage_new ();

  coverage = _vogue_fc_font_map_get_coverage (fontmap, fcfont);
  g_object_unref (fontmap);
  return coverage;
}

/* For Xft, it would be slightly more efficient to simply to
 * call Xft, and also more robust against changes in Xft.
 * But for now, we simply use the same code for all backends.
 *
 * The code in this function is partly based on code from Xft,
 * Copyright 2000 Keith Packard
 */
static void
get_face_metrics (VogueFcFont      *fcfont,
		  VogueFontMetrics *metrics)
{
  hb_font_t *hb_font = vogue_font_get_hb_font (PANGO_FONT (fcfont));
  hb_font_extents_t extents;

  FcMatrix *fc_matrix;
  gboolean have_transform = FALSE;

  hb_font_get_extents_for_direction (hb_font, HB_DIRECTION_LTR, &extents);

  if  (FcPatternGetMatrix (fcfont->font_pattern,
			   FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    {
      have_transform = (fc_matrix->xx != 1 || fc_matrix->xy != 0 ||
			fc_matrix->yx != 0 || fc_matrix->yy != 1);
    }

  if (have_transform)
    {
      metrics->descent =  - extents.descender * fc_matrix->yy;
      metrics->ascent = extents.ascender * fc_matrix->yy;
      metrics->height = (extents.ascender - extents.descender + extents.line_gap) * fc_matrix->yy;
    }
  else
    {
      metrics->descent = - extents.descender;
      metrics->ascent = extents.ascender;
      metrics->height = extents.ascender - extents.descender + extents.line_gap;
    }

  metrics->underline_thickness = PANGO_SCALE;
  metrics->underline_position = - PANGO_SCALE;
  metrics->strikethrough_thickness = PANGO_SCALE;
  metrics->strikethrough_position = metrics->ascent / 2;

  /* FIXME: use the right hb version */
#if HB_VERSION_ATLEAST(2,5,4)
  hb_position_t position;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_UNDERLINE_SIZE, &position))
    metrics->underline_thickness = position;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_UNDERLINE_OFFSET, &position))
    metrics->underline_position = position;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_STRIKEOUT_SIZE, &position))
    metrics->strikethrough_thickness = position;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_STRIKEOUT_OFFSET, &position))
    metrics->strikethrough_position = position;
#endif
}

VogueFontMetrics *
vogue_fc_font_create_base_metrics_for_context (VogueFcFont   *fcfont,
					       VogueContext  *context)
{
  VogueFontMetrics *metrics;
  metrics = vogue_font_metrics_new ();

  get_face_metrics (fcfont, metrics);

  return metrics;
}

static int
max_glyph_width (VogueLayout *layout)
{
  int max_width = 0;
  GSList *l, *r;

  for (l = vogue_layout_get_lines_readonly (layout); l; l = l->next)
    {
      VogueLayoutLine *line = l->data;

      for (r = line->runs; r; r = r->next)
	{
	  VogueGlyphString *glyphs = ((VogueGlyphItem *)r->data)->glyphs;
	  int i;

	  for (i = 0; i < glyphs->num_glyphs; i++)
	    if (glyphs->glyphs[i].geometry.width > max_width)
	      max_width = glyphs->glyphs[i].geometry.width;
	}
    }

  return max_width;
}

static VogueFontMetrics *
vogue_fc_font_get_metrics (VogueFont     *font,
			   VogueLanguage *language)
{
  VogueFcFont *fcfont = PANGO_FC_FONT (font);
  VogueFcMetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;
  static int in_get_metrics;

  const char *sample_str = vogue_language_get_sample_string (language);

  tmp_list = fcfont->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;

      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      VogueFontMap *fontmap;
      VogueContext *context;

      fontmap = g_weak_ref_get ((GWeakRef *) &fcfont->fontmap);
      if (!fontmap)
	return vogue_font_metrics_new ();

      info = g_slice_new0 (VogueFcMetricsInfo);

      /* Note: we need to add info to the list before calling
       * into VogueLayout below, to prevent recursion
       */
      fcfont->metrics_by_lang = g_slist_prepend (fcfont->metrics_by_lang,
						 info);

      info->sample_str = sample_str;

      context = vogue_font_map_create_context (fontmap);
      vogue_context_set_language (context, language);

      info->metrics = vogue_fc_font_create_base_metrics_for_context (fcfont, context);

      if (!in_get_metrics)
        {
          /* Compute derived metrics */
          VogueLayout *layout;
          VogueRectangle extents;
          const char *sample_str = vogue_language_get_sample_string (language);
          VogueFontDescription *desc = vogue_font_describe_with_absolute_size (font);
          gulong sample_str_width;

          in_get_metrics = 1;

          layout = vogue_layout_new (context);
          vogue_layout_set_font_description (layout, desc);
          vogue_font_description_free (desc);

          vogue_layout_set_text (layout, sample_str, -1);
          vogue_layout_get_extents (layout, NULL, &extents);

          sample_str_width = vogue_utf8_strwidth (sample_str);
          g_assert (sample_str_width > 0);
          info->metrics->approximate_char_width = extents.width / sample_str_width;

          vogue_layout_set_text (layout, "0123456789", -1);
          info->metrics->approximate_digit_width = max_glyph_width (layout);

          g_object_unref (layout);

          in_get_metrics = 0;
        }

      g_object_unref (context);
      g_object_unref (fontmap);
    }

  return vogue_font_metrics_ref (info->metrics);
}

static VogueFontMap *
vogue_fc_font_get_font_map (VogueFont *font)
{
  VogueFcFont *fcfont = PANGO_FC_FONT (font);

  /* MT-unsafe.  Oh well...  The API is unsafe. */
  return fcfont->fontmap;
}

static gboolean
vogue_fc_font_real_has_char (VogueFcFont *font,
			     gunichar     wc)
{
  FcCharSet *charset;

  if (FcPatternGetCharSet (font->font_pattern,
			   FC_CHARSET, 0, &charset) != FcResultMatch)
    return FALSE;

  return FcCharSetHasChar (charset, wc);
}

static guint
vogue_fc_font_real_get_glyph (VogueFcFont *font,
			      gunichar     wc)
{
  hb_font_t *hb_font = vogue_font_get_hb_font (PANGO_FONT (font));
  hb_codepoint_t glyph = PANGO_GET_UNKNOWN_GLYPH (wc);

  hb_font_get_nominal_glyph (hb_font, wc, &glyph);

  return glyph;
}

/**
 * vogue_fc_font_lock_face:
 * @font: a #VogueFcFont.
 *
 * Gets the FreeType <type>FT_Face</type> associated with a font,
 * This face will be kept around until you call
 * vogue_fc_font_unlock_face().
 *
 * Return value: the FreeType <type>FT_Face</type> associated with @font.
 *
 * Since: 1.4
 * Deprecated: 1.44: Use vogue_font_get_hb_font() instead
 **/
FT_Face
vogue_fc_font_lock_face (VogueFcFont *font)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), NULL);

  return PANGO_FC_FONT_LOCK_FACE (font);
}

/**
 * vogue_fc_font_unlock_face:
 * @font: a #VogueFcFont.
 *
 * Releases a font previously obtained with
 * vogue_fc_font_lock_face().
 *
 * Since: 1.4
 * Deprecated: 1.44: Use vogue_font_get_hb_font() instead
 **/
void
vogue_fc_font_unlock_face (VogueFcFont *font)
{
  g_return_if_fail (PANGO_IS_FC_FONT (font));

  PANGO_FC_FONT_UNLOCK_FACE (font);
}

/**
 * vogue_fc_font_has_char:
 * @font: a #VogueFcFont
 * @wc: Unicode codepoint to look up
 *
 * Determines whether @font has a glyph for the codepoint @wc.
 *
 * Return value: %TRUE if @font has the requested codepoint.
 *
 * Since: 1.4
 * Deprecated: 1.44: Use vogue_font_has_char()
 **/
gboolean
vogue_fc_font_has_char (VogueFcFont *font,
			gunichar     wc)
{
  VogueFcFontPrivate *priv = font->priv;
  FcCharSet *charset;

  g_return_val_if_fail (PANGO_IS_FC_FONT (font), FALSE);

  if (priv->decoder)
    {
      charset = vogue_fc_decoder_get_charset (priv->decoder, font);
      return FcCharSetHasChar (charset, wc);
    }

  return PANGO_FC_FONT_GET_CLASS (font)->has_char (font, wc);
}

/**
 * vogue_fc_font_get_glyph:
 * @font: a #VogueFcFont
 * @wc: Unicode character to look up
 *
 * Gets the glyph index for a given Unicode character
 * for @font. If you only want to determine
 * whether the font has the glyph, use vogue_fc_font_has_char().
 *
 * Return value: the glyph index, or 0, if the Unicode
 *   character doesn't exist in the font.
 *
 * Since: 1.4
 **/
VogueGlyph
vogue_fc_font_get_glyph (VogueFcFont *font,
			 gunichar     wc)
{
  VogueFcFontPrivate *priv = font->priv;

  /* Replace NBSP with a normal space; it should be invariant that
   * they shape the same other than breaking properties.
   */
  if (wc == 0xA0)
	  wc = 0x20;

  if (priv->decoder)
    return vogue_fc_decoder_get_glyph (priv->decoder, font, wc);

  return PANGO_FC_FONT_GET_CLASS (font)->get_glyph (font, wc);
}


/**
 * vogue_fc_font_get_unknown_glyph:
 * @font: a #VogueFcFont
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Use PANGO_GET_UNKNOWN_GLYPH() instead.
 *
 * Return value: a glyph index into @font.
 *
 * Since: 1.4
 **/
VogueGlyph
vogue_fc_font_get_unknown_glyph (VogueFcFont *font,
				 gunichar     wc)
{
  if (font && PANGO_FC_FONT_GET_CLASS (font)->get_unknown_glyph)
    return PANGO_FC_FONT_GET_CLASS (font)->get_unknown_glyph (font, wc);

  return PANGO_GET_UNKNOWN_GLYPH (wc);
}

void
_vogue_fc_font_shutdown (VogueFcFont *font)
{
  g_return_if_fail (PANGO_IS_FC_FONT (font));

  if (PANGO_FC_FONT_GET_CLASS (font)->shutdown)
    PANGO_FC_FONT_GET_CLASS (font)->shutdown (font);
}

/**
 * vogue_fc_font_kern_glyphs:
 * @font: a #VogueFcFont
 * @glyphs: a #VogueGlyphString
 *
 * This function used to adjust each adjacent pair of glyphs
 * in @glyphs according to kerning information in @font.
 *
 * Since 1.44, it does nothing.
 *
 *
 * Since: 1.4
 * Deprecated: 1.32
 **/
void
vogue_fc_font_kern_glyphs (VogueFcFont      *font,
			   VogueGlyphString *glyphs)
{
}

/**
 * _vogue_fc_font_get_decoder:
 * @font: a #VogueFcFont
 *
 * This will return any custom decoder set on this font.
 *
 * Return value: The custom decoder
 *
 * Since: 1.6
 **/

VogueFcDecoder *
_vogue_fc_font_get_decoder (VogueFcFont *font)
{
  VogueFcFontPrivate *priv = font->priv;

  return priv->decoder;
}

/**
 * _vogue_fc_font_set_decoder:
 * @font: a #VogueFcFont
 * @decoder: a #VogueFcDecoder to set for this font
 *
 * This sets a custom decoder for this font.  Any previous decoder
 * will be released before this one is set.
 *
 * Since: 1.6
 **/

void
_vogue_fc_font_set_decoder (VogueFcFont    *font,
			    VogueFcDecoder *decoder)
{
  VogueFcFontPrivate *priv = font->priv;

  if (priv->decoder)
    g_object_unref (priv->decoder);

  priv->decoder = decoder;

  if (priv->decoder)
    g_object_ref (priv->decoder);
}

VogueFcFontKey *
_vogue_fc_font_get_font_key (VogueFcFont *fcfont)
{
  VogueFcFontPrivate *priv = fcfont->priv;

  return priv->key;
}

void
_vogue_fc_font_set_font_key (VogueFcFont    *fcfont,
			     VogueFcFontKey *key)
{
  VogueFcFontPrivate *priv = fcfont->priv;

  priv->key = key;
}

/**
 * vogue_fc_font_get_raw_extents:
 * @fcfont: a #VogueFcFont
 * @glyph: the glyph index to load
 * @ink_rect: (out) (optional): location to store ink extents of the
 *   glyph, or %NULL
 * @logical_rect: (out) (optional): location to store logical extents
 *   of the glyph or %NULL
 *
 * Gets the extents of a single glyph from a font. The extents are in
 * user space; that is, they are not transformed by any matrix in effect
 * for the font.
 *
 * Long term, this functionality probably belongs in the default
 * implementation of the get_glyph_extents() virtual function.
 * The other possibility would be to to make it public in something
 * like it's current form, and also expose glyph information
 * caching functionality similar to vogue_ft2_font_set_glyph_info().
 *
 * Since: 1.6
 **/
void
vogue_fc_font_get_raw_extents (VogueFcFont    *fcfont,
			       VogueGlyph      glyph,
			       VogueRectangle *ink_rect,
			       VogueRectangle *logical_rect)
{
  g_return_if_fail (PANGO_IS_FC_FONT (fcfont));

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->width = 0;
	  ink_rect->y = 0;
	  ink_rect->height = 0;
	}

      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = 0;
	  logical_rect->y = 0;
	  logical_rect->height = 0;
	}
    }
  else
    {
      hb_font_t *hb_font = vogue_font_get_hb_font (PANGO_FONT (fcfont));
      hb_glyph_extents_t extents;
      hb_font_extents_t font_extents;

      hb_font_get_glyph_extents (hb_font, glyph, &extents);
      hb_font_get_extents_for_direction (hb_font, HB_DIRECTION_LTR, &font_extents);

      if (ink_rect)
	{
	  ink_rect->x = extents.x_bearing;
	  ink_rect->width = extents.width;
	  ink_rect->y = -extents.y_bearing;
	  ink_rect->height = -extents.height;
	}

      if (logical_rect)
	{
          hb_position_t x, y;

          hb_font_get_glyph_advance_for_direction (hb_font,
                                                   glyph,
                                                   HB_DIRECTION_LTR,
                                                   &x, &y);

	  logical_rect->x = 0;
	  logical_rect->width = x;
	  logical_rect->y = - font_extents.ascender;
	  logical_rect->height = font_extents.ascender - font_extents.descender;
	}
    }
}

static void
vogue_fc_font_get_features (VogueFont    *font,
                            hb_feature_t *features,
                            guint         len,
                            guint        *num_features)
{
  /* Setup features from fontconfig pattern. */
  VogueFcFont *fc_font = PANGO_FC_FONT (font);
  if (fc_font->font_pattern)
    {
      char *s;
      while (*num_features < len &&
             FcResultMatch == FcPatternGetString (fc_font->font_pattern,
                                                  PANGO_FC_FONT_FEATURES,
                                                  *num_features,
                                                  (FcChar8 **) &s))
        {
          gboolean ret = hb_feature_from_string (s, -1, &features[*num_features]);
          features[*num_features].start = 0;
          features[*num_features].end   = (unsigned int) -1;
          if (ret)
            (*num_features)++;
        }
    }
}

extern gpointer get_gravity_class (void);

static VogueGravity
vogue_fc_font_key_get_gravity (VogueFcFontKey *key)
{
  const FcPattern *pattern;
  VogueGravity gravity = PANGO_GRAVITY_SOUTH;
  FcChar8 *s;

  pattern = vogue_fc_font_key_get_pattern (key);
  if (FcPatternGetString (pattern, PANGO_FC_GRAVITY, 0, (FcChar8 **)&s) == FcResultMatch)
    {
      GEnumValue *value = g_enum_get_value_by_nick (get_gravity_class (), (char *)s);
      gravity = value->value;
    }

  return gravity;
}

static double
get_font_size (VogueFcFontKey *key)
{
  const FcPattern *pattern;
  double size;
  double dpi;

  pattern = vogue_fc_font_key_get_pattern (key);
  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
    return size;

  /* Just in case FC_PIXEL_SIZE got unset between vogue_fc_make_pattern()
   * and here.  That would be very weird.
   */

  if (FcPatternGetDouble (pattern, FC_DPI, 0, &dpi) != FcResultMatch)
    dpi = 72;

  if (FcPatternGetDouble (pattern, FC_SIZE, 0, &size) == FcResultMatch)
    return size * dpi / 72.;

  /* Whatever */
  return 18.;
}

static void
parse_variations (const char            *variations,
                  hb_ot_var_axis_info_t *axes,
                  int                    n_axes,
                  float                 *coords)
{
  const char *p;
  const char *end;
  hb_variation_t var;
  int i;

  p = variations;
  while (p && *p)
    {
      end = strchr (p, ',');
      if (hb_variation_from_string (p, end ? end - p: -1, &var))
        {
          for (i = 0; i < n_axes; i++)
            {
              if (axes[i].tag == var.tag)
                {
                  coords[axes[i].axis_index] = var.value;
                  break;
                }
            }
        }

      p = end ? end + 1 : NULL;
    }
}

static hb_font_t *
vogue_fc_font_create_hb_font (VogueFont *font)
{
  VogueFcFont *fc_font = PANGO_FC_FONT (font);
  VogueFcFontKey *key;
  hb_face_t *hb_face;
  hb_font_t *hb_font;
  double x_scale_inv, y_scale_inv;
  double x_scale, y_scale;
  double size;

  x_scale_inv = y_scale_inv = 1.0;
  size = 1.0;

  key = _vogue_fc_font_get_font_key (fc_font);
  if (key)
    {
      const FcPattern *pattern = vogue_fc_font_key_get_pattern (key);
      const VogueMatrix *matrix;
      VogueMatrix matrix2;
      VogueGravity gravity;
      FcMatrix fc_matrix, *fc_matrix_val;
      double x, y;
      int i;

      matrix = vogue_fc_font_key_get_matrix (key);
      vogue_matrix_get_font_scale_factors (matrix, &x_scale_inv, &y_scale_inv);

      FcMatrixInit (&fc_matrix);
      for (i = 0; FcPatternGetMatrix (pattern, FC_MATRIX, i, &fc_matrix_val) == FcResultMatch; i++)
        FcMatrixMultiply (&fc_matrix, &fc_matrix, fc_matrix_val);

      matrix2.xx = fc_matrix.xx;
      matrix2.yx = fc_matrix.yx;
      matrix2.xy = fc_matrix.xy;
      matrix2.yy = fc_matrix.yy;
      vogue_matrix_get_font_scale_factors (&matrix2, &x, &y);

      x_scale_inv /= x;
      y_scale_inv /= y;

      gravity = vogue_fc_font_key_get_gravity (key);
      if (PANGO_GRAVITY_IS_IMPROPER (gravity))
        {
          x_scale_inv = -x_scale_inv;
          y_scale_inv = -y_scale_inv;
        }
      size = get_font_size (key);
    }

  x_scale = 1. / x_scale_inv;
  y_scale = 1. / y_scale_inv;

  hb_face = vogue_fc_font_map_get_hb_face (PANGO_FC_FONT_MAP (fc_font->fontmap), fc_font);

  hb_font = hb_font_create (hb_face);
  hb_font_set_scale (hb_font,
                     size * PANGO_SCALE * x_scale,
                     size * PANGO_SCALE * y_scale);

  if (key)
    {
      FcPattern *pattern = vogue_fc_font_key_get_pattern (key);
      const char *variations;
      int index;
      unsigned int n_axes;
      hb_ot_var_axis_info_t *axes;
      float *coords;
      int i;

      n_axes = hb_ot_var_get_axis_infos (hb_face, 0, NULL, NULL);
      if (n_axes == 0)
        goto done;

      axes = g_new0 (hb_ot_var_axis_info_t, n_axes);
      coords = g_new (float, n_axes);

      hb_ot_var_get_axis_infos (hb_face, 0, &n_axes, axes);
      for (i = 0; i < n_axes; i++)
        coords[axes[i].axis_index] = axes[i].default_value;

      if (FcPatternGetInteger (pattern, FC_INDEX, 0, &index) == FcResultMatch &&
          index != 0)
        {
          unsigned int instance = (index >> 16) - 1;
          hb_ot_var_named_instance_get_design_coords (hb_face, instance, &n_axes, coords);
        }

      if (FcPatternGetString (pattern, PANGO_FC_FONT_VARIATIONS, 0, (FcChar8 **)&variations) == FcResultMatch)
        parse_variations (variations, axes, n_axes, coords);

      variations = vogue_fc_font_key_get_variations (key);
      if (variations)
        parse_variations (variations, axes, n_axes, coords);

      hb_font_set_var_coords_design (hb_font, coords, n_axes);

      g_free (coords);
      g_free (axes);
    }

done:
  return hb_font;
}
