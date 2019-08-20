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

#ifndef __PANGO_FC_FONT_MAP_PRIVATE_H__
#define __PANGO_FC_FONT_MAP_PRIVATE_H__

#include <vogue/voguefc-fontmap.h>
#include <vogue/voguefc-decoder.h>
#include <vogue/voguefc-font-private.h>
#include <vogue/vogue-fontmap-private.h>
#include <vogue/vogue-fontset-private.h>

#include <fontconfig/fontconfig.h>

G_BEGIN_DECLS


/**
 * VogueFcFontsetKey:
 *
 * An opaque structure containing all the information needed for
 * loading a fontset with the VogueFc fontmap.
 *
 * Since: 1.24
 **/
typedef struct _VogueFcFontsetKey  VogueFcFontsetKey;

PANGO_AVAILABLE_IN_1_24
VogueLanguage              *vogue_fc_fontset_key_get_language      (const VogueFcFontsetKey *key);
PANGO_AVAILABLE_IN_1_24
const VogueFontDescription *vogue_fc_fontset_key_get_description   (const VogueFcFontsetKey *key);
PANGO_AVAILABLE_IN_1_24
const VogueMatrix          *vogue_fc_fontset_key_get_matrix        (const VogueFcFontsetKey *key);
PANGO_AVAILABLE_IN_1_24
double                      vogue_fc_fontset_key_get_absolute_size (const VogueFcFontsetKey *key);
PANGO_AVAILABLE_IN_1_24
double                      vogue_fc_fontset_key_get_resolution    (const VogueFcFontsetKey *key);
PANGO_AVAILABLE_IN_1_24
gpointer                    vogue_fc_fontset_key_get_context_key   (const VogueFcFontsetKey *key);

/**
 * VogueFcFontKey:
 *
 * An opaque structure containing all the information needed for
 * loading a font with the VogueFc fontmap.
 *
 * Since: 1.24
 **/
typedef struct _VogueFcFontKey     VogueFcFontKey;

PANGO_AVAILABLE_IN_1_24
const FcPattern   *vogue_fc_font_key_get_pattern     (const VogueFcFontKey *key);
PANGO_AVAILABLE_IN_1_24
const VogueMatrix *vogue_fc_font_key_get_matrix      (const VogueFcFontKey *key);
PANGO_AVAILABLE_IN_1_24
gpointer           vogue_fc_font_key_get_context_key (const VogueFcFontKey *key);
PANGO_AVAILABLE_IN_1_40
const char        *vogue_fc_font_key_get_variations  (const VogueFcFontKey *key);


#define PANGO_FC_FONT_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FC_FONT_MAP, VogueFcFontMapClass))
#define PANGO_IS_FC_FONT_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FC_FONT_MAP))
#define PANGO_FC_FONT_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FC_FONT_MAP, VogueFcFontMapClass))

/**
 * VogueFcFontMap:
 *
 * #VogueFcFontMap is a base class for font map implementations
 * using the Fontconfig and FreeType libraries. To create a new
 * backend using Fontconfig and FreeType, you derive from this class
 * and implement a new_font() virtual function that creates an
 * instance deriving from #VogueFcFont.
 **/
struct _VogueFcFontMap
{
  VogueFontMap parent_instance;

  VogueFcFontMapPrivate *priv;
};

/**
 * VogueFcFontMapClass:
 * @default_substitute: (nullable): Substitutes in default
 *  values for unspecified fields in a #FcPattern. This will
 *  be called prior to creating a font for the pattern. May be
 *  %NULL.  Deprecated in favor of @font_key_substitute().
 * @new_font: Creates a new #VogueFcFont for the specified
 *  pattern of the appropriate type for this font map. The
 *  @pattern argument must be passed to the "pattern" property
 *  of #VogueFcFont when you call g_object_new(). Deprecated
 *  in favor of @create_font().
 * @get_resolution: Gets the resolution (the scale factor
 *  between logical and absolute font sizes) that the backend
 *  will use for a particular fontmap and context. @context
 *  may be null.
 * @context_key_get: Gets an opaque key holding backend
 *  specific options for the context that will affect
 *  fonts created by @create_font(). The result must point to
 *  persistant storage owned by the fontmap. This key
 *  is used to index hash tables used to look up fontsets
 *  and fonts.
 * @context_key_copy: Copies a context key. Vogue uses this
 *  to make a persistant copy of the value returned from
 *  @context_key_get.
 * @context_key_free: Frees a context key copied with
 *  @context_key_copy.
 * @context_key_hash: Gets a hash value for a context key
 * @context_key_equal: Compares two context keys for equality.
 * @fontset_key_substitute: (nullable): Substitutes in
 *  default values for unspecified fields in a
 *  #FcPattern. This will be called prior to creating a font
 *  for the pattern. May be %NULL.  (Since: 1.24)
 * @create_font: (nullable): Creates a new #VogueFcFont for
 *  the specified pattern of the appropriate type for this
 *  font map using information from the font key that is
 *  passed in. The @pattern member of @font_key can be
 *  retrieved using vogue_fc_font_key_get_pattern() and must
 *  be passed to the "pattern" property of #VogueFcFont when
 *  you call g_object_new().  If %NULL, new_font() is used.
 *  (Since: 1.24)
 *
 * Class structure for #VogueFcFontMap.
 **/
struct _VogueFcFontMapClass
{
  /*< private >*/
  VogueFontMapClass parent_class;

  /*< public >*/
  /* Deprecated in favor of fontset_key_substitute */
  void         (*default_substitute) (VogueFcFontMap   *fontmap,
				      FcPattern        *pattern);
  /* Deprecated in favor of create_font */
  VogueFcFont  *(*new_font)          (VogueFcFontMap  *fontmap,
				      FcPattern       *pattern);

  double       (*get_resolution)     (VogueFcFontMap             *fcfontmap,
				      VogueContext               *context);

  gconstpointer (*context_key_get)   (VogueFcFontMap             *fcfontmap,
				      VogueContext               *context);
  gpointer     (*context_key_copy)   (VogueFcFontMap             *fcfontmap,
				      gconstpointer               key);
  void         (*context_key_free)   (VogueFcFontMap             *fcfontmap,
				      gpointer                    key);
  guint32      (*context_key_hash)   (VogueFcFontMap             *fcfontmap,
				      gconstpointer               key);
  gboolean     (*context_key_equal)  (VogueFcFontMap             *fcfontmap,
				      gconstpointer               key_a,
				      gconstpointer               key_b);
  void         (*fontset_key_substitute)(VogueFcFontMap             *fontmap,

				      VogueFcFontsetKey          *fontsetkey,
				      FcPattern                  *pattern);
  VogueFcFont  *(*create_font)       (VogueFcFontMap             *fontmap,
				      VogueFcFontKey             *fontkey);
  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved1) (void);
  void (*_vogue_reserved2) (void);
  void (*_vogue_reserved3) (void);
  void (*_vogue_reserved4) (void);
};

#ifndef PANGO_DISABLE_DEPRECATED
PANGO_DEPRECATED_IN_1_22_FOR(vogue_font_map_create_context)
VogueContext * vogue_fc_font_map_create_context (VogueFcFontMap *fcfontmap);
#endif
PANGO_AVAILABLE_IN_1_4
void           vogue_fc_font_map_shutdown       (VogueFcFontMap *fcfontmap);


G_END_DECLS

#endif /* __PANGO_FC_FONT_MAP_PRIVATE_H__ */
