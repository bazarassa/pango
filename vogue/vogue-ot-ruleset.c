/* Vogue
 * vogue-ot-ruleset.c: Shaping using OpenType features
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

#include "config.h"

#include "vogue-ot-private.h"

static void vogue_ot_ruleset_finalize   (GObject        *object);

/**
 * VogueOTRuleset:
 *
 * The #VogueOTRuleset structure holds a
 * set of features selected from the tables in an OpenType font.
 * (A feature is an operation such as adjusting glyph positioning
 * that should be applied to a text feature such as a certain
 * type of accent.) A #VogueOTRuleset
 * is created with vogue_ot_ruleset_new(), features are added
 * to it with vogue_ot_ruleset_add_feature(), then it is
 * applied to a #VogueGlyphString with vogue_ot_ruleset_shape().
 */
G_DEFINE_TYPE (VogueOTRuleset, vogue_ot_ruleset, G_TYPE_OBJECT);

static void
vogue_ot_ruleset_class_init (VogueOTRulesetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = vogue_ot_ruleset_finalize;
}

static void
vogue_ot_ruleset_init (VogueOTRuleset *ruleset)
{
}

static void
vogue_ot_ruleset_finalize (GObject *object)
{
  G_OBJECT_CLASS (vogue_ot_ruleset_parent_class)->finalize (object);
}

/**
 * vogue_ot_ruleset_get_for_description:
 * @info: a #VogueOTInfo.
 * @desc: a #VogueOTRulesetDescription.
 *
 * Returns a ruleset for the given OpenType info and ruleset
 * description.  Rulesets are created on demand using
 * vogue_ot_ruleset_new_from_description().
 * The returned ruleset should not be modified or destroyed.
 *
 * The static feature map members of @desc should be alive as
 * long as @info is.
 *
 * Return value: the #VogueOTRuleset for @desc. This object will have
 * the same lifetime as @info.
 *
 * Since: 1.18
 **/
const VogueOTRuleset *
vogue_ot_ruleset_get_for_description (VogueOTInfo                     *info,
				      const VogueOTRulesetDescription *desc)
{
  static VogueOTRuleset *ruleset; /* MT-safe */

  if (g_once_init_enter (&ruleset))
    g_once_init_leave (&ruleset, g_object_new (PANGO_TYPE_OT_RULESET, NULL));

  return ruleset;
}

/**
 * vogue_ot_ruleset_new:
 * @info: a #VogueOTInfo.
 *
 * Creates a new #VogueOTRuleset for the given OpenType info.
 *
 * Return value: the newly allocated #VogueOTRuleset, which
 *               should be freed with g_object_unref().
 **/
VogueOTRuleset *
vogue_ot_ruleset_new (VogueOTInfo *info)
{
  return g_object_new (PANGO_TYPE_OT_RULESET, NULL);
}

/**
 * vogue_ot_ruleset_new_for:
 * @info: a #VogueOTInfo.
 * @script: a #VogueScript.
 * @language: a #VogueLanguage.
 *
 * Creates a new #VogueOTRuleset for the given OpenType info, script, and
 * language.
 *
 * This function is part of a convenience scheme that highly simplifies
 * using a #VogueOTRuleset to represent features for a specific pair of script
 * and language.  So one can use this function passing in the script and
 * language of interest, and later try to add features to the ruleset by just
 * specifying the feature name or tag, without having to deal with finding
 * script, language, or feature indices manually.
 *
 * In excess to what vogue_ot_ruleset_new() does, this function will:
 * <itemizedlist>
 *   <listitem>
 *   Find the #VogueOTTag script and language tags associated with
 *   @script and @language using vogue_ot_tag_from_script() and
 *   vogue_ot_tag_from_language(),
 *   </listitem>
 *   <listitem>
 *   For each of table types %PANGO_OT_TABLE_GSUB and %PANGO_OT_TABLE_GPOS,
 *   find the script index of the script tag found and the language
 *   system index of the language tag found in that script system, using
 *   vogue_ot_info_find_script() and vogue_ot_info_find_language(),
 *   </listitem>
 *   <listitem>
 *   For found language-systems, if they have required feature
 *   index, add that feature to the ruleset using
 *   vogue_ot_ruleset_add_feature(),
 *   </listitem>
 *   <listitem>
 *   Remember found script and language indices for both table types,
 *   and use them in future vogue_ot_ruleset_maybe_add_feature() and
 *   vogue_ot_ruleset_maybe_add_features().
 *   </listitem>
 * </itemizedlist>
 *
 * Because of the way return values of vogue_ot_info_find_script() and
 * vogue_ot_info_find_language() are ignored, this function automatically
 * finds and uses the 'DFLT' script and the default language-system.
 *
 * Return value: the newly allocated #VogueOTRuleset, which
 *               should be freed with g_object_unref().
 *
 * Since: 1.18
 **/
