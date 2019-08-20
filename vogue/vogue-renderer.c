/* Vogue
 * vogue-renderer.h: Base class for rendering
 *
 * Copyright (C) 2004 Red Hat, Inc.
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
#include <stdlib.h>

#include "vogue-renderer.h"
#include "vogue-impl-utils.h"
#include "vogue-layout-private.h"

#define N_RENDER_PARTS 4

#define PANGO_IS_RENDERER_FAST(renderer) (renderer != NULL)
#define IS_VALID_PART(part) ((guint)part < N_RENDER_PARTS)

typedef struct _LineState LineState;
typedef struct _Point Point;

struct _Point
{
  double x, y;
};

struct _LineState
{
  VogueUnderline underline;
  VogueRectangle underline_rect;

  gboolean strikethrough;
  VogueRectangle strikethrough_rect;

  int logical_rect_end;
};

struct _VogueRendererPrivate
{
  VogueColor color[N_RENDER_PARTS];
  gboolean color_set[N_RENDER_PARTS];
  guint16 alpha[N_RENDER_PARTS];

  VogueLayoutLine *line;
  LineState *line_state;
};

static void vogue_renderer_finalize                     (GObject          *gobject);
static void vogue_renderer_default_draw_glyphs          (VogueRenderer    *renderer,
							 VogueFont        *font,
							 VogueGlyphString *glyphs,
							 int               x,
							 int               y);
static void vogue_renderer_default_draw_glyph_item      (VogueRenderer    *renderer,
							 const char       *text,
							 VogueGlyphItem   *glyph_item,
							 int               x,
							 int               y);
static void vogue_renderer_default_draw_rectangle       (VogueRenderer    *renderer,
							 VogueRenderPart   part,
							 int               x,
							 int               y,
							 int               width,
							 int               height);
static void vogue_renderer_default_draw_error_underline (VogueRenderer    *renderer,
							 int               x,
							 int               y,
							 int               width,
							 int               height);
static void vogue_renderer_default_prepare_run          (VogueRenderer    *renderer,
							 VogueLayoutRun   *run);

static void vogue_renderer_prepare_run (VogueRenderer  *renderer,
					VogueLayoutRun *run);

static void
to_device (VogueMatrix *matrix,
	   double       x,
	   double       y,
	   Point       *result)
{
  if (matrix)
    {
      result->x = (x * matrix->xx + y * matrix->xy) / PANGO_SCALE + matrix->x0;
      result->y = (x * matrix->yx + y * matrix->yy) / PANGO_SCALE + matrix->y0;
    }
  else
    {
      result->x = x / PANGO_SCALE;
      result->y = y / PANGO_SCALE;
    }
}

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (VogueRenderer, vogue_renderer, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (VogueRenderer))

static void
vogue_renderer_class_init (VogueRendererClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  klass->draw_glyphs = vogue_renderer_default_draw_glyphs;
  klass->draw_glyph_item = vogue_renderer_default_draw_glyph_item;
  klass->draw_rectangle = vogue_renderer_default_draw_rectangle;
  klass->draw_error_underline = vogue_renderer_default_draw_error_underline;
  klass->prepare_run = vogue_renderer_default_prepare_run;

  gobject_class->finalize = vogue_renderer_finalize;
}

static void
vogue_renderer_init (VogueRenderer *renderer)
{
  renderer->priv = vogue_renderer_get_instance_private (renderer);
  renderer->matrix = NULL;
}

static void
vogue_renderer_finalize (GObject *gobject)
{
  VogueRenderer *renderer = PANGO_RENDERER (gobject);

  if (renderer->matrix)
    vogue_matrix_free (renderer->matrix);

  G_OBJECT_CLASS (vogue_renderer_parent_class)->finalize (gobject);
}

/**
 * vogue_renderer_draw_layout:
 * @renderer: a #VogueRenderer
 * @layout: a #VogueLayout
 * @x: X position of left edge of baseline, in user space coordinates
 *   in Vogue units.
 * @y: Y position of left edge of baseline, in user space coordinates
 *    in Vogue units.
 *
 * Draws @layout with the specified #VogueRenderer.
 *
 * Since: 1.8
 **/
void
vogue_renderer_draw_layout (VogueRenderer    *renderer,
			    VogueLayout      *layout,
			    int               x,
			    int               y)
{
  VogueLayoutIter iter;

  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  /* We only change the matrix if the renderer isn't already
   * active.
   */
  if (!renderer->active_count)
    {
      VogueContext *context = vogue_layout_get_context (layout);
      vogue_renderer_set_matrix (renderer,
				 vogue_context_get_matrix (context));
    }

  vogue_renderer_activate (renderer);

  _vogue_layout_get_iter (layout, &iter);

  do
    {
      VogueRectangle   logical_rect;
      VogueLayoutLine *line;
      int              baseline;

      line = vogue_layout_iter_get_line_readonly (&iter);

      vogue_layout_iter_get_line_extents (&iter, NULL, &logical_rect);
      baseline = vogue_layout_iter_get_baseline (&iter);

      vogue_renderer_draw_layout_line (renderer,
				       line,
				       x + logical_rect.x,
				       y + baseline);
    }
  while (vogue_layout_iter_next_line (&iter));

  _vogue_layout_iter_destroy (&iter);

  vogue_renderer_deactivate (renderer);
}

