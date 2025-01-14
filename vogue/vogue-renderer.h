/* Vogue
 * vogue-renderer.h: Base class for rendering
 *
 * Copyright (C) 2004, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __PANGO_RENDERER_H_
#define __PANGO_RENDERER_H_

#include <vogue/vogue-layout.h>

G_BEGIN_DECLS

#define PANGO_TYPE_RENDERER            (vogue_renderer_get_type())
#define PANGO_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_RENDERER, VogueRenderer))
#define PANGO_IS_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_RENDERER))
#define PANGO_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_RENDERER, VogueRendererClass))
#define PANGO_IS_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_RENDERER))
#define PANGO_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_RENDERER, VogueRendererClass))

typedef struct _VogueRenderer        VogueRenderer;
typedef struct _VogueRendererClass   VogueRendererClass;
typedef struct _VogueRendererPrivate VogueRendererPrivate;

/**
 * VogueRenderPart:
 * @PANGO_RENDER_PART_FOREGROUND: the text itself
 * @PANGO_RENDER_PART_BACKGROUND: the area behind the text
 * @PANGO_RENDER_PART_UNDERLINE: underlines
 * @PANGO_RENDER_PART_STRIKETHROUGH: strikethrough lines
 *
 * #VogueRenderPart defines different items to render for such
 * purposes as setting colors.
 *
 * Since: 1.8
 **/
/* When extending, note N_RENDER_PARTS #define in vogue-renderer.c */
typedef enum
{
  PANGO_RENDER_PART_FOREGROUND,
  PANGO_RENDER_PART_BACKGROUND,
  PANGO_RENDER_PART_UNDERLINE,
  PANGO_RENDER_PART_STRIKETHROUGH
} VogueRenderPart;

/**
 * VogueRenderer:
 * @matrix: (nullable): the current transformation matrix for
 *    the Renderer; may be %NULL, which should be treated the
 *    same as the identity matrix.
 *
 * #VogueRenderer is a base class for objects that are used to
 * render Vogue objects such as #VogueGlyphString and
 * #VogueLayout.
 *
 * Since: 1.8
 **/
struct _VogueRenderer
{
  /*< private >*/
  GObject parent_instance;

  VogueUnderline underline;
  gboolean strikethrough;
  int active_count;

  /*< public >*/
  VogueMatrix *matrix;          /* May be NULL */

  /*< private >*/
  VogueRendererPrivate *priv;
};

/**
 * VogueRendererClass:
 * @draw_glyphs: draws a #VogueGlyphString
 * @draw_rectangle: draws a rectangle
 * @draw_error_underline: draws a squiggly line that approximately
 * covers the given rectangle in the style of an underline used to
 * indicate a spelling error.
 * @draw_shape: draw content for a glyph shaped with #VogueAttrShape.
 *   @x, @y are the coordinates of the left edge of the baseline,
 *   in user coordinates.
 * @draw_trapezoid: draws a trapezoidal filled area
 * @draw_glyph: draws a single glyph
 * @part_changed: do renderer specific processing when rendering
 *  attributes change
 * @begin: Do renderer-specific initialization before drawing
 * @end: Do renderer-specific cleanup after drawing
 * @prepare_run: updates the renderer for a new run
 * @draw_glyph_item: draws a #VogueGlyphItem
 *
 * Class structure for #VogueRenderer.
 *
 * The following vfuncs take user space coordinates in Vogue units
 * and have default implementations:
 * - draw_glyphs
 * - draw_rectangle
 * - draw_error_underline
 * - draw_shape
 * - draw_glyph_item
 *
 * The default draw_shape implementation draws nothing.
 *
 * The following vfuncs take device space coordinates as doubles
 * and must be implemented:
 * - draw_trapezoid
 * - draw_glyph
 *
 * Since: 1.8
 */
struct _VogueRendererClass
{
  /*< private >*/
  GObjectClass parent_class;

  /* vtable - not signals */
  /*< public >*/

  void (*draw_glyphs)          (VogueRenderer    *renderer,
                                VogueFont        *font,
                                VogueGlyphString *glyphs,
                                int               x,
                                int               y);
  void (*draw_rectangle)       (VogueRenderer    *renderer,
                                VogueRenderPart   part,
                                int               x,
                                int               y,
                                int               width,
                                int               height);
  void (*draw_error_underline) (VogueRenderer    *renderer,
                                int               x,
                                int               y,
                                int               width,
                                int               height);
  void (*draw_shape)           (VogueRenderer    *renderer,
                                VogueAttrShape   *attr,
                                int               x,
                                int               y);

