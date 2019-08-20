/* Vogue
 * voguefc-font.h: Base fontmap type for fontconfig-based backends
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

#ifndef __PANGO_FC_FONT_PRIVATE_H__
#define __PANGO_FC_FONT_PRIVATE_H__

#include <vogue/voguefc-font.h>
#include <vogue/vogue-font-private.h>

G_BEGIN_DECLS

/**
 * PANGO_RENDER_TYPE_FC:
 *
 * A string constant used to identify shape engines that work
 * with the fontconfig based backends. See the @engine_type field
 * of #VogueEngineInfo.
 **/
#define PANGO_RENDER_TYPE_FC "VogueRenderFc"

#define PANGO_FC_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FC_FONT, VogueFcFontClass))
#define PANGO_IS_FC_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FC_FONT))
#define PANGO_FC_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FC_FONT, VogueFcFontClass))

/**
 * VogueFcFontClass:
 * @lock_face: Returns the FT_Face of the font and increases
 *  the reference count for the face by one.
 * @unlock_face: Decreases the reference count for the
 *  FT_Face of the font by one. When the count is zero,
 *  the #VogueFcFont subclass is allowed to free the
 *  FT_Face.
 * @has_char: Return %TRUE if the the font contains a glyph
 *   corresponding to the specified character.
 * @get_glyph: Gets the glyph that corresponds to the given
 *   Unicode character.
 * @get_unknown_glyph: (nullable): Gets the glyph that
 *   should be used to display an unknown-glyph indication
 *   for the specified Unicode character.  May be %NULL.
 * @shutdown: (nullable): Performs any font-specific
 *   shutdown code that needs to be done when
 *   vogue_fc_font_map_shutdown is called.  May be %NULL.
 *
 * Class structure for #VogueFcFont.
 **/
struct _VogueFcFontClass
{
  /*< private >*/
  VogueFontClass parent_class;

  /*< public >*/
  FT_Face    (*lock_face)         (VogueFcFont      *font);
  void       (*unlock_face)       (VogueFcFont      *font);
  gboolean   (*has_char)          (VogueFcFont      *font,
				   gunichar          wc);
  guint      (*get_glyph)         (VogueFcFont      *font,
				   gunichar          wc);
  VogueGlyph (*get_unknown_glyph) (VogueFcFont      *font,
				   gunichar          wc);
  void       (*shutdown)          (VogueFcFont      *font);
  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved1) (void);
  void (*_vogue_reserved2) (void);
  void (*_vogue_reserved3) (void);
  void (*_vogue_reserved4) (void);
};


G_END_DECLS
#endif /* __PANGO_FC_FONT_PRIVATE_H__ */
