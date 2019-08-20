/* Vogue
 * voguecairo-fontmap.c: Cairo font handling
 *
 * Copyright (C) 2000-2005 Red Hat Software
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

#include "voguecairo.h"
#include "voguecairo-private.h"
#include "vogue-impl-utils.h"

#include <string.h>

typedef struct _VogueCairoContextInfo VogueCairoContextInfo;

struct _VogueCairoContextInfo
{
  double dpi;
  gboolean set_options_explicit;

  cairo_font_options_t *set_options;
  cairo_font_options_t *surface_options;
  cairo_font_options_t *merged_options;

  VogueCairoShapeRendererFunc shape_renderer_func;
  gpointer                    shape_renderer_data;
  GDestroyNotify              shape_renderer_notify;
};

static void
free_context_info (VogueCairoContextInfo *info)
{
  if (info->set_options)
    cairo_font_options_destroy (info->set_options);
  if (info->surface_options)
    cairo_font_options_destroy (info->surface_options);
  if (info->merged_options)
    cairo_font_options_destroy (info->merged_options);

  if (info->shape_renderer_notify)
    info->shape_renderer_notify (info->shape_renderer_data);

  g_slice_free (VogueCairoContextInfo, info);
}

static VogueCairoContextInfo *
get_context_info (VogueContext *context,
		  gboolean      create)
{
  static GQuark context_info_quark; /* MT-safe */
  VogueCairoContextInfo *info;

  if (G_UNLIKELY (!context_info_quark))
    context_info_quark = g_quark_from_static_string ("vogue-cairo-context-info");

retry:
  info = g_object_get_qdata (G_OBJECT (context), context_info_quark);

  if (G_UNLIKELY (!info) && create)
    {
      info = g_slice_new0 (VogueCairoContextInfo);
      info->dpi = -1.0;

      if (!g_object_replace_qdata (G_OBJECT (context), context_info_quark, NULL,
                                   info, (GDestroyNotify)free_context_info,
                                   NULL))
        {
          free_context_info (info);
          goto retry;
        }
    }

  return info;
}

static void
_vogue_cairo_update_context (cairo_t      *cr,
			     VogueContext *context)
{
  VogueCairoContextInfo *info;
  cairo_matrix_t cairo_matrix;
  cairo_surface_t *target;
  VogueMatrix vogue_matrix;
  const VogueMatrix *current_matrix, identity_matrix = PANGO_MATRIX_INIT;
  const cairo_font_options_t *merged_options;
  cairo_font_options_t *old_merged_options;
  gboolean changed = FALSE;

  info = get_context_info (context, TRUE);

  target = cairo_get_target (cr);

  if (!info->surface_options)
    info->surface_options = cairo_font_options_create ();
  cairo_surface_get_font_options (target, info->surface_options);
  if (!info->set_options_explicit)
  {
    if (!info->set_options)
      info->set_options = cairo_font_options_create ();
    cairo_get_font_options (cr, info->set_options);
  }

  old_merged_options = info->merged_options;
  info->merged_options = NULL;

  merged_options = _vogue_cairo_context_get_merged_font_options (context);

  if (old_merged_options)
    {
      if (!cairo_font_options_equal (merged_options, old_merged_options))
	changed = TRUE;
      cairo_font_options_destroy (old_merged_options);
      old_merged_options = NULL;
    }
  else
    changed = TRUE;

  cairo_get_matrix (cr, &cairo_matrix);
  vogue_matrix.xx = cairo_matrix.xx;
  vogue_matrix.yx = cairo_matrix.yx;
  vogue_matrix.xy = cairo_matrix.xy;
  vogue_matrix.yy = cairo_matrix.yy;
  vogue_matrix.x0 = 0;
  vogue_matrix.y0 = 0;

  current_matrix = vogue_context_get_matrix (context);
  if (!current_matrix)
    current_matrix = &identity_matrix;

  /* layout is matrix-independent if metrics-hinting is off.
   * also ignore matrix translation offsets */
  if ((cairo_font_options_get_hint_metrics (merged_options) != CAIRO_HINT_METRICS_OFF) &&
      (0 != memcmp (&vogue_matrix, current_matrix, sizeof (VogueMatrix))))
    changed = TRUE;

  vogue_context_set_matrix (context, &vogue_matrix);

  if (changed)
    vogue_context_changed (context);
}

/**
 * vogue_cairo_update_context:
 * @cr: a Cairo context
 * @context: a #VogueContext, from a voguecairo font map
 *
 * Updates a #VogueContext previously created for use with Cairo to
 * match the current transformation and target surface of a Cairo
 * context. If any layouts have been created for the context,
 * it's necessary to call vogue_layout_context_changed() on those
 * layouts.
 *
 * Since: 1.10
 **/
