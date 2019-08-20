/* Vogue
 * fonts.c:
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
 * SECTION:fonts
 * @short_description:Structures representing abstract fonts
 * @title: Fonts
 *
 * Vogue supports a flexible architecture where a
 * particular rendering architecture can supply an
 * implementation of fonts. The #VogueFont structure
 * represents an abstract rendering-system-independent font.
 * Vogue provides routines to list available fonts, and
 * to load a font matching a given description.
 */

#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "vogue-types.h"
#include "vogue-font-private.h"
#include "vogue-fontmap.h"
#include "vogue-impl-utils.h"

struct _VogueFontDescription
{
  char *family_name;

  VogueStyle style;
  VogueVariant variant;
  VogueWeight weight;
  VogueStretch stretch;
  VogueGravity gravity;

  char *variations;

  guint16 mask;
  guint static_family : 1;
  guint static_variations : 1;
  guint size_is_absolute : 1;

  int size;
};

G_DEFINE_BOXED_TYPE (VogueFontDescription, vogue_font_description,
                     vogue_font_description_copy,
                     vogue_font_description_free);

static const VogueFontDescription pfd_defaults = {
  NULL,			/* family_name */

  PANGO_STYLE_NORMAL,	/* style */
  PANGO_VARIANT_NORMAL,	/* variant */
  PANGO_WEIGHT_NORMAL,	/* weight */
  PANGO_STRETCH_NORMAL,	/* stretch */
  PANGO_GRAVITY_SOUTH,  /* gravity */
  NULL,                 /* variations */

  0,			/* mask */
  0,			/* static_family */
  0,			/* static_variations*/
  0,    		/* size_is_absolute */

  0,			/* size */
};

/**
 * vogue_font_description_new:
 *
 * Creates a new font description structure with all fields unset.
 *
 * Return value: the newly allocated #VogueFontDescription, which
 *               should be freed using vogue_font_description_free().
 **/
VogueFontDescription *
vogue_font_description_new (void)
{
  VogueFontDescription *desc = g_slice_new (VogueFontDescription);

  *desc = pfd_defaults;

  return desc;
}

/**
 * vogue_font_description_set_family:
 * @desc: a #VogueFontDescription.
 * @family: a string representing the family name.
 *
 * Sets the family name field of a font description. The family
 * name represents a family of related font styles, and will
 * resolve to a particular #VogueFontFamily. In some uses of
 * #VogueFontDescription, it is also possible to use a comma
 * separated list of family names for this field.
 **/
void
vogue_font_description_set_family (VogueFontDescription *desc,
				   const char           *family)
{
  g_return_if_fail (desc != NULL);

  vogue_font_description_set_family_static (desc, family ? g_strdup (family) : NULL);
  if (family)
    desc->static_family = FALSE;
}

/**
 * vogue_font_description_set_family_static:
 * @desc: a #VogueFontDescription
 * @family: a string representing the family name.
 *
 * Like vogue_font_description_set_family(), except that no
 * copy of @family is made. The caller must make sure that the
 * string passed in stays around until @desc has been freed
 * or the name is set again. This function can be used if
 * @family is a static string such as a C string literal, or
 * if @desc is only needed temporarily.
 **/
void
vogue_font_description_set_family_static (VogueFontDescription *desc,
					  const char           *family)
{
  g_return_if_fail (desc != NULL);

  if (desc->family_name == family)
    return;

  if (desc->family_name && !desc->static_family)
    g_free (desc->family_name);

  if (family)
    {
      desc->family_name = (char *)family;
      desc->static_family = TRUE;
      desc->mask |= PANGO_FONT_MASK_FAMILY;
    }
  else
    {
      desc->family_name = pfd_defaults.family_name;
      desc->static_family = pfd_defaults.static_family;
      desc->mask &= ~PANGO_FONT_MASK_FAMILY;
    }
}

/**
 * vogue_font_description_get_family:
 * @desc: a #VogueFontDescription.
 *
 * Gets the family name field of a font description. See
 * vogue_font_description_set_family().
 *
 * Return value: (nullable): the family name field for the font
 *               description, or %NULL if not previously set.  This
 *               has the same life-time as the font description itself
 *               and should not be freed.
 **/
const char *
vogue_font_description_get_family (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return desc->family_name;
}

/**
 * vogue_font_description_set_style:
 * @desc: a #VogueFontDescription
 * @style: the style for the font description
 *
 * Sets the style field of a #VogueFontDescription. The
 * #VogueStyle enumeration describes whether the font is slanted and
 * the manner in which it is slanted; it can be either
 * #PANGO_STYLE_NORMAL, #PANGO_STYLE_ITALIC, or #PANGO_STYLE_OBLIQUE.
 * Most fonts will either have a italic style or an oblique
 * style, but not both, and font matching in Vogue will
 * match italic specifications with oblique fonts and vice-versa
 * if an exact match is not found.
 **/
void
vogue_font_description_set_style (VogueFontDescription *desc,
				  VogueStyle            style)
{
  g_return_if_fail (desc != NULL);

  desc->style = style;
  desc->mask |= PANGO_FONT_MASK_STYLE;
}

/**
 * vogue_font_description_get_style:
 * @desc: a #VogueFontDescription
 *
 * Gets the style field of a #VogueFontDescription. See
 * vogue_font_description_set_style().
 *
 * Return value: the style field for the font description.
 *   Use vogue_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 **/
VogueStyle
vogue_font_description_get_style (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.style);

  return desc->style;
}

/**
 * vogue_font_description_set_variant:
 * @desc: a #VogueFontDescription
 * @variant: the variant type for the font description.
 *
 * Sets the variant field of a font description. The #VogueVariant
 * can either be %PANGO_VARIANT_NORMAL or %PANGO_VARIANT_SMALL_CAPS.
 **/
void
vogue_font_description_set_variant (VogueFontDescription *desc,
				    VogueVariant          variant)
{
  g_return_if_fail (desc != NULL);

  desc->variant = variant;
  desc->mask |= PANGO_FONT_MASK_VARIANT;
}

/**
 * vogue_font_description_get_variant:
 * @desc: a #VogueFontDescription.
 *
 * Gets the variant field of a #VogueFontDescription. See
 * vogue_font_description_set_variant().
 *
 * Return value: the variant field for the font description. Use
 *   vogue_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 **/
VogueVariant
vogue_font_description_get_variant (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.variant);

  return desc->variant;
}

/**
 * vogue_font_description_set_weight:
 * @desc: a #VogueFontDescription
 * @weight: the weight for the font description.
 *
 * Sets the weight field of a font description. The weight field
 * specifies how bold or light the font should be. In addition
 * to the values of the #VogueWeight enumeration, other intermediate
 * numeric values are possible.
 **/
void
vogue_font_description_set_weight (VogueFontDescription *desc,
				   VogueWeight          weight)
{
  g_return_if_fail (desc != NULL);

  desc->weight = weight;
  desc->mask |= PANGO_FONT_MASK_WEIGHT;
}

/**
 * vogue_font_description_get_weight:
 * @desc: a #VogueFontDescription
 *
 * Gets the weight field of a font description. See
 * vogue_font_description_set_weight().
 *
 * Return value: the weight field for the font description. Use
 *   vogue_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 **/
VogueWeight
vogue_font_description_get_weight (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.weight);

  return desc->weight;
}

/**
 * vogue_font_description_set_stretch:
 * @desc: a #VogueFontDescription
 * @stretch: the stretch for the font description
 *
 * Sets the stretch field of a font description. The stretch field
 * specifies how narrow or wide the font should be.
 **/
void
vogue_font_description_set_stretch (VogueFontDescription *desc,
				    VogueStretch          stretch)
{
  g_return_if_fail (desc != NULL);

  desc->stretch = stretch;
  desc->mask |= PANGO_FONT_MASK_STRETCH;
}

/**
 * vogue_font_description_get_stretch:
 * @desc: a #VogueFontDescription.
 *
 * Gets the stretch field of a font description.
 * See vogue_font_description_set_stretch().
 *
 * Return value: the stretch field for the font description. Use
 *   vogue_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 **/
VogueStretch
vogue_font_description_get_stretch (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.stretch);

  return desc->stretch;
}

/**
 * vogue_font_description_set_size:
 * @desc: a #VogueFontDescription
 * @size: the size of the font in points, scaled by PANGO_SCALE. (That is,
 *        a @size value of 10 * PANGO_SCALE is a 10 point font. The conversion
 *        factor between points and device units depends on system configuration
 *        and the output device. For screen display, a logical DPI of 96 is
 *        common, in which case a 10 point font corresponds to a 10 * (96 / 72) = 13.3
 *        pixel font. Use vogue_font_description_set_absolute_size() if you need
 *        a particular size in device units.
 *
 * Sets the size field of a font description in fractional points. This is mutually
 * exclusive with vogue_font_description_set_absolute_size().
 **/
