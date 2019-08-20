/* Vogue
 * vogue-fontset.c:
 *
 * Copyright (C) 2001 Red Hat Software
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

/*
 * VogueFontset
 */

#include "vogue-types.h"
#include "vogue-font-private.h"
#include "vogue-fontset-private.h"
#include "vogue-impl-utils.h"

static VogueFontMetrics *vogue_fontset_real_get_metrics (VogueFontset      *fontset);


G_DEFINE_ABSTRACT_TYPE (VogueFontset, vogue_fontset, G_TYPE_OBJECT);

static void
vogue_fontset_init (VogueFontset *self)
{
}

static void
vogue_fontset_class_init (VogueFontsetClass *class)
{
  class->get_metrics = vogue_fontset_real_get_metrics;
}


/**
 * vogue_fontset_get_font:
 * @fontset: a #VogueFontset
 * @wc: a Unicode character
 *
 * Returns the font in the fontset that contains the best glyph for the
 * Unicode character @wc.
 *
 * Return value: (transfer full): a #VogueFont. The caller must call
 *          g_object_unref when finished with the font.
 **/
VogueFont *
vogue_fontset_get_font (VogueFontset  *fontset,
			guint          wc)
{

  g_return_val_if_fail (PANGO_IS_FONTSET (fontset), NULL);

  return PANGO_FONTSET_GET_CLASS (fontset)->get_font (fontset, wc);
}

/**
 * vogue_fontset_get_metrics:
 * @fontset: a #VogueFontset
 *
 * Get overall metric information for the fonts in the fontset.
 *
 * Return value: a #VogueFontMetrics object. The caller must call vogue_font_metrics_unref()
 *   when finished using the object.
 **/
VogueFontMetrics *
vogue_fontset_get_metrics (VogueFontset  *fontset)
{
  g_return_val_if_fail (PANGO_IS_FONTSET (fontset), NULL);

  return PANGO_FONTSET_GET_CLASS (fontset)->get_metrics (fontset);
}

/**
 * vogue_fontset_foreach:
 * @fontset: a #VogueFontset
 * @func: (closure data) (scope call): Callback function
 * @data: (closure): data to pass to the callback function
 *
 * Iterates through all the fonts in a fontset, calling @func for
 * each one. If @func returns %TRUE, that stops the iteration.
 *
 * Since: 1.4
 **/
void
vogue_fontset_foreach (VogueFontset           *fontset,
		       VogueFontsetForeachFunc func,
		       gpointer                data)
{
  g_return_if_fail (PANGO_IS_FONTSET (fontset));
  g_return_if_fail (func != NULL);

  PANGO_FONTSET_GET_CLASS (fontset)->foreach (fontset, func, data);
}

static gboolean
get_first_metrics_foreach (VogueFontset  *fontset,
			   VogueFont     *font,
			   gpointer       data)
{
  VogueFontMetrics *fontset_metrics = data;
  VogueLanguage *language = PANGO_FONTSET_GET_CLASS (fontset)->get_language (fontset);
  VogueFontMetrics *font_metrics = vogue_font_get_metrics (font, language);
  guint save_ref_count;

  /* Initialize the fontset metrics to metrics of the first font in the
   * fontset; saving the refcount and restoring it is a bit of hack but avoids
   * having to update this code for each metrics addition.
   */
  save_ref_count = fontset_metrics->ref_count;
  *fontset_metrics = *font_metrics;
  fontset_metrics->ref_count = save_ref_count;

  vogue_font_metrics_unref (font_metrics);

  return TRUE;			/* Stops iteration */
}

