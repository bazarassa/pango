/* Vogue
 * vogue-layout.h: High-level layout driver
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

#ifndef __PANGO_LAYOUT_H__
#define __PANGO_LAYOUT_H__

#include <vogue/vogue-attributes.h>
#include <vogue/vogue-context.h>
#include <vogue/vogue-glyph-item.h>
#include <vogue/vogue-tabs.h>

G_BEGIN_DECLS

typedef struct _VogueLayout      VogueLayout;
typedef struct _VogueLayoutClass VogueLayoutClass;
typedef struct _VogueLayoutLine  VogueLayoutLine;

/**
 * VogueLayoutRun:
 *
 * The #VogueLayoutRun structure represents a single run within
 * a #VogueLayoutLine; it is simply an alternate name for
 * #VogueGlyphItem.
 * See the #VogueGlyphItem docs for details on the fields.
 */
typedef VogueGlyphItem VogueLayoutRun;

/**
 * VogueAlignment:
 * @PANGO_ALIGN_LEFT: Put all available space on the right
 * @PANGO_ALIGN_CENTER: Center the line within the available space
 * @PANGO_ALIGN_RIGHT: Put all available space on the left
 *
 * A #VogueAlignment describes how to align the lines of a #VogueLayout within the
 * available space. If the #VogueLayout is set to justify
 * using vogue_layout_set_justify(), this only has effect for partial lines.
 */
typedef enum {
  PANGO_ALIGN_LEFT,
  PANGO_ALIGN_CENTER,
  PANGO_ALIGN_RIGHT
} VogueAlignment;

/**
 * VogueWrapMode:
 * @PANGO_WRAP_WORD: wrap lines at word boundaries.
 * @PANGO_WRAP_CHAR: wrap lines at character boundaries.
 * @PANGO_WRAP_WORD_CHAR: wrap lines at word boundaries, but fall back to character boundaries if there is not
 * enough space for a full word.
 *
 * A #VogueWrapMode describes how to wrap the lines of a #VogueLayout to the desired width.
 */
typedef enum {
  PANGO_WRAP_WORD,
  PANGO_WRAP_CHAR,
  PANGO_WRAP_WORD_CHAR
} VogueWrapMode;

/**
 * VogueEllipsizeMode:
 * @PANGO_ELLIPSIZE_NONE: No ellipsization
 * @PANGO_ELLIPSIZE_START: Omit characters at the start of the text
 * @PANGO_ELLIPSIZE_MIDDLE: Omit characters in the middle of the text
 * @PANGO_ELLIPSIZE_END: Omit characters at the end of the text
 *
 * The #VogueEllipsizeMode type describes what sort of (if any)
 * ellipsization should be applied to a line of text. In
 * the ellipsization process characters are removed from the
 * text in order to make it fit to a given width and replaced
 * with an ellipsis.
 */
typedef enum {
  PANGO_ELLIPSIZE_NONE,
  PANGO_ELLIPSIZE_START,
  PANGO_ELLIPSIZE_MIDDLE,
  PANGO_ELLIPSIZE_END
} VogueEllipsizeMode;

/**
 * VogueLayoutLine:
 * @layout: (allow-none): the layout this line belongs to, might be %NULL
 * @start_index: start of line as byte index into layout->text
 * @length: length of line in bytes
 * @runs: (allow-none) (element-type Vogue.LayoutRun): list of runs in the
 *        line, from left to right
 * @is_paragraph_start: #TRUE if this is the first line of the paragraph
 * @resolved_dir: #Resolved VogueDirection of line
 *
 * The #VogueLayoutLine structure represents one of the lines resulting
 * from laying out a paragraph via #VogueLayout. #VogueLayoutLine
 * structures are obtained by calling vogue_layout_get_line() and
 * are only valid until the text, attributes, or settings of the
 * parent #VogueLayout are modified.
 *
 * Routines for rendering VogueLayout objects are provided in
 * code specific to each rendering system.
 */
struct _VogueLayoutLine
{
  VogueLayout *layout;
  gint         start_index;     /* start of line as byte index into layout->text */
  gint         length;		/* length of line in bytes */
  GSList      *runs;
  guint        is_paragraph_start : 1;  /* TRUE if this is the first line of the paragraph */
  guint        resolved_dir : 3;  /* Resolved VogueDirection of line */
};

