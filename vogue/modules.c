/* Vogue
 * modules.c:
 *
 * Copyright (C) 1999 Red Hat Software
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
 * SECTION:modules
 * @short_description:Support for loadable modules
 * @title:Modules
 *
 * Functions and macros in this section were used to support
 * loading dynamic modules that add engines to Vogue at run time.
 *
 * That is no longer the case, and these APIs should not be
 * used anymore.
 *
 * Deprecated: 1.38
 */
#include "config.h"

#include "vogue-modules.h"

/**
 * vogue_find_map: (skip)
 * @language: the language tag for which to find the map
 * @engine_type_id: the engine type for the map to find
 * @render_type_id: the render type for the map to find
 *
 * Do not use.  Does not do anything.
 *
 * Return value: (transfer none) (nullable): %NULL.
 *
 * Deprecated: 1.38
 **/
VogueMap *
vogue_find_map (VogueLanguage *language G_GNUC_UNUSED,
		guint          engine_type_id G_GNUC_UNUSED,
		guint          render_type_id G_GNUC_UNUSED)
{
  return NULL;
}

/**
 * vogue_map_get_engine: (skip)
 * @map: a #VogueMap
 * @script: a #VogueScript
 *
 * Do not use.  Does not do anything.
 *
 * Return value: (transfer none) (nullable): %NULL.
 *
 * Deprecated: 1.38
 **/
VogueEngine *
vogue_map_get_engine (VogueMap   *map G_GNUC_UNUSED,
		      VogueScript script G_GNUC_UNUSED)
{
  return NULL;
}

/**
 * vogue_map_get_engines: (skip)
 * @map: a #VogueMap
 * @script: a #VogueScript
 * @exact_engines: (nullable): location to store list of engines that exactly
 *  handle this script.
 * @fallback_engines: (nullable): location to store list of engines that
 *  approximately handle this script.
 *
 * Do not use.  Does not do anything.
 *
 * Since: 1.4
 * Deprecated: 1.38
 **/
void
vogue_map_get_engines (VogueMap     *map G_GNUC_UNUSED,
		       VogueScript   script G_GNUC_UNUSED,
		       GSList      **exact_engines,
		       GSList      **fallback_engines)
{
  if (exact_engines)
    *exact_engines = NULL;
  if (fallback_engines)
    *fallback_engines = NULL;
}

/**
 * vogue_module_register: (skip)
 * @module: a #VogueIncludedModule
 *
 * Do not use.  Does not do anything.
 *
 * Deprecated: 1.38
 **/
void
vogue_module_register (VogueIncludedModule *module G_GNUC_UNUSED)
{
}