static void
draw_underline (VogueRenderer *renderer,
		LineState     *state)
{
  VogueRectangle *rect = &state->underline_rect;
  VogueUnderline underline = state->underline;

  state->underline = PANGO_UNDERLINE_NONE;

  switch (underline)
    {
    case PANGO_UNDERLINE_NONE:
      break;
    case PANGO_UNDERLINE_DOUBLE:
      vogue_renderer_draw_rectangle (renderer,
				     PANGO_RENDER_PART_UNDERLINE,
				     rect->x,
				     rect->y + 2 * rect->height,
				     rect->width,
				     rect->height);
      /* Fall through */
    case PANGO_UNDERLINE_SINGLE:
    case PANGO_UNDERLINE_LOW:
      vogue_renderer_draw_rectangle (renderer,
				     PANGO_RENDER_PART_UNDERLINE,
				     rect->x,
				     rect->y,
				     rect->width,
				     rect->height);
      break;
    case PANGO_UNDERLINE_ERROR:
      vogue_renderer_draw_error_underline (renderer,
					   rect->x,
					   rect->y,
					   rect->width,
					   3 * rect->height);
      break;
    }
}

static void
draw_strikethrough (VogueRenderer *renderer,
		    LineState     *state)
{
  VogueRectangle *rect = &state->strikethrough_rect;
  gboolean strikethrough = state->strikethrough;

  state->strikethrough = FALSE;

  if (strikethrough)
    vogue_renderer_draw_rectangle (renderer,
				   PANGO_RENDER_PART_STRIKETHROUGH,
				   rect->x,
				   rect->y,
				   rect->width,
				   rect->height);
}

static void
handle_line_state_change (VogueRenderer  *renderer,
			  VogueRenderPart part)
{
  LineState *state = renderer->priv->line_state;
  if (!state)
    return;

  if (part == PANGO_RENDER_PART_UNDERLINE &&
      state->underline != PANGO_UNDERLINE_NONE)
    {
      VogueRectangle *rect = &state->underline_rect;

      rect->width = state->logical_rect_end - rect->x;
      draw_underline (renderer, state);
      state->underline = renderer->underline;
      rect->x = state->logical_rect_end;
      rect->width = 0;
    }

  if (part == PANGO_RENDER_PART_STRIKETHROUGH &&
      state->strikethrough)
    {
      VogueRectangle *rect = &state->strikethrough_rect;

      rect->width = state->logical_rect_end - rect->x;
      draw_strikethrough (renderer, state);
      state->strikethrough = renderer->strikethrough;
      rect->x = state->logical_rect_end;
      rect->width = 0;
    }
}

static void
add_underline (VogueRenderer    *renderer,
	       LineState        *state,
	       VogueFontMetrics *metrics,
	       int               base_x,
	       int               base_y,
	       VogueRectangle   *ink_rect,
	       VogueRectangle   *logical_rect)
{
  VogueRectangle *current_rect = &state->underline_rect;
  VogueRectangle new_rect;

  int underline_thickness = vogue_font_metrics_get_underline_thickness (metrics);
  int underline_position = vogue_font_metrics_get_underline_position (metrics);

  new_rect.x = base_x + logical_rect->x;
  new_rect.width = logical_rect->width;
  new_rect.height = underline_thickness;
  new_rect.y = base_y;

  switch (renderer->underline)
    {
    case PANGO_UNDERLINE_NONE:
      g_assert_not_reached ();
      break;
    case PANGO_UNDERLINE_SINGLE:
    case PANGO_UNDERLINE_DOUBLE:
    case PANGO_UNDERLINE_ERROR:
      new_rect.y -= underline_position;
      break;
    case PANGO_UNDERLINE_LOW:
      new_rect.y += ink_rect->y + ink_rect->height + underline_thickness;
      break;
    }

  if (renderer->underline == state->underline &&
      new_rect.y == current_rect->y &&
      new_rect.height == current_rect->height)
    {
      current_rect->width = new_rect.x + new_rect.width - current_rect->x;
    }
  else
    {
      draw_underline (renderer, state);

      *current_rect = new_rect;
      state->underline = renderer->underline;
    }
}

static void
add_strikethrough (VogueRenderer    *renderer,
		   LineState        *state,
		   VogueFontMetrics *metrics,
		   int               base_x,
		   int               base_y,
		   VogueRectangle   *ink_rect G_GNUC_UNUSED,
		   VogueRectangle   *logical_rect)
{
  VogueRectangle *current_rect = &state->strikethrough_rect;
  VogueRectangle new_rect;

  int strikethrough_thickness = vogue_font_metrics_get_strikethrough_thickness (metrics);
  int strikethrough_position = vogue_font_metrics_get_strikethrough_position (metrics);

  new_rect.x = base_x + logical_rect->x;
  new_rect.width = logical_rect->width;
  new_rect.y = base_y - strikethrough_position;
  new_rect.height = strikethrough_thickness;

  if (state->strikethrough &&
      new_rect.y == current_rect->y &&
      new_rect.height == current_rect->height)
    {
      current_rect->width = new_rect.x + new_rect.width - current_rect->x;
    }
  else
    {
      draw_strikethrough (renderer, state);

      *current_rect = new_rect;
      state->strikethrough = TRUE;
    }
}

static void
get_item_properties (VogueItem       *item,
		     gint            *rise,
		     VogueAttrShape **shape_attr)
{
  GSList *l;

  if (rise)
    *rise = 0;

  if (shape_attr)
    *shape_attr = NULL;

  for (l = item->analysis.extra_attrs; l; l = l->next)
    {
      VogueAttribute *attr = l->data;

      switch ((int) attr->klass->type)
	{
	case PANGO_ATTR_SHAPE:
	  if (shape_attr)
	    *shape_attr = (VogueAttrShape *)attr;
	  break;

	case PANGO_ATTR_RISE:
	  if (rise)
	    *rise = ((VogueAttrInt *)attr)->value;
	  break;

	default:
	  break;
	}
    }
}

