/* Vogue
 * voguecairo-font.c: Cairo font handling
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
#include <string.h>

#include "voguecairo.h"
#include "voguecairo-private.h"
#include "vogue-font-private.h"
#include "vogue-impl-utils.h"

#define PANGO_CAIRO_FONT_PRIVATE(font)		\
  ((VogueCairoFontPrivate *)			\
   (font == NULL ? NULL :			\
    G_STRUCT_MEMBER_P (font,			\
    PANGO_CAIRO_FONT_GET_IFACE(PANGO_CAIRO_FONT(font))->cf_priv_offset)))

typedef VogueCairoFontIface VogueCairoFontInterface;
G_DEFINE_INTERFACE (VogueCairoFont, vogue_cairo_font, PANGO_TYPE_FONT)

static void
vogue_cairo_font_default_init (VogueCairoFontIface *iface)
{
}


static VogueCairoFontPrivateScaledFontData *
_vogue_cairo_font_private_scaled_font_data_create (void)
{
  return g_slice_new (VogueCairoFontPrivateScaledFontData);
}

static void
_vogue_cairo_font_private_scaled_font_data_destroy (VogueCairoFontPrivateScaledFontData *data)
{
  if (data)
    {
      cairo_font_options_destroy (data->options);
      g_slice_free (VogueCairoFontPrivateScaledFontData, data);
    }
}

cairo_scaled_font_t *
_vogue_cairo_font_private_get_scaled_font (VogueCairoFontPrivate *cf_priv)
{
  cairo_font_face_t *font_face;

  if (G_LIKELY (cf_priv->scaled_font))
    return cf_priv->scaled_font;

  /* need to create it */

  if (G_UNLIKELY (cf_priv->data == NULL))
    {
      /* we have tried to create and failed before */
      return NULL;
    }

  font_face = (* PANGO_CAIRO_FONT_GET_IFACE (cf_priv->cfont)->create_font_face) (cf_priv->cfont);
  if (G_UNLIKELY (font_face == NULL))
    goto done;

  cf_priv->scaled_font = cairo_scaled_font_create (font_face,
						   &cf_priv->data->font_matrix,
						   &cf_priv->data->ctm,
						   cf_priv->data->options);

  cairo_font_face_destroy (font_face);

done:

  if (G_UNLIKELY (cf_priv->scaled_font == NULL || cairo_scaled_font_status (cf_priv->scaled_font) != CAIRO_STATUS_SUCCESS))
    {
      cairo_scaled_font_t *scaled_font = cf_priv->scaled_font;
      VogueFont *font = PANGO_FONT (cf_priv->cfont);
      static GQuark warned_quark = 0; /* MT-safe */
      if (!warned_quark)
	warned_quark = g_quark_from_static_string ("voguecairo-scaledfont-warned");

      if (!g_object_get_qdata (G_OBJECT (font), warned_quark))
	{
	  VogueFontDescription *desc;
	  char *s;

	  desc = vogue_font_describe (font);
	  s = vogue_font_description_to_string (desc);
	  vogue_font_description_free (desc);

	  g_warning ("failed to create cairo %s, expect ugly output. the offending font is '%s'",
		     font_face ? "scaled font" : "font face",
		     s);

	  if (!font_face)
		g_warning ("font_face is NULL");
	  else
		g_warning ("font_face status is: %s",
			   cairo_status_to_string (cairo_font_face_status (font_face)));

	  if (!scaled_font)
		g_warning ("scaled_font is NULL");
	  else
		g_warning ("scaled_font status is: %s",
			   cairo_status_to_string (cairo_scaled_font_status (scaled_font)));

	  g_free (s);

	  g_object_set_qdata_full (G_OBJECT (font), warned_quark,
				   GINT_TO_POINTER (1), NULL);
	}
    }

  _vogue_cairo_font_private_scaled_font_data_destroy (cf_priv->data);
  cf_priv->data = NULL;

  return cf_priv->scaled_font;
}

/**
 * vogue_cairo_font_get_scaled_font:
 * @font: a #VogueFont from a #VogueCairoFontMap
 *
 * Gets the #cairo_scaled_font_t used by @font.
 * The scaled font can be referenced and kept using
 * cairo_scaled_font_reference().
 *
 * Return value: (nullable): the #cairo_scaled_font_t used by @font,
 *               or %NULL if @font is %NULL.
 *
 * Since: 1.18
 **/
