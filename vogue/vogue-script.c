/* Vogue
 * vogue-script.c: Script tag handling
 *
 * Copyright (C) 2002 Red Hat Software
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
 * Implementation of vogue_script_iter is derived from ICU:
 *
 *  icu/sources/common/usc_impl.c
 *
 **********************************************************************
 *   Copyright (C) 1999-2002, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * of the copyright holder.
 */

/**
 * SECTION:scripts
 * @short_description:Identifying writing systems and languages
 * @title:Scripts and Languages
 *
 * The functions in this section are used to identify the writing
 * system, or <firstterm>script</firstterm> of individual characters
 * and of ranges within a larger text string.
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>

#include "vogue-script.h"
#include "vogue-script-private.h"

/**
 * vogue_script_for_unichar:
 * @ch: a Unicode character
 *
 * Looks up the script for a particular character (as defined by
 * Unicode Standard Annex \#24). No check is made for @ch being a
 * valid Unicode character; if you pass in invalid character, the
 * result is undefined.
 *
 * Note that while the return type of this function is declared
 * as VogueScript, as of Vogue 1.18, this function simply returns
 * the return value of g_unichar_get_script(). Callers must be
 * prepared to handle unknown values.
 *
 * Return value: the #VogueScript for the character.
 *
 * Since: 1.4
 * Deprecated: 1.44. Use g_unichar_get_script()
 **/
VogueScript
vogue_script_for_unichar (gunichar ch)
{
  return (VogueScript)g_unichar_get_script (ch);
}

/**********************************************************************/

static VogueScriptIter *vogue_script_iter_copy (VogueScriptIter *iter);

G_DEFINE_BOXED_TYPE (VogueScriptIter,
                     vogue_script_iter,
                     vogue_script_iter_copy,
                     vogue_script_iter_free)

VogueScriptIter *
_vogue_script_iter_init (VogueScriptIter *iter,
	                 const char      *text,
			 int              length)
{
  iter->text_start = text;
  if (length >= 0)
    iter->text_end = text + length;
  else
    iter->text_end = text + strlen (text);

  iter->script_start = text;
  iter->script_end = text;
  iter->script_code = PANGO_SCRIPT_COMMON;

  iter->paren_sp = -1;

  vogue_script_iter_next (iter);

  return iter;
}

/**
 * vogue_script_iter_new:
 * @text: a UTF-8 string
 * @length: length of @text, or -1 if @text is nul-terminated.
 *
 * Create a new #VogueScriptIter, used to break a string of
 * Unicode text into runs by Unicode script. No copy is made of
 * @text, so the caller needs to make sure it remains valid until
 * the iterator is freed with vogue_script_iter_free().
 *
 * Return value: the new script iterator, initialized
 *  to point at the first range in the text, which should be
 *  freed with vogue_script_iter_free(). If the string is
 *  empty, it will point at an empty range.
 *
 * Since: 1.4
 **/
VogueScriptIter *
vogue_script_iter_new (const char *text,
		       int         length)
{
  return _vogue_script_iter_init (g_slice_new (VogueScriptIter), text, length);
}

static VogueScriptIter *
vogue_script_iter_copy (VogueScriptIter *iter)
{
  return g_slice_dup (VogueScriptIter, iter);
}

void
_vogue_script_iter_fini (VogueScriptIter *iter)
{
}

/**
 * vogue_script_iter_free:
 * @iter: a #VogueScriptIter
 *
 * Frees a #VogueScriptIter created with vogue_script_iter_new().
 *
 * Since: 1.4
 **/
void
vogue_script_iter_free (VogueScriptIter *iter)
{
  _vogue_script_iter_fini (iter);
  g_slice_free (VogueScriptIter, iter);
}

/**
 * vogue_script_iter_get_range:
 * @iter: a #VogueScriptIter
 * @start: (out) (allow-none): location to store start position of the range, or %NULL
 * @end: (out) (allow-none): location to store end position of the range, or %NULL
 * @script: (out) (allow-none): location to store script for range, or %NULL
 *
 * Gets information about the range to which @iter currently points.
 * The range is the set of locations p where *start <= p < *end.
 * (That is, it doesn't include the character stored at *end)
 *
 * Note that while the type of the @script argument is declared
 * as VogueScript, as of Vogue 1.18, this function simply returns
 * GUnicodeScript values. Callers must be prepared to handle unknown
 * values.
 *
 * Since: 1.4
 **/
void
vogue_script_iter_get_range (VogueScriptIter  *iter,
                             const char      **start,
                             const char      **end,
                             VogueScript      *script)
{
  if (start)
    *start = iter->script_start;
  if (end)
    *end = iter->script_end;
  if (script)
    *script = iter->script_code;
}

