/* Vogue
 * glyphstring.c:
 *
 * Copyright (C) 1999 Red Hat Software
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
#include <glib.h>
#include "vogue-glyph.h"
#include "vogue-font.h"
#include "vogue-impl-utils.h"

/**
 * vogue_glyph_string_new:
 *
 * Create a new #VogueGlyphString.
 *
 * Return value: the newly allocated #VogueGlyphString, which
 *               should be freed with vogue_glyph_string_free().
 */
VogueGlyphString *
vogue_glyph_string_new (void)
{
  VogueGlyphString *string = g_slice_new (VogueGlyphString);

  string->num_glyphs = 0;
  string->space = 0;
  string->glyphs = NULL;
  string->log_clusters = NULL;

  return string;
}

/**
 * vogue_glyph_string_set_size:
 * @string:    a #VogueGlyphString.
 * @new_len:   the new length of the string.
 *
 * Resize a glyph string to the given length.
 */
void
vogue_glyph_string_set_size (VogueGlyphString *string, gint new_len)
{
  g_return_if_fail (new_len >= 0);

  while (new_len > string->space)
    {
      if (string->space == 0)
	{
	  string->space = 4;
	}
      else
	{
	  const guint max_space =
	    MIN (G_MAXINT, G_MAXSIZE / MAX (sizeof(VogueGlyphInfo), sizeof(gint)));

	  guint more_space = (guint)string->space * 2;

	  if (more_space > max_space)
	    {
	      more_space = max_space;

	      if ((guint)new_len > max_space)
		{
		  g_error ("%s: failed to allocate glyph string of length %i\n",
			   G_STRLOC, new_len);
		}
	    }

	  string->space = more_space;
	}
    }

  string->glyphs = g_realloc (string->glyphs, string->space * sizeof (VogueGlyphInfo));
  string->log_clusters = g_realloc (string->log_clusters, string->space * sizeof (gint));
  string->num_glyphs = new_len;
}

G_DEFINE_BOXED_TYPE (VogueGlyphString, vogue_glyph_string,
                     vogue_glyph_string_copy,
                     vogue_glyph_string_free);

/**
 * vogue_glyph_string_copy:
 * @string: (nullable): a #VogueGlyphString, may be %NULL
 *
 * Copy a glyph string and associated storage.
 *
 * Return value: (nullable): the newly allocated #VogueGlyphString,
 *               which should be freed with vogue_glyph_string_free(),
 *               or %NULL if @string was %NULL.
 */
VogueGlyphString *
vogue_glyph_string_copy (VogueGlyphString *string)
{
  VogueGlyphString *new_string;

  if (string == NULL)
    return NULL;
  
  new_string = g_slice_new (VogueGlyphString);

  *new_string = *string;

  new_string->glyphs = g_memdup (string->glyphs,
				 string->space * sizeof (VogueGlyphInfo));
  new_string->log_clusters = g_memdup (string->log_clusters,
				       string->space * sizeof (gint));

  return new_string;
}

/**
 * vogue_glyph_string_free:
 * @string: (nullable): a #VogueGlyphString, may be %NULL
 *
 * Free a glyph string and associated storage.
 */
void
vogue_glyph_string_free (VogueGlyphString *string)
{
  if (string == NULL)
    return;

  g_free (string->glyphs);
  g_free (string->log_clusters);
  g_slice_free (VogueGlyphString, string);
}

/**
 * vogue_glyph_string_extents_range:
 * @glyphs:   a #VogueGlyphString
 * @start:    start index
 * @end:      end index (the range is the set of bytes with
	      indices such that start <= index < end)
 * @font:     a #VogueFont
 * @ink_rect: (out caller-allocates) (optional): rectangle used to
 *            store the extents of the glyph string range as drawn or
 *            %NULL to indicate that the result is not needed.
 * @logical_rect: (out caller-allocates) (optional): rectangle used to
 *            store the logical extents of the glyph string range or
 *            %NULL to indicate that the result is not needed.
 *
 * Computes the extents of a sub-portion of a glyph string. The extents are
 * relative to the start of the glyph string range (the origin of their
 * coordinate system is at the start of the range, not at the start of the entire
 * glyph string).
 **/
