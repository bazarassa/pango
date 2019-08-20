/* Vogue
 * vogueft2-private.h:
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

#ifndef __PANGOFT2_PRIVATE_H__
#define __PANGOFT2_PRIVATE_H__

#include <vogue/vogueft2.h>
#include <vogue/voguefc-fontmap-private.h>
#include <vogue/vogue-renderer.h>
#include <fontconfig/fontconfig.h>

/* Debugging... */
/*#define DEBUGGING 1*/

#if defined(DEBUGGING) && DEBUGGING
#ifdef __GNUC__
#define PING(printlist)					\
(g_print ("%s:%d ", __PRETTY_FUNCTION__, __LINE__),	\
 g_print printlist,					\
 g_print ("\n"))
#else
#define PING(printlist)					\
(g_print ("%s:%d ", __FILE__, __LINE__),		\
 g_print printlist,					\
 g_print ("\n"))
#endif
#else  /* !DEBUGGING */
#define PING(printlist)
#endif

typedef struct _VogueFT2Font      VogueFT2Font;
typedef struct _VogueFT2GlyphInfo VogueFT2GlyphInfo;
typedef struct _VogueFT2Renderer  VogueFT2Renderer;

struct _VogueFT2Font
{
  VogueFcFont font;

  FT_Face face;
  int load_flags;

  int size;

  GSList *metrics_by_lang;

  GHashTable *glyph_info;
  GDestroyNotify glyph_cache_destroy;
};

struct _VogueFT2GlyphInfo
{
  VogueRectangle logical_rect;
  VogueRectangle ink_rect;
  void *cached_glyph;
};

#define PANGO_TYPE_FT2_FONT              (vogue_ft2_font_get_type ())
#define PANGO_FT2_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FT2_FONT, VogueFT2Font))
#define PANGO_FT2_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_FONT))

_PANGO_EXTERN
GType vogue_ft2_font_get_type (void) G_GNUC_CONST;

VogueFT2Font * _vogue_ft2_font_new                (VogueFT2FontMap   *ft2fontmap,
						   FcPattern         *pattern);
FT_Library     _vogue_ft2_font_map_get_library    (VogueFontMap      *fontmap);
void _vogue_ft2_font_map_default_substitute (VogueFcFontMap *fcfontmap,
					     FcPattern      *pattern);

void *_vogue_ft2_font_get_cache_glyph_data    (VogueFont      *font,
					       int             glyph_index);
void  _vogue_ft2_font_set_cache_glyph_data    (VogueFont      *font,
					       int             glyph_index,
					       void           *cached_glyph);
void  _vogue_ft2_font_set_glyph_cache_destroy (VogueFont      *font,
					       GDestroyNotify  destroy_notify);

#define PANGO_TYPE_FT2_RENDERER            (vogue_ft2_renderer_get_type())
#define PANGO_FT2_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FT2_RENDERER, VogueFT2Renderer))
#define PANGO_IS_FT2_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_RENDERER))

_PANGO_EXTERN
GType vogue_ft2_renderer_get_type    (void) G_GNUC_CONST;

VogueRenderer *_vogue_ft2_font_map_get_renderer (VogueFT2FontMap *ft2fontmap);

#endif /* __PANGOFT2_PRIVATE_H__ */
