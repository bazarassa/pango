/* Vogue
 * testboundaries.c: Test text boundary algorithms
 *
 * Copyright (C) 1999-2000 Red Hat Software
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <vogue/vogue.h>

#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#define CHFORMAT "%0#6x"

/* FIXME for now this just tests that the breaking of some sample
 * text conforms to certain rules and invariants. But eventually
 * we should also have test-result pairs, i.e. a string and some
 * encoding of the correct way to break the string, to check
 * more precisely that things worked
 */


static int offset = 0;
static int line = 0;
static gunichar current_wc = 0;
static const char *line_start = NULL;
static const char *line_end = NULL;

static void fail (const char *format, ...) G_GNUC_PRINTF (1, 2) G_GNUC_NORETURN;
static void fail (const char *format, ...)
{
  char *str;
  char *line_text;

  va_list args;

  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  line_text = g_strndup (line_start, line_end - line_start);

  fprintf (stderr, "line %d offset %d char is " CHFORMAT ": %s\n (line is '%s')\n", line, offset, current_wc, str, line_text);
  g_free (str);
  g_free (line_text);

  exit (1);
}

typedef void (* CharForeachFunc) (gunichar      wc,
				  gunichar      prev_wc,
				  gunichar      next_wc,
				  GUnicodeType  type,
				  GUnicodeType  prev_type,
				  GUnicodeType  next_type,
				  VogueLogAttr *attr,
				  VogueLogAttr *prev_attr,
				  VogueLogAttr *next_attr,
				  gpointer      data);

static void
log_attr_foreach (const char     *text,
		  VogueLogAttr   *attrs,
		  CharForeachFunc func,
		  gpointer        data)
{
  const gchar *next = text;
  gint length = strlen (text);
  const gchar *end = text + length;
  gint i = 0;
  gunichar prev_wc;
  gunichar next_wc;
  GUnicodeType prev_type;
  GUnicodeType next_type;

  if (next == end)
    return;

  offset = 0;
  line = 1;

  prev_type = (GUnicodeType) -1;
  prev_wc = 0;

  next_wc = g_utf8_get_char (next);
  next_type = g_unichar_type (next_wc);

  line_start = text;
  line_end = text;

  while (next_wc != 0)
    {
      GUnicodeType type;
      gunichar wc;

      wc = next_wc;
      type = next_type;

      current_wc = wc;

      next = g_utf8_next_char (next);
      line_end = next;

      if (next >= end)
	next_wc = 0;
      else
	next_wc = g_utf8_get_char (next);

      if (next_wc)
	next_type = g_unichar_type (next_wc);

      (* func) (wc, prev_wc, next_wc,
		type, prev_type, next_type,
		&attrs[i],
		i != 0 ? &attrs[i-1] : NULL,
		next_wc != 0 ? &attrs[i+1] : NULL,
		data);

      prev_type = type;
      prev_wc = wc;
      ++i;
      ++offset;
      if (wc == '\n')
	{
	  ++line;
	  offset = 0;
	  line_start = next;
	  line_end = next;
	}
    }
}