static VogueFontMetrics *
vogue_fontset_real_get_metrics (VogueFontset  *fontset)
{
  VogueFontMetrics *metrics, *raw_metrics;
  const char *sample_str;
  const char *p;
  int count;
  GHashTable *fonts_seen;
  VogueFont *font;
  VogueLanguage *language;

  language = PANGO_FONTSET_GET_CLASS (fontset)->get_language (fontset);
  sample_str = vogue_language_get_sample_string (language);

  count = 0;
  metrics = vogue_font_metrics_new ();
  fonts_seen = g_hash_table_new_full (NULL, NULL, g_object_unref, NULL);

  /* Initialize the metrics from the first font in the fontset */
  vogue_fontset_foreach (fontset, get_first_metrics_foreach, metrics);

  p = sample_str;
  while (*p)
    {
      gunichar wc = g_utf8_get_char (p);
      font = vogue_fontset_get_font (fontset, wc);
      if (font)
	{
	  if (g_hash_table_lookup (fonts_seen, font) == NULL)
	    {
	      raw_metrics = vogue_font_get_metrics (font, language);
	      g_hash_table_insert (fonts_seen, font, font);

	      if (count == 0)
		{
		  metrics->ascent = raw_metrics->ascent;
		  metrics->descent = raw_metrics->descent;
		  metrics->approximate_char_width = raw_metrics->approximate_char_width;
		  metrics->approximate_digit_width = raw_metrics->approximate_digit_width;
		}
	      else
		{
		  metrics->ascent = MAX (metrics->ascent, raw_metrics->ascent);
		  metrics->descent = MAX (metrics->descent, raw_metrics->descent);
		  metrics->approximate_char_width += raw_metrics->approximate_char_width;
		  metrics->approximate_digit_width += raw_metrics->approximate_digit_width;
		}
	      count++;
	      vogue_font_metrics_unref (raw_metrics);
	    }
	  else
	    g_object_unref (font);
	}

      p = g_utf8_next_char (p);
    }

  g_hash_table_destroy (fonts_seen);

  if (count)
    {
      metrics->approximate_char_width /= count;
      metrics->approximate_digit_width /= count;
    }

  return metrics;
}


/*
 * VogueFontsetSimple
 */

#define PANGO_FONTSET_SIMPLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONTSET_SIMPLE, VogueFontsetSimpleClass))
#define PANGO_IS_FONTSET_SIMPLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONTSET_SIMPLE))
#define PANGO_FONTSET_SIMPLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONTSET_SIMPLE, VogueFontsetSimpleClass))

static void              vogue_fontset_simple_finalize     (GObject                 *object);
static VogueFontMetrics *vogue_fontset_simple_get_metrics  (VogueFontset            *fontset);
static VogueLanguage *   vogue_fontset_simple_get_language (VogueFontset            *fontset);
static  VogueFont *      vogue_fontset_simple_get_font     (VogueFontset            *fontset,
							    guint                    wc);
static void              vogue_fontset_simple_foreach      (VogueFontset            *fontset,
							    VogueFontsetForeachFunc  func,
							    gpointer                 data);

struct _VogueFontsetSimple
{
  VogueFontset parent_instance;

  GPtrArray *fonts;
  GPtrArray *coverages;
  VogueLanguage *language;
};

struct _VogueFontsetSimpleClass
{
  VogueFontsetClass parent_class;
};

/**
 * vogue_fontset_simple_new:
 * @language: a #VogueLanguage tag
 *
 * Creates a new #VogueFontsetSimple for the given language.
 *
 * Return value: the newly allocated #VogueFontsetSimple, which should
 *               be freed with g_object_unref().
 **/
VogueFontsetSimple *
vogue_fontset_simple_new (VogueLanguage *language)
{
  VogueFontsetSimple *fontset;

  fontset = g_object_new (PANGO_TYPE_FONTSET_SIMPLE, NULL);
  fontset->language = language;

  return fontset;
}


G_DEFINE_TYPE (VogueFontsetSimple, vogue_fontset_simple, PANGO_TYPE_FONTSET);

static void
vogue_fontset_simple_class_init (VogueFontsetSimpleClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontsetClass *fontset_class = PANGO_FONTSET_CLASS (class);

  object_class->finalize = vogue_fontset_simple_finalize;

  fontset_class->get_font = vogue_fontset_simple_get_font;
  fontset_class->get_metrics = vogue_fontset_simple_get_metrics;
  fontset_class->get_language = vogue_fontset_simple_get_language;
  fontset_class->foreach = vogue_fontset_simple_foreach;
}