void
vogue_font_description_set_size (VogueFontDescription *desc,
				 gint                  size)
{
  g_return_if_fail (desc != NULL);
  g_return_if_fail (size >= 0);

  desc->size = size;
  desc->size_is_absolute = FALSE;
  desc->mask |= PANGO_FONT_MASK_SIZE;
}

/**
 * vogue_font_description_get_size:
 * @desc: a #VogueFontDescription
 *
 * Gets the size field of a font description.
 * See vogue_font_description_set_size().
 *
 * Return value: the size field for the font description in points or device units.
 *   You must call vogue_font_description_get_size_is_absolute()
 *   to find out which is the case. Returns 0 if the size field has not
 *   previously been set or it has been set to 0 explicitly.
 *   Use vogue_font_description_get_set_fields() to
 *   find out if the field was explicitly set or not.
 **/
gint
vogue_font_description_get_size (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.size);

  return desc->size;
}

/**
 * vogue_font_description_set_absolute_size:
 * @desc: a #VogueFontDescription
 * @size: the new size, in Vogue units. There are %PANGO_SCALE Vogue units in one
 *   device unit. For an output backend where a device unit is a pixel, a @size
 *   value of 10 * PANGO_SCALE gives a 10 pixel font.
 *
 * Sets the size field of a font description, in device units. This is mutually
 * exclusive with vogue_font_description_set_size() which sets the font size
 * in points.
 *
 * Since: 1.8
 **/
void
vogue_font_description_set_absolute_size (VogueFontDescription *desc,
					  double                size)
{
  g_return_if_fail (desc != NULL);
  g_return_if_fail (size >= 0);

  desc->size = size;
  desc->size_is_absolute = TRUE;
  desc->mask |= PANGO_FONT_MASK_SIZE;
}

/**
 * vogue_font_description_get_size_is_absolute:
 * @desc: a #VogueFontDescription
 *
 * Determines whether the size of the font is in points (not absolute) or device units (absolute).
 * See vogue_font_description_set_size() and vogue_font_description_set_absolute_size().
 *
 * Return value: whether the size for the font description is in
 *   points or device units.  Use vogue_font_description_get_set_fields() to
 *   find out if the size field of the font description was explicitly set or not.
 *
 * Since: 1.8
 **/
gboolean
vogue_font_description_get_size_is_absolute (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.size_is_absolute);

  return desc->size_is_absolute;
}

/**
 * vogue_font_description_set_gravity:
 * @desc: a #VogueFontDescription
 * @gravity: the gravity for the font description.
 *
 * Sets the gravity field of a font description. The gravity field
 * specifies how the glyphs should be rotated.  If @gravity is
 * %PANGO_GRAVITY_AUTO, this actually unsets the gravity mask on
 * the font description.
 *
 * This function is seldom useful to the user.  Gravity should normally
 * be set on a #VogueContext.
 *
 * Since: 1.16
 **/
void
vogue_font_description_set_gravity (VogueFontDescription *desc,
				    VogueGravity          gravity)
{
  g_return_if_fail (desc != NULL);

  if (gravity == PANGO_GRAVITY_AUTO)
    {
      vogue_font_description_unset_fields (desc, PANGO_FONT_MASK_GRAVITY);
      return;
    }

  desc->gravity = gravity;
  desc->mask |= PANGO_FONT_MASK_GRAVITY;
}

/**
 * vogue_font_description_get_gravity:
 * @desc: a #VogueFontDescription
 *
 * Gets the gravity field of a font description. See
 * vogue_font_description_set_gravity().
 *
 * Return value: the gravity field for the font description. Use
 *   vogue_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 *
 * Since: 1.16
 **/
VogueGravity
vogue_font_description_get_gravity (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.gravity);

  return desc->gravity;
}

/**
 * vogue_font_description_set_variations_static:
 * @desc: a #VogueFontDescription
 * @variations: a string representing the variations
 *
 * Like vogue_font_description_set_variations(), except that no
 * copy of @variations is made. The caller must make sure that the
 * string passed in stays around until @desc has been freed
 * or the name is set again. This function can be used if
 * @variations is a static string such as a C string literal, or
 * if @desc is only needed temporarily.
 *
 * Since: 1.42
 **/
void
vogue_font_description_set_variations_static (VogueFontDescription *desc,
                                              const char           *variations)
{
  g_return_if_fail (desc != NULL);

  if (desc->variations == variations)
    return;

  if (desc->variations && !desc->static_variations)
    g_free (desc->variations);

  if (variations)
    {
      desc->variations = (char *)variations;
      desc->static_variations = TRUE;
      desc->mask |= PANGO_FONT_MASK_VARIATIONS;
    }
  else
    {
      desc->variations = pfd_defaults.variations;
      desc->static_variations = pfd_defaults.static_variations;
      desc->mask &= ~PANGO_FONT_MASK_VARIATIONS;
    }
}

/**
 * vogue_font_description_set_variations:
 * @desc: a #VogueFontDescription.
 * @variations: a string representing the variations
 *
 * Sets the variations field of a font description. OpenType
 * font variations allow to select a font instance by specifying
 * values for a number of axes, such as width or weight.
 *
 * The format of the variations string is AXIS1=VALUE,AXIS2=VALUE...,
 * with each AXIS a 4 character tag that identifies a font axis,
 * and each VALUE a floating point number. Unknown axes are ignored,
 * and values are clamped to their allowed range.
 *
 * Vogue does not currently have a way to find supported axes of
 * a font. Both harfbuzz or freetype have API for this.
 *
 * Since: 1.42
 **/
void
vogue_font_description_set_variations (VogueFontDescription *desc,
                                       const char           *variations)
{
  g_return_if_fail (desc != NULL);

  vogue_font_description_set_variations_static (desc, g_strdup (variations));
  if (variations)
    desc->static_variations = FALSE;
}

/**
 * vogue_font_description_get_variations:
 * @desc: a #VogueFontDescription
 *
 * Gets the variations field of a font description. See
 * vogue_font_description_set_variations().
 *
 * Return value: (nullable): the varitions field for the font
 *               description, or %NULL if not previously set.  This
 *               has the same life-time as the font description itself
 *               and should not be freed.
 *
 * Since: 1.42
 **/
const char *
vogue_font_description_get_variations (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return desc->variations;
}

/**
 * vogue_font_description_get_set_fields:
 * @desc: a #VogueFontDescription
 *
 * Determines which fields in a font description have been set.
 *
 * Return value: a bitmask with bits set corresponding to the
 *   fields in @desc that have been set.
 **/
VogueFontMask
vogue_font_description_get_set_fields (const VogueFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.mask);

  return desc->mask;
}

/**
 * vogue_font_description_unset_fields:
 * @desc: a #VogueFontDescription
 * @to_unset: bitmask of fields in the @desc to unset.
 *
 * Unsets some of the fields in a #VogueFontDescription.  The unset
 * fields will get back to their default values.
 **/
void
vogue_font_description_unset_fields (VogueFontDescription *desc,
				     VogueFontMask         to_unset)
{
  VogueFontDescription unset_desc;

  g_return_if_fail (desc != NULL);

  unset_desc = pfd_defaults;
  unset_desc.mask = to_unset;

  vogue_font_description_merge_static (desc, &unset_desc, TRUE);

  desc->mask &= ~to_unset;
}

/**
 * vogue_font_description_merge:
 * @desc: a #VogueFontDescription
 * @desc_to_merge: (allow-none): the #VogueFontDescription to merge from, or %NULL
 * @replace_existing: if %TRUE, replace fields in @desc with the
 *   corresponding values from @desc_to_merge, even if they
 *   are already exist.
 *
 * Merges the fields that are set in @desc_to_merge into the fields in
 * @desc.  If @replace_existing is %FALSE, only fields in @desc that
 * are not already set are affected. If %TRUE, then fields that are
 * already set will be replaced as well.
 *
 * If @desc_to_merge is %NULL, this function performs nothing.
 **/
void
vogue_font_description_merge (VogueFontDescription       *desc,
			      const VogueFontDescription *desc_to_merge,
			      gboolean                    replace_existing)
{
  gboolean family_merged;
  gboolean variations_merged;

  g_return_if_fail (desc != NULL);

  if (desc_to_merge == NULL)
    return;

  family_merged = desc_to_merge->family_name && (replace_existing || !desc->family_name);
  variations_merged = desc_to_merge->variations && (replace_existing || !desc->variations);

  vogue_font_description_merge_static (desc, desc_to_merge, replace_existing);

  if (family_merged)
    {
      desc->family_name = g_strdup (desc->family_name);
      desc->static_family = FALSE;
    }

  if (variations_merged)
    {
      desc->variations = g_strdup (desc->variations);
      desc->static_variations = FALSE;
    }
}

