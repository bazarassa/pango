/* Vogue
 * vogueft2.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#ifndef __PANGOFT2_H__
#define __PANGOFT2_H__

#include <fontconfig/fontconfig.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <vogue/vogue-layout.h>
#include <vogue/voguefc-font.h>

G_BEGIN_DECLS

#ifndef PANGO_DISABLE_DEPRECATED
/**
 * PANGO_RENDER_TYPE_FT2:
 *
 * A string constant that was used to identify shape engines that work
 * with the FreeType backend. See %PANGO_RENDER_TYPE_FC for the replacement.
 */
#define PANGO_RENDER_TYPE_FT2 "VogueRenderFT2"
#endif

#define PANGO_TYPE_FT2_FONT_MAP              (vogue_ft2_font_map_get_type ())
#define PANGO_FT2_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FT2_FONT_MAP, VogueFT2FontMap))
#define PANGO_FT2_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_FONT_MAP))

typedef struct _VogueFT2FontMap      VogueFT2FontMap;

/**
 * VogueFT2SubstituteFunc:
 * @pattern: the <type>FcPattern</type> to tweak.
 * @data: user data.
 *
 * Function type for doing final config tweaking on prepared FcPatterns.
 */
typedef void (*VogueFT2SubstituteFunc) (FcPattern *pattern,
					gpointer   data);

/* Calls for applications */

PANGO_AVAILABLE_IN_ALL
void vogue_ft2_render             (FT_Bitmap         *bitmap,
				   VogueFont         *font,
				   VogueGlyphString  *glyphs,
				   gint               x,
				   gint               y);
PANGO_AVAILABLE_IN_1_6
void vogue_ft2_render_transformed (FT_Bitmap         *bitmap,
				   const VogueMatrix *matrix,
				   VogueFont         *font,
				   VogueGlyphString  *glyphs,
				   int                x,
				   int                y);

PANGO_AVAILABLE_IN_ALL
void vogue_ft2_render_layout_line          (FT_Bitmap        *bitmap,
					    VogueLayoutLine  *line,
					    int               x,
					    int               y);
PANGO_AVAILABLE_IN_1_6
void vogue_ft2_render_layout_line_subpixel (FT_Bitmap        *bitmap,
					    VogueLayoutLine  *line,
					    int               x,
					    int               y);
PANGO_AVAILABLE_IN_ALL
void vogue_ft2_render_layout               (FT_Bitmap        *bitmap,
					    VogueLayout      *layout,
					    int               x,
					    int               y);
PANGO_AVAILABLE_IN_1_6
void vogue_ft2_render_layout_subpixel      (FT_Bitmap        *bitmap,
					    VogueLayout      *layout,
					    int               x,
					    int               y);

PANGO_AVAILABLE_IN_ALL
GType vogue_ft2_font_map_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_2
VogueFontMap *vogue_ft2_font_map_new                    (void);
PANGO_AVAILABLE_IN_1_2
void          vogue_ft2_font_map_set_resolution         (VogueFT2FontMap        *fontmap,
							 double                  dpi_x,
							 double                  dpi_y);
PANGO_AVAILABLE_IN_1_2
void          vogue_ft2_font_map_set_default_substitute (VogueFT2FontMap        *fontmap,
							 VogueFT2SubstituteFunc  func,
							 gpointer                data,
							 GDestroyNotify          notify);
PANGO_AVAILABLE_IN_1_2
void          vogue_ft2_font_map_substitute_changed     (VogueFT2FontMap         *fontmap);
#ifndef PANGO_DISABLE_DEPRECATED
PANGO_DEPRECATED_IN_1_22_FOR(vogue_font_map_create_context)
VogueContext *vogue_ft2_font_map_create_context         (VogueFT2FontMap         *fontmap);
#endif


/* API for rendering modules
 */
#ifndef PANGO_DISABLE_DEPRECATED
PANGO_DEPRECATED_FOR(vogue_font_map_create_context)
VogueContext      *vogue_ft2_get_context          (double dpi_x,
						   double dpi_y);
PANGO_DEPRECATED_FOR(vogue_ft2_font_map_new)
VogueFontMap      *vogue_ft2_font_map_for_display (void);
PANGO_DEPRECATED
void               vogue_ft2_shutdown_display     (void);

PANGO_DEPRECATED_FOR(PANGO_GET_UNKNOWN_GLYPH)
VogueGlyph     vogue_ft2_get_unknown_glyph (VogueFont       *font);
PANGO_DEPRECATED_FOR(vogue_fc_font_kern_glyphs)
int            vogue_ft2_font_get_kerning  (VogueFont       *font,
					    VogueGlyph       left,
					    VogueGlyph       right);
PANGO_DEPRECATED_FOR(vogue_fc_font_lock_face)
FT_Face        vogue_ft2_font_get_face     (VogueFont       *font);
PANGO_DEPRECATED_FOR(vogue_font_get_coverage)
VogueCoverage *vogue_ft2_font_get_coverage (VogueFont       *font,
					    VogueLanguage   *language);
#endif /* PANGO_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __PANGOFT2_H__ */
