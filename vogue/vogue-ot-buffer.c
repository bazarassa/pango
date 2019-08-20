/* Vogue
 * vogue-ot-buffer.c: Buffer of glyphs for shaping/positioning
 *
 * Copyright (C) 2004 Red Hat Software
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

#include "vogue-ot-private.h"

/**
 * vogue_ot_buffer_new
 * @font: a #VogueFcFont
 *
 * Creates a new #VogueOTBuffer for the given OpenType font.
 *
 * Return value: the newly allocated #VogueOTBuffer, which should
 *               be freed with vogue_ot_buffer_destroy().
 *
 * Since: 1.4
 **/
VogueOTBuffer *
vogue_ot_buffer_new (VogueFcFont *font)
{
  VogueOTBuffer *buffer = g_slice_new (VogueOTBuffer);

  buffer->buffer = hb_buffer_create ();

  return buffer;
}

/**
 * vogue_ot_buffer_destroy
 * @buffer: a #VogueOTBuffer
 *
 * Destroys a #VogueOTBuffer and free all associated memory.
 *
 * Since: 1.4
 **/
void
vogue_ot_buffer_destroy (VogueOTBuffer *buffer)
{
  hb_buffer_destroy (buffer->buffer);
  g_slice_free (VogueOTBuffer, buffer);
}

/**
 * vogue_ot_buffer_clear
 * @buffer: a #VogueOTBuffer
 *
 * Empties a #VogueOTBuffer, make it ready to add glyphs to.
 *
 * Since: 1.4
 **/
void
vogue_ot_buffer_clear (VogueOTBuffer *buffer)
{
  hb_buffer_reset (buffer->buffer);
}

/**
 * vogue_ot_buffer_add_glyph
 * @buffer: a #VogueOTBuffer
 * @glyph: the glyph index to add, like a #VogueGlyph
 * @properties: the glyph properties
 * @cluster: the cluster that this glyph belongs to
 *
 * Appends a glyph to a #VogueOTBuffer, with @properties identifying which
 * features should be applied on this glyph.  See vogue_ot_ruleset_add_feature().
 *
 * Since: 1.4
 **/
void
vogue_ot_buffer_add_glyph (VogueOTBuffer *buffer,
			   guint          glyph,
			   guint          properties,
			   guint          cluster)
{
  hb_buffer_add (buffer->buffer, glyph, cluster);
}

/**
 * vogue_ot_buffer_set_rtl
 * @buffer: a #VogueOTBuffer
 * @rtl: %TRUE for right-to-left text
 *
 * Sets whether glyphs will be rendered right-to-left.  This setting
 * is needed for proper horizontal positioning of right-to-left scripts.
 *
 * Since: 1.4
 **/
void
vogue_ot_buffer_set_rtl (VogueOTBuffer *buffer,
			 gboolean       rtl)
{
  hb_buffer_set_direction (buffer->buffer, rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
}

/**
 * vogue_ot_buffer_set_zero_width_marks
 * @buffer: a #VogueOTBuffer
 * @zero_width_marks: %TRUE if characters with a mark class should
 *  be forced to zero width.
 *
 * Sets whether characters with a mark class should be forced to zero width.
 * This setting is needed for proper positioning of Arabic accents,
 * but will produce incorrect results with standard OpenType Indic
 * fonts.
 *
 * Since: 1.6
 **/
void
vogue_ot_buffer_set_zero_width_marks (VogueOTBuffer     *buffer,
				      gboolean           zero_width_marks)
{
}

/**
 * vogue_ot_buffer_get_glyphs
 * @buffer: a #VogueOTBuffer
 * @glyphs: (array length=n_glyphs) (out) (optional): location to
 *   store the array of glyphs, or %NULL
 * @n_glyphs: (out) (optional): location to store the number of
 *   glyphs, or %NULL
 *
 * Gets the glyph array contained in a #VogueOTBuffer.  The glyphs are
 * owned by the buffer and should not be freed, and are only valid as long
 * as buffer is not modified.
 *
 * Since: 1.4
 **/
void
vogue_ot_buffer_get_glyphs (const VogueOTBuffer  *buffer,
			    VogueOTGlyph        **glyphs,
			    int                  *n_glyphs)
{
  if (glyphs)
    *glyphs = (VogueOTGlyph *) hb_buffer_get_glyph_infos (buffer->buffer, NULL);

  if (n_glyphs)
    *n_glyphs = hb_buffer_get_length (buffer->buffer);
}

/**
 * vogue_ot_buffer_output
 * @buffer: a #VogueOTBuffer
 * @glyphs: a #VogueGlyphString
 *
 * Exports the glyphs in a #VogueOTBuffer into a #VogueGlyphString.  This is
 * typically used after the OpenType layout processing is over, to convert the
 * resulting glyphs into a generic Vogue glyph string.
 *
 * Since: 1.4
 **/
void
vogue_ot_buffer_output (const VogueOTBuffer *buffer,
			VogueGlyphString    *glyphs)
{
  unsigned int i;
  int last_cluster;

  unsigned int num_glyphs;
  hb_buffer_t *hb_buffer = buffer->buffer;
  hb_glyph_info_t *hb_glyph;
  hb_glyph_position_t *hb_position;

  if (HB_DIRECTION_IS_BACKWARD (hb_buffer_get_direction (buffer->buffer)))
    hb_buffer_reverse (buffer->buffer);

  /* Copy glyphs into output glyph string */
  num_glyphs = hb_buffer_get_length (hb_buffer);
  hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, NULL);
  hb_position = hb_buffer_get_glyph_positions (hb_buffer, NULL);
  vogue_glyph_string_set_size (glyphs, num_glyphs);
  last_cluster = -1;
  for (i = 0; i < num_glyphs; i++)
    {
      glyphs->glyphs[i].glyph = hb_glyph->codepoint;
      glyphs->log_clusters[i] = hb_glyph->cluster;
      glyphs->glyphs[i].attr.is_cluster_start = glyphs->log_clusters[i] != last_cluster;
      last_cluster = glyphs->log_clusters[i];

      glyphs->glyphs[i].geometry.width = hb_position->x_advance;
      glyphs->glyphs[i].geometry.x_offset = hb_position->x_offset;
      glyphs->glyphs[i].geometry.y_offset = hb_position->y_offset;

      hb_glyph++;
      hb_position++;
    }

  if (HB_DIRECTION_IS_BACKWARD (hb_buffer_get_direction (buffer->buffer)))
    hb_buffer_reverse (buffer->buffer);
}
