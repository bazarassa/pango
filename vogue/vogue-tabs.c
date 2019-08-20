/* Vogue
 * vogue-tabs.c: Tab-related stuff
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

/**
 * SECTION:tab-stops
 * @short_description:Structures for storing tab stops
 * @title:Tab Stops
 *
 * Functions in this section are used to deal with #VogueTabArray objects
 * that can be used to set tab stop positions in a #VogueLayout.
 */
#include "config.h"
#include "vogue-tabs.h"
#include "vogue-impl-utils.h"
#include <string.h>

typedef struct _VogueTab VogueTab;

struct _VogueTab
{
  gint location;	        /* Offset in pixels of this tab stop
				 * from the left margin of the text.
				 */
  VogueTabAlign alignment;      /* Where the tab stop appears relative
				 * to the text.
				 */
};

/**
 * VogueTabArray:
 *
 * A #VogueTabArray struct contains an array
 * of tab stops. Each tab stop has an alignment and a position.
 */
struct _VogueTabArray
{
  gint size;
  gint allocated;
  gboolean positions_in_pixels;
  VogueTab *tabs;
};

static void
init_tabs (VogueTabArray *array, gint start, gint end)
{
  while (start < end)
    {
      array->tabs[start].location = 0;
      array->tabs[start].alignment = PANGO_TAB_LEFT;
      ++start;
    }
}

/**
 * vogue_tab_array_new:
 * @initial_size: Initial number of tab stops to allocate, can be 0
 * @positions_in_pixels: whether positions are in pixel units
 *
 * Creates an array of @initial_size tab stops. Tab stops are specified in
 * pixel units if @positions_in_pixels is %TRUE, otherwise in Vogue
 * units. All stops are initially at position 0.
 *
 * Return value: the newly allocated #VogueTabArray, which should
 *               be freed with vogue_tab_array_free().
 **/
VogueTabArray*
vogue_tab_array_new (gint initial_size,
		     gboolean positions_in_pixels)
{
  VogueTabArray *array;

  g_return_val_if_fail (initial_size >= 0, NULL);

  /* alloc enough to treat array->tabs as an array of length
   * size, though it's declared as an array of length 1.
   * If we allowed tab array resizing we'd need to drop this
   * optimization.
   */
  array = g_slice_new (VogueTabArray);
  array->size = initial_size;
  array->allocated = initial_size;

  if (array->allocated > 0)
    {
      array->tabs = g_new (VogueTab, array->allocated);
      init_tabs (array, 0, array->allocated);
    }
  else
    array->tabs = NULL;

  array->positions_in_pixels = positions_in_pixels;

  return array;
}

/**
 * vogue_tab_array_new_with_positions:
 * @size: number of tab stops in the array
 * @positions_in_pixels: whether positions are in pixel units
 * @first_alignment: alignment of first tab stop
 * @first_position: position of first tab stop
 * @...: additional alignment/position pairs
 *
 * This is a convenience function that creates a #VogueTabArray
 * and allows you to specify the alignment and position of each
 * tab stop. You <emphasis>must</emphasis> provide an alignment
 * and position for @size tab stops.
 *
 * Return value: the newly allocated #VogueTabArray, which should
 *               be freed with vogue_tab_array_free().
 **/
VogueTabArray  *
vogue_tab_array_new_with_positions (gint           size,
				    gboolean       positions_in_pixels,
				    VogueTabAlign  first_alignment,
				    gint           first_position,
				    ...)
{
  VogueTabArray *array;
  va_list args;
  int i;

  g_return_val_if_fail (size >= 0, NULL);

  array = vogue_tab_array_new (size, positions_in_pixels);

  if (size == 0)
    return array;

  array->tabs[0].alignment = first_alignment;
  array->tabs[0].location = first_position;

  if (size == 1)
    return array;

  va_start (args, first_position);

  i = 1;
  while (i < size)
    {
      VogueTabAlign align = va_arg (args, VogueTabAlign);
      int pos = va_arg (args, int);

      array->tabs[i].alignment = align;
      array->tabs[i].location = pos;

      ++i;
    }

  va_end (args);

  return array;
}

G_DEFINE_BOXED_TYPE (VogueTabArray, vogue_tab_array,
                     vogue_tab_array_copy,
                     vogue_tab_array_free);

/**
 * vogue_tab_array_copy:
 * @src: #VogueTabArray to copy
 *
 * Copies a #VogueTabArray
 *
 * Return value: the newly allocated #VogueTabArray, which should
 *               be freed with vogue_tab_array_free().
 **/
VogueTabArray*
vogue_tab_array_copy (VogueTabArray *src)
{
  VogueTabArray *copy;

  g_return_val_if_fail (src != NULL, NULL);

  copy = vogue_tab_array_new (src->size, src->positions_in_pixels);

  memcpy (copy->tabs, src->tabs, sizeof(VogueTab)*src->size);

  return copy;
}