void
vogue_cairo_update_context (cairo_t      *cr,
			    VogueContext *context)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO_IS_CONTEXT (context));

  _vogue_cairo_update_context (cr, context);
}

/**
 * vogue_cairo_context_set_resolution:
 * @context: a #VogueContext, from a voguecairo font map
 * @dpi: the resolution in "dots per inch". (Physical inches aren't actually
 *   involved; the terminology is conventional.) A 0 or negative value
 *   means to use the resolution from the font map.
 *
 * Sets the resolution for the context. This is a scale factor between
 * points specified in a #VogueFontDescription and Cairo units. The
 * default value is 96, meaning that a 10 point font will be 13
 * units high. (10 * 96. / 72. = 13.3).
 *
 * Since: 1.10
 **/
void
vogue_cairo_context_set_resolution (VogueContext *context,
				    double        dpi)
{
  VogueCairoContextInfo *info = get_context_info (context, TRUE);
  info->dpi = dpi;
}

/**
 * vogue_cairo_context_get_resolution:
 * @context: a #VogueContext, from a voguecairo font map
 *
 * Gets the resolution for the context. See vogue_cairo_context_set_resolution()
 *
 * Return value: the resolution in "dots per inch". A negative value will
 *  be returned if no resolution has previously been set.
 *
 * Since: 1.10
 **/
double
vogue_cairo_context_get_resolution (VogueContext *context)
{
  VogueCairoContextInfo *info = get_context_info (context, FALSE);

  if (info)
    return info->dpi;
  else
    return -1.0;
}

/**
 * vogue_cairo_context_set_font_options:
 * @context: a #VogueContext, from a voguecairo font map
 * @options: (nullable): a #cairo_font_options_t, or %NULL to unset
 *           any previously set options. A copy is made.
 *
 * Sets the font options used when rendering text with this context.
 * These options override any options that vogue_cairo_update_context()
 * derives from the target surface.
 *
 * Since: 1.10
 */
void
vogue_cairo_context_set_font_options (VogueContext               *context,
				      const cairo_font_options_t *options)
{
  VogueCairoContextInfo *info;

  g_return_if_fail (PANGO_IS_CONTEXT (context));

  info  = get_context_info (context, TRUE);

  if (info->set_options || options)
    vogue_context_changed (context);

 if (info->set_options)
    cairo_font_options_destroy (info->set_options);

  if (options)
  {
    info->set_options = cairo_font_options_copy (options);
    info->set_options_explicit = TRUE;
  }
  else
  {
    info->set_options = NULL;
    info->set_options_explicit = FALSE;
  }

  if (info->merged_options)
    {
      cairo_font_options_destroy (info->merged_options);
      info->merged_options = NULL;
    }
}

/**
 * vogue_cairo_context_get_font_options:
 * @context: a #VogueContext, from a voguecairo font map
 *
 * Retrieves any font rendering options previously set with
 * vogue_cairo_context_set_font_options(). This function does not report options
 * that are derived from the target surface by vogue_cairo_update_context()
 *
 * Return value: (nullable): the font options previously set on the
 *   context, or %NULL if no options have been set. This value is
 *   owned by the context and must not be modified or freed.
 *
 * Since: 1.10
 **/
const cairo_font_options_t *
vogue_cairo_context_get_font_options (VogueContext *context)
{
  VogueCairoContextInfo *info;

  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  info = get_context_info (context, FALSE);

  if (info)
    return info->set_options;
  else
    return NULL;
}

/**
 * _vogue_cairo_context_merge_font_options:
 * @context: a #VogueContext
 * @options: a #cairo_font_options_t
 *
 * Merge together options from the target surface and explicitly set
 * on the context.
 *
 * Return value: the combined set of font options. This value is owned
 * by the context and must not be modified or freed.
 **/
const cairo_font_options_t *
_vogue_cairo_context_get_merged_font_options (VogueContext *context)
{
  VogueCairoContextInfo *info = get_context_info (context, TRUE);

  if (!info->merged_options)
    {
      info->merged_options = cairo_font_options_create ();

      if (info->surface_options)
	cairo_font_options_merge (info->merged_options, info->surface_options);
      if (info->set_options)
	cairo_font_options_merge (info->merged_options, info->set_options);
    }

  return info->merged_options;
}

/**
 * vogue_cairo_context_set_shape_renderer:
 * @context: a #VogueContext, from a voguecairo font map
 * @func: (nullable): Callback function for rendering attributes of
 *        type %PANGO_ATTR_SHAPE, or %NULL to disable shape rendering.
 * @data: User data that will be passed to @func.
 * @dnotify: Callback that will be called when the
 *           context is freed to release @data, or %NULL.
 *
 * Sets callback function for context to use for rendering attributes
 * of type %PANGO_ATTR_SHAPE.  See #VogueCairoShapeRendererFunc for
 * details.
 *
 * Since: 1.18
 */
