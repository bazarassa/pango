/* Vogue
 * vogue-ot.h:
 *
 * Copyright (C) 2000,2007 Red Hat Software
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

#ifndef __PANGO_OT_H__
#define __PANGO_OT_H__

/* Deprecated.  Use HarfBuzz directly! */

#include <vogue/voguefc-font.h>
#include <vogue/vogue-glyph.h>
#include <vogue/vogue-font.h>
#include <vogue/vogue-script.h>
#include <vogue/vogue-language.h>

#include <ft2build.h>
#include FT_FREETYPE_H

G_BEGIN_DECLS

#ifndef PANGO_DISABLE_DEPRECATED

/**
 * VogueOTTag:
 *
 * The #VogueOTTag typedef is used to represent TrueType and OpenType
 * four letter tags inside Vogue. Use PANGO_OT_TAG_MAKE()
 * or PANGO_OT_TAG_MAKE_FROM_STRING() macros to create <type>VogueOTTag</type>s manually.
 */
typedef guint32 VogueOTTag;

/**
 * PANGO_OT_TAG_MAKE:
 * @c1: First character.
 * @c2: Second character.
 * @c3: Third character.
 * @c4: Fourth character.
 *
 * Creates a #VogueOTTag from four characters.  This is similar and
 * compatible with the <function>FT_MAKE_TAG()</function> macro from FreeType.
 */
/**
 * PANGO_OT_TAG_MAKE_FROM_STRING:
 * @s: The string representation of the tag.
 *
 * Creates a #VogueOTTag from a string. The string should be at least
 * four characters long (pad with space characters if needed), and need
 * not be nul-terminated.  This is a convenience wrapper around
 * PANGO_OT_TAG_MAKE(), but cannot be used in certain situations, for
 * example, as a switch expression, as it dereferences pointers.
 */
#define PANGO_OT_TAG_MAKE(c1,c2,c3,c4)		((VogueOTTag) FT_MAKE_TAG (c1, c2, c3, c4))
#define PANGO_OT_TAG_MAKE_FROM_STRING(s)	(PANGO_OT_TAG_MAKE(((const char *) s)[0], \
								   ((const char *) s)[1], \
								   ((const char *) s)[2], \
								   ((const char *) s)[3]))

typedef struct _VogueOTInfo       VogueOTInfo;
typedef struct _VogueOTBuffer     VogueOTBuffer;
typedef struct _VogueOTGlyph      VogueOTGlyph;
typedef struct _VogueOTRuleset    VogueOTRuleset;
typedef struct _VogueOTFeatureMap VogueOTFeatureMap;
typedef struct _VogueOTRulesetDescription VogueOTRulesetDescription;

/**
 * VogueOTTableType:
 * @PANGO_OT_TABLE_GSUB: The GSUB table.
 * @PANGO_OT_TABLE_GPOS: The GPOS table.
 *
 * The <type>VogueOTTableType</type> enumeration values are used to
 * identify the various OpenType tables in the
 * <function>vogue_ot_info_*</function> functions.
 */
typedef enum
{
  PANGO_OT_TABLE_GSUB,
  PANGO_OT_TABLE_GPOS
} VogueOTTableType;

/**
 * PANGO_OT_ALL_GLYPHS:
 *
 * This is used as the property bit in vogue_ot_ruleset_add_feature() when a
 * feature should be applied to all glyphs.
 *
 * Since: 1.16
 */
/**
 * PANGO_OT_NO_FEATURE:
 *
 * This is used as a feature index that represent no feature, that is, should be
 * skipped.  It may be returned as feature index by vogue_ot_info_find_feature()
 * if the feature is not found, and vogue_ot_ruleset_add_feature() function
 * automatically skips this value, so no special handling is required by the user.
 *
 * Since: 1.18
 */
