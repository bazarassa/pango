/* Vogue
 * voguefc-fontmap.h: Base fontmap type for fontconfig-based backends
 *
 * Copyright (C) 2003 Red Hat Software
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

#ifndef __PANGO_FC_FONT_MAP_H__
#define __PANGO_FC_FONT_MAP_H__

#include <vogue/vogue.h>
#include <fontconfig/fontconfig.h>
#include <vogue/voguefc-decoder.h>
#include <vogue/voguefc-font.h>
#include <hb.h>

G_BEGIN_DECLS


/*
 * VogueFcFontMap
 */

#define PANGO_TYPE_FC_FONT_MAP              (vogue_fc_font_map_get_type ())
#define PANGO_FC_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FC_FONT_MAP, VogueFcFontMap))
#define PANGO_IS_FC_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FC_FONT_MAP))

typedef struct _VogueFcFontMap        VogueFcFontMap;
typedef struct _VogueFcFontMapClass   VogueFcFontMapClass;
typedef struct _VogueFcFontMapPrivate VogueFcFontMapPrivate;

PANGO_AVAILABLE_IN_ALL
GType vogue_fc_font_map_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_4
void           vogue_fc_font_map_cache_clear    (VogueFcFontMap *fcfontmap);

PANGO_AVAILABLE_IN_1_38
void
vogue_fc_font_map_config_changed (VogueFcFontMap *fcfontmap);

PANGO_AVAILABLE_IN_1_38
void
vogue_fc_font_map_set_config (VogueFcFontMap *fcfontmap,
			      FcConfig       *fcconfig);
PANGO_AVAILABLE_IN_1_38
FcConfig *
vogue_fc_font_map_get_config (VogueFcFontMap *fcfontmap);

/**
 * VogueFcDecoderFindFunc:
 * @pattern: a fully resolved #FcPattern specifying the font on the system
 * @user_data: user data passed to vogue_fc_font_map_add_decoder_find_func()
 *
 * Callback function passed to vogue_fc_font_map_add_decoder_find_func().
 *
 * Return value: a new reference to a custom decoder for this pattern,
 *  or %NULL if the default decoder handling should be used.
 **/
typedef VogueFcDecoder * (*VogueFcDecoderFindFunc) (FcPattern *pattern,
						    gpointer   user_data);

PANGO_AVAILABLE_IN_1_6
void vogue_fc_font_map_add_decoder_find_func (VogueFcFontMap        *fcfontmap,
					      VogueFcDecoderFindFunc findfunc,
					      gpointer               user_data,
					      GDestroyNotify         dnotify);
PANGO_AVAILABLE_IN_1_26
VogueFcDecoder *vogue_fc_font_map_find_decoder (VogueFcFontMap *fcfontmap,
					        FcPattern      *pattern);

PANGO_AVAILABLE_IN_1_4
VogueFontDescription *vogue_fc_font_description_from_pattern (FcPattern *pattern,
							      gboolean   include_size);

PANGO_AVAILABLE_IN_1_44
hb_face_t * vogue_fc_font_map_get_hb_face (VogueFcFontMap *fcfontmap,
                                           VogueFcFont    *fcfont);

/**
 * PANGO_FC_GRAVITY:
 *
 * String representing a fontconfig property name that Vogue sets on any
 * fontconfig pattern it passes to fontconfig if a #VogueGravity other
 * than %PANGO_GRAVITY_SOUTH is desired.
 *
 * The property will have a #VogueGravity value as a string, like "east".
 * This can be used to write fontconfig configuration rules to choose
 * different fonts for horizontal and vertical writing directions.
 *
 * Since: 1.20
 */
#define PANGO_FC_GRAVITY "voguegravity"

/**
 * PANGO_FC_VERSION:
 *
 * String representing a fontconfig property name that Vogue sets on any
 * fontconfig pattern it passes to fontconfig.
 *
 * The property will have an integer value equal to what
 * vogue_version() returns.
 * This can be used to write fontconfig configuration rules that only affect
 * certain vogue versions (or only vogue-using applications, or only
 * non-vogue-using applications).
 *
 * Since: 1.20
 */
#define PANGO_FC_VERSION "vogueversion"

/**
 * PANGO_FC_PRGNAME:
 *
 * String representing a fontconfig property name that Vogue sets on any
 * fontconfig pattern it passes to fontconfig.
 *
 * The property will have a string equal to what
 * g_get_prgname() returns.
 * This can be used to write fontconfig configuration rules that only affect
 * certain applications.
 *
 * This is equivalent to FC_PRGNAME in versions of fontconfig that have that.
 *
 * Since: 1.24
 */
#define PANGO_FC_PRGNAME "prgname"

/**
 * PANGO_FC_FONT_FEATURES:
 *
 * String representing a fontconfig property name that Vogue reads from font
 * patterns to populate list of OpenType features to be enabled for the font
 * by default.
 *
 * The property will have a number of string elements, each of which is the
 * OpenType feature tag of one feature to enable.
 *
 * This is equivalent to FC_FONT_FEATURES in versions of fontconfig that have that.
 *
 * Since: 1.34
 */
#define PANGO_FC_FONT_FEATURES "fontfeatures"

/**
 * PANGO_FC_FONT_VARIATIONS:
 *
 * String representing a fontconfig property name that Vogue reads from font
 * patterns to populate list of OpenType font variations to be used for a font.
 *
 * The property will have a string elements, each of which a comma-separated
 * list of OpenType axis setting of the form AXIS=VALUE.
 */
#define PANGO_FC_FONT_VARIATIONS "fontvariations"

G_END_DECLS

#endif /* __PANGO_FC_FONT_MAP_H__ */