void
vogue_glyph_string_extents_range (VogueGlyphString *glyphs,
				  int               start,
				  int               end,
				  VogueFont        *font,
				  VogueRectangle   *ink_rect,
				  VogueRectangle   *logical_rect)
{
  int x_pos = 0;
  int i;

  /* Note that the handling of empty rectangles for ink
   * and logical rectangles is different. A zero-height ink
   * rectangle makes no contribution to the overall ink rect,
   * while a zero-height logical rect still reserves horizontal
   * width. Also, we may return zero-width, positive height
   * logical rectangles, while we'll never do that for the
   * ink rect.
   */
  g_return_if_fail (start <= end);

  if (G_UNLIKELY (!ink_rect && !logical_rect))
    return;

  if (ink_rect)
    {
      ink_rect->x = 0;
      ink_rect->y = 0;
      ink_rect->width = 0;
      ink_rect->height = 0;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = 0;
      logical_rect->width = 0;
      logical_rect->height = 0;
    }

  for (i = start; i < end; i++)
    {
      VogueRectangle glyph_ink;
      VogueRectangle glyph_logical;

      VogueGlyphGeometry *geometry = &glyphs->glyphs[i].geometry;

      vogue_font_get_glyph_extents (font, glyphs->glyphs[i].glyph,
				    ink_rect ? &glyph_ink : NULL,
				    logical_rect ? &glyph_logical : NULL);

      if (ink_rect && glyph_ink.width != 0 && glyph_ink.height != 0)
	{
	  if (ink_rect->width == 0 || ink_rect->height == 0)
	    {
	      ink_rect->x = x_pos + glyph_ink.x + geometry->x_offset;
	      ink_rect->width = glyph_ink.width;
	      ink_rect->y = glyph_ink.y + geometry->y_offset;
	      ink_rect->height = glyph_ink.height;
	    }
	  else
	    {
	      int new_x, new_y;

	      new_x = MIN (ink_rect->x, x_pos + glyph_ink.x + geometry->x_offset);
	      ink_rect->width = MAX (ink_rect->x + ink_rect->width,
				     x_pos + glyph_ink.x + glyph_ink.width + geometry->x_offset) - new_x;
	      ink_rect->x = new_x;

	      new_y = MIN (ink_rect->y, glyph_ink.y + geometry->y_offset);
	      ink_rect->height = MAX (ink_rect->y + ink_rect->height,
				      glyph_ink.y + glyph_ink.height + geometry->y_offset) - new_y;
	      ink_rect->y = new_y;
	    }
	}

      if (logical_rect)
	{
	  logical_rect->width += geometry->width;

	  if (i == start)
	    {
	      logical_rect->y = glyph_logical.y;
	      logical_rect->height = glyph_logical.height;
	    }
	  else
	    {
	      int new_y = MIN (logical_rect->y, glyph_logical.y);
	      logical_rect->height = MAX (logical_rect->y + logical_rect->height,
					  glyph_logical.y + glyph_logical.height) - new_y;
	      logical_rect->y = new_y;
	    }
	}

      x_pos += geometry->width;
    }
}

/**
 * vogue_glyph_string_extents:
 * @glyphs:   a #VogueGlyphString
 * @font:     a #VogueFont
 * @ink_rect: (out) (allow-none): rectangle used to store the extents of the glyph string
 *            as drawn or %NULL to indicate that the result is not needed.
 * @logical_rect: (out) (allow-none): rectangle used to store the logical extents of the
 *            glyph string or %NULL to indicate that the result is not needed.
 *
 * Compute the logical and ink extents of a glyph string. See the documentation
 * for vogue_font_get_glyph_extents() for details about the interpretation
 * of the rectangles.
 *
 * Examples of logical (red) and ink (green) rects:
 *
 * ![](rects1.png) ![](rects2.png)
 */
void
vogue_glyph_string_extents (VogueGlyphString *glyphs,
			    VogueFont        *font,
			    VogueRectangle   *ink_rect,
			    VogueRectangle   *logical_rect)
{
  vogue_glyph_string_extents_range (glyphs, 0, glyphs->num_glyphs,
				    font, ink_rect, logical_rect);
}

/**
 * vogue_glyph_string_get_width:
 * @glyphs:   a #VogueGlyphString
 *
 * Computes the logical width of the glyph string as can also be computed
 * using vogue_glyph_string_extents().  However, since this only computes the
 * width, it's much faster.  This is in fact only a convenience function that
 * computes the sum of geometry.width for each glyph in the @glyphs.
 *
 * Return value: the logical width of the glyph string.
 *
 * Since: 1.14
 */
int
vogue_glyph_string_get_width (VogueGlyphString *glyphs)
{
  int i;
  int width = 0;

  for (i = 0; i < glyphs->num_glyphs; i++)
    width += glyphs->glyphs[i].geometry.width;

  return width;
}