static void
check_line_char (gunichar      wc,
		 gunichar      prev_wc,
		 gunichar      next_wc,
		 GUnicodeType  type,
		 GUnicodeType  prev_type,
		 GUnicodeType  next_type,
		 VogueLogAttr *attr,
		 VogueLogAttr *prev_attr,
		 VogueLogAttr *next_attr,
		 gpointer      data)
{
  GUnicodeBreakType break_type;
  GUnicodeBreakType prev_break_type;

  break_type = g_unichar_break_type (wc);
  if (prev_wc)
    prev_break_type = g_unichar_break_type (prev_wc);
  else
    prev_break_type = G_UNICODE_BREAK_UNKNOWN;

  if (wc == '\n')
    {
      if (prev_wc == '\r')
	{
	  if (attr->is_line_break)
	    fail ("line break between \\r and \\n");
	}

      if (next_attr && !next_attr->is_line_break)
	fail ("no line break after \\n");
    }

  if (attr->is_line_break && prev_wc == 0)
    fail ("first char in string should not be marked as a line break");

  if (break_type == G_UNICODE_BREAK_SPACE)
    {
      if (attr->is_line_break && prev_attr != NULL &&
	  !attr->is_mandatory_break &&
	  !(next_wc && g_unichar_break_type (next_wc) == G_UNICODE_BREAK_COMBINING_MARK))
	fail ("can't break lines before a space unless a mandatory break char precedes it or a combining mark follows; prev char was " CHFORMAT, prev_wc);
    }

  if (attr->is_mandatory_break && !attr->is_line_break)
    fail ("mandatory breaks must also be marked as regular breaks");



  /* FIXME use the break tables from break.c to automatically
   * check invariants for each cell in the table. Shouldn't
   * be that hard to do.
   */

  if (break_type == G_UNICODE_BREAK_OPEN_PUNCTUATION &&
      prev_break_type == G_UNICODE_BREAK_OPEN_PUNCTUATION &&
      attr->is_line_break &&
      !attr->is_mandatory_break)
    fail ("can't break between two open punctuation chars");

  if (break_type == G_UNICODE_BREAK_CLOSE_PUNCTUATION &&
      prev_break_type == G_UNICODE_BREAK_CLOSE_PUNCTUATION &&
      attr->is_line_break &&
      !attr->is_mandatory_break)
    fail ("can't break between two close punctuation chars");

  if (break_type == G_UNICODE_BREAK_QUOTATION &&
      prev_break_type == G_UNICODE_BREAK_ALPHABETIC &&
      attr->is_line_break &&
      !attr->is_mandatory_break)
    fail ("can't break letter-quotemark sequence");
}

static void
check_line_invariants (const char   *text,
		       VogueLogAttr *attrs)
{
  log_attr_foreach (text, attrs, check_line_char, NULL);
}

static void
check_word_invariants (const char   *text,
		       VogueLogAttr *attrs)
{


}

static void
check_sentence_invariants (const char   *text,
			   VogueLogAttr *attrs)
{


}

static void
check_grapheme_invariants (const char   *text,
			   VogueLogAttr *attrs)
{


}

#if 0
static void print_sentences (const char   *text,
			     VogueLogAttr *attrs);
static void
print_sentences (const char   *text,
		 VogueLogAttr *attrs)
{
  const char *p;
  const char *last;
  int i = 0;

  last = text;
  p = text;

  while (*p)
    {
      if (attrs[i].is_sentence_boundary)
	{
	  char *s = g_strndup (last, p - last);
	  printf ("%s\n", s);
	  g_free (s);
	  last = p;
	}

      p = g_utf8_next_char (p);
      ++i;
    }
}
#endif

static void
check_invariants (const char *text)
{
  int len;
  VogueLogAttr *attrs;

  if (!g_utf8_validate (text, -1, NULL))
    fail ("Invalid UTF-8 in test text");

  len = g_utf8_strlen (text, -1);
  attrs = g_new0 (VogueLogAttr, len + 1);

  vogue_get_log_attrs (text,
		       -1,
		       0,
		       vogue_language_from_string ("C"),
		       attrs,
		       len + 1);

  check_line_invariants (text, attrs);
  check_sentence_invariants (text, attrs);
  check_grapheme_invariants (text, attrs);
  check_word_invariants (text, attrs);

#if 0
  print_sentences (text, attrs);
#endif

  g_free (attrs);
}

static void
test_boundaries (void)
{
  gchar *text;
  const gchar *filename;
#if GLIB_CHECK_VERSION(2, 37, 2)
  filename = g_test_get_filename (G_TEST_DIST, "boundaries.utf8", NULL);
#else
  filename = SRCDIR "/boundaries.utf8";
#endif

  g_print ("sample file: %s\n", filename);

  if (!g_file_get_contents (filename, &text, NULL, NULL))
    fail ("Couldn't open sample text file");

  check_invariants (text);

  g_free (text);

  printf ("testboundaries passed\n");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/text/boundaries", test_boundaries);

  return g_test_run ();
}

