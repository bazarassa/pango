/* Vogue
 * vogue-ot-private.h: Implementation details for Vogue OpenType code
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

#ifndef __PANGO_OT_PRIVATE_H__
#define __PANGO_OT_PRIVATE_H__

#include <glib-object.h>

#include <vogue/vogue-ot.h>
#include <hb-ot.h>
#include <hb-ft.h>
#include <hb-glib.h>

#include "voguefc-private.h"

G_BEGIN_DECLS

typedef struct _VogueOTInfoClass VogueOTInfoClass;

/**
 * VogueOTInfo:
 *
 * The #VogueOTInfo struct contains the various
 * tables associated with an OpenType font. It contains only private fields and
 * should only be accessed via the <function>vogue_ot_info_*</function> functions
 * which are documented below. To obtain a #VogueOTInfo,
 * use vogue_ot_info_get().
 */
struct _VogueOTInfo
{
  GObject parent_instance;

  FT_Face face;
  hb_face_t *hb_face;
};

struct _VogueOTInfoClass
{
  GObjectClass parent_class;
};


typedef struct _VogueOTRulesetClass VogueOTRulesetClass;

struct _VogueOTRuleset
{
  GObject parent_instance;
};

struct _VogueOTRulesetClass
{
  GObjectClass parent_class;
};

/**
 * VogueOTBuffer:
 *
 * The #VogueOTBuffer structure is used to store strings of glyphs associated
 * with a #VogueFcFont, suitable for OpenType layout processing.  It contains
 * only private fields and should only be accessed via the
 * <function>vogue_ot_buffer_*</function> functions which are documented below.
 * To obtain a #VogueOTBuffer, use vogue_ot_buffer_new().
 */
struct _VogueOTBuffer
{
  hb_buffer_t *buffer;
};

G_END_DECLS

#endif /* __PANGO_OT_PRIVATE_H__ */