/**
 * vogue_font_description_merge_static:
 * @desc: a #VogueFontDescription
 * @desc_to_merge: the #VogueFontDescription to merge from
 * @replace_existing: if %TRUE, replace fields in @desc with the
 *   corresponding values from @desc_to_merge, even if they
 *   are already exist.
 *
 * Like vogue_font_description_merge(), but only a shallow copy is made
 * of the family name and other allocated fields. @desc can only be
 * used until @desc_to_merge is modified or freed. This is meant
 * to be used when the merged font description is only needed temporarily.
 **/
void
vogue_font_description_merge_static (VogueFontDescription       *desc,
				     const VogueFontDescription *desc_to_merge,
				     gboolean                    replace_existing)
{
  VogueFontMask new_mask;

  g_return_if_fail (desc != NULL);
  g_return_if_fail (desc_to_merge != NULL);

  if (replace_existing)
    new_mask = desc_to_merge->mask;
  else
    new_mask = desc_to_merge->mask & ~desc->mask;

  if (new_mask & PANGO_FONT_MASK_FAMILY)
    vogue_font_description_set_family_static (desc, desc_to_merge->family_name);
  if (new_mask & PANGO_FONT_MASK_STYLE)
    desc->style = desc_to_merge->style;
  if (new_mask & PANGO_FONT_MASK_VARIANT)
    desc->variant = desc_to_merge->variant;
  if (new_mask & PANGO_FONT_MASK_WEIGHT)
    desc->weight = desc_to_merge->weight;
  if (new_mask & PANGO_FONT_MASK_STRETCH)
    desc->stretch = desc_to_merge->stretch;
  if (new_mask & PANGO_FONT_MASK_SIZE)
    {
      desc->size = desc_to_merge->size;
      desc->size_is_absolute = desc_to_merge->size_is_absolute;
    }
  if (new_mask & PANGO_FONT_MASK_GRAVITY)
    desc->gravity = desc_to_merge->gravity;
  if (new_mask & PANGO_FONT_MASK_VARIATIONS)
    vogue_font_description_set_variations_static (desc, desc_to_merge->variations);

  desc->mask |= new_mask;
}

static gint
compute_distance (const VogueFontDescription *a,
		  const VogueFontDescription *b)
{
  if (a->style == b->style)
    {
      return abs((int)(a->weight) - (int)(b->weight));
    }
  else if (a->style != PANGO_STYLE_NORMAL &&
	   b->style != PANGO_STYLE_NORMAL)
    {
      /* Equate oblique and italic, but with a big penalty
       */
      return 1000000 + abs ((int)(a->weight) - (int)(b->weight));
    }
  else
    return G_MAXINT;
}

/**
 * vogue_font_description_better_match:
 * @desc: a #VogueFontDescription
 * @old_match: (allow-none): a #VogueFontDescription, or %NULL
 * @new_match: a #VogueFontDescription
 *
 * Determines if the style attributes of @new_match are a closer match
 * for @desc than those of @old_match are, or if @old_match is %NULL,
 * determines if @new_match is a match at all.
 * Approximate matching is done for
 * weight and style; other style attributes must match exactly.
 * Style attributes are all attributes other than family and size-related
 * attributes.  Approximate matching for style considers PANGO_STYLE_OBLIQUE
 * and PANGO_STYLE_ITALIC as matches, but not as good a match as when the
 * styles are equal.
 *
 * Note that @old_match must match @desc.
 *
 * Return value: %TRUE if @new_match is a better match
 **/
gboolean
vogue_font_description_better_match (const VogueFontDescription *desc,
				     const VogueFontDescription *old_match,
				     const VogueFontDescription *new_match)
{
  g_return_val_if_fail (desc != NULL, G_MAXINT);
  g_return_val_if_fail (new_match != NULL, G_MAXINT);

  if (new_match->variant == desc->variant &&
      new_match->stretch == desc->stretch &&
      new_match->gravity == desc->gravity)
    {
      int old_distance = old_match ? compute_distance (desc, old_match) : G_MAXINT;
      int new_distance = compute_distance (desc, new_match);

      if (new_distance < old_distance)
	return TRUE;
    }

  return FALSE;
}

/**
 * vogue_font_description_copy:
 * @desc: (nullable): a #VogueFontDescription, may be %NULL
 *
 * Make a copy of a #VogueFontDescription.
 *
 * Return value: (nullable): the newly allocated
 *               #VogueFontDescription, which should be freed with
 *               vogue_font_description_free(), or %NULL if @desc was
 *               %NULL.
 **/
VogueFontDescription *
vogue_font_description_copy  (const VogueFontDescription  *desc)
{
  VogueFontDescription *result;

  if (desc == NULL)
    return NULL;

  result = g_slice_new (VogueFontDescription);

  *result = *desc;

  if (result->family_name)
    {
      result->family_name = g_strdup (result->family_name);
      result->static_family = FALSE;
    }

  result->variations = g_strdup (result->variations);
  result->static_variations = FALSE;

  return result;
}

/**
 * vogue_font_description_copy_static:
 * @desc: (nullable): a #VogueFontDescription, may be %NULL
 *
 * Like vogue_font_description_copy(), but only a shallow copy is made
 * of the family name and other allocated fields. The result can only
 * be used until @desc is modified or freed. This is meant to be used
 * when the copy is only needed temporarily.
 *
 * Return value: (nullable): the newly allocated
 *               #VogueFontDescription, which should be freed with
 *               vogue_font_description_free(), or %NULL if @desc was
 *               %NULL.
 **/
VogueFontDescription *
vogue_font_description_copy_static (const VogueFontDescription *desc)
{
  VogueFontDescription *result;

  if (desc == NULL)
    return NULL;

  result = g_slice_new (VogueFontDescription);

  *result = *desc;
  if (result->family_name)
    result->static_family = TRUE;


  if (result->variations)
    result->static_variations = TRUE;

  return result;
}

/**
 * vogue_font_description_equal:
 * @desc1: a #VogueFontDescription
 * @desc2: another #VogueFontDescription
 *
 * Compares two font descriptions for equality. Two font descriptions
 * are considered equal if the fonts they describe are provably identical.
 * This means that their masks do not have to match, as long as other fields
 * are all the same. (Two font descriptions may result in identical fonts
 * being loaded, but still compare %FALSE.)
 *
 * Return value: %TRUE if the two font descriptions are identical,
 *		 %FALSE otherwise.
 **/
gboolean
vogue_font_description_equal (const VogueFontDescription  *desc1,
			      const VogueFontDescription  *desc2)
{
  g_return_val_if_fail (desc1 != NULL, FALSE);
  g_return_val_if_fail (desc2 != NULL, FALSE);

  return desc1->style == desc2->style &&
	 desc1->variant == desc2->variant &&
	 desc1->weight == desc2->weight &&
	 desc1->stretch == desc2->stretch &&
	 desc1->size == desc2->size &&
	 desc1->size_is_absolute == desc2->size_is_absolute &&
	 desc1->gravity == desc2->gravity &&
	 (desc1->family_name == desc2->family_name ||
	  (desc1->family_name && desc2->family_name && g_ascii_strcasecmp (desc1->family_name, desc2->family_name) == 0)) &&
         (g_strcmp0 (desc1->variations, desc2->variations) == 0);
}

