/* Vogue
 * vogue-coverage.c: Coverage maps for fonts
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
 * SECTION:coverage-maps
 * @short_description:Unicode character range coverage storage
 * @title:Coverage Maps
 *
 * It is often necessary in Vogue to determine if a particular font can
 * represent a particular character, and also how well it can represent
 * that character. The #VogueCoverage is a data structure that is used
 * to represent that information.
 */
#include "config.h"
#include <string.h>

#include "vogue-coverage-private.h"

G_DEFINE_TYPE (VogueCoverage, vogue_coverage, G_TYPE_OBJECT)

static void
vogue_coverage_init (VogueCoverage *coverage)
{
}

static void
vogue_coverage_finalize (GObject *object)
{
  VogueCoverage *coverage = PANGO_COVERAGE (object);

  if (coverage->chars)
    hb_set_destroy (coverage->chars);

  G_OBJECT_CLASS (vogue_coverage_parent_class)->finalize (object);
}

static VogueCoverageLevel
vogue_coverage_real_get (VogueCoverage *coverage,
		         int            index)
{
  if (coverage->chars == NULL)
    return PANGO_COVERAGE_NONE;

  if (hb_set_has (coverage->chars, (hb_codepoint_t)index))
    return PANGO_COVERAGE_EXACT;
  else
    return PANGO_COVERAGE_NONE;
}

static void
vogue_coverage_real_set (VogueCoverage     *coverage,
		         int                index,
		         VogueCoverageLevel level)
{
  if (coverage->chars == NULL)
    coverage->chars = hb_set_create ();

  if (level != PANGO_COVERAGE_NONE)
    hb_set_add (coverage->chars, (hb_codepoint_t)index);
  else
    hb_set_del (coverage->chars, (hb_codepoint_t)index);
}

static VogueCoverage *
vogue_coverage_real_copy (VogueCoverage *coverage)
{
  VogueCoverage *copy;

  g_return_val_if_fail (coverage != NULL, NULL);

  copy = g_object_new (PANGO_TYPE_COVERAGE, NULL);
  if (coverage->chars)
    {
      int i;

      copy->chars = hb_set_create ();
      for (i = hb_set_get_min (coverage->chars); i <= hb_set_get_max (coverage->chars); i++)
        {
          if (hb_set_has (coverage->chars, (hb_codepoint_t)i))
            hb_set_add (copy->chars, (hb_codepoint_t)i);
        }
    }

  return copy;
}

static void
vogue_coverage_class_init (VogueCoverageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = vogue_coverage_finalize;

  class->get = vogue_coverage_real_get;
  class->set = vogue_coverage_real_set;
  class->copy = vogue_coverage_real_copy;
}

/**
 * vogue_coverage_new:
 *
 * Create a new #VogueCoverage
 *
 * Return value: the newly allocated #VogueCoverage,
 *               initialized to %PANGO_COVERAGE_NONE
 *               with a reference count of one, which
 *               should be freed with vogue_coverage_unref().
 **/
VogueCoverage *
vogue_coverage_new (void)
{
  return g_object_new (PANGO_TYPE_COVERAGE, NULL);
}

/**
 * vogue_coverage_copy:
 * @coverage: a #VogueCoverage
 *
 * Copy an existing #VogueCoverage. (This function may now be unnecessary
 * since we refcount the structure. File a bug if you use it.)
 *
 * Return value: (transfer full): the newly allocated #VogueCoverage,
 *               with a reference count of one, which should be freed
 *               with vogue_coverage_unref().
 **/
VogueCoverage *
vogue_coverage_copy (VogueCoverage *coverage)
{
  return PANGO_COVERAGE_GET_CLASS (coverage)->copy (coverage);
}

/**
 * vogue_coverage_ref:
 * @coverage: (not nullable): a #VogueCoverage
 *
 * Increase the reference count on the #VogueCoverage by one
 *
 * Return value: (transfer full): @coverage
 **/
VogueCoverage *
vogue_coverage_ref (VogueCoverage *coverage)
{
  return g_object_ref (coverage);
}

/**
 * vogue_coverage_unref:
 * @coverage: (transfer full) (not nullable): a #VogueCoverage
 *
 * Decrease the reference count on the #VogueCoverage by one.
 * If the result is zero, free the coverage and all associated memory.
 **/
void
vogue_coverage_unref (VogueCoverage *coverage)
{
  g_object_unref (coverage);
}

/**
 * vogue_coverage_get:
 * @coverage: a #VogueCoverage
 * @index_: the index to check
 *
 * Determine whether a particular index is covered by @coverage
 *
 * Return value: the coverage level of @coverage for character @index_.
 **/
VogueCoverageLevel
vogue_coverage_get (VogueCoverage *coverage,
		    int            index)
{
  return PANGO_COVERAGE_GET_CLASS (coverage)->get (coverage, index);
}

/**
 * vogue_coverage_set:
 * @coverage: a #VogueCoverage
 * @index_: the index to modify
 * @level: the new level for @index_
 *
 * Modify a particular index within @coverage
 **/
void
vogue_coverage_set (VogueCoverage     *coverage,
		    int                index,
		    VogueCoverageLevel level)
{
  PANGO_COVERAGE_GET_CLASS (coverage)->set (coverage, index, level);
}

/**
 * vogue_coverage_max:
 * @coverage: a #VogueCoverage
 * @other: another #VogueCoverage
 *
 * Set the coverage for each index in @coverage to be the max (better)
 * value of the current coverage for the index and the coverage for
 * the corresponding index in @other.
 *
 * Deprecated: 1.44: This function does nothing
 **/
void
vogue_coverage_max (VogueCoverage *coverage,
		    VogueCoverage *other)
{
}

/**
 * vogue_coverage_to_bytes:
 * @coverage: a #VogueCoverage
 * @bytes: (out) (array length=n_bytes) (element-type guint8):
 *   location to store result (must be freed with g_free())
 * @n_bytes: (out): location to store size of result
 *
 * Convert a #VogueCoverage structure into a flat binary format
 *
 * Deprecated: 1.44: This returns %NULL
 **/
void
vogue_coverage_to_bytes (VogueCoverage  *coverage,
			 guchar        **bytes,
			 int            *n_bytes)
{
  *bytes = NULL;
  *n_bytes = 0;
}

/**
 * vogue_coverage_from_bytes:
 * @bytes: (array length=n_bytes) (element-type guint8): binary data
 *   representing a #VogueCoverage
 * @n_bytes: the size of @bytes in bytes
 *
 * Convert data generated from vogue_coverage_to_bytes() back
 * to a #VogueCoverage
 *
 * Return value: (transfer full) (nullable): a newly allocated
 *               #VogueCoverage, or %NULL if the data was invalid.
 *
 * Deprecated: 1.44: This returns %NULL
 **/
VogueCoverage *
vogue_coverage_from_bytes (guchar *bytes,
			   int     n_bytes)
{
  return NULL;
}