static void
vogue_fontset_simple_init (VogueFontsetSimple *fontset)
{
  fontset->fonts = g_ptr_array_new ();
  fontset->coverages = g_ptr_array_new ();
  fontset->language = NULL;
}

static void
vogue_fontset_simple_finalize (GObject *object)
{
  VogueFontsetSimple *fontset = PANGO_FONTSET_SIMPLE (object);
  VogueCoverage *coverage;
  unsigned int i;

  for (i = 0; i < fontset->fonts->len; i++)
    g_object_unref (g_ptr_array_index(fontset->fonts, i));

  g_ptr_array_free (fontset->fonts, TRUE);

  for (i = 0; i < fontset->coverages->len; i++)
    {
      coverage = g_ptr_array_index (fontset->coverages, i);
      if (coverage)
	vogue_coverage_unref (coverage);
    }

  g_ptr_array_free (fontset->coverages, TRUE);

  G_OBJECT_CLASS (vogue_fontset_simple_parent_class)->finalize (object);
}

/**
 * vogue_fontset_simple_append:
 * @fontset: a #VogueFontsetSimple.
 * @font: a #VogueFont.
 *
 * Adds a font to the fontset.
 **/
void
vogue_fontset_simple_append (VogueFontsetSimple *fontset,
			     VogueFont          *font)
{
  g_ptr_array_add (fontset->fonts, font);
  g_ptr_array_add (fontset->coverages, NULL);
}

/**
 * vogue_fontset_simple_size:
 * @fontset: a #VogueFontsetSimple.
 *
 * Returns the number of fonts in the fontset.
 *
 * Return value: the size of @fontset.
 **/
int
vogue_fontset_simple_size (VogueFontsetSimple *fontset)
{
  return fontset->fonts->len;
}

static VogueLanguage *
vogue_fontset_simple_get_language (VogueFontset  *fontset)
{
  VogueFontsetSimple *simple = PANGO_FONTSET_SIMPLE (fontset);

  return simple->language;
}

static VogueFontMetrics *
vogue_fontset_simple_get_metrics (VogueFontset  *fontset)
{
  VogueFontsetSimple *simple = PANGO_FONTSET_SIMPLE (fontset);

  if (simple->fonts->len == 1)
    return vogue_font_get_metrics (PANGO_FONT (g_ptr_array_index(simple->fonts, 0)),
				   simple->language);

  return PANGO_FONTSET_CLASS (vogue_fontset_simple_parent_class)->get_metrics (fontset);
}

static VogueFont *
vogue_fontset_simple_get_font (VogueFontset  *fontset,
			       guint          wc)
{
  VogueFontsetSimple *simple = PANGO_FONTSET_SIMPLE (fontset);
  VogueCoverageLevel best_level = PANGO_COVERAGE_NONE;
  VogueCoverageLevel level;
  VogueFont *font;
  VogueCoverage *coverage;
  int result = -1;
  unsigned int i;

  for (i = 0; i < simple->fonts->len; i++)
    {
      coverage = g_ptr_array_index (simple->coverages, i);

      if (coverage == NULL)
	{
	  font = g_ptr_array_index (simple->fonts, i);

	  coverage = vogue_font_get_coverage (font, simple->language);
	  g_ptr_array_index (simple->coverages, i) = coverage;
	}

      level = vogue_coverage_get (coverage, wc);

      if (result == -1 || level > best_level)
	{
	  result = i;
	  best_level = level;
	  if (level == PANGO_COVERAGE_EXACT)
	    break;
	}
    }

  if (G_UNLIKELY (result == -1))
    return NULL;

  font = g_ptr_array_index(simple->fonts, result);
  return g_object_ref (font);
}

static void
vogue_fontset_simple_foreach (VogueFontset           *fontset,
			      VogueFontsetForeachFunc func,
			      gpointer                data)
{
  VogueFontsetSimple *simple = PANGO_FONTSET_SIMPLE (fontset);
  unsigned int i;

  for (i = 0; i < simple->fonts->len; i++)
    {
      if ((*func) (fontset,
		   g_ptr_array_index (simple->fonts, i),
		   data))
	return;
    }
}