#define TOLOWER(c) \
  (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

static guint
case_insensitive_hash (const char *key)
{
  const char *p = key;
  guint h = TOLOWER (*p);

  if (h)
    {
      for (p += 1; *p != '\0'; p++)
	h = (h << 5) - h + TOLOWER (*p);
    }

  return h;
}

/**
 * vogue_font_description_hash:
 * @desc: a #VogueFontDescription
 *
 * Computes a hash of a #VogueFontDescription structure suitable
 * to be used, for example, as an argument to g_hash_table_new().
 * The hash value is independent of @desc->mask.
 *
 * Return value: the hash value.
 **/
guint
vogue_font_description_hash (const VogueFontDescription *desc)
{
  guint hash = 0;

  g_return_val_if_fail (desc != NULL, 0);

  if (desc->family_name)
    hash = case_insensitive_hash (desc->family_name);
  if (desc->variations)
    hash ^= g_str_hash (desc->variations);
  hash ^= desc->size;
  hash ^= desc->size_is_absolute ? 0xc33ca55a : 0;
  hash ^= desc->style << 16;
  hash ^= desc->variant << 18;
  hash ^= desc->weight << 16;
  hash ^= desc->stretch << 26;
  hash ^= desc->gravity << 28;

  return hash;
}

/**
 * vogue_font_description_free:
 * @desc: (nullable): a #VogueFontDescription, may be %NULL
 *
 * Frees a font description.
 **/
void
vogue_font_description_free  (VogueFontDescription  *desc)
{
  if (desc == NULL)
    return;

  if (desc->family_name && !desc->static_family)
    g_free (desc->family_name);

  if (desc->variations && !desc->static_variations)
    g_free (desc->variations);

  g_slice_free (VogueFontDescription, desc);
}

/**
 * vogue_font_descriptions_free:
 * @descs: (allow-none) (array length=n_descs) (transfer full): a pointer
 * to an array of #VogueFontDescription, may be %NULL
 * @n_descs: number of font descriptions in @descs
 *
 * Frees an array of font descriptions.
 **/
void
vogue_font_descriptions_free (VogueFontDescription **descs,
			      int                    n_descs)
{
  int i;

  if (descs == NULL)
    return;

  for (i = 0; i<n_descs; i++)
    vogue_font_description_free (descs[i]);
  g_free (descs);
}

typedef struct
{
  int value;
  const char str[16];
} FieldMap;

static const FieldMap style_map[] = {
  { PANGO_STYLE_NORMAL, "" },
  { PANGO_STYLE_NORMAL, "Roman" },
  { PANGO_STYLE_OBLIQUE, "Oblique" },
  { PANGO_STYLE_ITALIC, "Italic" }
};

static const FieldMap variant_map[] = {
  { PANGO_VARIANT_NORMAL, "" },
  { PANGO_VARIANT_SMALL_CAPS, "Small-Caps" }
};

static const FieldMap weight_map[] = {
  { PANGO_WEIGHT_THIN, "Thin" },
  { PANGO_WEIGHT_ULTRALIGHT, "Ultra-Light" },
  { PANGO_WEIGHT_ULTRALIGHT, "Extra-Light" },
  { PANGO_WEIGHT_LIGHT, "Light" },
  { PANGO_WEIGHT_SEMILIGHT, "Semi-Light" },
  { PANGO_WEIGHT_SEMILIGHT, "Demi-Light" },
  { PANGO_WEIGHT_BOOK, "Book" },
  { PANGO_WEIGHT_NORMAL, "" },
  { PANGO_WEIGHT_NORMAL, "Regular" },
  { PANGO_WEIGHT_MEDIUM, "Medium" },
  { PANGO_WEIGHT_SEMIBOLD, "Semi-Bold" },
  { PANGO_WEIGHT_SEMIBOLD, "Demi-Bold" },
  { PANGO_WEIGHT_BOLD, "Bold" },
  { PANGO_WEIGHT_ULTRABOLD, "Ultra-Bold" },
  { PANGO_WEIGHT_ULTRABOLD, "Extra-Bold" },
  { PANGO_WEIGHT_HEAVY, "Heavy" },
  { PANGO_WEIGHT_HEAVY, "Black" },
  { PANGO_WEIGHT_ULTRAHEAVY, "Ultra-Heavy" },
  { PANGO_WEIGHT_ULTRAHEAVY, "Extra-Heavy" },
  { PANGO_WEIGHT_ULTRAHEAVY, "Ultra-Black" },
  { PANGO_WEIGHT_ULTRAHEAVY, "Extra-Black" }
};

static const FieldMap stretch_map[] = {
  { PANGO_STRETCH_ULTRA_CONDENSED, "Ultra-Condensed" },
  { PANGO_STRETCH_EXTRA_CONDENSED, "Extra-Condensed" },
  { PANGO_STRETCH_CONDENSED,       "Condensed" },
  { PANGO_STRETCH_SEMI_CONDENSED,  "Semi-Condensed" },
  { PANGO_STRETCH_NORMAL,          "" },
  { PANGO_STRETCH_SEMI_EXPANDED,   "Semi-Expanded" },
  { PANGO_STRETCH_EXPANDED,        "Expanded" },
  { PANGO_STRETCH_EXTRA_EXPANDED,  "Extra-Expanded" },
  { PANGO_STRETCH_ULTRA_EXPANDED,  "Ultra-Expanded" }
};

static const FieldMap gravity_map[] = {
  { PANGO_GRAVITY_SOUTH, "Not-Rotated" },
  { PANGO_GRAVITY_SOUTH, "South" },
  { PANGO_GRAVITY_NORTH, "Upside-Down" },
  { PANGO_GRAVITY_NORTH, "North" },
  { PANGO_GRAVITY_EAST,  "Rotated-Left" },
  { PANGO_GRAVITY_EAST,  "East" },
  { PANGO_GRAVITY_WEST,  "Rotated-Right" },
  { PANGO_GRAVITY_WEST,  "West" }
};

static gboolean
field_matches (const gchar *s1,
	       const gchar *s2,
	       gsize n)
{
  gint c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (n && *s1 && *s2)
    {
      c1 = (gint)(guchar) TOLOWER (*s1);
      c2 = (gint)(guchar) TOLOWER (*s2);
      if (c1 != c2) {
        if (c1 == '-') {
	  s1++;
	  continue;
	}
	return FALSE;
      }
      s1++; s2++;
      n--;
    }

  return n == 0 && *s1 == '\0';
}

static gboolean
parse_int (const char *word,
	   size_t      wordlen,
	   int        *out)
{
  char *end;
  long val = strtol (word, &end, 10);
  int i = val;

  if (end != word && (end == word + wordlen) && val >= 0 && val == i)
    {
      if (out)
        *out = i;

      return TRUE;
    }

  return FALSE;
}

static gboolean
find_field (const char *what,
	    const FieldMap *map,
	    int n_elements,
	    const char *str,
	    int len,
	    int *val)
{
  int i;
  gboolean had_prefix = FALSE;

  if (what)
    {
      i = strlen (what);
      if (len > i && 0 == strncmp (what, str, i) && str[i] == '=')
	{
	  str += i + 1;
	  len -= i + 1;
	  had_prefix = TRUE;
	}
    }

  for (i=0; i<n_elements; i++)
    {
      if (map[i].str[0] && field_matches (map[i].str, str, len))
	{
	  if (val)
	    *val = map[i].value;
	  return TRUE;
	}
    }

  if (!what || had_prefix)
    return parse_int (str, len, val);

  return FALSE;
}

static gboolean
find_field_any (const char *str, int len, VogueFontDescription *desc)
{
  if (field_matches ("Normal", str, len))
    return TRUE;

#define FIELD(NAME, MASK) \
  G_STMT_START { \
  if (find_field (G_STRINGIFY (NAME), NAME##_map, G_N_ELEMENTS (NAME##_map), str, len, \
		  desc ? (int *)(void *)&desc->NAME : NULL)) \
    { \
      if (desc) \
	desc->mask |= MASK; \
      return TRUE; \
    } \
  } G_STMT_END

  FIELD (weight,  PANGO_FONT_MASK_WEIGHT);
  FIELD (style,   PANGO_FONT_MASK_STYLE);
  FIELD (stretch, PANGO_FONT_MASK_STRETCH);
  FIELD (variant, PANGO_FONT_MASK_VARIANT);
  FIELD (gravity, PANGO_FONT_MASK_GRAVITY);

#undef FIELD

  return FALSE;
}

static const char *
getword (const char *str, const char *last, size_t *wordlen, const char *stop)
{
  const char *result;

  while (last > str && g_ascii_isspace (*(last - 1)))
    last--;

  result = last;
  while (result > str && !g_ascii_isspace (*(result - 1)) && !strchr (stop, *(result - 1)))
    result--;

  *wordlen = last - result;

  return result;
}

static gboolean
parse_size (const char *word,
	    size_t      wordlen,
	    int        *vogue_size,
	    gboolean   *size_is_absolute)
{
  char *end;
  double size = g_ascii_strtod (word, &end);

  if (end != word &&
      (end == word + wordlen ||
       (end + 2 == word + wordlen && !strncmp (end, "px", 2))
      ) && size >= 0 && size <= 1000000) /* word is a valid float */
    {
      if (vogue_size)
	*vogue_size = (int)(size * PANGO_SCALE + 0.5);

      if (size_is_absolute)
	*size_is_absolute = end < word + wordlen;

      return TRUE;
    }

  return FALSE;
}

static gboolean
parse_variations (const char  *word,
                  size_t       wordlen,
                  char       **variations)
{
  if (word[0] != '@')
    {
      *variations = NULL;
      return FALSE;
    }

  /* XXX: actually validate here */
  *variations = g_strndup (word + 1, wordlen - 1);

  return TRUE;
}

/**
 * vogue_font_description_from_string:
 * @str: string representation of a font description.
 *
 * Creates a new font description from a string representation in the
 * form
 *
 * "\[FAMILY-LIST] \[STYLE-OPTIONS] \[SIZE] \[VARIATIONS]",
 *
 * where FAMILY-LIST is a comma-separated list of families optionally
 * terminated by a comma, STYLE_OPTIONS is a whitespace-separated list
 * of words where each word describes one of style, variant, weight,
 * stretch, or gravity, and SIZE is a decimal number (size in points)
 * or optionally followed by the unit modifier "px" for absolute size.
 * VARIATIONS is a comma-separated list of font variation
 * specifications of the form "\@axis=value" (the = sign is optional).
 *
 * The following words are understood as styles:
 * "Normal", "Roman", "Oblique", "Italic".
 *
 * The following words are understood as variants:
 * "Small-Caps".
 *
 * The following words are understood as weights:
 * "Thin", "Ultra-Light", "Extra-Light", "Light", "Semi-Light",
 * "Demi-Light", "Book", "Regular", "Medium", "Semi-Bold", "Demi-Bold",
 * "Bold", "Ultra-Bold", "Extra-Bold", "Heavy", "Black", "Ultra-Black",
 * "Extra-Black".
 *
 * The following words are understood as stretch values:
 * "Ultra-Condensed", "Extra-Condensed", "Condensed", "Semi-Condensed",
 * "Semi-Expanded", "Expanded", "Extra-Expanded", "Ultra-Expanded".
 *
 * The following words are understood as gravity values:
 * "Not-Rotated", "South", "Upside-Down", "North", "Rotated-Left",
 * "East", "Rotated-Right", "West".
 *
 * Any one of the options may be absent. If FAMILY-LIST is absent, then
 * the family_name field of the resulting font description will be
 * initialized to %NULL. If STYLE-OPTIONS is missing, then all style
 * options will be set to the default values. If SIZE is missing, the
 * size in the resulting font description will be set to 0.
 *
 * A typical example:
 *
 * "Cantarell Italic Light 15 \@wght=200"
 *
 * Return value: a new #VogueFontDescription.
 **/
VogueFontDescription *
vogue_font_description_from_string (const char *str)
{
  VogueFontDescription *desc;
  const char *p, *last;
  size_t len, wordlen;

  g_return_val_if_fail (str != NULL, NULL);

  desc = vogue_font_description_new ();

  desc->mask = PANGO_FONT_MASK_STYLE |
	       PANGO_FONT_MASK_WEIGHT |
	       PANGO_FONT_MASK_VARIANT |
	       PANGO_FONT_MASK_STRETCH;

  len = strlen (str);
  last = str + len;
  p = getword (str, last, &wordlen, "");
  /* Look for variations at the end of the string */
  if (wordlen != 0)
    {
      if (parse_variations (p, wordlen, &desc->variations))
        {
	  desc->mask |= PANGO_FONT_MASK_VARIATIONS;
	  last = p;
        }
    }

  p = getword (str, last, &wordlen, ",");
  /* Look for a size */
  if (wordlen != 0)
    {
      gboolean size_is_absolute;
      if (parse_size (p, wordlen, &desc->size, &size_is_absolute))
	{
	  desc->size_is_absolute = size_is_absolute;
	  desc->mask |= PANGO_FONT_MASK_SIZE;
	  last = p;
	}
    }

  /* Now parse style words
   */
  p = getword (str, last, &wordlen, ",");
  while (wordlen != 0)
    {
      if (!find_field_any (p, wordlen, desc))
	break;
      else
	{
	  last = p;
	  p = getword (str, last, &wordlen, ",");
	}
    }

  /* Remainder (str => p) is family list. Trim off trailing commas and leading and trailing white space
   */

  while (last > str && g_ascii_isspace (*(last - 1)))
    last--;

  if (last > str && *(last - 1) == ',')
    last--;

  while (last > str && g_ascii_isspace (*(last - 1)))
    last--;

  while (last > str && g_ascii_isspace (*str))
    str++;

  if (str != last)
    {
      int i;
      char **families;

      desc->family_name = g_strndup (str, last - str);

      /* Now sanitize it to trim space from around individual family names.
       * bug #499624 */

      families = g_strsplit (desc->family_name, ",", -1);

      for (i = 0; families[i]; i++)
	g_strstrip (families[i]);

      g_free (desc->family_name);
      desc->family_name = g_strjoinv (",", families);
      g_strfreev (families);

      desc->mask |= PANGO_FONT_MASK_FAMILY;
    }

  return desc;
}

static void
append_field (GString *str, const char *what, const FieldMap *map, int n_elements, int val)
{
  int i;
  for (i=0; i<n_elements; i++)
    {
      if (map[i].value != val)
        continue;

      if (G_LIKELY (map[i].str[0]))
	{
	  if (G_LIKELY (str->len > 0 && str->str[str->len -1] != ' '))
	    g_string_append_c (str, ' ');
	  g_string_append (str, map[i].str);
	}
      return;
    }

  if (G_LIKELY (str->len > 0 || str->str[str->len -1] != ' '))
    g_string_append_c (str, ' ');
  g_string_append_printf (str, "%s=%d", what, val);
}

/**
 * vogue_font_description_to_string:
 * @desc: a #VogueFontDescription
 *
 * Creates a string representation of a font description. See
 * vogue_font_description_from_string() for a description of the
 * format of the string representation. The family list in the
 * string description will only have a terminating comma if the
 * last word of the list is a valid style option.
 *
 * Return value: a new string that must be freed with g_free().
 **/
char *
vogue_font_description_to_string (const VogueFontDescription  *desc)
{
  GString *result;

  g_return_val_if_fail (desc != NULL, NULL);

  result = g_string_new (NULL);

  if (G_LIKELY (desc->family_name && desc->mask & PANGO_FONT_MASK_FAMILY))
    {
      const char *p;
      size_t wordlen;

      g_string_append (result, desc->family_name);

      /* We need to add a trailing comma if the family name ends
       * in a keyword like "Bold", or if the family name ends in
       * a number and no keywords will be added.
       */
      p = getword (desc->family_name, desc->family_name + strlen(desc->family_name), &wordlen, ",");
      if (wordlen != 0 &&
	  (find_field_any (p, wordlen, NULL) ||
	   (parse_size (p, wordlen, NULL, NULL) &&
	    desc->weight == PANGO_WEIGHT_NORMAL &&
	    desc->style == PANGO_STYLE_NORMAL &&
	    desc->stretch == PANGO_STRETCH_NORMAL &&
	    desc->variant == PANGO_VARIANT_NORMAL &&
	    (desc->mask & (PANGO_FONT_MASK_GRAVITY | PANGO_FONT_MASK_SIZE)) == 0)))
	g_string_append_c (result, ',');
    }

#define FIELD(NAME, MASK) \
  append_field (result, G_STRINGIFY (NAME), NAME##_map, G_N_ELEMENTS (NAME##_map), desc->NAME)

  FIELD (weight,  PANGO_FONT_MASK_WEIGHT);
  FIELD (style,   PANGO_FONT_MASK_STYLE);
  FIELD (stretch, PANGO_FONT_MASK_STRETCH);
  FIELD (variant, PANGO_FONT_MASK_VARIANT);
  if (desc->mask & PANGO_FONT_MASK_GRAVITY)
    FIELD (gravity, PANGO_FONT_MASK_GRAVITY);

#undef FIELD

  if (result->len == 0)
    g_string_append (result, "Normal");

  if (desc->mask & PANGO_FONT_MASK_SIZE)
    {
      char buf[G_ASCII_DTOSTR_BUF_SIZE];

      if (result->len > 0 || result->str[result->len -1] != ' ')
	g_string_append_c (result, ' ');

      g_ascii_dtostr (buf, sizeof (buf), (double)desc->size / PANGO_SCALE);
      g_string_append (result, buf);

      if (desc->size_is_absolute)
	g_string_append (result, "px");
    }

  if (desc->variations && desc->mask & PANGO_FONT_MASK_VARIATIONS)
    {
      g_string_append (result, " @");
      g_string_append (result, desc->variations);
    }

  return g_string_free (result, FALSE);
}

/**
 * vogue_font_description_to_filename:
 * @desc: a #VogueFontDescription
 *
 * Creates a filename representation of a font description. The
 * filename is identical to the result from calling
 * vogue_font_description_to_string(), but with underscores instead of
 * characters that are untypical in filenames, and in lower case only.
 *
 * Return value: a new string that must be freed with g_free().
 **/
char *
vogue_font_description_to_filename (const VogueFontDescription  *desc)
{
  char *result;
  char *p;

  g_return_val_if_fail (desc != NULL, NULL);

  result = vogue_font_description_to_string (desc);

  p = result;
  while (*p)
    {
      if (G_UNLIKELY ((guchar) *p >= 128))
        /* skip over non-ASCII chars */;
      else if (strchr ("-+_.", *p) == NULL && !g_ascii_isalnum (*p))
	*p = '_';
      else
	*p = g_ascii_tolower (*p);
      p++;
    }

  return result;
}


static gboolean
parse_field (const char *what,
	     const FieldMap *map,
	     int n_elements,
	     const char *str,
	     int *val,
	     gboolean warn)
{
  gboolean found;
  int len = strlen (str);

  if (G_UNLIKELY (*str == '\0'))
    return FALSE;

  if (field_matches ("Normal", str, len))
    {
      /* find the map entry with empty string */
      int i;

      for (i = 0; i < n_elements; i++)
        if (map[i].str[0] == '\0')
	  {
	    *val = map[i].value;
	    return TRUE;
	  }

      *val = 0;
      return TRUE;
    }

  found = find_field (NULL, map, n_elements, str, len, val);

  if (!found && warn)
    {
	int i;
	GString *s = g_string_new (NULL);

	for (i = 0; i < n_elements; i++)
	  {
	    if (i)
	      g_string_append_c (s, '/');
	    g_string_append (s, map[i].str[0] == '\0' ? "Normal" : map[i].str);
	  }

	g_warning ("%s must be one of %s or a number",
		   what,
		   s->str);

	g_string_free (s, TRUE);
    }

  return found;
}

#define FIELD(NAME, MASK) \
  parse_field (G_STRINGIFY (NAME), NAME##_map, G_N_ELEMENTS (NAME##_map), str, (int *)(void *)NAME, warn)

/**
 * vogue_parse_style:
 * @str: a string to parse.
 * @style: (out): a #VogueStyle to store the result
 *   in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font style. The allowed values are "normal",
 * "italic" and "oblique", case variations being
 * ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 **/
gboolean
vogue_parse_style (const char *str,
		   VogueStyle *style,
		   gboolean    warn)
{
  return FIELD (style,   PANGO_FONT_MASK_STYLE);
}

/**
 * vogue_parse_variant:
 * @str: a string to parse.
 * @variant: (out): a #VogueVariant to store the
 *   result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font variant. The allowed values are "normal"
 * and "smallcaps" or "small_caps", case variations being
 * ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 **/
gboolean
vogue_parse_variant (const char   *str,
		     VogueVariant *variant,
		     gboolean	   warn)
{
  return FIELD (variant, PANGO_FONT_MASK_VARIANT);
}

/**
 * vogue_parse_weight:
 * @str: a string to parse.
 * @weight: (out): a #VogueWeight to store the result
 *   in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font weight. The allowed values are "heavy",
 * "ultrabold", "bold", "normal", "light", "ultraleight"
 * and integers. Case variations are ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 **/
gboolean
vogue_parse_weight (const char  *str,
		    VogueWeight *weight,
		    gboolean     warn)
{
  return FIELD (weight,  PANGO_FONT_MASK_WEIGHT);
}

/**
 * vogue_parse_stretch:
 * @str: a string to parse.
 * @stretch: (out): a #VogueStretch to store the
 *   result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font stretch. The allowed values are
 * "ultra_condensed", "extra_condensed", "condensed",
 * "semi_condensed", "normal", "semi_expanded", "expanded",
 * "extra_expanded" and "ultra_expanded". Case variations are
 * ignored and the '_' characters may be omitted.
 *
 * Return value: %TRUE if @str was successfully parsed.
 **/
gboolean
vogue_parse_stretch (const char   *str,
		     VogueStretch *stretch,
		     gboolean	   warn)
{
  return FIELD (stretch, PANGO_FONT_MASK_STRETCH);
}




/*
 * VogueFont
 */

typedef struct {
  hb_font_t *hb_font;
} VogueFontPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (VogueFont, vogue_font, G_TYPE_OBJECT)

static void
vogue_font_finalize (GObject *object)
{
  VogueFont *font = PANGO_FONT (object);
  VogueFontPrivate *priv = vogue_font_get_instance_private (font);

  hb_font_destroy (priv->hb_font);

  G_OBJECT_CLASS (vogue_font_parent_class)->finalize (object);
}

static void
vogue_font_class_init (VogueFontClass *class G_GNUC_UNUSED)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = vogue_font_finalize;
}

static void
vogue_font_init (VogueFont *font G_GNUC_UNUSED)
{
}

/**
 * vogue_font_describe:
 * @font: a #VogueFont
 *
 * Returns a description of the font, with font size set in points.
 * Use vogue_font_describe_with_absolute_size() if you want the font
 * size in device units.
 *
 * Return value: a newly-allocated #VogueFontDescription object.
 **/
VogueFontDescription *
vogue_font_describe (VogueFont      *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return PANGO_FONT_GET_CLASS (font)->describe (font);
}

/**
 * vogue_font_describe_with_absolute_size:
 * @font: a #VogueFont
 *
 * Returns a description of the font, with absolute font size set
 * (in device units). Use vogue_font_describe() if you want the font
 * size in points.
 *
 * Return value: a newly-allocated #VogueFontDescription object.
 *
 * Since: 1.14
 **/
VogueFontDescription *
vogue_font_describe_with_absolute_size (VogueFont      *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  if (G_UNLIKELY (!PANGO_FONT_GET_CLASS (font)->describe_absolute))
    {
      g_warning ("describe_absolute not implemented for this font class, report this as a bug");
      return vogue_font_describe (font);
    }

  return PANGO_FONT_GET_CLASS (font)->describe_absolute (font);
}

/**
 * vogue_font_get_coverage:
 * @font: a #VogueFont
 * @language: the language tag
 *
 * Computes the coverage map for a given font and language tag.
 *
 * Return value: (transfer full): a newly-allocated #VogueCoverage
 *   object.
 **/
VogueCoverage *
vogue_font_get_coverage (VogueFont     *font,
			 VogueLanguage *language)
{
  g_return_val_if_fail (font != NULL, NULL);

  return PANGO_FONT_GET_CLASS (font)->get_coverage (font, language);
}

/**
 * vogue_font_find_shaper:
 * @font: a #VogueFont
 * @language: the language tag
 * @ch: a Unicode character.
 *
 * Finds the best matching shaper for a font for a particular
 * language tag and character point.
 *
 * Return value: (transfer none): the best matching shaper.
 * Deprecated: Shape engines are no longer used
 **/
VogueEngineShape *
vogue_font_find_shaper (VogueFont     *font,
			VogueLanguage *language,
			guint32        ch)
{
  return NULL;
}

/**
 * vogue_font_get_glyph_extents:
 * @font: (nullable): a #VogueFont
 * @glyph: the glyph index
 * @ink_rect: (out) (allow-none): rectangle used to store the extents of the glyph
 *            as drawn or %NULL to indicate that the result is not needed.
 * @logical_rect: (out) (allow-none): rectangle used to store the logical extents of
 *            the glyph or %NULL to indicate that the result is not needed.
 *
 * Gets the logical and ink extents of a glyph within a font. The
 * coordinate system for each rectangle has its origin at the
 * base line and horizontal origin of the character with increasing
 * coordinates extending to the right and down. The macros PANGO_ASCENT(),
 * PANGO_DESCENT(), PANGO_LBEARING(), and PANGO_RBEARING() can be used to convert
 * from the extents rectangle to more traditional font metrics. The units
 * of the rectangles are in 1/PANGO_SCALE of a device unit.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 **/
void
vogue_font_get_glyph_extents  (VogueFont      *font,
			       VogueGlyph      glyph,
			       VogueRectangle *ink_rect,
			       VogueRectangle *logical_rect)
{
  if (G_UNLIKELY (!font))
    {
      if (ink_rect)
	{
	  ink_rect->x = PANGO_SCALE;
	  ink_rect->y = - (PANGO_UNKNOWN_GLYPH_HEIGHT - 1) * PANGO_SCALE;
	  ink_rect->height = (PANGO_UNKNOWN_GLYPH_HEIGHT - 2) * PANGO_SCALE;
	  ink_rect->width = (PANGO_UNKNOWN_GLYPH_WIDTH - 2) * PANGO_SCALE;
	}
      if (logical_rect)
	{
	  logical_rect->x = logical_rect->y = 0;
	  logical_rect->y = - PANGO_UNKNOWN_GLYPH_HEIGHT * PANGO_SCALE;
	  logical_rect->height = PANGO_UNKNOWN_GLYPH_HEIGHT * PANGO_SCALE;
	  logical_rect->width = PANGO_UNKNOWN_GLYPH_WIDTH * PANGO_SCALE;
	}
      return;
    }

  PANGO_FONT_GET_CLASS (font)->get_glyph_extents (font, glyph, ink_rect, logical_rect);
}

/**
 * vogue_font_get_metrics:
 * @font: (nullable): a #VogueFont
 * @language: (allow-none): language tag used to determine which script to get the metrics
 *            for, or %NULL to indicate to get the metrics for the entire font.
 *
 * Gets overall metric information for a font. Since the metrics may be
 * substantially different for different scripts, a language tag can
 * be provided to indicate that the metrics should be retrieved that
 * correspond to the script(s) used by that language.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 *
 * Return value: a #VogueFontMetrics object. The caller must call vogue_font_metrics_unref()
 *   when finished using the object.
 **/
VogueFontMetrics *
vogue_font_get_metrics (VogueFont        *font,
			VogueLanguage    *language)
{
  if (G_UNLIKELY (!font))
    {
      VogueFontMetrics *metrics = vogue_font_metrics_new ();

      metrics->ascent = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_HEIGHT;
      metrics->descent = 0;
      metrics->height = 0;
      metrics->approximate_char_width = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_WIDTH;
      metrics->approximate_digit_width = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_WIDTH;
      metrics->underline_position = -PANGO_SCALE;
      metrics->underline_thickness = PANGO_SCALE;
      metrics->strikethrough_position = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_HEIGHT / 2;
      metrics->strikethrough_thickness = PANGO_SCALE;

      return metrics;
    }

  return PANGO_FONT_GET_CLASS (font)->get_metrics (font, language);
}

/**
 * vogue_font_get_font_map:
 * @font: (nullable): a #VogueFont, or %NULL
 *
 * Gets the font map for which the font was created.
 *
 * Note that the font maintains a <firstterm>weak</firstterm> reference
 * to the font map, so if all references to font map are dropped, the font
 * map will be finalized even if there are fonts created with the font
 * map that are still alive.  In that case this function will return %NULL.
 * It is the responsibility of the user to ensure that the font map is kept
 * alive.  In most uses this is not an issue as a #VogueContext holds
 * a reference to the font map.
 *
 * Return value: (transfer none) (nullable): the #VogueFontMap for the
 *               font, or %NULL if @font is %NULL.
 *
 * Since: 1.10
 **/
VogueFontMap *
vogue_font_get_font_map (VogueFont *font)
{
  if (G_UNLIKELY (!font))
    return NULL;

  if (PANGO_FONT_GET_CLASS (font)->get_font_map)
    return PANGO_FONT_GET_CLASS (font)->get_font_map (font);
  else
    return NULL;
}

/**
 * vogue_font_get_hb_font: (skip)
 * @font: a #VogueFont
 *
 * Get a hb_font_t object backing this font.
 *
 * Note that the objects returned by this function
 * are cached and immutable. If you need to make
 * changes to the hb_font_t, use hb_font_create_sub_font().
 *
 * Returns: (transfer none) (nullable): the hb_font_t object backing the
 *          font, or %NULL if the font does not have one
 *
 * Since: 1.44
 */
hb_font_t *
vogue_font_get_hb_font (VogueFont *font)
{
  VogueFontPrivate *priv = vogue_font_get_instance_private (font);

  g_return_val_if_fail (PANGO_IS_FONT (font), NULL);

  if (priv->hb_font)
    return priv->hb_font;

  priv->hb_font = PANGO_FONT_GET_CLASS (font)->create_hb_font (font);

  hb_font_make_immutable (priv->hb_font);

  return priv->hb_font;
}

G_DEFINE_BOXED_TYPE (VogueFontMetrics, vogue_font_metrics,
                     vogue_font_metrics_ref,
                     vogue_font_metrics_unref);

/**
 * vogue_font_metrics_new:
 *
 * Creates a new #VogueFontMetrics structure. This is only for
 * internal use by Vogue backends and there is no public way
 * to set the fields of the structure.
 *
 * Return value: a newly-created #VogueFontMetrics structure
 *   with a reference count of 1.
 **/
VogueFontMetrics *
vogue_font_metrics_new (void)
{
  VogueFontMetrics *metrics = g_slice_new0 (VogueFontMetrics);
  metrics->ref_count = 1;

  return metrics;
}

/**
 * vogue_font_metrics_ref:
 * @metrics: (nullable): a #VogueFontMetrics structure, may be %NULL
 *
 * Increase the reference count of a font metrics structure by one.
 *
 * Return value: (nullable): @metrics
 **/
VogueFontMetrics *
vogue_font_metrics_ref (VogueFontMetrics *metrics)
{
  if (metrics == NULL)
    return NULL;

  g_atomic_int_inc ((int *) &metrics->ref_count);

  return metrics;
}

/**
 * vogue_font_metrics_unref:
 * @metrics: (nullable): a #VogueFontMetrics structure, may be %NULL
 *
 * Decrease the reference count of a font metrics structure by one. If
 * the result is zero, frees the structure and any associated
 * memory.
 **/
void
vogue_font_metrics_unref (VogueFontMetrics *metrics)
{
  if (metrics == NULL)
    return;

  g_return_if_fail (metrics->ref_count > 0 );

  if (g_atomic_int_dec_and_test ((int *) &metrics->ref_count))
    g_slice_free (VogueFontMetrics, metrics);
}

/**
 * vogue_font_metrics_get_ascent:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the ascent from a font metrics structure. The ascent is
 * the distance from the baseline to the logical top of a line
 * of text. (The logical top may be above or below the top of the
 * actual drawn ink. It is necessary to lay out the text to figure
 * where the ink will be.)
 *
 * Return value: the ascent, in Vogue units.
 **/
int
vogue_font_metrics_get_ascent (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->ascent;
}

/**
 * vogue_font_metrics_get_descent:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the descent from a font metrics structure. The descent is
 * the distance from the baseline to the logical bottom of a line
 * of text. (The logical bottom may be above or below the bottom of the
 * actual drawn ink. It is necessary to lay out the text to figure
 * where the ink will be.)
 *
 * Return value: the descent, in Vogue units.
 **/
int
vogue_font_metrics_get_descent (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->descent;
}

/**
 * vogue_font_metrics_get_height:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the line height from a font metrics structure. The
 * line height is the distance between successive baselines
 * in wrapped text.
 *
 * If the line height is not available, 0 is returned.
 *
 * Return value: the height, in Vogue units
 *
 * Since: 1.44
 */
int
vogue_font_metrics_get_height (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->height;
}

/**
 * vogue_font_metrics_get_approximate_char_width:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the approximate character width for a font metrics structure.
 * This is merely a representative value useful, for example, for
 * determining the initial size for a window. Actual characters in
 * text will be wider and narrower than this.
 *
 * Return value: the character width, in Vogue units.
 **/
int
vogue_font_metrics_get_approximate_char_width (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->approximate_char_width;
}

/**
 * vogue_font_metrics_get_approximate_digit_width:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the approximate digit width for a font metrics structure.
 * This is merely a representative value useful, for example, for
 * determining the initial size for a window. Actual digits in
 * text can be wider or narrower than this, though this value
 * is generally somewhat more accurate than the result of
 * vogue_font_metrics_get_approximate_char_width() for digits.
 *
 * Return value: the digit width, in Vogue units.
 **/
int
vogue_font_metrics_get_approximate_digit_width (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->approximate_digit_width;
}

/**
 * vogue_font_metrics_get_underline_position:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the suggested position to draw the underline.
 * The value returned is the distance <emphasis>above</emphasis> the
 * baseline of the top of the underline. Since most fonts have
 * underline positions beneath the baseline, this value is typically
 * negative.
 *
 * Return value: the suggested underline position, in Vogue units.
 *
 * Since: 1.6
 **/
int
vogue_font_metrics_get_underline_position (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->underline_position;
}

/**
 * vogue_font_metrics_get_underline_thickness:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the suggested thickness to draw for the underline.
 *
 * Return value: the suggested underline thickness, in Vogue units.
 *
 * Since: 1.6
 **/
int
vogue_font_metrics_get_underline_thickness (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->underline_thickness;
}

/**
 * vogue_font_metrics_get_strikethrough_position:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the suggested position to draw the strikethrough.
 * The value returned is the distance <emphasis>above</emphasis> the
 * baseline of the top of the strikethrough.
 *
 * Return value: the suggested strikethrough position, in Vogue units.
 *
 * Since: 1.6
 **/
int
vogue_font_metrics_get_strikethrough_position (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->strikethrough_position;
}

/**
 * vogue_font_metrics_get_strikethrough_thickness:
 * @metrics: a #VogueFontMetrics structure
 *
 * Gets the suggested thickness to draw for the strikethrough.
 *
 * Return value: the suggested strikethrough thickness, in Vogue units.
 *
 * Since: 1.6
 **/
int
vogue_font_metrics_get_strikethrough_thickness (VogueFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->strikethrough_thickness;
}

/*
 * VogueFontFamily
 */

G_DEFINE_ABSTRACT_TYPE (VogueFontFamily, vogue_font_family, G_TYPE_OBJECT)

static void
vogue_font_family_class_init (VogueFontFamilyClass *class G_GNUC_UNUSED)
{
}

static void
vogue_font_family_init (VogueFontFamily *family G_GNUC_UNUSED)
{
}

/**
 * vogue_font_family_get_name:
 * @family: a #VogueFontFamily
 *
 * Gets the name of the family. The name is unique among all
 * fonts for the font backend and can be used in a #VogueFontDescription
 * to specify that a face from this family is desired.
 *
 * Return value: the name of the family. This string is owned
 *   by the family object and must not be modified or freed.
 **/
const char *
vogue_font_family_get_name (VogueFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), NULL);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->get_name (family);
}