static void
draw_shaped_glyphs (VogueRenderer    *renderer,
		    VogueGlyphString *glyphs,
		    VogueAttrShape   *attr,
		    int               x,
		    int               y)
{
  VogueRendererClass *class = PANGO_RENDERER_GET_CLASS (renderer);
  int i;

  if (!class->draw_shape)
    return;

  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      VogueGlyphInfo *gi = &glyphs->glyphs[i];

      class->draw_shape (renderer, attr, x, y);

      x += gi->geometry.width;
    }
}


/**
 * vogue_renderer_draw_layout_line:
 * @renderer: a #VogueRenderer
 * @line: a #VogueLayoutLine
 * @x: X position of left edge of baseline, in user space coordinates
 *   in Vogue units.
 * @y: Y position of left edge of baseline, in user space coordinates
 *    in Vogue units.
 *
 * Draws @line with the specified #VogueRenderer.
 *
 * Since: 1.8
 **/
void
vogue_renderer_draw_layout_line (VogueRenderer    *renderer,
				 VogueLayoutLine  *line,
				 int               x,
				 int               y)
{
  int x_off = 0;
  int glyph_string_width;
  LineState state;
  GSList *l;
  gboolean got_overall = FALSE;
  VogueRectangle overall_rect;
  const char *text;

  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));

  /* We only change the matrix if the renderer isn't already
   * active.
   */
  if (!renderer->active_count)
    vogue_renderer_set_matrix (renderer,
			       G_LIKELY (line->layout) ?
			       vogue_context_get_matrix
			       (vogue_layout_get_context (line->layout)) :
			       NULL);

  vogue_renderer_activate (renderer);

  renderer->priv->line = line;
  renderer->priv->line_state = &state;

  state.underline = PANGO_UNDERLINE_NONE;
  state.strikethrough = FALSE;

  text = G_LIKELY (line->layout) ? vogue_layout_get_text (line->layout) : NULL;

  for (l = line->runs; l; l = l->next)
    {
      VogueFontMetrics *metrics;
      gint rise;
      VogueLayoutRun *run = l->data;
      VogueAttrShape *shape_attr;
      VogueRectangle ink_rect, *ink = NULL;
      VogueRectangle logical_rect, *logical = NULL;

      if (run->item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
	logical = &logical_rect;

      vogue_renderer_prepare_run (renderer, run);

      get_item_properties (run->item, &rise, &shape_attr);

      if (shape_attr)
	{
	  ink = &ink_rect;
	  logical = &logical_rect;
          _vogue_shape_get_extents (run->glyphs->num_glyphs,
				    &shape_attr->ink_rect,
				    &shape_attr->logical_rect,
				    ink,
				    logical);
	  glyph_string_width = logical->width;
	}
      else
	{
	  if (renderer->underline != PANGO_UNDERLINE_NONE ||
	      renderer->strikethrough)
	    {
	      ink = &ink_rect;
	      logical = &logical_rect;
	    }
	  if (G_UNLIKELY (ink || logical))
	    vogue_glyph_string_extents (run->glyphs, run->item->analysis.font,
					ink, logical);
	  if (logical)
	    glyph_string_width = logical_rect.width;
	  else
	    glyph_string_width = vogue_glyph_string_get_width (run->glyphs);
	}

      state.logical_rect_end = x + x_off + glyph_string_width;

      if (run->item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
	{
	  gboolean is_hinted = ((logical_rect.y | logical_rect.height) & (PANGO_SCALE - 1)) == 0;
	  int adjustment = logical_rect.y + logical_rect.height / 2;

	  if (is_hinted)
	    adjustment = PANGO_UNITS_ROUND (adjustment);

	  rise += adjustment;
	}


      if (renderer->priv->color_set[PANGO_RENDER_PART_BACKGROUND])
	{
	  if (!got_overall)
	    {
	      vogue_layout_line_get_extents (line, NULL, &overall_rect);
	      got_overall = TRUE;
	    }

	  vogue_renderer_draw_rectangle (renderer,
					 PANGO_RENDER_PART_BACKGROUND,
					 x + x_off,
					 y + overall_rect.y,
					 glyph_string_width,
					 overall_rect.height);
	}

      if (shape_attr)
	{
	  draw_shaped_glyphs (renderer, run->glyphs, shape_attr, x + x_off, y - rise);
	}
      else
	{
	  vogue_renderer_draw_glyph_item (renderer,
					  text,
					  run,
					  x + x_off, y - rise);
	}

      if (renderer->underline != PANGO_UNDERLINE_NONE ||
	  renderer->strikethrough)
	{
	  metrics = vogue_font_get_metrics (run->item->analysis.font,
					    run->item->analysis.language);

	  if (renderer->underline != PANGO_UNDERLINE_NONE)
	    add_underline (renderer, &state,metrics,
			   x + x_off, y - rise,
			   ink, logical);

	  if (renderer->strikethrough)
	    add_strikethrough (renderer, &state, metrics,
			       x + x_off, y - rise,
			       ink, logical);

	  vogue_font_metrics_unref (metrics);
	}

      if (renderer->underline == PANGO_UNDERLINE_NONE &&
	  state.underline != PANGO_UNDERLINE_NONE)
	draw_underline (renderer, &state);

      if (!renderer->strikethrough && state.strikethrough)
	draw_strikethrough (renderer, &state);

      x_off += glyph_string_width;
    }

  /* Finish off any remaining underlines
   */
  draw_underline (renderer, &state);
  draw_strikethrough (renderer, &state);

  renderer->priv->line_state = NULL;
  renderer->priv->line = NULL;

  vogue_renderer_deactivate (renderer);
}

/**
 * vogue_renderer_draw_glyphs:
 * @renderer: a #VogueRenderer
 * @font: a #VogueFont
 * @glyphs: a #VogueGlyphString
 * @x: X position of left edge of baseline, in user space coordinates
 *   in Vogue units.
 * @y: Y position of left edge of baseline, in user space coordinates
 *    in Vogue units.
 *
 * Draws the glyphs in @glyphs with the specified #VogueRenderer.
 *
 * Since: 1.8
 **/
void
vogue_renderer_draw_glyphs (VogueRenderer    *renderer,
			    VogueFont        *font,
			    VogueGlyphString *glyphs,
			    int               x,
			    int               y)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));

  vogue_renderer_activate (renderer);

  PANGO_RENDERER_GET_CLASS (renderer)->draw_glyphs (renderer, font, glyphs, x, y);

  vogue_renderer_deactivate (renderer);
}

