/* Vogue
 * vogue-fontset.h: Font set handling
 *
 * Copyright (C) 2001 Red Hat Software
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

#ifndef __PANGO_FONTSET_H__
#define __PANGO_FONTSET_H__

#include <vogue/vogue-coverage.h>
#include <vogue/vogue-types.h>

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * VogueFontset
 */

/**
 * PANGO_TYPE_FONTSET:
 *
 * The #GObject type for #VogueFontset.
 */
#define PANGO_TYPE_FONTSET              (vogue_fontset_get_type ())
#define PANGO_FONTSET(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONTSET, VogueFontset))
#define PANGO_IS_FONTSET(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONTSET))

PANGO_AVAILABLE_IN_ALL
GType vogue_fontset_get_type (void) G_GNUC_CONST;

typedef struct _VogueFontset        VogueFontset;

/**
 * VogueFontsetForeachFunc:
 * @fontset: a #VogueFontset
 * @font: a font from @fontset
 * @user_data: callback data
 *
 * A callback function used by vogue_fontset_foreach() when enumerating
 * the fonts in a fontset.
 *
 * Returns: if %TRUE, stop iteration and return immediately.
 *
 * Since: 1.4
 **/
typedef gboolean (*VogueFontsetForeachFunc) (VogueFontset  *fontset,
					     VogueFont     *font,
					     gpointer       user_data);

PANGO_AVAILABLE_IN_ALL
VogueFont *       vogue_fontset_get_font    (VogueFontset           *fontset,
					     guint                   wc);
PANGO_AVAILABLE_IN_ALL
VogueFontMetrics *vogue_fontset_get_metrics (VogueFontset           *fontset);
PANGO_AVAILABLE_IN_1_4
void              vogue_fontset_foreach     (VogueFontset           *fontset,
					     VogueFontsetForeachFunc func,
					     gpointer                data);


G_END_DECLS

#endif /* __PANGO_FONTSET_H__ */