/**
 * vogue_font_family_list_faces:
 * @family: a #VogueFontFamily
 * @faces: (out) (allow-none) (array length=n_faces) (transfer container):
 *   location to store an array of pointers to #VogueFontFace objects,
 *   or %NULL. This array should be freed with g_free() when it is no
 *   longer needed.
 * @n_faces: (out): location to store number of elements in @faces.
 *
 * Lists the different font faces that make up @family. The faces
 * in a family share a common design, but differ in slant, weight,
 * width and other aspects.
 **/
void
vogue_font_family_list_faces (VogueFontFamily  *family,
			      VogueFontFace  ***faces,
			      int              *n_faces)
{
  g_return_if_fail (PANGO_IS_FONT_FAMILY (family));

  PANGO_FONT_FAMILY_GET_CLASS (family)->list_faces (family, faces, n_faces);
}

/**
 * vogue_font_family_is_monospace:
 * @family: a #VogueFontFamily
 *
 * A monospace font is a font designed for text display where the the
 * characters form a regular grid. For Western languages this would
 * mean that the advance width of all characters are the same, but
 * this categorization also includes Asian fonts which include
 * double-width characters: characters that occupy two grid cells.
 * g_unichar_iswide() returns a result that indicates whether a
 * character is typically double-width in a monospace font.
 *
 * The best way to find out the grid-cell size is to call
 * vogue_font_metrics_get_approximate_digit_width(), since the results
 * of vogue_font_metrics_get_approximate_char_width() may be affected
 * by double-width characters.
 *
 * Return value: %TRUE if the family is monospace.
 *
 * Since: 1.4
 **/