static const gunichar paired_chars[] = {
  0x0028, 0x0029, /* ascii paired punctuation */
  0x003c, 0x003e,
  0x005b, 0x005d,
  0x007b, 0x007d,
  0x00ab, 0x00bb, /* guillemets */
  0x0f3a, 0x0f3b, /* tibetan */
  0x0f3c, 0x0f3d,
  0x169b, 0x169c, /* ogham */
  0x2018, 0x2019, /* general punctuation */
  0x201c, 0x201d,
  0x2039, 0x203a,
  0x2045, 0x2046,
  0x207d, 0x207e,
  0x208d, 0x208e,
  0x27e6, 0x27e7, /* math */
  0x27e8, 0x27e9,
  0x27ea, 0x27eb,
  0x27ec, 0x27ed,
  0x27ee, 0x27ef,
  0x2983, 0x2984,
  0x2985, 0x2986,
  0x2987, 0x2988,
  0x2989, 0x298a,
  0x298b, 0x298c,
  0x298d, 0x298e,
  0x298f, 0x2990,
  0x2991, 0x2992,
  0x2993, 0x2994,
  0x2995, 0x2996,
  0x2997, 0x2998,
  0x29fc, 0x29fd,
  0x2e02, 0x2e03,
  0x2e04, 0x2e05,
  0x2e09, 0x2e0a,
  0x2e0c, 0x2e0d,
  0x2e1c, 0x2e1d,
  0x2e20, 0x2e21,
  0x2e22, 0x2e23,
  0x2e24, 0x2e25,
  0x2e26, 0x2e27,
  0x2e28, 0x2e29,
  0x3008, 0x3009, /* chinese paired punctuation */
  0x300a, 0x300b,
  0x300c, 0x300d,
  0x300e, 0x300f,
  0x3010, 0x3011,
  0x3014, 0x3015,
  0x3016, 0x3017,
  0x3018, 0x3019,
  0x301a, 0x301b,
  0xfe59, 0xfe5a,
  0xfe5b, 0xfe5c,
  0xfe5d, 0xfe5e,
  0xff08, 0xff09,
  0xff3b, 0xff3d,
  0xff5b, 0xff5d,
  0xff5f, 0xff60,
  0xff62, 0xff63
};

static int
get_pair_index (gunichar ch)
{
  int lower = 0;
  int upper = G_N_ELEMENTS (paired_chars) - 1;

  while (lower <= upper)
    {
      int mid = (lower + upper) / 2;

      if (ch < paired_chars[mid])
	upper = mid - 1;
      else if (ch > paired_chars[mid])
	lower = mid + 1;
      else
	return mid;
    }

  return -1;
}

/* duplicated in vogue-language.c */
#define REAL_SCRIPT(script) \
  ((script) > PANGO_SCRIPT_INHERITED && (script) != PANGO_SCRIPT_UNKNOWN)

#define SAME_SCRIPT(script1, script2) \
  (!REAL_SCRIPT (script1) || !REAL_SCRIPT (script2) || (script1) == (script2))

#define IS_OPEN(pair_index) (((pair_index) & 1) == 0)

/**
 * vogue_script_iter_next:
 * @iter: a #VogueScriptIter
 *
 * Advances a #VogueScriptIter to the next range. If @iter
 * is already at the end, it is left unchanged and %FALSE
 * is returned.
 *
 * Return value: %TRUE if @iter was successfully advanced.
 *
 * Since: 1.4
 **/
gboolean
vogue_script_iter_next (VogueScriptIter *iter)
{
  int start_sp;

  if (iter->script_end == iter->text_end)
    return FALSE;

  start_sp = iter->paren_sp;
  iter->script_code = PANGO_SCRIPT_COMMON;
  iter->script_start = iter->script_end;

  for (; iter->script_end < iter->text_end; iter->script_end = g_utf8_next_char (iter->script_end))
    {
      gunichar ch = g_utf8_get_char (iter->script_end);
      VogueScript sc;
      int pair_index;

      sc = (VogueScript)g_unichar_get_script (ch);
      if (sc != PANGO_SCRIPT_COMMON)
	pair_index = -1;
      else
	pair_index = get_pair_index (ch);

      /*
       * Paired character handling:
       *
       * if it's an open character, push it onto the stack.
       * if it's a close character, find the matching open on the
       * stack, and use that script code. Any non-matching open
       * characters above it on the stack will be poped.
       */
      if (pair_index >= 0)
	{
	  if (IS_OPEN (pair_index))
	    {
	      /*
	       * If the paren stack is full, empty it. This
	       * means that deeply nested paired punctuation
	       * characters will be ignored, but that's an unusual
	       * case, and it's better to ignore them than to
	       * write off the end of the stack...
	       */
	      if (++iter->paren_sp >= PAREN_STACK_DEPTH)
		iter->paren_sp = 0;

	      iter->paren_stack[iter->paren_sp].pair_index = pair_index;
	      iter->paren_stack[iter->paren_sp].script_code = iter->script_code;
	    }
	  else if (iter->paren_sp >= 0)
	    {
	      int pi = pair_index & ~1;

	      while (iter->paren_sp >= 0 && iter->paren_stack[iter->paren_sp].pair_index != pi)
		iter->paren_sp--;

	      if (iter->paren_sp < start_sp)
		start_sp = iter->paren_sp;

	      if (iter->paren_sp >= 0)
		sc = iter->paren_stack[iter->paren_sp].script_code;
	    }
	}

      if (SAME_SCRIPT (iter->script_code, sc))
	{
	  if (!REAL_SCRIPT (iter->script_code) && REAL_SCRIPT (sc))
	    {
	      iter->script_code = sc;

	      /*
	       * now that we have a final script code, fix any open
	       * characters we pushed before we knew the script code.
	       */
	      while (start_sp < iter->paren_sp)
		iter->paren_stack[++start_sp].script_code = iter->script_code;
	    }

	  /*
	   * if this character is a close paired character,
	   * pop it from the stack
	   */
	  if (pair_index >= 0 && !IS_OPEN (pair_index) && iter->paren_sp >= 0)
	    {
	      iter->paren_sp--;

	      if (iter->paren_sp < start_sp)
		start_sp = iter->paren_sp;
	    }
	}
      else
	{
	  /* Different script, we're done */
	  break;
	}
    }

  return TRUE;
}

/**********************************************************
 * End of code from ICU
 **********************************************************/