static void
vogue_renderer_default_draw_glyphs (VogueRenderer    *renderer,
				    VogueFont        *font,
				    VogueGlyphString *glyphs,
				    int               x,
				    int               y)
{
  int i;
  int x_position = 0;

  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      VogueGlyphInfo *gi = &glyphs->glyphs[i];
      Point p;

      to_device (renderer->matrix,
		 x + x_position + gi->geometry.x_offset,
		 y +              gi->geometry.y_offset,
		 &p);

      vogue_renderer_draw_glyph (renderer, font, gi->glyph, p.x, p.y);

      x_position += gi->geometry.width;
    }
}

/**
 * vogue_renderer_draw_glyph_item:
 * @renderer: a #VogueRenderer
 * @text: (allow-none): the UTF-8 text that @glyph_item refers to, or %NULL
 * @glyph_item: a #VogueGlyphItem
 * @x: X position of left edge of baseline, in user space coordinates
 *   in Vogue units.
 * @y: Y position of left edge of baseline, in user space coordinates
 *    in Vogue units.
 *
 * Draws the glyphs in @glyph_item with the specified #VogueRenderer,
 * embedding the text associated with the glyphs in the output if the
 * output format supports it (PDF for example).
 *
 * Note that @text is the start of the text for layout, which is then
 * indexed by <literal>@glyph_item->item->offset</literal>.
 *
 * If @text is %NULL, this simply calls vogue_renderer_draw_glyphs().
 *
 * The default implementation of this method simply falls back to
 * vogue_renderer_draw_glyphs().
 *
 * Since: 1.22
 **/
void
vogue_renderer_draw_glyph_item (VogueRenderer    *renderer,
				const char       *text,
				VogueGlyphItem   *glyph_item,
				int               x,
				int               y)
{
  if (!text)
    {
      vogue_renderer_draw_glyphs (renderer,
				  glyph_item->item->analysis.font,
				  glyph_item->glyphs,
				  x, y);
      return;
    }

  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));

  vogue_renderer_activate (renderer);

  PANGO_RENDERER_GET_CLASS (renderer)->draw_glyph_item (renderer, text, glyph_item, x, y);

  vogue_renderer_deactivate (renderer);
}

static void
vogue_renderer_default_draw_glyph_item (VogueRenderer    *renderer,
					const char       *text G_GNUC_UNUSED,
					VogueGlyphItem   *glyph_item,
					int               x,
					int               y)
{
  vogue_renderer_draw_glyphs (renderer,
			      glyph_item->item->analysis.font,
			      glyph_item->glyphs,
			      x, y);
}

/**
 * vogue_renderer_draw_rectangle:
 * @renderer: a #VogueRenderer
 * @part: type of object this rectangle is part of
 * @x: X position at which to draw rectangle, in user space coordinates in Vogue units
 * @y: Y position at which to draw rectangle, in user space coordinates in Vogue units
 * @width: width of rectangle in Vogue units in user space coordinates
 * @height: height of rectangle in Vogue units in user space coordinates
 *
 * Draws an axis-aligned rectangle in user space coordinates with the
 * specified #VogueRenderer.
 *
 * This should be called while @renderer is already active.  Use
 * vogue_renderer_activate() to activate a renderer.
 *
 * Since: 1.8
 **/
void
vogue_renderer_draw_rectangle (VogueRenderer   *renderer,
			       VogueRenderPart  part,
			       int              x,
			       int              y,
			       int              width,
			       int              height)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));
  g_return_if_fail (IS_VALID_PART (part));
  g_return_if_fail (renderer->active_count > 0);

  PANGO_RENDERER_GET_CLASS (renderer)->draw_rectangle (renderer, part, x, y, width, height);
}

static int
compare_points (const void *a,
		const void *b)
{
  const Point *pa = a;
  const Point *pb = b;

  if (pa->y < pb->y)
    return -1;
  else if (pa->y > pb->y)
    return 1;
  else if (pa->x < pb->x)
    return -1;
  else if (pa->x > pb->x)
    return 1;
  else
    return 0;
}

