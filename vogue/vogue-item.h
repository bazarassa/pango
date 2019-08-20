/* Vogue
 * vogue-item.h: Structure for storing run information
 *
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __PANGO_ITEM_H__
#define __PANGO_ITEM_H__

#include <vogue/vogue-types.h>
#include <vogue/vogue-attributes.h>

G_BEGIN_DECLS

typedef struct _VogueAnalysis VogueAnalysis;
typedef struct _VogueItem VogueItem;

/**
 * PANGO_ANALYSIS_FLAG_CENTERED_BASELINE:
 *
 * Whether the segment should be shifted to center around the baseline.
 * Used in vertical writing directions mostly.
 *
 * Since: 1.16
 */
#define PANGO_ANALYSIS_FLAG_CENTERED_BASELINE (1 << 0)

/**
 * PANGO_ANALYSIS_FLAG_IS_ELLIPSIS:
 *
 * This flag is used to mark runs that hold ellipsized text,
 * in an ellipsized layout.
 *
 * Since: 1.36.7
 */
#define PANGO_ANALYSIS_FLAG_IS_ELLIPSIS (1 << 1)

/**
 * PANGO_ANALYSIS_FLAG_NEED_HYPHEN:
 *
 * This flag tells Vogue to add a hyphen at the end of the
 * run during shaping.
 *
 * Since: 1.44
 */
#define PANGO_ANALYSIS_FLAG_NEED_HYPHEN (1 << 2)

/**
 * VogueAnalysis:
 * @shape_engine: unused
 * @lang_engine: unused
 * @font: the font for this segment.
 * @level: the bidirectional level for this segment.
 * @gravity: the glyph orientation for this segment (A #VogueGravity).
 * @flags: boolean flags for this segment (Since: 1.16).
 * @script: the detected script for this segment (A #VogueScript) (Since: 1.18).
 * @language: the detected language for this segment.
 * @extra_attrs: extra attributes for this segment.
 *
 * The #VogueAnalysis structure stores information about
 * the properties of a segment of text.
 */
struct _VogueAnalysis
{
  VogueEngineShape *shape_engine;
  VogueEngineLang  *lang_engine;
  VogueFont *font;

  guint8 level;
  guint8 gravity;
  guint8 flags;

  guint8 script;
  VogueLanguage *language;

  GSList *extra_attrs;
};

/**
 * VogueItem:
 * @offset: byte offset of the start of this item in text.
 * @length: length of this item in bytes.
 * @num_chars: number of Unicode characters in the item.
 * @analysis: analysis results for the item.
 *
 * The #VogueItem structure stores information about a segment of text.
 */
struct _VogueItem
{
  gint offset;
  gint length;
  gint num_chars;
  VogueAnalysis analysis;
};

#define PANGO_TYPE_ITEM (vogue_item_get_type ())

PANGO_AVAILABLE_IN_ALL
GType vogue_item_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
VogueItem *vogue_item_new   (void);
PANGO_AVAILABLE_IN_ALL
VogueItem *vogue_item_copy  (VogueItem  *item);
PANGO_AVAILABLE_IN_ALL
void       vogue_item_free  (VogueItem  *item);
PANGO_AVAILABLE_IN_ALL
VogueItem *vogue_item_split (VogueItem  *orig,
			     int         split_index,
			     int         split_offset);
PANGO_AVAILABLE_IN_1_44
void       vogue_item_apply_attrs (VogueItem         *item,
                                   VogueAttrIterator *iter);

G_END_DECLS

#endif /* __PANGO_ITEM_H__ */
