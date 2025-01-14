/* Vogue
 * test-layout.c: Test Vogue Layout
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

#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#include "config.h"
#include <vogue/voguecairo.h>
#include "test-common.h"


static VogueContext *context;


static const gchar *
enum_value_nick (GType type, gint value)
{
  GEnumClass *eclass;
  GEnumValue *ev;

  eclass = g_type_class_ref (type);
  ev = g_enum_get_value (eclass, value);
  g_type_class_unref (eclass);

  if (ev)
    return ev->value_nick;
  else
    return "?";
}

static const gchar *
direction_name (VogueDirection dir)
{
  return enum_value_nick (PANGO_TYPE_DIRECTION, dir);
}

static const gchar *
gravity_name (VogueGravity gravity)
{
  return enum_value_nick (PANGO_TYPE_GRAVITY, gravity);
}

static const gchar *
script_name (VogueScript script)
{
  return enum_value_nick (PANGO_TYPE_SCRIPT, script);
}

static gchar *
font_name (VogueFont *font)
{
  VogueFontDescription *desc;
  gchar *name;

  desc = vogue_font_describe (font);
  name = vogue_font_description_to_string (desc);
  vogue_font_description_free (desc);

  return name;
}

static void
dump_lines (VogueLayout *layout, GString *string)
{
  VogueLayoutIter *iter;
  const gchar *text;
  gint index, index2;
  gboolean has_more;
  gchar *char_str;
  gint i;
  VogueLayoutLine *line;

  text = vogue_layout_get_text (layout);
  iter = vogue_layout_get_iter (layout);

  has_more = TRUE;
  index = vogue_layout_iter_get_index (iter);
  index2 = 0;
  i = 0;
  while (has_more)
    {
      line = vogue_layout_iter_get_line (iter);
      has_more = vogue_layout_iter_next_line (iter);
      i++;

      if (has_more)
        {
          index2 = vogue_layout_iter_get_index (iter);
          char_str = g_strndup (text + index, index2 - index);
        }
      else
        {
          char_str = g_strdup (text + index);
        }

      g_string_append_printf (string, "i=%d, index=%d, paragraph-start=%d, dir=%s '%s'\n",
                              i, index, line->is_paragraph_start, direction_name (line->resolved_dir),
                              char_str);
      g_free (char_str);

      index = index2;
    }
  vogue_layout_iter_free (iter);
}

static void
dump_runs (VogueLayout *layout, GString *string)
{
  VogueLayoutIter *iter;
  VogueLayoutRun *run;
  VogueItem *item;
  const gchar *text;
  gint index;
  gboolean has_more;
  gchar *char_str;
  gint i;
  gchar *font = 0;

  text = vogue_layout_get_text (layout);
  iter = vogue_layout_get_iter (layout);

  has_more = TRUE;
  i = 0;
  while (has_more)
    {
      run = vogue_layout_iter_get_run (iter);
      index = vogue_layout_iter_get_index (iter);
      has_more = vogue_layout_iter_next_run (iter);
      i++;

      if (run)
        {
          item = ((VogueGlyphItem*)run)->item;
          char_str = g_strndup (text + item->offset, item->length);
          font = font_name (item->analysis.font);
          g_string_append_printf (string, "i=%d, index=%d, chars=%d, level=%d, gravity=%s, flags=%d, font=%s, script=%s, language=%s, '%s'\n",
                                  i, index, item->num_chars, item->analysis.level,
                                  gravity_name (item->analysis.gravity),
                                  item->analysis.flags,
                                  "OMITTED", /* for some reason, this fails on build.gnome.org, so leave it out */
                                  script_name (item->analysis.script),
                                  vogue_language_to_string (item->analysis.language),
                                  char_str);
          print_attributes (item->analysis.extra_attrs, string);
          g_free (font);
          g_free (char_str);
        }
      else
        {
          g_string_append_printf (string, "i=%d, index=%d, no run, line end\n",
                                  i, index);
        }
    }
  vogue_layout_iter_free (iter);
}

