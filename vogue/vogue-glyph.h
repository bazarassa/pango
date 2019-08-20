/* Vogue
 * vogue-glyph.h: Glyph storage
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

#ifndef __PANGO_GLYPH_H__
#define __PANGO_GLYPH_H__

#include <vogue/vogue-types.h>
#include <vogue/vogue-item.h>

G_BEGIN_DECLS

typedef struct _VogueGlyphGeometry VogueGlyphGeometry;
typedef struct _VogueGlyphVisAttr VogueGlyphVisAttr;
typedef struct _VogueGlyphInfo VogueGlyphInfo;
typedef struct _VogueGlyphString VogueGlyphString;

/* 1024ths of a device unit */
/**
 * VogueGlyphUnit:
 *
 * The #VogueGlyphUnit type is used to store dimensions within
 * Vogue. Dimensions are stored in 1/%PANGO_SCALE of a device unit.
 * (A device unit might be a pixel for screen display, or
 * a point on a printer.) %PANGO_SCALE is currently 1024, and
 * may change in the future (unlikely though), but you should not
 * depend on its exact value. The PANGO_PIXELS() macro can be used
 * to convert from glyph units into device units with correct rounding.
 */
typedef gint32 VogueGlyphUnit;

/* Positioning information about a glyph
 */
/**
 * VogueGlyphGeometry:
 * @width: the logical width to use for the the character.
 * @x_offset: horizontal offset from nominal character position.
 * @y_offset: vertical offset from nominal character position.
 *
 * The #VogueGlyphGeometry structure contains width and positioning
 * information for a single glyph.
 */
struct _VogueGlyphGeometry
{
  VogueGlyphUnit width;
  VogueGlyphUnit x_offset;
  VogueGlyphUnit y_offset;
};

/* Visual attributes of a glyph
 */
/**
 * VogueGlyphVisAttr:
 * @is_cluster_start: set for the first logical glyph in each cluster. (Clusters
 * are stored in visual order, within the cluster, glyphs
 * are always ordered in logical order, since visual
 * order is meaningless; that is, in Arabic text, accent glyphs
 * follow the glyphs for the base character.)
 *
 * The VogueGlyphVisAttr is used to communicate information between
 * the shaping phase and the rendering phase.  More attributes may be
 * added in the future.
 */
struct _VogueGlyphVisAttr
{
  guint is_cluster_start : 1;
};

/* A single glyph
 */
/**
 * VogueGlyphInfo:
 * @glyph: the glyph itself.
 * @geometry: the positional information about the glyph.
 * @attr: the visual attributes of the glyph.
 *
 * The #VogueGlyphInfo structure represents a single glyph together with
 * positioning information and visual attributes.
 * It contains the following fields.
 */
struct _VogueGlyphInfo
{
  VogueGlyph    glyph;
  VogueGlyphGeometry geometry;
  VogueGlyphVisAttr  attr;
};

/* A string of glyphs with positional information and visual attributes -
 * ready for drawing
 */
/**
 * VogueGlyphString:
 * @num_glyphs: number of the glyphs in this glyph string.
 * @glyphs: (array length=num_glyphs): array of glyph information
 *          for the glyph string.
 * @log_clusters: logical cluster info, indexed by the byte index
 *                within the text corresponding to the glyph string.
 *
 * The #VogueGlyphString structure is used to store strings
 * of glyphs with geometry and visual attribute information.
 * The storage for the glyph information is owned
 * by the structure which simplifies memory management.
 */
struct _VogueGlyphString {
  gint num_glyphs;

  VogueGlyphInfo *glyphs;
  gint *log_clusters;

  /*< private >*/
  gint space;
};

/**
 * PANGO_TYPE_GLYPH_STRING:
 *
 * The #GObject type for #VogueGlyphString.
 */
#define PANGO_TYPE_GLYPH_STRING (vogue_glyph_string_get_type ())

PANGO_AVAILABLE_IN_ALL
VogueGlyphString *vogue_glyph_string_new      (void);
PANGO_AVAILABLE_IN_ALL
void              vogue_glyph_string_set_size (VogueGlyphString *string,
					       gint              new_len);
PANGO_AVAILABLE_IN_ALL
GType             vogue_glyph_string_get_type (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
VogueGlyphString *vogue_glyph_string_copy     (VogueGlyphString *string);
PANGO_AVAILABLE_IN_ALL
void              vogue_glyph_string_free     (VogueGlyphString *string);
PANGO_AVAILABLE_IN_ALL
void              vogue_glyph_string_extents  (VogueGlyphString *glyphs,
					       VogueFont        *font,
					       VogueRectangle   *ink_rect,
					       VogueRectangle   *logical_rect);
PANGO_AVAILABLE_IN_1_14
int               vogue_glyph_string_get_width(VogueGlyphString *glyphs);

PANGO_AVAILABLE_IN_ALL
void              vogue_glyph_string_extents_range  (VogueGlyphString *glyphs,
						     int               start,
						     int               end,
						     VogueFont        *font,
						     VogueRectangle   *ink_rect,
						     VogueRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void vogue_glyph_string_get_logical_widths (VogueGlyphString *glyphs,
					    const char       *text,
					    int               length,
					    int               embedding_level,
					    int              *logical_widths);

PANGO_AVAILABLE_IN_ALL
void vogue_glyph_string_index_to_x (VogueGlyphString *glyphs,
				    char             *text,
				    int               length,
				    VogueAnalysis    *analysis,
				    int               index_,
				    gboolean          trailing,
				    int              *x_pos);
PANGO_AVAILABLE_IN_ALL
void vogue_glyph_string_x_to_index (VogueGlyphString *glyphs,
				    char             *text,
				    int               length,
				    VogueAnalysis    *analysis,
				    int               x_pos,
				    int              *index_,
				    int              *trailing);

/* Turn a string of characters into a string of glyphs
 */
PANGO_AVAILABLE_IN_ALL
void vogue_shape (const char          *text,
                  int                  length,
                  const VogueAnalysis *analysis,
                  VogueGlyphString    *glyphs);

PANGO_AVAILABLE_IN_1_32
void vogue_shape_full (const char          *item_text,
                       int                  item_length,
                       const char          *paragraph_text,
                       int                  paragraph_length,
                       const VogueAnalysis *analysis,
                       VogueGlyphString    *glyphs);

/**
 * VogueShapeFlags:
 * @PANGO_SHAPE_NONE: Default value.
 * @PANGO_SHAPE_ROUND_POSITIONS: Round glyph positions
 *     and widths to whole device units. This option should
 *     be set if the target renderer can't do subpixel
 *     positioning of glyphs.
 *
 * Flags influencing the shaping process.
 * These can be passed to vogue_shape_with_flags().
 */
typedef enum {
  PANGO_SHAPE_NONE            = 0,
  PANGO_SHAPE_ROUND_POSITIONS = 1 << 0,
} VogueShapeFlags;

PANGO_AVAILABLE_IN_1_44
void vogue_shape_with_flags (const char          *item_text,
                             int                  item_length,
                             const char          *paragraph_text,
                             int                  paragraph_length,
                             const VogueAnalysis *analysis,
                             VogueGlyphString    *glyphs,
                             VogueShapeFlags      flags);

PANGO_AVAILABLE_IN_ALL
GList *vogue_reorder_items (GList *logical_items);

G_END_DECLS

#endif /* __PANGO_GLYPH_H__ */