void
vogue_cairo_context_set_shape_renderer (VogueContext                *context,
					VogueCairoShapeRendererFunc  func,
					gpointer                     data,
					GDestroyNotify               dnotify)
{
  VogueCairoContextInfo *info;

  g_return_if_fail (PANGO_IS_CONTEXT (context));

  info  = get_context_info (context, TRUE);

  if (info->shape_renderer_notify)
    info->shape_renderer_notify (info->shape_renderer_data);

  info->shape_renderer_func   = func;
  info->shape_renderer_data   = data;
  info->shape_renderer_notify = dnotify;
}

/**
 * vogue_cairo_context_get_shape_renderer: (skip)
 * @context: a #VogueContext, from a voguecairo font map
 * @data: Pointer to #gpointer to return user data
 *
 * Sets callback function for context to use for rendering attributes
 * of type %PANGO_ATTR_SHAPE.  See #VogueCairoShapeRendererFunc for
 * details.
 *
 * Retrieves callback function and associated user data for rendering
 * attributes of type %PANGO_ATTR_SHAPE as set by
 * vogue_cairo_context_set_shape_renderer(), if any.
 *
 * Return value: (nullable): the shape rendering callback previously
 *   set on the context, or %NULL if no shape rendering callback have
 *   been set.
 *
 * Since: 1.18
 */
VogueCairoShapeRendererFunc
vogue_cairo_context_get_shape_renderer (VogueContext                *context,
					gpointer                    *data)
{
  VogueCairoContextInfo *info;

  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  info = get_context_info (context, FALSE);

  if (info)
    {
      if (data)
        *data = info->shape_renderer_data;
      return info->shape_renderer_func;
    }
  else
    {
      if (data)
        *data = NULL;
      return NULL;
    }
}

/**
 * vogue_cairo_create_context:
 * @cr: a Cairo context
 *
 * Creates a context object set up to match the current transformation
 * and target surface of the Cairo context.  This context can then be
 * used to create a layout using vogue_layout_new().
 *
 * This function is a convenience function that creates a context using
 * the default font map, then updates it to @cr.  If you just need to
 * create a layout for use with @cr and do not need to access #VogueContext
 * directly, you can use vogue_cairo_create_layout() instead.
 *
 * Return value: (transfer full): the newly created #VogueContext. Free with
 *   g_object_unref().
 *
 * Since: 1.22
 **/
VogueContext *
vogue_cairo_create_context (cairo_t *cr)
{
  VogueFontMap *fontmap;
  VogueContext *context;

  g_return_val_if_fail (cr != NULL, NULL);

  fontmap = vogue_cairo_font_map_get_default ();
  context = vogue_font_map_create_context (fontmap);
  vogue_cairo_update_context (cr, context);

  return context;
}

/**
 * vogue_cairo_create_layout:
 * @cr: a Cairo context
 *
 * Creates a layout object set up to match the current transformation
 * and target surface of the Cairo context.  This layout can then be
 * used for text measurement with functions like
 * vogue_layout_get_size() or drawing with functions like
 * vogue_cairo_show_layout(). If you change the transformation
 * or target surface for @cr, you need to call vogue_cairo_update_layout()
 *
 * This function is the most convenient way to use Cairo with Vogue,
 * however it is slightly inefficient since it creates a separate
 * #VogueContext object for each layout. This might matter in an
 * application that was laying out large amounts of text.
 *
 * Return value: (transfer full): the newly created #VogueLayout. Free with
 *   g_object_unref().
 *
 * Since: 1.10
 **/
VogueLayout *
vogue_cairo_create_layout  (cairo_t *cr)
{
  VogueContext *context;
  VogueLayout *layout;

  g_return_val_if_fail (cr != NULL, NULL);

  context = vogue_cairo_create_context (cr);
  layout = vogue_layout_new (context);
  g_object_unref (context);

  return layout;
}

/**
 * vogue_cairo_update_layout:
 * @cr: a Cairo context
 * @layout: a #VogueLayout, from vogue_cairo_create_layout()
 *
 * Updates the private #VogueContext of a #VogueLayout created with
 * vogue_cairo_create_layout() to match the current transformation
 * and target surface of a Cairo context.
 *
 * Since: 1.10
 **/
void
vogue_cairo_update_layout (cairo_t     *cr,
			   VogueLayout *layout)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  _vogue_cairo_update_context (cr, vogue_layout_get_context (layout));
}

