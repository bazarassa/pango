/* Vogue
 * voguecairo-coretext.h:
 *
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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

#ifndef __PANGOCAIRO_CORETEXT_H__
#define __PANGOCAIRO_CORETEXT_H__

#include "voguecoretext-private.h"
#include <vogue/voguecairo.h>
#include <cairo-quartz.h>

G_BEGIN_DECLS

#define PANGO_TYPE_CAIRO_CORE_TEXT_FONT_MAP       (vogue_cairo_core_text_font_map_get_type ())
#define PANGO_CAIRO_CORE_TEXT_FONT_MAP(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_CORE_TEXT_FONT_MAP, VogueCairoCoreTextFontMap))
#define PANGO_IS_CAIRO_CORE_TEXT_FONT_MAP(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_CORE_TEXT_FONT_MAP))

typedef struct _VogueCairoCoreTextFontMap VogueCairoCoreTextFontMap;

struct _VogueCairoCoreTextFontMap
{
  VogueCoreTextFontMap parent_instance;

  guint serial;
  gdouble dpi;
};

_PANGO_EXTERN
GType vogue_cairo_core_text_font_map_get_type (void) G_GNUC_CONST;

VogueCoreTextFont *
_vogue_cairo_core_text_font_new (VogueCairoCoreTextFontMap  *cafontmap,
                                 VogueCoreTextFontKey       *key);

G_END_DECLS

#endif /* __PANGOCAIRO_CORETEXT_H__ */
