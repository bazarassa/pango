/* Vogue
 * vogue-tabs.h: Tab-related stuff
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

#ifndef __PANGO_TABS_H__
#define __PANGO_TABS_H__

#include <vogue/vogue-types.h>

G_BEGIN_DECLS

typedef struct _VogueTabArray VogueTabArray;

/**
 * VogueTabAlign:
 * @PANGO_TAB_LEFT: the tab stop appears to the left of the text.
 *
 * A #VogueTabAlign specifies where a tab stop appears relative to the text.
 */
typedef enum
{
  PANGO_TAB_LEFT

  /* These are not supported now, but may be in the
   * future.
   *
   *  PANGO_TAB_RIGHT,
   *  PANGO_TAB_CENTER,
   *  PANGO_TAB_NUMERIC
   */
} VogueTabAlign;

/**
 * PANGO_TYPE_TAB_ARRAY:
 *
 * The #GObject type for #VogueTabArray.
 */
#define PANGO_TYPE_TAB_ARRAY (vogue_tab_array_get_type ())

PANGO_AVAILABLE_IN_ALL
VogueTabArray  *vogue_tab_array_new                 (gint           initial_size,
						     gboolean       positions_in_pixels);
PANGO_AVAILABLE_IN_ALL
VogueTabArray  *vogue_tab_array_new_with_positions  (gint           size,
						     gboolean       positions_in_pixels,
						     VogueTabAlign  first_alignment,
						     gint           first_position,
						     ...);
PANGO_AVAILABLE_IN_ALL
GType           vogue_tab_array_get_type            (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
VogueTabArray  *vogue_tab_array_copy                (VogueTabArray *src);
PANGO_AVAILABLE_IN_ALL
void            vogue_tab_array_free                (VogueTabArray *tab_array);
PANGO_AVAILABLE_IN_ALL
gint            vogue_tab_array_get_size            (VogueTabArray *tab_array);
PANGO_AVAILABLE_IN_ALL
void            vogue_tab_array_resize              (VogueTabArray *tab_array,
						     gint           new_size);
PANGO_AVAILABLE_IN_ALL
void            vogue_tab_array_set_tab             (VogueTabArray *tab_array,
						     gint           tab_index,
						     VogueTabAlign  alignment,
						     gint           location);
PANGO_AVAILABLE_IN_ALL
void            vogue_tab_array_get_tab             (VogueTabArray *tab_array,
						     gint           tab_index,
						     VogueTabAlign *alignment,
						     gint          *location);
PANGO_AVAILABLE_IN_ALL
void            vogue_tab_array_get_tabs            (VogueTabArray *tab_array,
						     VogueTabAlign **alignments,
						     gint          **locations);

PANGO_AVAILABLE_IN_ALL
gboolean        vogue_tab_array_get_positions_in_pixels (VogueTabArray *tab_array);


G_END_DECLS

#endif /* __PANGO_TABS_H__ */