#define PANGO_TYPE_LAYOUT              (vogue_layout_get_type ())
#define PANGO_LAYOUT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_LAYOUT, VogueLayout))
#define PANGO_LAYOUT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_LAYOUT, VogueLayoutClass))
#define PANGO_IS_LAYOUT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_LAYOUT))
#define PANGO_IS_LAYOUT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_LAYOUT))
#define PANGO_LAYOUT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_LAYOUT, VogueLayoutClass))

/* The VogueLayout and VogueLayoutClass structs are private; if you
 * need to create a subclass of these, file a bug.
 */

PANGO_AVAILABLE_IN_ALL
GType        vogue_layout_get_type       (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
VogueLayout *vogue_layout_new            (VogueContext   *context);
PANGO_AVAILABLE_IN_ALL
VogueLayout *vogue_layout_copy           (VogueLayout    *src);

PANGO_AVAILABLE_IN_ALL
VogueContext  *vogue_layout_get_context    (VogueLayout    *layout);

PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_attributes (VogueLayout    *layout,
					    VogueAttrList  *attrs);
PANGO_AVAILABLE_IN_ALL
VogueAttrList *vogue_layout_get_attributes (VogueLayout    *layout);

PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_text       (VogueLayout    *layout,
					    const char     *text,
					    int             length);
PANGO_AVAILABLE_IN_ALL
const char    *vogue_layout_get_text       (VogueLayout    *layout);

PANGO_AVAILABLE_IN_1_30
gint           vogue_layout_get_character_count (VogueLayout *layout);

PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_markup     (VogueLayout    *layout,
					    const char     *markup,
					    int             length);

PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_markup_with_accel (VogueLayout    *layout,
						   const char     *markup,
						   int             length,
						   gunichar        accel_marker,
						   gunichar       *accel_char);

PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_font_description (VogueLayout                *layout,
						  const VogueFontDescription *desc);

PANGO_AVAILABLE_IN_1_8
const VogueFontDescription *vogue_layout_get_font_description (VogueLayout *layout);

PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_width            (VogueLayout                *layout,
						  int                         width);
PANGO_AVAILABLE_IN_ALL
int            vogue_layout_get_width            (VogueLayout                *layout);
PANGO_AVAILABLE_IN_1_20
void           vogue_layout_set_height           (VogueLayout                *layout,
						  int                         height);
PANGO_AVAILABLE_IN_1_20
int            vogue_layout_get_height           (VogueLayout                *layout);
PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_wrap             (VogueLayout                *layout,
						  VogueWrapMode               wrap);
PANGO_AVAILABLE_IN_ALL
VogueWrapMode  vogue_layout_get_wrap             (VogueLayout                *layout);
PANGO_AVAILABLE_IN_1_16
gboolean       vogue_layout_is_wrapped           (VogueLayout                *layout);
PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_indent           (VogueLayout                *layout,
						  int                         indent);
PANGO_AVAILABLE_IN_ALL
int            vogue_layout_get_indent           (VogueLayout                *layout);
PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_spacing          (VogueLayout                *layout,
						  int                         spacing);
PANGO_AVAILABLE_IN_ALL
int            vogue_layout_get_spacing          (VogueLayout                *layout);
PANGO_AVAILABLE_IN_1_44
void           vogue_layout_set_line_spacing     (VogueLayout                *layout,
                                                  float                       factor);
PANGO_AVAILABLE_IN_1_44
float          vogue_layout_get_line_spacing     (VogueLayout                *layout);
PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_justify          (VogueLayout                *layout,
						  gboolean                    justify);
PANGO_AVAILABLE_IN_ALL
gboolean       vogue_layout_get_justify          (VogueLayout                *layout);
PANGO_AVAILABLE_IN_1_4
void           vogue_layout_set_auto_dir         (VogueLayout                *layout,
						  gboolean                    auto_dir);
PANGO_AVAILABLE_IN_1_4
gboolean       vogue_layout_get_auto_dir         (VogueLayout                *layout);
PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_alignment        (VogueLayout                *layout,
						  VogueAlignment              alignment);
PANGO_AVAILABLE_IN_ALL
VogueAlignment vogue_layout_get_alignment        (VogueLayout                *layout);

PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_tabs             (VogueLayout                *layout,
						  VogueTabArray              *tabs);

PANGO_AVAILABLE_IN_ALL
VogueTabArray* vogue_layout_get_tabs             (VogueLayout                *layout);

PANGO_AVAILABLE_IN_ALL
void           vogue_layout_set_single_paragraph_mode (VogueLayout                *layout,
						       gboolean                    setting);
PANGO_AVAILABLE_IN_ALL
gboolean       vogue_layout_get_single_paragraph_mode (VogueLayout                *layout);

PANGO_AVAILABLE_IN_1_6
void               vogue_layout_set_ellipsize (VogueLayout        *layout,
					       VogueEllipsizeMode  ellipsize);
PANGO_AVAILABLE_IN_1_6
VogueEllipsizeMode vogue_layout_get_ellipsize (VogueLayout        *layout);
PANGO_AVAILABLE_IN_1_16
gboolean           vogue_layout_is_ellipsized (VogueLayout        *layout);

PANGO_AVAILABLE_IN_1_16
int      vogue_layout_get_unknown_glyphs_count (VogueLayout    *layout);

PANGO_AVAILABLE_IN_ALL
void     vogue_layout_context_changed (VogueLayout    *layout);
PANGO_AVAILABLE_IN_1_32
guint    vogue_layout_get_serial      (VogueLayout    *layout);

PANGO_AVAILABLE_IN_ALL
void     vogue_layout_get_log_attrs (VogueLayout    *layout,
				     VogueLogAttr  **attrs,
				     gint           *n_attrs);

PANGO_AVAILABLE_IN_1_30
const VogueLogAttr *vogue_layout_get_log_attrs_readonly (VogueLayout *layout,
							 gint        *n_attrs);

PANGO_AVAILABLE_IN_ALL
void     vogue_layout_index_to_pos         (VogueLayout    *layout,
					    int             index_,
					    VogueRectangle *pos);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_index_to_line_x      (VogueLayout    *layout,
					    int             index_,
					    gboolean        trailing,
					    int            *line,
					    int            *x_pos);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_get_cursor_pos       (VogueLayout    *layout,
					    int             index_,
					    VogueRectangle *strong_pos,
					    VogueRectangle *weak_pos);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_move_cursor_visually (VogueLayout    *layout,
					    gboolean        strong,
					    int             old_index,
					    int             old_trailing,
					    int             direction,
					    int            *new_index,
					    int            *new_trailing);
PANGO_AVAILABLE_IN_ALL
gboolean vogue_layout_xy_to_index          (VogueLayout    *layout,
					    int             x,
					    int             y,
					    int            *index_,
					    int            *trailing);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_get_extents          (VogueLayout    *layout,
					    VogueRectangle *ink_rect,
					    VogueRectangle *logical_rect);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_get_pixel_extents    (VogueLayout    *layout,
					    VogueRectangle *ink_rect,
					    VogueRectangle *logical_rect);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_get_size             (VogueLayout    *layout,
					    int            *width,
					    int            *height);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_get_pixel_size       (VogueLayout    *layout,
					    int            *width,
					    int            *height);
PANGO_AVAILABLE_IN_1_22
int      vogue_layout_get_baseline         (VogueLayout    *layout);

PANGO_AVAILABLE_IN_ALL
int              vogue_layout_get_line_count       (VogueLayout    *layout);
PANGO_AVAILABLE_IN_ALL
VogueLayoutLine *vogue_layout_get_line             (VogueLayout    *layout,
						    int             line);
PANGO_AVAILABLE_IN_1_16
VogueLayoutLine *vogue_layout_get_line_readonly    (VogueLayout    *layout,
						    int             line);
PANGO_AVAILABLE_IN_ALL
GSList *         vogue_layout_get_lines            (VogueLayout    *layout);
PANGO_AVAILABLE_IN_1_16
GSList *         vogue_layout_get_lines_readonly   (VogueLayout    *layout);


#define PANGO_TYPE_LAYOUT_LINE (vogue_layout_line_get_type ())

PANGO_AVAILABLE_IN_ALL
GType    vogue_layout_line_get_type     (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_10
VogueLayoutLine *vogue_layout_line_ref   (VogueLayoutLine *line);
PANGO_AVAILABLE_IN_ALL
void             vogue_layout_line_unref (VogueLayoutLine *line);

PANGO_AVAILABLE_IN_ALL
gboolean vogue_layout_line_x_to_index   (VogueLayoutLine  *line,
					 int               x_pos,
					 int              *index_,
					 int              *trailing);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_line_index_to_x   (VogueLayoutLine  *line,
					 int               index_,
					 gboolean          trailing,
					 int              *x_pos);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_line_get_x_ranges (VogueLayoutLine  *line,
					 int               start_index,
					 int               end_index,
					 int             **ranges,
					 int              *n_ranges);
PANGO_AVAILABLE_IN_ALL
void     vogue_layout_line_get_extents  (VogueLayoutLine  *line,
					 VogueRectangle   *ink_rect,
					 VogueRectangle   *logical_rect);
PANGO_AVAILABLE_IN_1_44
void     vogue_layout_line_get_height   (VogueLayoutLine  *line,
					 int              *height);

PANGO_AVAILABLE_IN_ALL
void     vogue_layout_line_get_pixel_extents (VogueLayoutLine *layout_line,
					      VogueRectangle  *ink_rect,
					      VogueRectangle  *logical_rect);

typedef struct _VogueLayoutIter VogueLayoutIter;

#define PANGO_TYPE_LAYOUT_ITER         (vogue_layout_iter_get_type ())

PANGO_AVAILABLE_IN_ALL
GType            vogue_layout_iter_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
VogueLayoutIter *vogue_layout_get_iter  (VogueLayout     *layout);
PANGO_AVAILABLE_IN_1_20
VogueLayoutIter *vogue_layout_iter_copy (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_ALL
void             vogue_layout_iter_free (VogueLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
int              vogue_layout_iter_get_index  (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_ALL
VogueLayoutRun  *vogue_layout_iter_get_run    (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_1_16
VogueLayoutRun  *vogue_layout_iter_get_run_readonly   (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_ALL
VogueLayoutLine *vogue_layout_iter_get_line   (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_1_16
VogueLayoutLine *vogue_layout_iter_get_line_readonly  (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_ALL
gboolean         vogue_layout_iter_at_last_line (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_1_20
VogueLayout     *vogue_layout_iter_get_layout (VogueLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean vogue_layout_iter_next_char    (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_ALL
gboolean vogue_layout_iter_next_cluster (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_ALL
gboolean vogue_layout_iter_next_run     (VogueLayoutIter *iter);
PANGO_AVAILABLE_IN_ALL
gboolean vogue_layout_iter_next_line    (VogueLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
void vogue_layout_iter_get_char_extents    (VogueLayoutIter *iter,
					    VogueRectangle  *logical_rect);
PANGO_AVAILABLE_IN_ALL
void vogue_layout_iter_get_cluster_extents (VogueLayoutIter *iter,
					    VogueRectangle  *ink_rect,
					    VogueRectangle  *logical_rect);
PANGO_AVAILABLE_IN_ALL
void vogue_layout_iter_get_run_extents     (VogueLayoutIter *iter,
					    VogueRectangle  *ink_rect,
					    VogueRectangle  *logical_rect);
PANGO_AVAILABLE_IN_ALL
void vogue_layout_iter_get_line_extents    (VogueLayoutIter *iter,
					    VogueRectangle  *ink_rect,
					    VogueRectangle  *logical_rect);
/* All the yranges meet, unlike the logical_rect's (i.e. the yranges
 * assign between-line spacing to the nearest line)
 */
PANGO_AVAILABLE_IN_ALL
void vogue_layout_iter_get_line_yrange     (VogueLayoutIter *iter,
					    int             *y0_,
					    int             *y1_);
PANGO_AVAILABLE_IN_ALL
void vogue_layout_iter_get_layout_extents  (VogueLayoutIter *iter,
					    VogueRectangle  *ink_rect,
					    VogueRectangle  *logical_rect);
PANGO_AVAILABLE_IN_ALL
int  vogue_layout_iter_get_baseline        (VogueLayoutIter *iter);

G_END_DECLS

#endif /* __PANGO_LAYOUT_H__ */