gboolean
vogue_font_family_is_monospace (VogueFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), FALSE);

  if (PANGO_FONT_FAMILY_GET_CLASS (family)->is_monospace)
    return PANGO_FONT_FAMILY_GET_CLASS (family)->is_monospace (family);
  else
    return FALSE;
}

/**
 * vogue_font_family_is_variable:
 * @family: a #VogueFontFamily
 *
 * A variable font is a font which has axes that can be modified to
 * produce different faces.
 *
 * Return value: %TRUE if the family is variable
 *
 * Since: 1.44
 **/
gboolean
vogue_font_family_is_variable (VogueFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), FALSE);

  if (PANGO_FONT_FAMILY_GET_CLASS (family)->is_variable)
    return PANGO_FONT_FAMILY_GET_CLASS (family)->is_variable (family);
  else
    return FALSE;
}

/*
 * VogueFontFace
 */

G_DEFINE_ABSTRACT_TYPE (VogueFontFace, vogue_font_face, G_TYPE_OBJECT)

static void
vogue_font_face_class_init (VogueFontFaceClass *class G_GNUC_UNUSED)
{
}

static void
vogue_font_face_init (VogueFontFace *face G_GNUC_UNUSED)
{
}

/**
 * vogue_font_face_describe:
 * @face: a #VogueFontFace
 *
 * Returns the family, style, variant, weight and stretch of
 * a #VogueFontFace. The size field of the resulting font description
 * will be unset.
 *
 * Return value: a newly-created #VogueFontDescription structure
 *  holding the description of the face. Use vogue_font_description_free()
 *  to free the result.
 **/
