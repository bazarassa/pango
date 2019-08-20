/* Vogue
 * vogue-emoji-private.h: Emoji handling, private definitions
 *
 * Copyright (C) 2017 Google, Inc.
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

#ifndef __PANGO_EMOJI_PRIVATE_H__
#define __PANGO_EMOJI_PRIVATE_H__

#include <glib.h>

gboolean
_vogue_Is_Emoji_Base_Character (gunichar ch);

gboolean
_vogue_Is_Emoji_Extended_Pictographic (gunichar ch);

typedef struct _VogueEmojiIter VogueEmojiIter;

struct _VogueEmojiIter
{
  const gchar *text_start;
  const gchar *text_end;
  const gchar *start;
  const gchar *end;
  gboolean is_emoji;

  unsigned char *types;
  unsigned int n_chars;
  unsigned int cursor;
};

VogueEmojiIter *
_vogue_emoji_iter_init (VogueEmojiIter *iter,
			const char     *text,
			int             length);

gboolean
_vogue_emoji_iter_next (VogueEmojiIter *iter);

void
_vogue_emoji_iter_fini (VogueEmojiIter *iter);

#endif /* __PANGO_EMOJI_PRIVATE_H__ */