VogueOTRuleset *
vogue_ot_ruleset_new_for (VogueOTInfo       *info,
			  VogueScript        script,
			  VogueLanguage     *language)
{
  return g_object_new (PANGO_TYPE_OT_RULESET, NULL);
}

/**
 * vogue_ot_ruleset_new_from_description:
 * @info: a #VogueOTInfo.
 * @desc: a #VogueOTRulesetDescription.
 *
 * Creates a new #VogueOTRuleset for the given OpenType infor and
 * matching the given ruleset description.
 *
 * This is a convenience function that calls vogue_ot_ruleset_new_for() and
 * adds the static GSUB/GPOS features to the resulting ruleset, followed by
 * adding other features to both GSUB and GPOS.
 *
 * The static feature map members of @desc should be alive as
 * long as @info is.
 *
 * Return value: the newly allocated #VogueOTRuleset, which
 *               should be freed with g_object_unref().
 *
 * Since: 1.18
 **/
VogueOTRuleset *
vogue_ot_ruleset_new_from_description (VogueOTInfo                     *info,
				       const VogueOTRulesetDescription *desc)
{
  return g_object_new (PANGO_TYPE_OT_RULESET, NULL);
}

/**
 * vogue_ot_ruleset_add_feature:
 * @ruleset: a #VogueOTRuleset.
 * @table_type: the table type to add a feature to.
 * @feature_index: the index of the feature to add.
 * @property_bit: the property bit to use for this feature. Used to identify
 *                the glyphs that this feature should be applied to, or
 *                %PANGO_OT_ALL_GLYPHS if it should be applied to all glyphs.
 *
 * Adds a feature to the ruleset.
 **/
void
vogue_ot_ruleset_add_feature (VogueOTRuleset   *ruleset,
			      VogueOTTableType  table_type,
			      guint             feature_index,
			      gulong            property_bit)
{
}

/**
 * vogue_ot_ruleset_maybe_add_feature:
 * @ruleset: a #VogueOTRuleset.
 * @table_type: the table type to add a feature to.
 * @feature_tag: the tag of the feature to add.
 * @property_bit: the property bit to use for this feature. Used to identify
 *                the glyphs that this feature should be applied to, or
 *                %PANGO_OT_ALL_GLYPHS if it should be applied to all glyphs.
 *
 * This is a convenience function that first tries to find the feature
 * using vogue_ot_info_find_feature() and the ruleset script and language
 * passed to vogue_ot_ruleset_new_for(),
 * and if the feature is found, adds it to the ruleset.
 *
 * If @ruleset was not created using vogue_ot_ruleset_new_for(), this function
 * does nothing.
 *
 * Return value: %TRUE if the feature was found and added to ruleset,
 *               %FALSE otherwise.
 *
 * Since: 1.18
 **/
gboolean
vogue_ot_ruleset_maybe_add_feature (VogueOTRuleset          *ruleset,
				    VogueOTTableType         table_type,
				    VogueOTTag               feature_tag,
				    gulong                   property_bit)
{
  return FALSE;
}

/**
 * vogue_ot_ruleset_maybe_add_features:
 * @ruleset: a #VogueOTRuleset.
 * @table_type: the table type to add features to.
 * @features: array of feature name and property bits to add.
 * @n_features: number of feature records in @features array.
 *
 * This is a convenience function that 
 * for each feature in the feature map array @features
 * converts the feature name to a #VogueOTTag feature tag using PANGO_OT_TAG_MAKE()
 * and calls vogue_ot_ruleset_maybe_add_feature() on it.
 *
 * Return value: The number of features in @features that were found
 *               and added to @ruleset.
 *
 * Since: 1.18
 **/