VogueFontDescription *
vogue_font_face_describe (VogueFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->describe (face);
}

/**
 * vogue_font_face_is_synthesized:
 * @face: a #VogueFontFace
 *
 * Returns whether a #VogueFontFace is synthesized by the underlying
 * font rendering engine from another face, perhaps by shearing, emboldening,
 * or lightening it.
 *
 * Return value: whether @face is synthesized.
 *
 * Since: 1.18
 **/
gboolean
vogue_font_face_is_synthesized (VogueFontFace  *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), FALSE);

  if (PANGO_FONT_FACE_GET_CLASS (face)->is_synthesized != NULL)
    return PANGO_FONT_FACE_GET_CLASS (face)->is_synthesized (face);
  else
    return FALSE;
}

/**
 * vogue_font_face_get_face_name:
 * @face: a #VogueFontFace.
 *
 * Gets a name representing the style of this face among the
 * different faces in the #VogueFontFamily for the face. This
 * name is unique among all faces in the family and is suitable
 * for displaying to users.
 *
 * Return value: the face name for the face. This string is
 *   owned by the face object and must not be modified or freed.
 **/
const char *
vogue_font_face_get_face_name (VogueFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->get_face_name (face);
}

/**
 * vogue_font_face_list_sizes:
 * @face: a #VogueFontFace.
 * @sizes: (out) (array length=n_sizes) (nullable) (optional):
 *         location to store a pointer to an array of int. This array
 *         should be freed with g_free().
 * @n_sizes: location to store the number of elements in @sizes
 *
 * List the available sizes for a font. This is only applicable to bitmap
 * fonts. For scalable fonts, stores %NULL at the location pointed to by
 * @sizes and 0 at the location pointed to by @n_sizes. The sizes returned
 * are in Vogue units and are sorted in ascending order.
 *
 * Since: 1.4
 **/