/**
 * vogue_glyph_string_get_logical_widths:
 * @glyphs: a #VogueGlyphString
 * @text: the text corresponding to the glyphs
 * @length: the length of @text, in bytes
 * @embedding_level: the embedding level of the string
 * @logical_widths: (array): an array whose length is the number of
 *                  characters in text (equal to g_utf8_strlen (text,
 *                  length) unless text has NUL bytes) to be filled in
 *                  with the resulting character widths.
 *
 * Given a #VogueGlyphString resulting from vogue_shape() and the corresponding
 * text, determine the screen width corresponding to each character. When
 * multiple characters compose a single cluster, the width of the entire
 * cluster is divided equally among the characters.
 *
 * See also vogue_glyph_item_get_logical_widths().
 **/
void
vogue_glyph_string_get_logical_widths (VogueGlyphString *glyphs,
				       const char       *text,
				       int               length,
				       int               embedding_level,
				       int              *logical_widths)
{
  /* Build a VogueGlyphItem and call the other API */
  VogueItem item = {0, length, vogue_utf8_strlen (text, length),
		    {NULL, NULL, NULL,
		     embedding_level, PANGO_GRAVITY_AUTO, 0,
		     PANGO_SCRIPT_UNKNOWN, NULL,
		     NULL}};
  VogueGlyphItem glyph_item = {&item, glyphs};

  vogue_glyph_item_get_logical_widths (&glyph_item, text, logical_widths);
}

/* The initial implementation here is script independent,
 * but it might actually need to be virtualized into the
 * rendering modules. Otherwise, we probably will end up
 * enforcing unnatural cursor behavior for some languages.
 *
 * The only distinction that Uniscript makes is whether
 * cursor positioning is allowed within clusters or not.
 */

/**
 * vogue_glyph_string_index_to_x:
 * @glyphs:    the glyphs return from vogue_shape()
 * @text:      the text for the run
 * @length:    the number of bytes (not characters) in @text.
 * @analysis:  the analysis information return from vogue_itemize()
 * @index_:    the byte index within @text
 * @trailing:  whether we should compute the result for the beginning (%FALSE)
 *             or end (%TRUE) of the character.
 * @x_pos:     (out): location to store result
 *
 * Converts from character position to x position. (X position
 * is measured from the left edge of the run). Character positions
 * are computed by dividing up each cluster into equal portions.
 */

void
vogue_glyph_string_index_to_x (VogueGlyphString *glyphs,
			       char             *text,
			       int               length,
			       VogueAnalysis    *analysis,
			       int               index,
			       gboolean          trailing,
			       int              *x_pos)
{
  int i;
  int start_xpos = 0;
  int end_xpos = 0;
  int width = 0;

  int start_index = -1;
  int end_index = -1;

  int cluster_chars = 0;
  int cluster_offset = 0;

  char *p;

  g_return_if_fail (glyphs != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (length == 0 || text != NULL);

  if (!x_pos) /* Allow the user to do the useless */
    return;

  if (glyphs->num_glyphs == 0)
    {
      *x_pos = 0;
      return;
    }

  /* Calculate the starting and ending character positions
   * and x positions for the cluster
   */
  if (analysis->level % 2) /* Right to left */
    {
      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	width += glyphs->glyphs[i].geometry.width;

      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	{
	  if (glyphs->log_clusters[i] > index)
	    {
	      end_index = glyphs->log_clusters[i];
	      end_xpos = width;
	      break;
	    }

	  if (glyphs->log_clusters[i] != start_index)
	    {
	      start_index = glyphs->log_clusters[i];
	      start_xpos = width;
	    }

	  width -= glyphs->glyphs[i].geometry.width;
	}
    }
  else /* Left to right */
    {
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  if (glyphs->log_clusters[i] > index)
	    {
	      end_index = glyphs->log_clusters[i];
	      end_xpos = width;
	      break;
	    }

	  if (glyphs->log_clusters[i] != start_index)
	    {
	      start_index = glyphs->log_clusters[i];
	      start_xpos = width;
	    }

	  width += glyphs->glyphs[i].geometry.width;
	}
    }

  if (end_index == -1)
    {
      end_index = length;
      end_xpos = (analysis->level % 2) ? 0 : width;
    }

  /* Calculate offset of character within cluster */

  p = text + start_index;
  while (p < text + end_index)
    {
      if (p < text + index)
	cluster_offset++;
      cluster_chars++;
      p = g_utf8_next_char (p);
    }

  if (trailing)
    cluster_offset += 1;

  if (G_UNLIKELY (!cluster_chars)) /* pedantic */
    {
      *x_pos = start_xpos;
      return;
    }

  *x_pos = ((cluster_chars - cluster_offset) * start_xpos +
	    cluster_offset * end_xpos) / cluster_chars;
}