static void
draw_rectangle (VogueRenderer   *renderer,
		VogueMatrix     *matrix,
		VogueRenderPart  part,
		int              x,
		int              y,
		int              width,
		int              height)
{
  Point points[4];

  /* Convert the points to device coordinates, and sort
   * in ascending Y order. (Ordering by X for ties)
   */
  to_device (matrix, x, y, &points[0]);
  to_device (matrix, x + width, y, &points[1]);
  to_device (matrix, x, y + height, &points[2]);
  to_device (matrix, x + width, y + height, &points[3]);

  qsort (points, 4, sizeof (Point), compare_points);

  /* There are essentially three cases. (There is a fourth
   * case where trapezoid B is degenerate and we just have
   * two triangles, but we don't need to handle it separately.)
   *
   *     1            2             3
   *
   *     ______       /\           /\
   *    /     /      /A \         /A \
   *   /  B  /      /____\       /____\
   *  /_____/      /  B  /       \  B  \
   *              /_____/         \_____\
   *              \ C  /           \ C  /
   *               \  /             \  /
   *                \/               \/
   */
  if (points[0].y == points[1].y)
    {
     /* Case 1 (pure shear) */
      vogue_renderer_draw_trapezoid (renderer, part,                                      /* B */
				     points[0].y, points[0].x, points[1].x,
				     points[2].y, points[2].x, points[3].x);
    }
  else if (points[1].x < points[2].x)
    {
      /* Case 2 */
      double tmp_width = ((points[2].x - points[0].x) * (points[1].y - points[0].y)) / (points[2].y - points[0].y);
      double base_width = tmp_width + points[0].x - points[1].x;

      vogue_renderer_draw_trapezoid (renderer, part,                                      /* A */
				     points[0].y, points[0].x, points[0].x,
				     points[1].y, points[1].x, points[1].x + base_width);
      vogue_renderer_draw_trapezoid (renderer, part,                                      /* B */
				     points[1].y, points[1].x, points[1].x + base_width,
				     points[2].y, points[2].x - base_width, points[2].x);
      vogue_renderer_draw_trapezoid (renderer, part,                                      /* C */
				     points[2].y, points[2].x - base_width, points[2].x,
				     points[3].y, points[3].x, points[3].x);
    }
  else
    {
      /* case 3 */
      double tmp_width = ((points[0].x - points[2].x) * (points[1].y - points[0].y)) / (points[2].y - points[0].y);
      double base_width = tmp_width + points[1].x - points[0].x;

      vogue_renderer_draw_trapezoid (renderer, part,                                     /* A */
				     points[0].y, points[0].x, points[0].x,
				     points[1].y,  points[1].x - base_width, points[1].x);
      vogue_renderer_draw_trapezoid (renderer, part,                                     /* B */
				     points[1].y, points[1].x - base_width, points[1].x,
				     points[2].y, points[2].x, points[2].x + base_width);
      vogue_renderer_draw_trapezoid (renderer, part,                                     /* C */
				     points[2].y, points[2].x, points[2].x + base_width,
				     points[3].y, points[3].x, points[3].x);
    }
}

static void
vogue_renderer_default_draw_rectangle (VogueRenderer  *renderer,
				       VogueRenderPart part,
				       int             x,
				       int             y,
				       int             width,
				       int             height)
{
  draw_rectangle (renderer, renderer->matrix, part, x, y, width, height);
}

/**
 * vogue_renderer_draw_error_underline:
 * @renderer: a #VogueRenderer
 * @x: X coordinate of underline, in Vogue units in user coordinate system
 * @y: Y coordinate of underline, in Vogue units in user coordinate system
 * @width: width of underline, in Vogue units in user coordinate system
 * @height: height of underline, in Vogue units in user coordinate system
 *
 * Draw a squiggly line that approximately covers the given rectangle
 * in the style of an underline used to indicate a spelling error.
 * (The width of the underline is rounded to an integer number
 * of up/down segments and the resulting rectangle is centered
 * in the original rectangle)
 *
 * This should be called while @renderer is already active.  Use
 * vogue_renderer_activate() to activate a renderer.
 *
 * Since: 1.8
 **/
void
vogue_renderer_draw_error_underline (VogueRenderer *renderer,
				     int            x,
				     int            y,
				     int            width,
				     int            height)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));
  g_return_if_fail (renderer->active_count > 0);

  PANGO_RENDERER_GET_CLASS (renderer)->draw_error_underline (renderer, x, y, width, height);
}

/* We are drawing an error underline that looks like one of:
 *
 *  /\      /\      /\        /\      /\               -
 * /  \    /  \    /  \      /  \    /  \              |
 * \   \  /\   \  /   /      \   \  /\   \             |
 *  \   \/B \   \/ C /        \   \/B \   \            | height = HEIGHT_SQUARES * square
 *   \ A \  /\ A \  /          \ A \  /\ A \           |
 *    \   \/  \   \/            \   \/  \   \          |
 *     \  /    \  /              \  /    \  /          |
 *      \/      \/                \/      \/           -
 *      |---|
 *    unit_width = (HEIGHT_SQUARES - 1) * square
 *
 * To do this conveniently, we work in a coordinate system where A,B,C
 * are axis aligned rectangles. (If fonts were square, the diagrams
 * would be clearer)
 *
 *             (0,0)
 *              /\      /\
 *             /  \    /  \
 *            /\  /\  /\  /
 *           /  \/  \/  \/
 *          /    \  /\  /
 *      Y axis    \/  \/
 *                 \  /\
 *                  \/  \
 *                       \ X axis
 *
 * Note that the long side in this coordinate system is HEIGHT_SQUARES + 1
 * units long
 *
 * The diagrams above are shown with HEIGHT_SQUARES an integer, but
 * that is actually incidental; the value 2.5 below seems better than
 * either HEIGHT_SQUARES=3 (a little long and skinny) or
 * HEIGHT_SQUARES=2 (a bit short and stubby)
 */

#define HEIGHT_SQUARES 2.5

