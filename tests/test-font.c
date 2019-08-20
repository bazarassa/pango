/* Vogue
 * test-font.c: Test VogueFontDescription
 *
 * Copyright (C) 2014 Red Hat, Inc
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

#include <glib.h>
#include <string.h>
#include <locale.h>

#include <vogue/voguecairo.h>

static VogueContext *context;

static void
test_parse (void)
{
  VogueFontDescription *desc;

  desc = vogue_font_description_from_string ("Cantarell 14");

  g_assert_cmpstr (vogue_font_description_get_family (desc), ==, "Cantarell");
  g_assert (!vogue_font_description_get_size_is_absolute (desc));
  g_assert_cmpint (vogue_font_description_get_size (desc), ==, 14 * PANGO_SCALE);
  g_assert_cmpint (vogue_font_description_get_style (desc), ==, PANGO_STYLE_NORMAL);
  g_assert_cmpint (vogue_font_description_get_variant (desc), ==, PANGO_VARIANT_NORMAL);
  g_assert_cmpint (vogue_font_description_get_weight (desc), ==, PANGO_WEIGHT_NORMAL);
  g_assert_cmpint (vogue_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  g_assert_cmpint (vogue_font_description_get_gravity (desc), ==, PANGO_GRAVITY_SOUTH);
  g_assert_cmpint (vogue_font_description_get_set_fields (desc), ==, PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT | PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH | PANGO_FONT_MASK_SIZE);

  vogue_font_description_free (desc); 

  desc = vogue_font_description_from_string ("Sans Bold Italic Condensed 22.5px");

  g_assert_cmpstr (vogue_font_description_get_family (desc), ==, "Sans");
  g_assert (vogue_font_description_get_size_is_absolute (desc)); 
  g_assert_cmpint (vogue_font_description_get_size (desc), ==, 225 * PANGO_SCALE / 10);
  g_assert_cmpint (vogue_font_description_get_style (desc), ==, PANGO_STYLE_ITALIC);
  g_assert_cmpint (vogue_font_description_get_variant (desc), ==, PANGO_VARIANT_NORMAL); 
  g_assert_cmpint (vogue_font_description_get_weight (desc), ==, PANGO_WEIGHT_BOLD);
  g_assert_cmpint (vogue_font_description_get_stretch (desc), ==, PANGO_STRETCH_CONDENSED); 
  g_assert_cmpint (vogue_font_description_get_gravity (desc), ==, PANGO_GRAVITY_SOUTH);  g_assert_cmpint (vogue_font_description_get_set_fields (desc), ==, PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT | PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH | PANGO_FONT_MASK_SIZE);

  vogue_font_description_free (desc); 
}

static void
test_roundtrip (void)
{
  VogueFontDescription *desc;
 gchar *str;

  desc = vogue_font_description_from_string ("Cantarell 14");
  str = vogue_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  vogue_font_description_free (desc); 
  g_free (str);

  desc = vogue_font_description_from_string ("Sans Bold Italic Condensed 22.5px");
  str = vogue_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Sans Bold Italic Condensed 22.5px");
  vogue_font_description_free (desc); 
  g_free (str);
}

static void
test_variation (void)
{
  VogueFontDescription *desc1;
  VogueFontDescription *desc2;
  gchar *str;

  desc1 = vogue_font_description_from_string ("Cantarell 14");
  g_assert (desc1 != NULL);
  g_assert ((vogue_font_description_get_set_fields (desc1) & PANGO_FONT_MASK_VARIATIONS) == 0);
  g_assert (vogue_font_description_get_variations (desc1) == NULL);

  str = vogue_font_description_to_string (desc1);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  g_free (str);

  desc2 = vogue_font_description_from_string ("Cantarell 14 @wght=100,wdth=235");
  g_assert (desc2 != NULL);
  g_assert ((vogue_font_description_get_set_fields (desc2) & PANGO_FONT_MASK_VARIATIONS) != 0);
  g_assert_cmpstr (vogue_font_description_get_variations (desc2), ==, "wght=100,wdth=235");

  str = vogue_font_description_to_string (desc2);
  g_assert_cmpstr (str, ==, "Cantarell 14 @wght=100,wdth=235");
  g_free (str);

  g_assert (!vogue_font_description_equal (desc1, desc2));

  vogue_font_description_set_variations (desc1, "wght=100,wdth=235");
  g_assert ((vogue_font_description_get_set_fields (desc1) & PANGO_FONT_MASK_VARIATIONS) != 0);
  g_assert_cmpstr (vogue_font_description_get_variations (desc1), ==, "wght=100,wdth=235");

  g_assert (vogue_font_description_equal (desc1, desc2));

  vogue_font_description_free (desc1);
  vogue_font_description_free (desc2);
}

static void
test_metrics (void)
{
  VogueFontDescription *desc;
  VogueFontMetrics *metrics;
  char *str;


  if (strcmp (G_OBJECT_TYPE_NAME (vogue_context_get_font_map (context)), "VogueCairoWin32FontMap") == 0)
    desc = vogue_font_description_from_string ("Verdana 11");
  else
    desc = vogue_font_description_from_string ("Cantarell 11");

  str = vogue_font_description_to_string (desc);

  metrics = vogue_context_get_metrics (context, desc, vogue_language_get_default ());

  g_test_message ("%s metrics\n"
                  "\tascent %d\n"
                  "\tdescent %d\n"
                  "\theight %d\n"
                  "\tchar width %d\n"
                  "\tdigit width %d\n"
                  "\tunderline position %d\n"
                  "\tunderline thickness %d\n"
                  "\tstrikethrough position %d\n"
                  "\tstrikethrough thickness %d\n",
                  str,
                  vogue_font_metrics_get_ascent (metrics),
                  vogue_font_metrics_get_descent (metrics),
                  vogue_font_metrics_get_height (metrics),
                  vogue_font_metrics_get_approximate_char_width (metrics),
                  vogue_font_metrics_get_approximate_digit_width (metrics),
                  vogue_font_metrics_get_underline_position (metrics),
                  vogue_font_metrics_get_underline_thickness (metrics),
                  vogue_font_metrics_get_strikethrough_position (metrics),
                  vogue_font_metrics_get_strikethrough_thickness (metrics));

  vogue_font_metrics_unref (metrics);
  g_free (str);
  vogue_font_description_free (desc);
}

static void
test_extents (void)
{
  char *str = "Composer";
  GList *items;
  VogueItem *item;
  VogueGlyphString *glyphs;
  VogueRectangle ink, log;
  VogueContext *context;

  context = vogue_font_map_create_context (vogue_cairo_font_map_get_default ());
  vogue_context_set_font_description (context, vogue_font_description_from_string ("Cantarell 11"));

  items = vogue_itemize (context, str, 0, strlen (str), NULL, NULL);
  glyphs = vogue_glyph_string_new ();
  item = items->data;
  vogue_shape (str, strlen (str), &item->analysis, glyphs);
  vogue_glyph_string_extents (glyphs, item->analysis.font, &ink, &log);

  g_assert_cmpint (ink.width, >=, 0);
  g_assert_cmpint (ink.height, >=, 0);
  g_assert_cmpint (log.width, >=, 0);
  g_assert_cmpint (log.height, >=, 0);

  vogue_glyph_string_free (glyphs);
  g_list_free_full (items, (GDestroyNotify)vogue_item_free);
  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  g_setenv ("LC_ALL", "C", TRUE);
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  context = vogue_font_map_create_context (vogue_cairo_font_map_get_default ());

  g_test_add_func ("/vogue/font/metrics", test_metrics);
  g_test_add_func ("/vogue/fontdescription/parse", test_parse);
  g_test_add_func ("/vogue/fontdescription/roundtrip", test_roundtrip);
  g_test_add_func ("/vogue/fontdescription/variation", test_variation);
  g_test_add_func ("/vogue/font/extents", test_extents);

  return g_test_run ();
}
