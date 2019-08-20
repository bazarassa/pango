/* Vogue
 * voguexft.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 SuSE Linux Ltd
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

#ifndef __PANGOXFT_H__
#define __PANGOXFT_H__

#include <vogue/vogue-context.h>
#include <vogue/vogue-ot.h>
#include <vogue/voguefc-font.h>
#include <vogue/vogue-layout.h>
#include <vogue/voguexft-render.h>

G_BEGIN_DECLS

/**
 * PANGO_RENDER_TYPE_XFT:
 *
 * A string constant that was used to identify shape engines that work
 * with the Xft backend. See %PANGO_RENDER_TYPE_FC for the replacement.
 */
#ifndef PANGO_DISABLE_DEPRECATED
#define PANGO_RENDER_TYPE_XFT "VogueRenderXft"
#endif

/**
 * VogueXftFontMap:
 *
 * #VogueXftFontMap is an implementation of #VogueFcFontMap suitable for
 * the Xft library as the renderer.  It is used in to create fonts of
 * type #VogueXftFont.
 */
#define PANGO_TYPE_XFT_FONT_MAP              (vogue_xft_font_map_get_type ())
#define PANGO_XFT_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT_MAP, VogueXftFontMap))
#define PANGO_XFT_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT_MAP))

typedef struct _VogueXftFontMap      VogueXftFontMap;

/**
 * VogueXftFont:
 *
 * #VogueXftFont is an implementation of #VogueFcFont using the Xft
 * library for rendering.  It is used in conjunction with #VogueXftFontMap.
 */
typedef struct _VogueXftFont    VogueXftFont;

/**
 * VogueXftSubstituteFunc:
 * @pattern: the FcPattern to tweak.
 * @data: user data.
 *
 * Function type for doing final config tweaking on prepared FcPatterns.
 */
typedef void (*VogueXftSubstituteFunc) (FcPattern *pattern,
					gpointer   data);

/* Calls for applications
 */
PANGO_AVAILABLE_IN_1_2
VogueFontMap *vogue_xft_get_font_map     (Display *display,
					  int      screen);
#ifndef PANGO_DISABLE_DEPRECATED
PANGO_DEPRECATED
VogueContext *vogue_xft_get_context      (Display *display,
					  int      screen);
#endif
PANGO_AVAILABLE_IN_1_2
void          vogue_xft_shutdown_display (Display *display,
					  int      screen);

PANGO_AVAILABLE_IN_1_2
void vogue_xft_set_default_substitute (Display                *display,
				       int                     screen,
				       VogueXftSubstituteFunc  func,
				       gpointer                data,
				       GDestroyNotify          notify);
PANGO_AVAILABLE_IN_1_2
void vogue_xft_substitute_changed     (Display                *display,
				       int                     screen);

PANGO_AVAILABLE_IN_ALL
GType vogue_xft_font_map_get_type (void) G_GNUC_CONST;

#define PANGO_XFT_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT, VogueXftFont))
#define PANGO_TYPE_XFT_FONT              (vogue_xft_font_get_type ())
#define PANGO_XFT_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT))

PANGO_AVAILABLE_IN_ALL
GType      vogue_xft_font_get_type (void) G_GNUC_CONST;

/* For shape engines
 */

#ifndef PANGO_DISABLE_DEPRECATED

PANGO_DEPRECATED
XftFont *     vogue_xft_font_get_font          (VogueFont *font);
PANGO_DEPRECATED
Display *     vogue_xft_font_get_display       (VogueFont *font);
PANGO_DEPRECATED_FOR(vogue_fc_font_lock_face)
FT_Face       vogue_xft_font_lock_face         (VogueFont *font);
PANGO_DEPRECATED_FOR(vogue_fc_font_unlock_face)
void	      vogue_xft_font_unlock_face       (VogueFont *font);
PANGO_DEPRECATED_FOR(vogue_fc_font_get_glyph)
guint	      vogue_xft_font_get_glyph	       (VogueFont *font,
						gunichar   wc);
PANGO_DEPRECATED_FOR(vogue_fc_font_has_char)
gboolean      vogue_xft_font_has_char          (VogueFont *font,
						gunichar   wc);
PANGO_DEPRECATED_FOR(PANGO_GET_UNKNOWN_GLYPH)
VogueGlyph    vogue_xft_font_get_unknown_glyph (VogueFont *font,
						gunichar   wc);
#endif /* PANGO_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __PANGOXFT_H__ */