static void
get_total_matrix (VogueMatrix       *total,
		  const VogueMatrix *global,
		  int                x,
		  int                y,
		  int                square)
{
  VogueMatrix local;
  gdouble scale = 0.5 * square;

  /* The local matrix translates from the axis aligned coordinate system
   * to the original user space coordinate system.
   */
  local.xx = scale;
  local.xy = - scale;
  local.yx = scale;
  local.yy = scale;
  local.x0 = 0;
  local.y0 = 0;

  *total = *global;
  vogue_matrix_concat (total, &local);

  total->x0 = (global->xx * x + global->xy * y) / PANGO_SCALE + global->x0;
  total->y0 = (global->yx * x + global->yy * y) / PANGO_SCALE + global->y0;
}

static void
vogue_renderer_default_draw_error_underline (VogueRenderer *renderer,
					     int            x,
					     int            y,
					     int            width,
					     int            height)
{
  int square = height / HEIGHT_SQUARES;
  int unit_width = (HEIGHT_SQUARES - 1) * square;
  int width_units = (width + unit_width / 2) / unit_width;
  const VogueMatrix identity = PANGO_MATRIX_INIT;
  const VogueMatrix *matrix;
  double dx, dx0, dy0;
  VogueMatrix total;
  int i;

  x += (width - width_units * unit_width) / 2;

  if (renderer->matrix)
    matrix = renderer->matrix;
  else
    matrix = &identity;

  get_total_matrix (&total, matrix, x, y, square);
  dx = unit_width * 2;
  dx0 = (matrix->xx * dx) / PANGO_SCALE;
  dy0 = (matrix->yx * dx) / PANGO_SCALE;

  i = (width_units - 1) / 2;
  while (TRUE)
    {
      draw_rectangle (renderer, &total, PANGO_RENDER_PART_UNDERLINE, /* A */
		      0,                      0,
		      HEIGHT_SQUARES * 2 - 1, 1);

      if (i <= 0)
        break;
      i--;

      draw_rectangle (renderer, &total, PANGO_RENDER_PART_UNDERLINE, /* B */
		      HEIGHT_SQUARES * 2 - 2, - (HEIGHT_SQUARES * 2 - 3),
		      1,                      HEIGHT_SQUARES * 2 - 3);

      total.x0 += dx0;
      total.y0 += dy0;
    }
  if (width_units % 2 == 0)
    {
      draw_rectangle (renderer, &total, PANGO_RENDER_PART_UNDERLINE, /* C */
		      HEIGHT_SQUARES * 2 - 2, - (HEIGHT_SQUARES * 2 - 2),
		      1,                      HEIGHT_SQUARES * 2 - 2);
    }
}

/**
 * vogue_renderer_draw_trapezoid:
 * @renderer: a #VogueRenderer
 * @part: type of object this trapezoid is part of
 * @y1_: Y coordinate of top of trapezoid
 * @x11: X coordinate of left end of top of trapezoid
 * @x21: X coordinate of right end of top of trapezoid
 * @y2: Y coordinate of bottom of trapezoid
 * @x12: X coordinate of left end of bottom of trapezoid
 * @x22: X coordinate of right end of bottom of trapezoid
 *
 * Draws a trapezoid with the parallel sides aligned with the X axis
 * using the given #VogueRenderer; coordinates are in device space.
 *
 * Since: 1.8
 **/
void
vogue_renderer_draw_trapezoid (VogueRenderer  *renderer,
			       VogueRenderPart part,
			       double          y1_,
			       double          x11,
			       double          x21,
			       double          y2,
			       double          x12,
			       double          x22)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));
  g_return_if_fail (renderer->active_count > 0);

  if (PANGO_RENDERER_GET_CLASS (renderer)->draw_trapezoid)
    PANGO_RENDERER_GET_CLASS (renderer)->draw_trapezoid (renderer, part,
							 y1_, x11, x21,
							 y2, x12, x22);
}

/**
 * vogue_renderer_draw_glyph:
 * @renderer: a #VogueRenderer
 * @font: a #VogueFont
 * @glyph: the glyph index of a single glyph
 * @x: X coordinate of left edge of baseline of glyph
 * @y: Y coordinate of left edge of baseline of glyph
 *
 * Draws a single glyph with coordinates in device space.
 *
 * Since: 1.8
 **/
void
vogue_renderer_draw_glyph (VogueRenderer *renderer,
			   VogueFont     *font,
			   VogueGlyph     glyph,
			   double         x,
			   double         y)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));
  g_return_if_fail (renderer->active_count > 0);

  if (glyph == PANGO_GLYPH_EMPTY) /* glyph PANGO_GLYPH_EMPTY never renders */
    return;

  if (PANGO_RENDERER_GET_CLASS (renderer)->draw_glyph)
    PANGO_RENDERER_GET_CLASS (renderer)->draw_glyph (renderer, font, glyph, x, y);
}

/**
 * vogue_renderer_activate:
 * @renderer: a #VogueRenderer
 *
 * Does initial setup before rendering operations on @renderer.
 * vogue_renderer_deactivate() should be called when done drawing.
 * Calls such as vogue_renderer_draw_layout() automatically
 * activate the layout before drawing on it. Calls to
 * vogue_renderer_activate() and vogue_renderer_deactivate() can
 * be nested and the renderer will only be initialized and
 * deinitialized once.
 *
 * Since: 1.8
 **/
void
vogue_renderer_activate (VogueRenderer *renderer)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));

  renderer->active_count++;
  if (renderer->active_count == 1)
    {
      if (PANGO_RENDERER_GET_CLASS (renderer)->begin)
	PANGO_RENDERER_GET_CLASS (renderer)->begin (renderer);
    }
}