cairo_scaled_font_t *
vogue_cairo_font_get_scaled_font (VogueCairoFont *cfont)
{
  VogueCairoFontPrivate *cf_priv;

  if (G_UNLIKELY (!cfont))
    return NULL;

  cf_priv = PANGO_CAIRO_FONT_PRIVATE (cfont);

  return _vogue_cairo_font_private_get_scaled_font (cf_priv);
}

/**
 * _vogue_cairo_font_install:
 * @font: a #VogueCairoFont
 * @cr: a #cairo_t
 *
 * Makes @font the current font for rendering in the specified
 * Cairo context.
 *
 * Return value: %TRUE if font was installed successfully, %FALSE otherwise.
 **/
gboolean
_vogue_cairo_font_install (VogueFont *font,
			   cairo_t   *cr)
{
  cairo_scaled_font_t *scaled_font = vogue_cairo_font_get_scaled_font ((VogueCairoFont *)font);

  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return FALSE;

  cairo_set_scaled_font (cr, scaled_font);

  return TRUE;
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

typedef struct _VogueCairoFontMetricsInfo
{
  const char       *sample_str;
  VogueFontMetrics *metrics;
} VogueCairoFontMetricsInfo;

VogueFontMetrics *
_vogue_cairo_font_get_metrics (VogueFont     *font,
			       VogueLanguage *language)
{
  VogueCairoFont *cfont = (VogueCairoFont *) font;
  VogueCairoFontPrivate *cf_priv = PANGO_CAIRO_FONT_PRIVATE (font);
  VogueCairoFontMetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;
  static int in_get_metrics;

  const char *sample_str = vogue_language_get_sample_string (language);

  tmp_list = cf_priv->metrics_by_lang;
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
      cairo_font_options_t *font_options;
      VogueLayout *layout;
      VogueRectangle extents;
      VogueFontDescription *desc;
      cairo_scaled_font_t *scaled_font;
      cairo_matrix_t cairo_matrix;
      VogueMatrix vogue_matrix;
      VogueMatrix identity = PANGO_MATRIX_INIT;
      glong sample_str_width;

      int height, shift;

      /* XXX this is racy.  need a ref'ing getter... */
      fontmap = vogue_font_get_font_map (font);
      if (!fontmap)
        return vogue_font_metrics_new ();
      fontmap = g_object_ref (fontmap);

      info = g_slice_new0 (VogueCairoFontMetricsInfo);

      cf_priv->metrics_by_lang = g_slist_prepend (cf_priv->metrics_by_lang, info);

      info->sample_str = sample_str;

      scaled_font = _vogue_cairo_font_private_get_scaled_font (cf_priv);

      context = vogue_font_map_create_context (fontmap);
      vogue_context_set_language (context, language);

      font_options = cairo_font_options_create ();
      cairo_scaled_font_get_font_options (scaled_font, font_options);
      vogue_cairo_context_set_font_options (context, font_options);
      cairo_font_options_destroy (font_options);

      info->metrics = (* PANGO_CAIRO_FONT_GET_IFACE (font)->create_base_metrics_for_context) (cfont, context);

      /* We now need to adjust the base metrics for ctm */
      cairo_scaled_font_get_ctm (scaled_font, &cairo_matrix);
      vogue_matrix.xx = cairo_matrix.xx;
      vogue_matrix.yx = cairo_matrix.yx;
      vogue_matrix.xy = cairo_matrix.xy;
      vogue_matrix.yy = cairo_matrix.yy;
      vogue_matrix.x0 = 0;
      vogue_matrix.y0 = 0;
      if (G_UNLIKELY (0 != memcmp (&identity, &vogue_matrix, 4 * sizeof (double))))
        {
	  double xscale = vogue_matrix_get_font_scale_factor (&vogue_matrix);
	  if (xscale) xscale = 1 / xscale;

	  info->metrics->ascent *= xscale;
	  info->metrics->descent *= xscale;
	  info->metrics->height *= xscale;
	  info->metrics->underline_position *= xscale;
	  info->metrics->underline_thickness *= xscale;
	  info->metrics->strikethrough_position *= xscale;
	  info->metrics->strikethrough_thickness *= xscale;
	}

      /* Set the matrix on the context so we don't have to adjust the derived
       * metrics. */
      vogue_context_set_matrix (context, &vogue_matrix);

      /* Ugly. We need to prevent recursion when we call into
       * VogueLayout to determine approximate char width.
       */
      if (!in_get_metrics)
        {
          in_get_metrics = 1;

          /* Update approximate_*_width now */
          layout = vogue_layout_new (context);
          desc = vogue_font_describe_with_absolute_size (font);
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

      /* We may actually reuse ascent/descent we got from cairo here.  that's
       * in cf_priv->font_extents.
       */
      height = info->metrics->ascent + info->metrics->descent;
      switch (cf_priv->gravity)
	{
	  default:
	  case PANGO_GRAVITY_AUTO:
	  case PANGO_GRAVITY_SOUTH:
	    break;
	  case PANGO_GRAVITY_NORTH:
	    info->metrics->ascent = info->metrics->descent;
	    break;
	  case PANGO_GRAVITY_EAST:
	  case PANGO_GRAVITY_WEST:
	    {
	      int ascent = height / 2;
	      if (cf_priv->is_hinted)
	        ascent = PANGO_UNITS_ROUND (ascent);
	      info->metrics->ascent = ascent;
	    }
	}
      shift = (height - info->metrics->ascent) - info->metrics->descent;
      info->metrics->descent += shift;
      info->metrics->underline_position -= shift;
      info->metrics->strikethrough_position -= shift;
      info->metrics->ascent = height - info->metrics->descent;

      g_object_unref (context);
      g_object_unref (fontmap);
    }

  return vogue_font_metrics_ref (info->metrics);
}

static VogueCairoFontHexBoxInfo *
_vogue_cairo_font_private_get_hex_box_info (VogueCairoFontPrivate *cf_priv)
{
  const char hexdigits[] = "0123456789ABCDEF";
  char c[2] = {0, 0};
  VogueFont *mini_font;
  VogueCairoFontHexBoxInfo *hbi;

  /* for metrics hinting */
  double scale_x = 1., scale_x_inv = 1., scale_y = 1., scale_y_inv = 1.;
  gboolean is_hinted;

  int i;
  int rows;
  double pad;
  double width = 0;
  double height = 0;
  cairo_font_options_t *font_options;
  cairo_font_extents_t font_extents;
  double size, mini_size;
  VogueFontDescription *desc;
  cairo_scaled_font_t *scaled_font, *scaled_mini_font;
  VogueMatrix vogue_ctm, vogue_font_matrix;
  cairo_matrix_t cairo_ctm, cairo_font_matrix;
  /*VogueGravity gravity;*/

  if (!cf_priv)
    return NULL;

  if (cf_priv->hbi)
    return cf_priv->hbi;

  scaled_font = _vogue_cairo_font_private_get_scaled_font (cf_priv);
  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return NULL;

  is_hinted = cf_priv->is_hinted;

  font_options = cairo_font_options_create ();
  desc = vogue_font_describe_with_absolute_size ((VogueFont *)cf_priv->cfont);
  /*gravity = vogue_font_description_get_gravity (desc);*/

  cairo_scaled_font_get_ctm (scaled_font, &cairo_ctm);
  cairo_scaled_font_get_font_matrix (scaled_font, &cairo_font_matrix);
  cairo_scaled_font_get_font_options (scaled_font, font_options);
  /* I started adding support for vertical hexboxes here, but it's too much
   * work.  Easier to do with cairo user fonts and vertical writing mode
   * support in cairo.
   */
  /*cairo_matrix_rotate (&cairo_ctm, vogue_gravity_to_rotation (gravity));*/
  vogue_ctm.xx = cairo_ctm.xx;
  vogue_ctm.yx = cairo_ctm.yx;
  vogue_ctm.xy = cairo_ctm.xy;
  vogue_ctm.yy = cairo_ctm.yy;
  vogue_ctm.x0 = cairo_ctm.x0;
  vogue_ctm.y0 = cairo_ctm.y0;
  vogue_font_matrix.xx = cairo_font_matrix.xx;
  vogue_font_matrix.yx = cairo_font_matrix.yx;
  vogue_font_matrix.xy = cairo_font_matrix.xy;
  vogue_font_matrix.yy = cairo_font_matrix.yy;
  vogue_font_matrix.x0 = cairo_font_matrix.x0;
  vogue_font_matrix.y0 = cairo_font_matrix.y0;

  size = vogue_matrix_get_font_scale_factor (&vogue_font_matrix) /
	 vogue_matrix_get_font_scale_factor (&vogue_ctm);

  if (is_hinted)
    {
      /* prepare for some hinting */
      double x, y;

      x = 1.; y = 0.;
      cairo_matrix_transform_distance (&cairo_ctm, &x, &y);
      scale_x = sqrt (x*x + y*y);
      scale_x_inv = 1 / scale_x;

      x = 0.; y = 1.;
      cairo_matrix_transform_distance (&cairo_ctm, &x, &y);
      scale_y = sqrt (x*x + y*y);
      scale_y_inv = 1 / scale_y;
    }

/* we hint to the nearest device units */
#define HINT(value, scale, scale_inv) (ceil ((value-1e-5) * scale) * scale_inv)
#define HINT_X(value) HINT ((value), scale_x, scale_x_inv)
#define HINT_Y(value) HINT ((value), scale_y, scale_y_inv)

  /* create mini_font description */
  {
    VogueFontMap *fontmap;
    VogueContext *context;

    /* XXX this is racy.  need a ref'ing getter... */
    fontmap = vogue_font_get_font_map ((VogueFont *)cf_priv->cfont);
    if (!fontmap)
      return NULL;
    fontmap = g_object_ref (fontmap);

    /* we inherit most font properties for the mini font.  just
     * change family and size.  means, you get bold hex digits
     * in the hexbox for a bold font.
     */

    /* We should rotate the box, not glyphs */
    vogue_font_description_unset_fields (desc, PANGO_FONT_MASK_GRAVITY);

    vogue_font_description_set_family_static (desc, "monospace");

    rows = 2;
    mini_size = size / 2.2;
    if (is_hinted)
      {
	mini_size = HINT_Y (mini_size);

	if (mini_size < 6.0)
	  {
	    rows = 1;
	    mini_size = MIN (MAX (size - 1, 0), 6.0);
	  }
      }

    vogue_font_description_set_absolute_size (desc, vogue_units_from_double (mini_size));

    /* load mini_font */

    context = vogue_font_map_create_context (fontmap);

    vogue_context_set_matrix (context, &vogue_ctm);
    vogue_context_set_language (context, vogue_script_get_sample_language (PANGO_SCRIPT_LATIN));
    vogue_cairo_context_set_font_options (context, font_options);
    mini_font = vogue_font_map_load_font (fontmap, context, desc);

    g_object_unref (context);
    g_object_unref (fontmap);
  }

  vogue_font_description_free (desc);
  cairo_font_options_destroy (font_options);


  scaled_mini_font = vogue_cairo_font_get_scaled_font ((VogueCairoFont *) mini_font);
  if (G_UNLIKELY (scaled_mini_font == NULL || cairo_scaled_font_status (scaled_mini_font) != CAIRO_STATUS_SUCCESS))
    return NULL;

  for (i = 0 ; i < 16 ; i++)
    {
      cairo_text_extents_t extents;

      c[0] = hexdigits[i];
      cairo_scaled_font_text_extents (scaled_mini_font, c, &extents);
      width = MAX (width, extents.width);
      height = MAX (height, extents.height);
    }

  cairo_scaled_font_extents (scaled_font, &font_extents);
  if (font_extents.ascent + font_extents.descent <= 0)
    {
      font_extents.ascent = PANGO_UNKNOWN_GLYPH_HEIGHT;
      font_extents.descent = 0;
    }

  pad = (font_extents.ascent + font_extents.descent) / 43;
  pad = MIN (pad, mini_size);

  hbi = g_slice_new (VogueCairoFontHexBoxInfo);
  hbi->font = (VogueCairoFont *) mini_font;
  hbi->rows = rows;

  hbi->digit_width  = width;
  hbi->digit_height = height;

  hbi->pad_x = pad;
  hbi->pad_y = pad;

  if (is_hinted)
    {
      hbi->digit_width  = HINT_X (hbi->digit_width);
      hbi->digit_height = HINT_Y (hbi->digit_height);
      hbi->pad_x = HINT_X (hbi->pad_x);
      hbi->pad_y = HINT_Y (hbi->pad_y);
    }

  hbi->line_width = MIN (hbi->pad_x, hbi->pad_y);

  hbi->box_height = 3 * hbi->pad_y + rows * (hbi->pad_y + hbi->digit_height);

  if (rows == 1 || hbi->box_height <= font_extents.ascent)
    {
      hbi->box_descent = 2 * hbi->pad_y;
    }
  else if (hbi->box_height <= font_extents.ascent + font_extents.descent - 2 * hbi->pad_y)
    {
      hbi->box_descent = 2 * hbi->pad_y + hbi->box_height - font_extents.ascent;
    }
  else
    {
      hbi->box_descent = font_extents.descent * hbi->box_height /
			 (font_extents.ascent + font_extents.descent);
    }
  if (is_hinted)
    {
       hbi->box_descent = HINT_Y (hbi->box_descent);
    }

  cf_priv->hbi = hbi;
  return hbi;
}

static void
_vogue_cairo_font_hex_box_info_destroy (VogueCairoFontHexBoxInfo *hbi)
{
  if (hbi)
    {
      g_object_unref (hbi->font);
      g_slice_free (VogueCairoFontHexBoxInfo, hbi);
    }
}

VogueCairoFontHexBoxInfo *
_vogue_cairo_font_get_hex_box_info (VogueCairoFont *cfont)
{
  VogueCairoFontPrivate *cf_priv = PANGO_CAIRO_FONT_PRIVATE (cfont);

  return _vogue_cairo_font_private_get_hex_box_info (cf_priv);
}

void
_vogue_cairo_font_private_initialize (VogueCairoFontPrivate      *cf_priv,
				      VogueCairoFont             *cfont,
				      VogueGravity                gravity,
				      const cairo_font_options_t *font_options,
				      const VogueMatrix          *vogue_ctm,
				      const cairo_matrix_t       *font_matrix)
{
  cairo_matrix_t gravity_matrix;

  cf_priv->cfont = cfont;
  cf_priv->gravity = gravity;

  cf_priv->data = _vogue_cairo_font_private_scaled_font_data_create (); 

  /* first apply gravity rotation, then font_matrix, such that
   * vertical italic text comes out "correct".  we don't do anything
   * like baseline adjustment etc though.  should be specially
   * handled when we support italic correction. */
  cairo_matrix_init_rotate(&gravity_matrix,
			   vogue_gravity_to_rotation (cf_priv->gravity));
  cairo_matrix_multiply (&cf_priv->data->font_matrix,
			 font_matrix,
			 &gravity_matrix);

  if (vogue_ctm)
    cairo_matrix_init (&cf_priv->data->ctm,
		       vogue_ctm->xx,
		       vogue_ctm->yx,
		       vogue_ctm->xy,
		       vogue_ctm->yy,
		       0., 0.);
  else
    cairo_matrix_init_identity (&cf_priv->data->ctm);

  cf_priv->data->options = cairo_font_options_copy (font_options);
  cf_priv->is_hinted = cairo_font_options_get_hint_metrics (font_options) != CAIRO_HINT_METRICS_OFF;

  cf_priv->scaled_font = NULL;
  cf_priv->hbi = NULL;
  cf_priv->glyph_extents_cache = NULL;
  cf_priv->metrics_by_lang = NULL;
}

static void
free_metrics_info (VogueCairoFontMetricsInfo *info)
{
  vogue_font_metrics_unref (info->metrics);
  g_slice_free (VogueCairoFontMetricsInfo, info);
}

void
_vogue_cairo_font_private_finalize (VogueCairoFontPrivate *cf_priv)
{
  _vogue_cairo_font_private_scaled_font_data_destroy (cf_priv->data);

  if (cf_priv->scaled_font)
    cairo_scaled_font_destroy (cf_priv->scaled_font);
  cf_priv->scaled_font = NULL;

  _vogue_cairo_font_hex_box_info_destroy (cf_priv->hbi);
  cf_priv->hbi = NULL;

  if (cf_priv->glyph_extents_cache)
    g_free (cf_priv->glyph_extents_cache);
  cf_priv->glyph_extents_cache = NULL;

  g_slist_foreach (cf_priv->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (cf_priv->metrics_by_lang);
  cf_priv->metrics_by_lang = NULL;
}

gboolean
_vogue_cairo_font_private_is_metrics_hinted (VogueCairoFontPrivate *cf_priv)
{
  return cf_priv->is_hinted;
}

static void
get_space_extents (VogueCairoFontPrivate *cf_priv,
                   VogueRectangle        *ink_rect,
                   VogueRectangle        *logical_rect)
{
  const char hexdigits[] = "0123456789ABCDEF";
  char c[2] = {0, 0};
  int i;
  double hex_width;
  int width;

  /* we don't render missing spaces as hex boxes,
   * so come up with some width to use. For lack
   * of anything better, use average hex digit width.
   */

  hex_width = 0;
  for (i = 0 ; i < 16 ; i++)
    {
      cairo_text_extents_t extents;

      c[0] = hexdigits[i];
      cairo_scaled_font_text_extents (cf_priv->scaled_font, c, &extents);
      hex_width += extents.width;
    }
  width = vogue_units_from_double (hex_width / 16);

  if (ink_rect)
    {
      ink_rect->x = ink_rect->y = ink_rect->height = 0;
      ink_rect->width = width;
    }
  if (logical_rect)
    {
      *logical_rect = cf_priv->font_extents;
      logical_rect->width = width;
    }
}

static void
_vogue_cairo_font_private_get_glyph_extents_missing (VogueCairoFontPrivate *cf_priv,
						     VogueGlyph             glyph,
						     VogueRectangle        *ink_rect,
						     VogueRectangle        *logical_rect)
{
  VogueCairoFontHexBoxInfo *hbi;
  gunichar ch;
  gint rows, cols;

  ch = glyph & ~PANGO_GLYPH_UNKNOWN_FLAG;

  if (ch == 0x20 || ch == 0x2423)
    {
      get_space_extents (cf_priv, ink_rect, logical_rect);
      return;
    }

  hbi = _vogue_cairo_font_private_get_hex_box_info (cf_priv);
  if (!hbi)
    {
      vogue_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  if (G_UNLIKELY (glyph == PANGO_GLYPH_INVALID_INPUT || ch > 0x10FFFF))
    {
      rows = hbi->rows;
      cols = 1;
    }
  else if (vogue_get_ignorable_size (ch, &rows, &cols))
    {
      /* We special-case ignorables when rendering hex boxes */
    }
  else
    {
      rows = hbi->rows;
      cols = ((glyph & ~PANGO_GLYPH_UNKNOWN_FLAG) > 0xffff ? 6 : 4) / rows;
    }

  if (ink_rect)
    {
      ink_rect->x = PANGO_SCALE * hbi->pad_x;
      ink_rect->y = PANGO_SCALE * (hbi->box_descent - hbi->box_height);
      ink_rect->width = PANGO_SCALE * (3 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      ink_rect->height = PANGO_SCALE * hbi->box_height;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = PANGO_SCALE * (hbi->box_descent - (hbi->box_height + hbi->pad_y));
      logical_rect->width = PANGO_SCALE * (5 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      logical_rect->height = PANGO_SCALE * (hbi->box_height + 2 * hbi->pad_y);
    }
}

#define GLYPH_CACHE_NUM_ENTRIES 256 /* should be power of two */
#define GLYPH_CACHE_MASK (GLYPH_CACHE_NUM_ENTRIES - 1)
/* An entry in the fixed-size cache for the glyph->extents mapping.
 * The cache is indexed by the lower N bits of the glyph (see
 * GLYPH_CACHE_NUM_ENTRIES).  For scripts with few glyphs,
 * this should provide pretty much instant lookups.
 */
struct _VogueCairoFontGlyphExtentsCacheEntry
{
  VogueGlyph     glyph;
  int            width;
  VogueRectangle ink_rect;
};

static gboolean
_vogue_cairo_font_private_glyph_extents_cache_init (VogueCairoFontPrivate *cf_priv)
{
  cairo_scaled_font_t *scaled_font = _vogue_cairo_font_private_get_scaled_font (cf_priv);
  cairo_font_extents_t font_extents;

  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return FALSE;

  cairo_scaled_font_extents (scaled_font, &font_extents);

  cf_priv->font_extents.x = 0;
  cf_priv->font_extents.width = 0;
  cf_priv->font_extents.height = vogue_units_from_double (font_extents.ascent + font_extents.descent);
  switch (cf_priv->gravity)
    {
      default:
      case PANGO_GRAVITY_AUTO:
      case PANGO_GRAVITY_SOUTH:
	cf_priv->font_extents.y = - vogue_units_from_double (font_extents.ascent);
	break;
      case PANGO_GRAVITY_NORTH:
	cf_priv->font_extents.y = - vogue_units_from_double (font_extents.descent);
	break;
      case PANGO_GRAVITY_EAST:
      case PANGO_GRAVITY_WEST:
	{
	  int ascent = vogue_units_from_double (font_extents.ascent + font_extents.descent) / 2;
	  if (cf_priv->is_hinted)
	    ascent = PANGO_UNITS_ROUND (ascent);
	  cf_priv->font_extents.y = - ascent;
	}
    }

  cf_priv->glyph_extents_cache = g_new0 (VogueCairoFontGlyphExtentsCacheEntry, GLYPH_CACHE_NUM_ENTRIES);
  /* Make sure all cache entries are invalid initially */
  cf_priv->glyph_extents_cache[0].glyph = 1; /* glyph 1 cannot happen in bucket 0 */

  return TRUE;
}


/* Fills in the glyph extents cache entry
 */
static void
compute_glyph_extents (VogueCairoFontPrivate  *cf_priv,
		       VogueGlyph              glyph,
		       VogueCairoFontGlyphExtentsCacheEntry *entry)
{
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;

  cairo_glyph.index = glyph;
  cairo_glyph.x = 0;
  cairo_glyph.y = 0;

  cairo_scaled_font_glyph_extents (_vogue_cairo_font_private_get_scaled_font (cf_priv),
				   &cairo_glyph, 1, &extents);

  entry->glyph = glyph;
  entry->width = vogue_units_from_double (extents.x_advance);
  entry->ink_rect.x = vogue_units_from_double (extents.x_bearing);
  entry->ink_rect.y = vogue_units_from_double (extents.y_bearing);
  entry->ink_rect.width = vogue_units_from_double (extents.width);
  entry->ink_rect.height = vogue_units_from_double (extents.height);
}

static VogueCairoFontGlyphExtentsCacheEntry *
_vogue_cairo_font_private_get_glyph_extents_cache_entry (VogueCairoFontPrivate  *cf_priv,
							 VogueGlyph              glyph)
{
  VogueCairoFontGlyphExtentsCacheEntry *entry;
  guint idx;

  idx = glyph & GLYPH_CACHE_MASK;
  entry = cf_priv->glyph_extents_cache + idx;

  if (entry->glyph != glyph)
    {
      compute_glyph_extents (cf_priv, glyph, entry);
    }

  return entry;
}

void
_vogue_cairo_font_private_get_glyph_extents (VogueCairoFontPrivate *cf_priv,
					     VogueGlyph             glyph,
					     VogueRectangle        *ink_rect,
					     VogueRectangle        *logical_rect)
{
  VogueCairoFontGlyphExtentsCacheEntry *entry;

  if (!cf_priv ||
      (cf_priv->glyph_extents_cache == NULL &&
       !_vogue_cairo_font_private_glyph_extents_cache_init (cf_priv)))
    {
      /* Get generic unknown-glyph extents. */
      vogue_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->y = ink_rect->width = ink_rect->height = 0;
      if (logical_rect)
	*logical_rect = cf_priv->font_extents;
      return;
    }
  else if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
      _vogue_cairo_font_private_get_glyph_extents_missing(cf_priv, glyph, ink_rect, logical_rect);
      return;
    }

  entry = _vogue_cairo_font_private_get_glyph_extents_cache_entry (cf_priv, glyph);

  if (ink_rect)
    *ink_rect = entry->ink_rect;
  if (logical_rect)
    {
      *logical_rect = cf_priv->font_extents;
      logical_rect->width = entry->width;
    }
}