static void
parse_params (const gchar        *str,
              gint               *width,
              gint               *ellipsize_at,
              VogueEllipsizeMode *ellipsize,
              VogueWrapMode      *wrap)
{
  gchar **strings;
  gchar **str2;
  gint i;
  GEnumClass *eclass;
  GEnumValue *ev;

  strings = g_strsplit (str, ",", -1);
  for (i = 0; strings[i]; i++)
    {
      str2 = g_strsplit (strings[i], "=", -1);
      if (strcmp (str2[0], "width") == 0)
        *width = (gint) g_ascii_strtoll (str2[1], NULL, 10);
      else if (strcmp (str2[0], "ellipsize-at") == 0)
        *ellipsize_at = (gint) g_ascii_strtoll (str2[1], NULL, 10);
      else if (strcmp (str2[0], "ellipsize") == 0)
        {
          eclass = g_type_class_ref (PANGO_TYPE_ELLIPSIZE_MODE);
          ev = g_enum_get_value_by_name (eclass, str2[1]);
          if (!ev)
            ev = g_enum_get_value_by_nick (eclass, str2[1]);
          if (ev)
            *ellipsize = ev->value;
          g_type_class_unref (eclass);
        }
      else if (strcmp (str2[0], "wrap") == 0)
        {
          eclass = g_type_class_ref (PANGO_TYPE_WRAP_MODE);
          ev = g_enum_get_value_by_name (eclass, str2[1]);
          if (!ev)
            ev = g_enum_get_value_by_nick (eclass, str2[1]);
          if (ev)
            *wrap = ev->value;
          g_type_class_unref (eclass);
        }
      g_strfreev (str2);
    }
  g_strfreev (strings);
}

static void
test_file (const gchar *filename, GString *string)
{
  gchar *contents;
  gchar *markup;
  gsize  length;
  GError *error = NULL;
  VogueLayout *layout;
  gchar *p;
  gint width = 0;
  gint ellipsize_at = 0;
  VogueEllipsizeMode ellipsize = PANGO_ELLIPSIZE_NONE;
  VogueWrapMode wrap = PANGO_WRAP_WORD;
  VogueFontDescription *desc;

  if (!g_file_get_contents (filename, &contents, &length, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      g_error_free (error);
      return;
    }

  p = strchr (contents, '\n');
  g_assert (p);
  markup = p + 1;
  *p = '\0';
  length = strlen (markup);

  parse_params (contents, &width, &ellipsize_at, &ellipsize, &wrap);

  layout = vogue_layout_new (context);
  desc = vogue_font_description_from_string ("Cantarell 11");
  vogue_layout_set_font_description (layout, desc);
  vogue_font_description_free (desc);

  vogue_layout_set_markup (layout, markup, length);
  g_free (contents);

  if (width != 0)
    vogue_layout_set_width (layout, width * PANGO_SCALE);
  vogue_layout_set_ellipsize (layout, ellipsize);
  vogue_layout_set_wrap (layout, wrap);

  g_string_append (string, vogue_layout_get_text (layout));
  g_string_append (string, "\n--- parameters\n\n");
  g_string_append_printf (string, "wrapped: %d\n", vogue_layout_is_wrapped (layout));
  g_string_append_printf (string, "ellipsized: %d\n", vogue_layout_is_ellipsized (layout));
  g_string_append_printf (string, "lines: %d\n", vogue_layout_get_line_count (layout));
  if (width != 0)
    g_string_append_printf (string, "width: %d\n", vogue_layout_get_width (layout));
  g_string_append (string, "\n--- attributes\n\n");
  print_attr_list (vogue_layout_get_attributes (layout), string);
  g_string_append (string, "\n--- lines\n\n");
  dump_lines (layout, string);
  g_string_append (string, "\n--- runs\n\n");
  dump_runs (layout, string);

  g_object_unref (layout);
}

static gchar *
get_expected_filename (const gchar *filename)
{
  gchar *f, *p, *expected;

  f = g_strdup (filename);
  p = strstr (f, ".markup");
  if (p)
    *p = 0;
  expected = g_strconcat (f, ".expected", NULL);

  g_free (f);

  return expected;
}

static void
test_layout (gconstpointer d)
{
  const gchar *filename = d;
  gchar *expected_file;
  GError *error = NULL;
  GString *dump;
  gchar *diff;

  expected_file = get_expected_filename (filename);

  dump = g_string_sized_new (0);

  test_file (filename, dump);

  diff = diff_with_file (expected_file, dump->str, dump->len, &error);
  g_assert_no_error (error);

  if (diff && diff[0])
    {
      g_printerr ("Contents don't match expected contents:\n%s", diff);
      g_test_fail ();
      g_free (diff);
    }

  g_string_free (dump, TRUE);
  g_free (expected_file);
}

int
main (int argc, char *argv[])
{
  GDir *dir;
  GError *error = NULL;
  const gchar *name;
  gchar *path;

  g_setenv ("LC_ALL", "en_US.UTF-8", TRUE);
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  context = vogue_font_map_create_context (vogue_cairo_font_map_get_default ());

  /* allow to easily generate expected output for new test cases */
  if (argc > 1)
    {
      GString *string;

      string = g_string_sized_new (0);
      test_file (argv[1], string);
      g_print ("%s", string->str);

      return 0;
    }

  path = g_test_build_filename (G_TEST_DIST, "layouts", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);
  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (!strstr (name, "markup"))
        continue;

      path = g_strdup_printf ("/layout/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "layouts", name, NULL),
                                 test_layout, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