void
vogue_font_face_list_sizes (VogueFontFace  *face,
			    int           **sizes,
			    int            *n_sizes)
{
  g_return_if_fail (PANGO_IS_FONT_FACE (face));
  g_return_if_fail (sizes == NULL || n_sizes != NULL);

  if (n_sizes == NULL)
    return;

  if (PANGO_FONT_FACE_GET_CLASS (face)->list_sizes != NULL)
    PANGO_FONT_FACE_GET_CLASS (face)->list_sizes (face, sizes, n_sizes);
  else
    {
      if (sizes != NULL)
	*sizes = NULL;
      *n_sizes = 0;
    }
}

/**
 * vogue_font_has_char:
 * @font: a #VogueFont
 * @wc: a Unicode character
 *
 * Returns whether the font provides a glyph for this character.
 *
 * Returns %TRUE if @font can render @wc
 *
 * Since: 1.44
 */
gboolean
vogue_font_has_char (VogueFont *font,
                     gunichar   wc)
{
  VogueCoverage *coverage = vogue_font_get_coverage (font, vogue_language_get_default ());
  VogueCoverageLevel result = vogue_coverage_get (coverage, wc);
  vogue_coverage_unref (coverage);
  return result != PANGO_COVERAGE_NONE;
}

/**
 * vogue_font_get_features:
 * @font: a #VogueFont
 * @features: (out caller-allocates) (array length=len): Array to features in
 * @len: the length of @features
 * @num_features: (inout): the number of used items in @features
 *
 * Obtain the OpenType features that are provided by the font.
 * These are passed to the rendering system, together with features
 * that have been explicitly set via attributes.
 *
 * Note that this does not include OpenType features which the
 * rendering system enables by default.
 *
 * Since: 1.44
 */
void
vogue_font_get_features (VogueFont    *font,
                         hb_feature_t *features,
                         guint         len,
                         guint        *num_features)
{
  if (PANGO_FONT_GET_CLASS (font)->get_features)
    PANGO_FONT_GET_CLASS (font)->get_features (font, features, len, num_features);
}
