/* vogue-list.c: List all fonts
 *
 * Copyright (C) 2018 Google
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
 *
 * Google Author(s): Behdad Esfahbod
 */

#include "config.h"
#include <vogue/voguecairo.h>
#include <hb-ot.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <locale.h>

/* FIXME: This doesn't work if the font has an avar table */
static float
denorm_coord (hb_ot_var_axis_info_t *axis, int coord)
{
  float r = coord / 16384.0;

  if (coord < 0)
    return axis->default_value + r * (axis->default_value - axis->min_value);
  else
    return axis->default_value + r * (axis->max_value - axis->default_value);
}

int
main (int    argc,
      char **argv)
{
  gboolean opt_verbose = FALSE;
  gboolean opt_metrics = FALSE;
  gboolean opt_variations = FALSE;
  GOptionEntry entries[] = {
    {"verbose", 0, 0, G_OPTION_ARG_NONE,    &opt_verbose, "Print verbose information", NULL },
    {"metrics", 0, 0, G_OPTION_ARG_NONE,    &opt_metrics, "Print font metrics", NULL },
    {"variations", 0, 0, G_OPTION_ARG_NONE,    &opt_variations, "Print font variations", NULL },
    { NULL, }
  };
  GOptionContext *context;
  VogueContext *ctx;
  VogueFontMap *fontmap;
  VogueFontFamily **families;
  int n_families;
  int i, j;
  int width;
  GError *error = NULL;

  g_set_prgname ("vogue-list");
  setlocale (LC_ALL, "");

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      if (error != NULL)
        g_printerr ("%s\n", error->message);
      else
        g_printerr ("Option parse error\n");
      exit (1);
    }

  g_option_context_free (context);

  /* Use VogueCairo to get default fontmap so it works on every platform. */
  fontmap = vogue_cairo_font_map_get_default ();
  ctx = vogue_font_map_create_context (fontmap);

  if (opt_verbose)
    g_print ("Using %s\n\n", G_OBJECT_TYPE_NAME (fontmap));

  vogue_font_map_list_families (fontmap, &families, &n_families);
  for (i = 0; i < n_families; i++)
    {
      VogueFontFace **faces;
      int n_faces;
      const char *kind;

      const char *family_name = vogue_font_family_get_name (families[i]);
      if (vogue_font_family_is_monospace (families[i]))
        {
          if (vogue_font_family_is_variable (families[i]))
            kind = "(monospace, variable)";
          else
            kind = "(monospace)";
        }
      else
        {
          if (vogue_font_family_is_variable (families[i]))
            kind = "(variable)";
          else
            kind = "";
        }

      g_print ("%s %s\n", family_name, kind);

      vogue_font_family_list_faces (families[i], &faces, &n_faces);

      width = 0;
      for (j = 0; j < n_faces; j++)
	{
	  const char *face_name = vogue_font_face_get_face_name (faces[j]);
	  gboolean is_synth = vogue_font_face_is_synthesized (faces[j]);
	  const char *synth_str = is_synth ? "*" : "";
          width = MAX (width, strlen (synth_str) + strlen (face_name));
        }

      for (j = 0; j < n_faces; j++)
	{
	  const char *face_name = vogue_font_face_get_face_name (faces[j]);
	  gboolean is_synth = vogue_font_face_is_synthesized (faces[j]);
	  const char *synth_str = is_synth ? "*" : "";
	  VogueFontDescription *desc = vogue_font_face_describe (faces[j]);
	  char *desc_str = vogue_font_description_to_string (desc);

	  g_print ("  %s%s: %*s%s\n", synth_str, face_name,
                   width - (int)strlen (face_name) - (int)strlen (synth_str), "", desc_str);

          if (opt_metrics)
            {
              VogueFontMetrics *metrics;

              vogue_font_description_set_absolute_size (desc, 10 * PANGO_SCALE);
              metrics = vogue_context_get_metrics (ctx, desc, vogue_language_from_string ("en-us"));
              g_print ("    (a %d d %d h %d cw %d dw %d u %d %d s %d %d)\n",
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
            }

          if (opt_variations &&
              vogue_font_family_is_variable (families[i]))
            {
              VogueFont *font;
              hb_font_t *hb_font;
              const int *coords;
              unsigned int length;

              vogue_font_description_set_absolute_size (desc, 10 * PANGO_SCALE);

              font = vogue_context_load_font (ctx, desc);
              hb_font = vogue_font_get_hb_font (font);
              coords = hb_font_get_var_coords_normalized (hb_font, &length);
              if (coords)
                {
                  hb_face_t *hb_face = hb_font_get_face (hb_font);
                  hb_ot_var_axis_info_t *axes;
                  unsigned int n_axes;
                  int i;

                  axes = g_new (hb_ot_var_axis_info_t, length);
                  n_axes = length;

                  hb_ot_var_get_axis_infos (hb_face, 0, &n_axes, axes);

                  for (i = 0; i < length; i++)
                    {
                      char name[20];
                      unsigned int namelen = 20;

                      hb_ot_name_get_utf8 (hb_face, axes[i].name_id, HB_LANGUAGE_INVALID, &namelen, name);

                      g_print ("    %s: %g (%g - %g, %g)\n",
                               name,
                               denorm_coord (&axes[i], coords[i]),
                               axes[i].min_value,
                               axes[i].max_value,
                               axes[i].default_value);
                    }
                  g_free (axes);
                }
            }

	  g_free (desc_str);
	  vogue_font_description_free (desc);
	}

      g_free (faces);
    }
  g_free (families);
  g_object_unref (fontmap);

  return 0;
}