/**
 * PANGO_OT_NO_SCRIPT:
 *
 * This is used as a script index that represent no script, that is, when the
 * requested script was not found, and a default ('DFLT') script was not found
 * either.  It may be returned as script index by vogue_ot_info_find_script()
 * if the script or a default script are not found, all other functions
 * taking a script index essentially return if the input script index is
 * this value, so no special handling is required by the user.
 *
 * Since: 1.18
 */
/**
 * PANGO_OT_DEFAULT_LANGUAGE:
 *
 * This is used as the language index in vogue_ot_info_find_feature() when
 * the default language system of the script is desired.
 *
 * It is also returned by vogue_ot_info_find_language() if the requested language
 * is not found, or the requested language tag was PANGO_OT_TAG_DEFAULT_LANGUAGE.
 * The end result is that one can always call vogue_ot_tag_from_language()
 * followed by vogue_ot_info_find_language() and pass the result to
 * vogue_ot_info_find_feature() without having to worry about falling back to
 * default language system explicitly.
 *
 * Since: 1.16
 */
#define PANGO_OT_ALL_GLYPHS			((guint) 0xFFFF)
#define PANGO_OT_NO_FEATURE			((guint) 0xFFFF)
#define PANGO_OT_NO_SCRIPT			((guint) 0xFFFF)
#define PANGO_OT_DEFAULT_LANGUAGE		((guint) 0xFFFF)

/**
 * PANGO_OT_TAG_DEFAULT_SCRIPT:
 *
 * This is a #VogueOTTag representing the special script tag 'DFLT'.  It is
 * returned as script tag by vogue_ot_tag_from_script() if the requested script
 * is not found.
 *
 * Since: 1.18
 */
/**
 * PANGO_OT_TAG_DEFAULT_LANGUAGE:
 *
 * This is a #VogueOTTag representing a special language tag 'dflt'.  It is
 * returned as language tag by vogue_ot_tag_from_language() if the requested
 * language is not found.  It is safe to pass this value to
 * vogue_ot_info_find_language() as that function falls back to returning default
 * language-system if the requested language tag is not found.
 *
 * Since: 1.18
 */
#define PANGO_OT_TAG_DEFAULT_SCRIPT		PANGO_OT_TAG_MAKE ('D', 'F', 'L', 'T')
#define PANGO_OT_TAG_DEFAULT_LANGUAGE		PANGO_OT_TAG_MAKE ('d', 'f', 'l', 't')

/* Note that this must match hb_glyph_info_t */
/**
 * VogueOTGlyph:
 * @glyph: the glyph itself.
 * @properties: the properties value, identifying which features should be
 * applied on this glyph.  See vogue_ot_ruleset_add_feature().
 * @cluster: the cluster that this glyph belongs to.
 * @component: a component value, set by the OpenType layout engine.
 * @ligID: a ligature index value, set by the OpenType layout engine.
 * @internal: for Vogue internal use
 *
 * The #VogueOTGlyph structure represents a single glyph together with
 * information used for OpenType layout processing of the glyph.
 * It contains the following fields.
 */
struct _VogueOTGlyph
{
  guint32  glyph;
  guint    properties;
  guint    cluster;
  gushort  component;
  gushort  ligID;

  guint    internal;
};

/**
 * VogueOTFeatureMap:
 * @feature_name: feature tag in represented as four-letter ASCII string.
 * @property_bit: the property bit to use for this feature.  See
 * vogue_ot_ruleset_add_feature() for details.
 *
 * The #VogueOTFeatureMap typedef is used to represent an OpenType
 * feature with the property bit associated with it.  The feature tag is
 * represented as a char array instead of a #VogueOTTag for convenience.
 *
 * Since: 1.18
 */
struct _VogueOTFeatureMap
{
  char     feature_name[5];
  gulong   property_bit;
};