guint
vogue_ot_ruleset_maybe_add_features (VogueOTRuleset          *ruleset,
				     VogueOTTableType         table_type,
				     const VogueOTFeatureMap *features,
				     guint                    n_features)
{
  return 0;
}

/**
 * vogue_ot_ruleset_get_feature_count:
 * @ruleset: a #VogueOTRuleset.
 * @n_gsub_features: (out) (optional): location to store number of
 *   GSUB features, or %NULL.
 * @n_gpos_features: (out) (optional): location to store number of
 *   GPOS features, or %NULL.
 *
 * Gets the number of GSUB and GPOS features in the ruleset.
 *
 * Return value: Total number of features in the @ruleset.
 *
 * Since: 1.18
 **/
guint
vogue_ot_ruleset_get_feature_count (const VogueOTRuleset   *ruleset,
				    guint                  *n_gsub_features,
				    guint                  *n_gpos_features)
{
  return 0;
}

/**
 * vogue_ot_ruleset_substitute:
 * @ruleset: a #VogueOTRuleset.
 * @buffer: a #VogueOTBuffer.
 *
 * Performs the OpenType GSUB substitution on @buffer using the features
 * in @ruleset
 *
 * Since: 1.4
 **/
void
vogue_ot_ruleset_substitute  (const VogueOTRuleset *ruleset,
			      VogueOTBuffer        *buffer)
{
}

/**
 * vogue_ot_ruleset_position:
 * @ruleset: a #VogueOTRuleset.
 * @buffer: a #VogueOTBuffer.
 *
 * Performs the OpenType GPOS positioning on @buffer using the features
 * in @ruleset
 *
 * Since: 1.4
 **/
void
vogue_ot_ruleset_position (const VogueOTRuleset *ruleset,
			   VogueOTBuffer        *buffer)
{
}


/* ruleset descriptions */

/**
 * vogue_ot_ruleset_description_hash:
 * @desc: a ruleset description
 *
 * Computes a hash of a #VogueOTRulesetDescription structure suitable
 * to be used, for example, as an argument to g_hash_table_new().
 *
 * Return value: the hash value.
 *
 * Since: 1.18
 **/
guint
vogue_ot_ruleset_description_hash  (const VogueOTRulesetDescription *desc)
{
  return 0;
}

/**
 * vogue_ot_ruleset_description_equal:
 * @desc1: a ruleset description
 * @desc2: a ruleset description
 *
 * Compares two ruleset descriptions for equality.
 * Two ruleset descriptions are considered equal if the rulesets
 * they describe are provably identical.  This means that their
 * script, language, and all feature sets should be equal.  For static feature
 * sets, the array addresses are compared directly, while for other
 * features, the list of features is compared one by one.
 * (Two ruleset descriptions may result in identical rulesets
 * being created, but still compare %FALSE.)
 *
 * Return value: %TRUE if two ruleset descriptions are identical,
 *               %FALSE otherwise.
 *
 * Since: 1.18
 **/
gboolean
vogue_ot_ruleset_description_equal (const VogueOTRulesetDescription *desc1,
				    const VogueOTRulesetDescription *desc2)
{
  return TRUE;
}

/**
 * vogue_ot_ruleset_description_copy:
 * @desc: ruleset description to copy
 *
 * Creates a copy of @desc, which should be freed with
 * vogue_ot_ruleset_description_free(). Primarily used internally
 * by vogue_ot_ruleset_get_for_description() to cache rulesets for
 * ruleset descriptions.
 *
 * Return value: the newly allocated #VogueOTRulesetDescription, which
 *               should be freed with vogue_ot_ruleset_description_free().
 *
 * Since: 1.18
 **/
VogueOTRulesetDescription *
vogue_ot_ruleset_description_copy  (const VogueOTRulesetDescription *desc)
{
  VogueOTRulesetDescription *copy;

  g_return_val_if_fail (desc != NULL, NULL);

  copy = g_slice_new (VogueOTRulesetDescription);

  *copy = *desc;

  return copy;
}

/**
 * vogue_ot_ruleset_description_free:
 * @desc: an allocated #VogueOTRulesetDescription
 *
 * Frees a ruleset description allocated by 
 * vogue_ot_ruleset_description_copy().
 *
 * Since: 1.18
 **/
void
vogue_ot_ruleset_description_free  (VogueOTRulesetDescription *desc)
{
  g_slice_free (VogueOTRulesetDescription, desc);
}