/**
 * vogue_glyph_string_x_to_index:
 * @glyphs:    the glyphs returned from vogue_shape()
 * @text:      the text for the run
 * @length:    the number of bytes (not characters) in text.
 * @analysis:  the analysis information return from vogue_itemize()
 * @x_pos:     the x offset (in Vogue units)
 * @index_:    (out): location to store calculated byte index within @text
 * @trailing:  (out): location to store a boolean indicating
 *             whether the user clicked on the leading or trailing
 *             edge of the character.
 *
 * Convert from x offset to character position. Character positions
 * are computed by dividing up each cluster into equal portions.
 * In scripts where positioning within a cluster is not allowed
 * (such as Thai), the returned value may not be a valid cursor
 * position; the caller must combine the result with the logical
 * attributes for the text to compute the valid cursor position.
 */
void
vogue_glyph_string_x_to_index (VogueGlyphString *glyphs,
			       char             *text,
			       int               length,
			       VogueAnalysis    *analysis,
			       int               x_pos,
			       int              *index,
			       gboolean         *trailing)
{
  int i;
  int start_xpos = 0;
  int end_xpos = 0;
  int width = 0;

  int start_index = -1;
  int end_index = -1;

  int cluster_chars = 0;
  char *p;

  gboolean found = FALSE;

  /* Find the cluster containing the position */

  width = 0;

  if (analysis->level % 2) /* Right to left */
    {
      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	width += glyphs->glyphs[i].geometry.width;

      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	{
	  if (glyphs->log_clusters[i] != start_index)
	    {
	      if (found)
		{
		  end_index = glyphs->log_clusters[i];
		  end_xpos = width;
		  break;
		}
	      else
		{
		  start_index = glyphs->log_clusters[i];
		  start_xpos = width;
		}
	    }

	  width -= glyphs->glyphs[i].geometry.width;

	  if (width <= x_pos && x_pos < width + glyphs->glyphs[i].geometry.width)
	    found = TRUE;
	}
    }
  else /* Left to right */
    {
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  if (glyphs->log_clusters[i] != start_index)
	    {
	      if (found)
		{
		  end_index = glyphs->log_clusters[i];
		  end_xpos = width;
		  break;
		}
	      else
		{
		  start_index = glyphs->log_clusters[i];
		  start_xpos = width;
		}
	    }

	  if (width <= x_pos && x_pos < width + glyphs->glyphs[i].geometry.width)
	    found = TRUE;

	  width += glyphs->glyphs[i].geometry.width;
	}
    }

  if (end_index == -1)
    {
      end_index = length;
      end_xpos = (analysis->level % 2) ? 0 : width;
    }

  /* Calculate number of chars within cluster */
  p = text + start_index;
  while (p < text + end_index)
    {
      p = g_utf8_next_char (p);
      cluster_chars++;
    }

  if (start_xpos == end_xpos)
    {
      if (index)
	*index = start_index;
      if (trailing)
	*trailing = FALSE;
    }
  else
    {
      double cp = ((double)(x_pos - start_xpos) * cluster_chars) / (end_xpos - start_xpos);

      /* LTR and right-to-left have to be handled separately
       * here because of the edge condition when we are exactly
       * at a pixel boundary; end_xpos goes with the next
       * character for LTR, with the previous character for RTL.
       */
      if (start_xpos < end_xpos) /* Left-to-right */
	{
	  if (index)
	    {
	      char *p = text + start_index;
	      int i = 0;

	      while (i + 1 <= cp)
		{
		  p = g_utf8_next_char (p);
		  i++;
		}

	      *index = (p - text);
	    }

	  if (trailing)
	    *trailing = (cp - (int)cp >= 0.5) ? TRUE : FALSE;
	}
      else /* Right-to-left */
	{
	  if (index)
	    {
	      char *p = text + start_index;
	      int i = 0;

	      while (i + 1 < cp)
		{
		  p = g_utf8_next_char (p);
		  i++;
		}

	      *index = (p - text);
	    }

	  if (trailing)
	    {
	      double cp_flip = cluster_chars - cp;
	      *trailing = (cp_flip - (int)cp_flip >= 0.5) ? FALSE : TRUE;
	    }
	}
    }
}
