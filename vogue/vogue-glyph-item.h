/* Vogue
 * vogue-glyph-item.h: Pair of VogueItem and a glyph string
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
 */

#ifndef __PANGO_GLYPH_ITEM_H__
#define __PANGO_GLYPH_ITEM_H__

#include <vogue/vogue-attributes.h>
#include <vogue/vogue-break.h>
#include <vogue/vogue-item.h>
#include <vogue/vogue-glyph.h>

G_BEGIN_DECLS

/**
 * VogueGlyphItem:
 * @item: corresponding #VogueItem.
 * @glyphs: corresponding #VogueGlyphString.
 *
 * A #VogueGlyphItem is a pair of a #VogueItem and the glyphs
 * resulting from shaping the text corresponding to an item.
 * As an example of the usage of #VogueGlyphItem, the results
 * of shaping text with #VogueLayout is a list of #VogueLayoutLine,
 * each of which contains a list of #VogueGlyphItem.
 */
typedef struct _VogueGlyphItem VogueGlyphItem;

struct _VogueGlyphItem
{
  VogueItem        *item;
  VogueGlyphString *glyphs;
};

/**
 * PANGO_TYPE_GLYPH_ITEM:
 *
 * The #GObject type for #VogueGlyphItem.
 */
#define PANGO_TYPE_GLYPH_ITEM (vogue_glyph_item_get_type ())

PANGO_AVAILABLE_IN_ALL
GType vogue_glyph_item_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_2
VogueGlyphItem *vogue_glyph_item_split        (VogueGlyphItem *orig,
					       const char     *text,
					       int             split_index);
PANGO_AVAILABLE_IN_1_20
VogueGlyphItem *vogue_glyph_item_copy         (VogueGlyphItem *orig);
PANGO_AVAILABLE_IN_1_6
void            vogue_glyph_item_free         (VogueGlyphItem *glyph_item);
PANGO_AVAILABLE_IN_1_2
GSList *        vogue_glyph_item_apply_attrs  (VogueGlyphItem *glyph_item,
					       const char     *text,
					       VogueAttrList  *list);
PANGO_AVAILABLE_IN_1_6
void            vogue_glyph_item_letter_space (VogueGlyphItem *glyph_item,
					       const char     *text,
					       VogueLogAttr   *log_attrs,
					       int             letter_spacing);
PANGO_AVAILABLE_IN_1_26
void 	  vogue_glyph_item_get_logical_widths (VogueGlyphItem *glyph_item,
					       const char     *text,
					       int            *logical_widths);


/**
 * VogueGlyphItemIter:
 *
 * A #VogueGlyphItemIter is an iterator over the clusters in a
 * #VogueGlyphItem.  The <firstterm>forward direction</firstterm> of the
 * iterator is the logical direction of text.  That is, with increasing
 * @start_index and @start_char values.  If @glyph_item is right-to-left
 * (that is, if <literal>@glyph_item->item->analysis.level</literal> is odd),
 * then @start_glyph decreases as the iterator moves forward.  Moreover,
 * in right-to-left cases, @start_glyph is greater than @end_glyph.
 *
 * An iterator should be initialized using either of
 * vogue_glyph_item_iter_init_start() and
 * vogue_glyph_item_iter_init_end(), for forward and backward iteration
 * respectively, and walked over using any desired mixture of
 * vogue_glyph_item_iter_next_cluster() and
 * vogue_glyph_item_iter_prev_cluster().  A common idiom for doing a
 * forward iteration over the clusters is:
 * <programlisting>
 * VogueGlyphItemIter cluster_iter;
 * gboolean have_cluster;
 *
 * for (have_cluster = vogue_glyph_item_iter_init_start (&amp;cluster_iter,
 *                                                       glyph_item, text);
 *      have_cluster;
 *      have_cluster = vogue_glyph_item_iter_next_cluster (&amp;cluster_iter))
 * {
 *   ...
 * }
 * </programlisting>
 *
 * Note that @text is the start of the text for layout, which is then
 * indexed by <literal>@glyph_item->item->offset</literal> to get to the
 * text of @glyph_item.  The @start_index and @end_index values can directly
 * index into @text.  The @start_glyph, @end_glyph, @start_char, and @end_char
 * values however are zero-based for the @glyph_item.  For each cluster, the
 * item pointed at by the start variables is included in the cluster while
 * the one pointed at by end variables is not.
 *
 * None of the members of a #VogueGlyphItemIter should be modified manually.
 *
 * Since: 1.22
 */
typedef struct _VogueGlyphItemIter VogueGlyphItemIter;

struct _VogueGlyphItemIter
{
  VogueGlyphItem *glyph_item;
  const gchar *text;

  int start_glyph;
  int start_index;
  int start_char;

  int end_glyph;
  int end_index;
  int end_char;
};

/**
 * PANGO_TYPE_GLYPH_ITEM_ITER:
 *
 * The #GObject type for #VogueGlyphItemIter.
 *
 * Since: 1.22
 */
#define PANGO_TYPE_GLYPH_ITEM_ITER (vogue_glyph_item_iter_get_type ())

PANGO_AVAILABLE_IN_1_22
GType               vogue_glyph_item_iter_get_type (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_1_22
VogueGlyphItemIter *vogue_glyph_item_iter_copy (VogueGlyphItemIter *orig);
PANGO_AVAILABLE_IN_1_22
void                vogue_glyph_item_iter_free (VogueGlyphItemIter *iter);

PANGO_AVAILABLE_IN_1_22
gboolean vogue_glyph_item_iter_init_start   (VogueGlyphItemIter *iter,
					     VogueGlyphItem     *glyph_item,
					     const char         *text);
PANGO_AVAILABLE_IN_1_22
gboolean vogue_glyph_item_iter_init_end     (VogueGlyphItemIter *iter,
					     VogueGlyphItem     *glyph_item,
					     const char         *text);
PANGO_AVAILABLE_IN_1_22
gboolean vogue_glyph_item_iter_next_cluster (VogueGlyphItemIter *iter);
PANGO_AVAILABLE_IN_1_22
gboolean vogue_glyph_item_iter_prev_cluster (VogueGlyphItemIter *iter);

G_END_DECLS

#endif /* __PANGO_GLYPH_ITEM_H__ */