  void (*draw_trapezoid)       (VogueRenderer    *renderer,
                                VogueRenderPart   part,
                                double            y1_,
                                double            x11,
                                double            x21,
                                double            y2,
                                double            x12,
                                double            x22);
  void (*draw_glyph)           (VogueRenderer    *renderer,
                                VogueFont        *font,
                                VogueGlyph        glyph,
                                double            x,
                                double            y);

  void (*part_changed)         (VogueRenderer    *renderer,
                                VogueRenderPart   part);

  void (*begin)                (VogueRenderer    *renderer);
  void (*end)                  (VogueRenderer    *renderer);

  void (*prepare_run)          (VogueRenderer    *renderer,
                                VogueLayoutRun   *run);

  void (*draw_glyph_item)      (VogueRenderer    *renderer,
                                const char       *text,
                                VogueGlyphItem   *glyph_item,
                                int               x,
                                int               y);

  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved2) (void);
  void (*_vogue_reserved3) (void);
  void (*_vogue_reserved4) (void);
};

PANGO_AVAILABLE_IN_1_8
GType vogue_renderer_get_type            (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_8
void vogue_renderer_draw_layout          (VogueRenderer    *renderer,
                                          VogueLayout      *layout,
                                          int               x,
                                          int               y);
PANGO_AVAILABLE_IN_1_8
void vogue_renderer_draw_layout_line     (VogueRenderer    *renderer,
                                          VogueLayoutLine  *line,
                                          int               x,
                                          int               y);
PANGO_AVAILABLE_IN_1_8
void vogue_renderer_draw_glyphs          (VogueRenderer    *renderer,
                                          VogueFont        *font,
                                          VogueGlyphString *glyphs,
                                          int               x,
                                          int               y);
PANGO_AVAILABLE_IN_1_22
void vogue_renderer_draw_glyph_item      (VogueRenderer    *renderer,
                                          const char       *text,
                                          VogueGlyphItem   *glyph_item,
                                          int               x,
                                          int               y);
PANGO_AVAILABLE_IN_1_8
void vogue_renderer_draw_rectangle       (VogueRenderer    *renderer,
                                          VogueRenderPart   part,
                                          int               x,
                                          int               y,
                                          int               width,
                                          int               height);
PANGO_AVAILABLE_IN_1_8
void vogue_renderer_draw_error_underline (VogueRenderer    *renderer,
                                          int               x,
                                          int               y,
                                          int               width,
                                          int               height);
PANGO_AVAILABLE_IN_1_8
void vogue_renderer_draw_trapezoid       (VogueRenderer    *renderer,
                                          VogueRenderPart   part,
                                          double            y1_,
                                          double            x11,
                                          double            x21,
                                          double            y2,
                                          double            x12,
                                          double            x22);
PANGO_AVAILABLE_IN_1_8
void vogue_renderer_draw_glyph           (VogueRenderer    *renderer,
                                          VogueFont        *font,
                                          VogueGlyph        glyph,
                                          double            x,
                                          double            y);

PANGO_AVAILABLE_IN_1_8
void vogue_renderer_activate             (VogueRenderer    *renderer);
PANGO_AVAILABLE_IN_1_8
void vogue_renderer_deactivate           (VogueRenderer    *renderer);

PANGO_AVAILABLE_IN_1_8
void vogue_renderer_part_changed         (VogueRenderer   *renderer,
                                          VogueRenderPart  part);

PANGO_AVAILABLE_IN_1_8
void        vogue_renderer_set_color     (VogueRenderer    *renderer,
                                          VogueRenderPart   part,
                                          const VogueColor *color);
PANGO_AVAILABLE_IN_1_8
VogueColor *vogue_renderer_get_color     (VogueRenderer    *renderer,
                                          VogueRenderPart   part);

PANGO_AVAILABLE_IN_1_38
void        vogue_renderer_set_alpha     (VogueRenderer    *renderer,
                                          VogueRenderPart   part,
                                          guint16           alpha);
PANGO_AVAILABLE_IN_1_38
guint16     vogue_renderer_get_alpha     (VogueRenderer    *renderer,
                                          VogueRenderPart   part);

PANGO_AVAILABLE_IN_1_8
void               vogue_renderer_set_matrix      (VogueRenderer     *renderer,
                                                   const VogueMatrix *matrix);
PANGO_AVAILABLE_IN_1_8
const VogueMatrix *vogue_renderer_get_matrix      (VogueRenderer     *renderer);

PANGO_AVAILABLE_IN_1_20
VogueLayout       *vogue_renderer_get_layout      (VogueRenderer     *renderer);
PANGO_AVAILABLE_IN_1_20
VogueLayoutLine   *vogue_renderer_get_layout_line (VogueRenderer     *renderer);

G_END_DECLS

#endif /* __PANGO_RENDERER_H_ */