/**
 * vogue_renderer_deactivate:
 * @renderer: a #VogueRenderer
 *
 * Cleans up after rendering operations on @renderer. See
 * docs for vogue_renderer_activate().
 *
 * Since: 1.8
 **/
void
vogue_renderer_deactivate (VogueRenderer *renderer)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));
  g_return_if_fail (renderer->active_count > 0);

  if (renderer->active_count == 1)
    {
      if (PANGO_RENDERER_GET_CLASS (renderer)->end)
	PANGO_RENDERER_GET_CLASS (renderer)->end (renderer);
    }
  renderer->active_count--;
}

/**
 * vogue_renderer_set_color:
 * @renderer: a #VogueRenderer
 * @part: the part to change the color of
 * @color: (allow-none): the new color or %NULL to unset the current color
 *
 * Sets the color for part of the rendering.
 * Also see vogue_renderer_set_alpha().
 *
 * Since: 1.8
 **/
void
vogue_renderer_set_color (VogueRenderer    *renderer,
			  VogueRenderPart   part,
			  const VogueColor *color)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));
  g_return_if_fail (IS_VALID_PART (part));

  if ((!color && !renderer->priv->color_set[part]) ||
      (color && renderer->priv->color_set[part] &&
       renderer->priv->color[part].red == color->red &&
       renderer->priv->color[part].green == color->green &&
       renderer->priv->color[part].blue == color->blue))
    return;

  vogue_renderer_part_changed (renderer, part);

  if (color)
    {
      renderer->priv->color_set[part] = TRUE;
      renderer->priv->color[part] = *color;
    }
  else
    {
      renderer->priv->color_set[part] = FALSE;
    }
}

/**
 * vogue_renderer_get_color:
 * @renderer: a #VogueRenderer
 * @part: the part to get the color for
 *
 * Gets the current rendering color for the specified part.
 *
 * Return value: (transfer none) (nullable): the color for the
 *   specified part, or %NULL if it hasn't been set and should be
 *   inherited from the environment.
 *
 * Since: 1.8
 **/
VogueColor *
vogue_renderer_get_color (VogueRenderer   *renderer,
			  VogueRenderPart  part)
{
  g_return_val_if_fail (PANGO_IS_RENDERER_FAST (renderer), NULL);
  g_return_val_if_fail (IS_VALID_PART (part), NULL);

  if (renderer->priv->color_set[part])
    return &renderer->priv->color[part];
  else
    return NULL;
}

/**
 * vogue_renderer_set_alpha:
 * @renderer: a #VogueRenderer
 * @part: the part to set the alpha for
 * @alpha: an alpha value between 1 and 65536, or 0 to unset the alpha
 *
 * Sets the alpha for part of the rendering.
 * Note that the alpha may only be used if a color is
 * specified for @part as well.
 *
 * Since: 1.38
 */
void
vogue_renderer_set_alpha (VogueRenderer   *renderer,
                          VogueRenderPart  part,
                          guint16          alpha)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));
  g_return_if_fail (IS_VALID_PART (part));

  if ((!alpha && !renderer->priv->alpha[part]) ||
      (alpha && renderer->priv->alpha[part] &&
       renderer->priv->alpha[part] == alpha))
    return;

  vogue_renderer_part_changed (renderer, part);

  renderer->priv->alpha[part] = alpha;
}

/**
 * vogue_renderer_get_alpha:
 * @renderer: a #VogueRenderer
 * @part: the part to get the alpha for
 *
 * Gets the current alpha for the specified part.
 *
 * Return value: the alpha for the specified part,
 *   or 0 if it hasn't been set and should be
 *   inherited from the environment.
 *
 * Since: 1.38
 */
guint16
vogue_renderer_get_alpha (VogueRenderer   *renderer,
                          VogueRenderPart  part)
{
  g_return_val_if_fail (PANGO_IS_RENDERER_FAST (renderer), 0);
  g_return_val_if_fail (IS_VALID_PART (part), 0);

  return renderer->priv->alpha[part];
}

/**
 * vogue_renderer_part_changed:
 * @renderer: a #VogueRenderer
 * @part: the part for which rendering has changed.
 *
 * Informs Vogue that the way that the rendering is done
 * for @part has changed in a way that would prevent multiple
 * pieces being joined together into one drawing call. For
 * instance, if a subclass of #VogueRenderer was to add a stipple
 * option for drawing underlines, it needs to call
 *
 * <informalexample><programlisting>
 * vogue_renderer_part_changed (render, PANGO_RENDER_PART_UNDERLINE);
 * </programlisting></informalexample>
 *
 * When the stipple changes or underlines with different stipples
 * might be joined together. Vogue automatically calls this for
 * changes to colors. (See vogue_renderer_set_color())
 *
 * Since: 1.8
 **/
void
vogue_renderer_part_changed (VogueRenderer    *renderer,
			     VogueRenderPart   part)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));
  g_return_if_fail (IS_VALID_PART (part));
  g_return_if_fail (renderer->active_count > 0);

  handle_line_state_change (renderer, part);

  if (PANGO_RENDERER_GET_CLASS (renderer)->part_changed)
    PANGO_RENDERER_GET_CLASS (renderer)->part_changed (renderer, part);
}

/**
 * vogue_renderer_prepare_run:
 * @renderer: a #VogueRenderer
 * @run: a #VogueLayoutRun
 *
 * Set up the state of the #VogueRenderer for rendering @run.
 *
 * Since: 1.8
 **/
