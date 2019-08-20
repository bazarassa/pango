/* Vogue
 * vogue-ot-tag.h:
 *
 * Copyright (C) 2007 Red Hat Software
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

/**
 * vogue_ot_tag_from_script:
 * @script: A #VogueScript
 *
 * Finds the OpenType script tag corresponding to @script.
 *
 * The %PANGO_SCRIPT_COMMON, %PANGO_SCRIPT_INHERITED, and
 * %PANGO_SCRIPT_UNKNOWN scripts are mapped to the OpenType
 * 'DFLT' script tag that is also defined as
 * %PANGO_OT_TAG_DEFAULT_SCRIPT.
 *
 * Note that multiple #VogueScript values may map to the same
 * OpenType script tag.  In particular, %PANGO_SCRIPT_HIRAGANA
 * and %PANGO_SCRIPT_KATAKANA both map to the OT tag 'kana'.
 *
 * Return value: #VogueOTTag corresponding to @script or
 * %PANGO_OT_TAG_DEFAULT_SCRIPT if none found.
 *
 * Since: 1.18
 **/
VogueOTTag
vogue_ot_tag_from_script (VogueScript script)
{
  unsigned int count = 1;
  hb_tag_t tags[1];

  hb_ot_tags_from_script_and_language (hb_glib_script_to_script ((GUnicodeScript)script),
                                       HB_LANGUAGE_INVALID,
                                       &count,
                                       tags,
                                       NULL, NULL);
  if (count > 0)
    return (VogueOTTag) tags[0];

  return PANGO_OT_TAG_DEFAULT_SCRIPT;
}

/**
 * vogue_ot_tag_to_script:
 * @script_tag: A #VogueOTTag OpenType script tag
 *
 * Finds the #VogueScript corresponding to @script_tag.
 *
 * The 'DFLT' script tag is mapped to %PANGO_SCRIPT_COMMON.
 *
 * Note that an OpenType script tag may correspond to multiple
 * #VogueScript values.  In such cases, the #VogueScript value
 * with the smallest value is returned.
 * In particular, %PANGO_SCRIPT_HIRAGANA
 * and %PANGO_SCRIPT_KATAKANA both map to the OT tag 'kana'.
 * This function will return %PANGO_SCRIPT_HIRAGANA for
 * 'kana'.
 *
 * Return value: #VogueScript corresponding to @script_tag or
 * %PANGO_SCRIPT_UNKNOWN if none found.
 *
 * Since: 1.18
 **/
VogueScript
vogue_ot_tag_to_script (VogueOTTag script_tag)
{
  return (VogueScript) hb_glib_script_from_script (hb_ot_tag_to_script ((hb_tag_t) script_tag));
}


/**
 * vogue_ot_tag_from_language:
 * @language: (nullable): A #VogueLanguage, or %NULL
 *
 * Finds the OpenType language-system tag best describing @language.
 *
 * Return value: #VogueOTTag best matching @language or
 * %PANGO_OT_TAG_DEFAULT_LANGUAGE if none found or if @language
 * is %NULL.
 *
 * Since: 1.18
 **/
VogueOTTag
vogue_ot_tag_from_language (VogueLanguage *language)
{
  unsigned int count = 1;
  hb_tag_t tags[1];

  hb_ot_tags_from_script_and_language (HB_SCRIPT_UNKNOWN,
                                       hb_language_from_string (vogue_language_to_string (language), -1),
                                       NULL, NULL,
                                       &count, tags);

  if (count > 0)
    return (VogueOTTag) tags[0];

  return PANGO_OT_TAG_DEFAULT_LANGUAGE;
}

/**
 * vogue_ot_tag_to_language:
 * @language_tag: A #VogueOTTag OpenType language-system tag
 *
 * Finds a #VogueLanguage corresponding to @language_tag.
 *
 * Return value: #VogueLanguage best matching @language_tag or
 * #VogueLanguage corresponding to the string "xx" if none found.
 *
 * Since: 1.18
 **/
VogueLanguage *
vogue_ot_tag_to_language (VogueOTTag language_tag)
{
  return vogue_language_from_string (hb_language_to_string (hb_ot_tag_to_language ((hb_tag_t) language_tag)));
}