/**
 * VogueOTRulesetDescription:
 * @script: a #VogueScript.
 * @language: a #VogueLanguage.
 * @static_gsub_features: (nullable): static map of GSUB features,
 * or %NULL.
 * @n_static_gsub_features: length of @static_gsub_features, or 0.
 * @static_gpos_features: (nullable): static map of GPOS features,
 * or %NULL.
 * @n_static_gpos_features: length of @static_gpos_features, or 0.
 * @other_features: (nullable): map of extra features to add to both
 * GSUB and GPOS, or %NULL.  Unlike the static maps, this pointer
 * need not live beyond the life of function calls taking this
 * struct.
 * @n_other_features: length of @other_features, or 0.
 *
 * The #VogueOTRuleset structure holds all the information needed
 * to build a complete #VogueOTRuleset from an OpenType font.
 * The main use of this struct is to act as the key for a per-font
 * hash of rulesets.  The user populates a ruleset description and
 * gets the ruleset using vogue_ot_ruleset_get_for_description()
 * or create a new one using vogue_ot_ruleset_new_from_description().
 *
 * Since: 1.18
 */
struct _VogueOTRulesetDescription {
  VogueScript               script;
  VogueLanguage            *language;
  const VogueOTFeatureMap  *static_gsub_features;
  guint                   n_static_gsub_features;
  const VogueOTFeatureMap  *static_gpos_features;
  guint                   n_static_gpos_features;
  const VogueOTFeatureMap  *other_features;
  guint                   n_other_features;
};


#define PANGO_TYPE_OT_INFO              (vogue_ot_info_get_type ())
#define PANGO_OT_INFO(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_OT_INFO, VogueOTInfo))
#define PANGO_IS_OT_INFO(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_OT_INFO))
PANGO_DEPRECATED
GType vogue_ot_info_get_type (void) G_GNUC_CONST;

#define PANGO_TYPE_OT_RULESET           (vogue_ot_ruleset_get_type ())
#define PANGO_OT_RULESET(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_OT_RULESET, VogueOTRuleset))
#define PANGO_IS_OT_RULESET(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_OT_RULESET))
PANGO_DEPRECATED
GType vogue_ot_ruleset_get_type (void) G_GNUC_CONST;


PANGO_DEPRECATED
VogueOTInfo *vogue_ot_info_get (FT_Face face);

PANGO_DEPRECATED
gboolean vogue_ot_info_find_script   (VogueOTInfo      *info,
				      VogueOTTableType  table_type,
				      VogueOTTag        script_tag,
				      guint            *script_index);

PANGO_DEPRECATED
gboolean vogue_ot_info_find_language (VogueOTInfo      *info,
				      VogueOTTableType  table_type,
				      guint             script_index,
				      VogueOTTag        language_tag,
				      guint            *language_index,
				      guint            *required_feature_index);
PANGO_DEPRECATED
gboolean vogue_ot_info_find_feature  (VogueOTInfo      *info,
				      VogueOTTableType  table_type,
				      VogueOTTag        feature_tag,
				      guint             script_index,
				      guint             language_index,
				      guint            *feature_index);

PANGO_DEPRECATED
VogueOTTag *vogue_ot_info_list_scripts   (VogueOTInfo      *info,
					  VogueOTTableType  table_type);
PANGO_DEPRECATED
VogueOTTag *vogue_ot_info_list_languages (VogueOTInfo      *info,
					  VogueOTTableType  table_type,
					  guint             script_index,
					  VogueOTTag        language_tag);
PANGO_DEPRECATED
VogueOTTag *vogue_ot_info_list_features  (VogueOTInfo      *info,
					  VogueOTTableType  table_type,
					  VogueOTTag        tag,
					  guint             script_index,
					  guint             language_index);

PANGO_DEPRECATED
VogueOTBuffer *vogue_ot_buffer_new        (VogueFcFont       *font);
PANGO_DEPRECATED
void           vogue_ot_buffer_destroy    (VogueOTBuffer     *buffer);
PANGO_DEPRECATED
void           vogue_ot_buffer_clear      (VogueOTBuffer     *buffer);
PANGO_DEPRECATED
void           vogue_ot_buffer_set_rtl    (VogueOTBuffer     *buffer,
					   gboolean           rtl);