/**
 * vogue_tab_array_free:
 * @tab_array: a #VogueTabArray
 *
 * Frees a tab array and associated resources.
 *
 **/
void
vogue_tab_array_free   (VogueTabArray *tab_array)
{
  g_return_if_fail (tab_array != NULL);

  g_free (tab_array->tabs);

  g_slice_free (VogueTabArray, tab_array);
}

/**
 * vogue_tab_array_get_size:
 * @tab_array: a #VogueTabArray
 *
 * Gets the number of tab stops in @tab_array.
 *
 * Return value: the number of tab stops in the array.
 **/
gint
vogue_tab_array_get_size (VogueTabArray *tab_array)
{
  g_return_val_if_fail (tab_array != NULL, 0);

  return tab_array->size;
}

/**
 * vogue_tab_array_resize:
 * @tab_array: a #VogueTabArray
 * @new_size: new size of the array
 *
 * Resizes a tab array. You must subsequently initialize any tabs that
 * were added as a result of growing the array.
 *
 **/
void
vogue_tab_array_resize (VogueTabArray *tab_array,
			gint           new_size)
{
  if (new_size > tab_array->allocated)
    {
      gint current_end = tab_array->allocated;

      /* Ratchet allocated size up above the index. */
      if (tab_array->allocated == 0)
	tab_array->allocated = 2;

      while (new_size > tab_array->allocated)
	tab_array->allocated = tab_array->allocated * 2;

      tab_array->tabs = g_renew (VogueTab, tab_array->tabs,
				 tab_array->allocated);

      init_tabs (tab_array, current_end, tab_array->allocated);
    }

  tab_array->size = new_size;
}

/**
 * vogue_tab_array_set_tab:
 * @tab_array: a #VogueTabArray
 * @tab_index: the index of a tab stop
 * @alignment: tab alignment
 * @location: tab location in Vogue units
 *
 * Sets the alignment and location of a tab stop.
 * @alignment must always be #PANGO_TAB_LEFT in the current
 * implementation.
 *
 **/
void
vogue_tab_array_set_tab  (VogueTabArray *tab_array,
			  gint           tab_index,
			  VogueTabAlign  alignment,
			  gint           location)
{
  g_return_if_fail (tab_array != NULL);
  g_return_if_fail (tab_index >= 0);
  g_return_if_fail (alignment == PANGO_TAB_LEFT);
  g_return_if_fail (location >= 0);

  if (tab_index >= tab_array->size)
    vogue_tab_array_resize (tab_array, tab_index + 1);

  tab_array->tabs[tab_index].alignment = alignment;
  tab_array->tabs[tab_index].location = location;
}

/**
 * vogue_tab_array_get_tab:
 * @tab_array: a #VogueTabArray
 * @tab_index: tab stop index
 * @alignment: (out) (allow-none): location to store alignment, or %NULL
 * @location: (out) (allow-none): location to store tab position, or %NULL
 *
 * Gets the alignment and position of a tab stop.
 *
 **/
void
vogue_tab_array_get_tab  (VogueTabArray *tab_array,
			  gint           tab_index,
			  VogueTabAlign *alignment,
			  gint          *location)
{
  g_return_if_fail (tab_array != NULL);
  g_return_if_fail (tab_index < tab_array->size);
  g_return_if_fail (tab_index >= 0);

  if (alignment)
    *alignment = tab_array->tabs[tab_index].alignment;

  if (location)
    *location = tab_array->tabs[tab_index].location;
}

/**
 * vogue_tab_array_get_tabs:
 * @tab_array: a #VogueTabArray
 * @alignments: (out) (allow-none): location to store an array of tab
 *   stop alignments, or %NULL
 * @locations: (out) (allow-none) (array): location to store an array
 *   of tab positions, or %NULL
 *
 * If non-%NULL, @alignments and @locations are filled with allocated
 * arrays of length vogue_tab_array_get_size(). You must free the
 * returned array.
 *
 **/
void
vogue_tab_array_get_tabs (VogueTabArray *tab_array,
			  VogueTabAlign **alignments,
			  gint          **locations)
{
  gint i;

  g_return_if_fail (tab_array != NULL);

  if (alignments)
    *alignments = g_new (VogueTabAlign, tab_array->size);

  if (locations)
    *locations = g_new (gint, tab_array->size);

  i = 0;
  while (i < tab_array->size)
    {
      if (alignments)
	(*alignments)[i] = tab_array->tabs[i].alignment;
      if (locations)
	(*locations)[i] = tab_array->tabs[i].location;

      ++i;
    }
}

/**
 * vogue_tab_array_get_positions_in_pixels:
 * @tab_array: a #VogueTabArray
 *
 * Returns %TRUE if the tab positions are in pixels, %FALSE if they are
 * in Vogue units.
 *
 * Return value: whether positions are in pixels.
 **/
gboolean
vogue_tab_array_get_positions_in_pixels (VogueTabArray *tab_array)
{
  g_return_val_if_fail (tab_array != NULL, FALSE);

  return tab_array->positions_in_pixels;
}
