/* Vogue
 * test-harfbuzz.c: Test Vogue harfbuzz apis
 *
 * Copyright (C) 2019 Red Hat, Inc.
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

#include <vogue/vogue.h>
#include <vogue/voguecairo.h>
#include "test-common.h"

static VogueContext *context;

/* Some basic checks that the hb_font_t returned
 * by vogue_font_get_hb_font is functional
 */
static void
test_hb_font (void)
{
  VogueFontDescription *desc;
  VogueFont *font;
  hb_font_t *hb_font;
  hb_bool_t res;
  hb_codepoint_t glyph;

 if (strcmp (G_OBJECT_TYPE_NAME (vogue_context_get_font_map (context)), "VogueCairoWin32FontMap") == 0)
    desc = vogue_font_description_from_string ("Verdana 11");
  else
    desc = vogue_font_description_from_string ("Cantarell 11");
  font = vogue_context_load_font (context, desc);
  hb_font = vogue_font_get_hb_font (font);

  g_assert (hb_font != NULL);

  res = hb_font_get_nominal_glyph (hb_font, 0x20, &glyph);

  g_assert (res);
  g_assert (glyph != 0);

  g_object_unref (font);
  vogue_font_description_free (desc);
}

int
main (int argc, char *argv[])
{
  VogueFontMap *fontmap;

  fontmap = vogue_cairo_font_map_get_default ();
  context = vogue_font_map_create_context (fontmap);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/harfbuzz/font", test_hb_font);

  return g_test_run ();
}