PANGO_DEPRECATED
void           vogue_ot_buffer_add_glyph  (VogueOTBuffer     *buffer,
					   guint              glyph,
					   guint              properties,
					   guint              cluster);
PANGO_DEPRECATED
void           vogue_ot_buffer_get_glyphs (const VogueOTBuffer  *buffer,
					   VogueOTGlyph        **glyphs,
					   int                  *n_glyphs);
PANGO_DEPRECATED
void           vogue_ot_buffer_output     (const VogueOTBuffer  *buffer,
					   VogueGlyphString     *glyphs);

PANGO_DEPRECATED
void           vogue_ot_buffer_set_zero_width_marks (VogueOTBuffer     *buffer,
						     gboolean           zero_width_marks);

PANGO_DEPRECATED
const VogueOTRuleset *vogue_ot_ruleset_get_for_description (VogueOTInfo                     *info,
							    const VogueOTRulesetDescription *desc);
PANGO_DEPRECATED
VogueOTRuleset *vogue_ot_ruleset_new (VogueOTInfo       *info);
PANGO_DEPRECATED
VogueOTRuleset *vogue_ot_ruleset_new_for (VogueOTInfo       *info,
					  VogueScript        script,
					  VogueLanguage     *language);
PANGO_DEPRECATED
VogueOTRuleset *vogue_ot_ruleset_new_from_description (VogueOTInfo                     *info,
						       const VogueOTRulesetDescription *desc);
PANGO_DEPRECATED
void            vogue_ot_ruleset_add_feature (VogueOTRuleset   *ruleset,
					      VogueOTTableType  table_type,
					      guint             feature_index,
					      gulong            property_bit);
PANGO_DEPRECATED
gboolean        vogue_ot_ruleset_maybe_add_feature (VogueOTRuleset   *ruleset,
						    VogueOTTableType  table_type,
						    VogueOTTag        feature_tag,
						    gulong            property_bit);
PANGO_DEPRECATED
guint           vogue_ot_ruleset_maybe_add_features (VogueOTRuleset          *ruleset,
						     VogueOTTableType         table_type,
						     const VogueOTFeatureMap *features,
						     guint                    n_features);
PANGO_DEPRECATED
guint           vogue_ot_ruleset_get_feature_count (const VogueOTRuleset   *ruleset,
						    guint                  *n_gsub_features,
						    guint                  *n_gpos_features);

PANGO_DEPRECATED
void            vogue_ot_ruleset_substitute  (const VogueOTRuleset   *ruleset,
					      VogueOTBuffer          *buffer);

PANGO_DEPRECATED
void            vogue_ot_ruleset_position    (const VogueOTRuleset   *ruleset,
					      VogueOTBuffer          *buffer);

PANGO_DEPRECATED
VogueScript     vogue_ot_tag_to_script     (VogueOTTag     script_tag) G_GNUC_CONST;

PANGO_DEPRECATED
VogueOTTag      vogue_ot_tag_from_script   (VogueScript    script) G_GNUC_CONST;

PANGO_DEPRECATED
VogueLanguage  *vogue_ot_tag_to_language   (VogueOTTag     language_tag) G_GNUC_CONST;

PANGO_DEPRECATED
VogueOTTag      vogue_ot_tag_from_language (VogueLanguage *language) G_GNUC_CONST;

PANGO_DEPRECATED
guint           vogue_ot_ruleset_description_hash  (const VogueOTRulesetDescription *desc) G_GNUC_PURE;

PANGO_DEPRECATED
gboolean        vogue_ot_ruleset_description_equal (const VogueOTRulesetDescription *desc1,
						    const VogueOTRulesetDescription *desc2) G_GNUC_PURE;

PANGO_DEPRECATED
VogueOTRulesetDescription *vogue_ot_ruleset_description_copy  (const VogueOTRulesetDescription *desc);

PANGO_DEPRECATED
void            vogue_ot_ruleset_description_free  (VogueOTRulesetDescription       *desc);


#endif /* PANGO_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __PANGO_OT_H__ */