static void
vogue_renderer_prepare_run (VogueRenderer  *renderer,
			    VogueLayoutRun *run)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));

  PANGO_RENDERER_GET_CLASS (renderer)->prepare_run (renderer, run);
}

static void
vogue_renderer_default_prepare_run (VogueRenderer  *renderer,
				    VogueLayoutRun *run)
{
  VogueColor *fg_color = NULL;
  VogueColor *bg_color = NULL;
  VogueColor *underline_color = NULL;
  VogueColor *strikethrough_color = NULL;
  guint16 fg_alpha = 0;
  guint16 bg_alpha = 0;
  GSList *l;

  renderer->underline = PANGO_UNDERLINE_NONE;
  renderer->strikethrough = FALSE;

  for (l = run->item->analysis.extra_attrs; l; l = l->next)
    {
      VogueAttribute *attr = l->data;

      switch ((int) attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  renderer->underline = ((VogueAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_STRIKETHROUGH:
	  renderer->strikethrough = ((VogueAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_FOREGROUND:
	  fg_color = &((VogueAttrColor *)attr)->color;
	  break;

	case PANGO_ATTR_BACKGROUND:
	  bg_color = &((VogueAttrColor *)attr)->color;
	  break;

	case PANGO_ATTR_UNDERLINE_COLOR:
	  underline_color = &((VogueAttrColor *)attr)->color;
	  break;

	case PANGO_ATTR_STRIKETHROUGH_COLOR:
	  strikethrough_color = &((VogueAttrColor *)attr)->color;
	  break;

	case PANGO_ATTR_FOREGROUND_ALPHA:
          fg_alpha = ((VogueAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_BACKGROUND_ALPHA:
          bg_alpha = ((VogueAttrInt *)attr)->value;
	  break;

	default:
	  break;
	}
    }

  if (!underline_color)
    underline_color = fg_color;

  if (!strikethrough_color)
    strikethrough_color = fg_color;

  vogue_renderer_set_color (renderer, PANGO_RENDER_PART_FOREGROUND, fg_color);
  vogue_renderer_set_color (renderer, PANGO_RENDER_PART_BACKGROUND, bg_color);
  vogue_renderer_set_color (renderer, PANGO_RENDER_PART_UNDERLINE, underline_color);
  vogue_renderer_set_color (renderer, PANGO_RENDER_PART_STRIKETHROUGH, strikethrough_color);

  vogue_renderer_set_alpha (renderer, PANGO_RENDER_PART_FOREGROUND, fg_alpha);
  vogue_renderer_set_alpha (renderer, PANGO_RENDER_PART_BACKGROUND, bg_alpha);
  vogue_renderer_set_alpha (renderer, PANGO_RENDER_PART_UNDERLINE, fg_alpha);
  vogue_renderer_set_alpha (renderer, PANGO_RENDER_PART_STRIKETHROUGH, fg_alpha);
}

/**
 * vogue_renderer_set_matrix:
 * @renderer: a #VogueRenderer
 * @matrix: (allow-none): a #VogueMatrix, or %NULL to unset any existing matrix.
 *  (No matrix set is the same as setting the identity matrix.)
 *
 * Sets the transformation matrix that will be applied when rendering.
 *
 * Since: 1.8
 **/
void
vogue_renderer_set_matrix (VogueRenderer     *renderer,
			   const VogueMatrix *matrix)
{
  g_return_if_fail (PANGO_IS_RENDERER_FAST (renderer));

  vogue_matrix_free (renderer->matrix);
  renderer->matrix = vogue_matrix_copy (matrix);
}

/**
 * vogue_renderer_get_matrix:
 * @renderer: a #VogueRenderer
 *
 * Gets the transformation matrix that will be applied when
 * rendering. See vogue_renderer_set_matrix().
 *
 * Return value: (nullable): the matrix, or %NULL if no matrix has
 *  been set (which is the same as the identity matrix). The returned
 *  matrix is owned by Vogue and must not be modified or freed.
 *
 * Since: 1.8
 **/
const VogueMatrix *
vogue_renderer_get_matrix (VogueRenderer *renderer)
{
  g_return_val_if_fail (PANGO_IS_RENDERER (renderer), NULL);

  return renderer->matrix;
}

/**
 * vogue_renderer_get_layout:
 * @renderer: a #VogueRenderer
 *
 * Gets the layout currently being rendered using @renderer.
 * Calling this function only makes sense from inside a subclass's
 * methods, like in its draw_shape vfunc, for example.
 *
 * The returned layout should not be modified while still being
 * rendered.
 *
 * Return value: (transfer none) (nullable): the layout, or %NULL if
 *  no layout is being rendered using @renderer at this time.
 *
 * Since: 1.20
 **/
VogueLayout *
vogue_renderer_get_layout (VogueRenderer *renderer)
{
  if (G_UNLIKELY (renderer->priv->line == NULL))
    return NULL;

  return renderer->priv->line->layout;
}

/**
 * vogue_renderer_get_layout_line:
 * @renderer: a #VogueRenderer
 *
 * Gets the layout line currently being rendered using @renderer.
 * Calling this function only makes sense from inside a subclass's
 * methods, like in its draw_shape vfunc, for example.
 *
 * The returned layout line should not be modified while still being
 * rendered.
 *
 * Return value: (transfer none) (nullable): the layout line, or %NULL
 *   if no layout line is being rendered using @renderer at this time.
 *
 * Since: 1.20
 **/
VogueLayoutLine *
vogue_renderer_get_layout_line (VogueRenderer     *renderer)
{
  return renderer->priv->line;
}
