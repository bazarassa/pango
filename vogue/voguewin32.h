/* Vogue
 * voguewin32.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
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

#ifndef __PANGOWIN32_H__
#define __PANGOWIN32_H__

#include <glib.h>
#include <vogue/vogue-font.h>
#include <vogue/vogue-layout.h>

G_BEGIN_DECLS

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600	/* To get ClearType-related macros */
#endif
#include <windows.h>
#undef STRICT

/**
 * PANGO_RENDER_TYPE_WIN32:
 *
 * A string constant identifying the Win32 renderer. The associated quark (see
 * g_quark_from_string()) is used to identify the renderer in vogue_find_map().
 */
#define PANGO_RENDER_TYPE_WIN32 "VogueRenderWin32"

/* Calls for applications
 */
#ifndef PANGO_DISABLE_DEPRECATED
PANGO_DEPRECATED_FOR(vogue_font_map_create_context)
VogueContext * vogue_win32_get_context        (void);
#endif

PANGO_AVAILABLE_IN_ALL
void           vogue_win32_render             (HDC               hdc,
					       VogueFont        *font,
					       VogueGlyphString *glyphs,
					       gint              x,
					       gint              y);
PANGO_AVAILABLE_IN_ALL
void           vogue_win32_render_layout_line (HDC               hdc,
					       VogueLayoutLine  *line,
					       int               x,
					       int               y);
PANGO_AVAILABLE_IN_ALL
void           vogue_win32_render_layout      (HDC               hdc,
					       VogueLayout      *layout,
					       int               x,
					       int               y);

PANGO_AVAILABLE_IN_ALL
void           vogue_win32_render_transformed (HDC         hdc,
					       const VogueMatrix *matrix,
					       VogueFont         *font,
					       VogueGlyphString  *glyphs,
					       int                x,
					       int                y);

#ifndef PANGO_DISABLE_DEPRECATED

/* For shape engines
 */

PANGO_DEPRECATED_FOR(PANGO_GET_UNKNOWN_GLYPH)
VogueGlyph     vogue_win32_get_unknown_glyph  (VogueFont        *font,
					       gunichar          wc);
PANGO_DEPRECATED
gint	      vogue_win32_font_get_glyph_index(VogueFont        *font,
					       gunichar          wc);

PANGO_DEPRECATED
HDC            vogue_win32_get_dc             (void);

PANGO_DEPRECATED
gboolean       vogue_win32_get_debug_flag     (void);

PANGO_DEPRECATED
gboolean vogue_win32_font_select_font        (VogueFont *font,
					      HDC        hdc);
PANGO_DEPRECATED
void     vogue_win32_font_done_font          (VogueFont *font);
PANGO_DEPRECATED
double   vogue_win32_font_get_metrics_factor (VogueFont *font);

#endif

/* API for libraries that want to use VogueWin32 mixed with classic
 * Win32 fonts.
 */
typedef struct _VogueWin32FontCache VogueWin32FontCache;

PANGO_AVAILABLE_IN_ALL
VogueWin32FontCache *vogue_win32_font_cache_new          (void);
PANGO_AVAILABLE_IN_ALL
void                 vogue_win32_font_cache_free         (VogueWin32FontCache *cache);

PANGO_AVAILABLE_IN_ALL
HFONT                vogue_win32_font_cache_load         (VogueWin32FontCache *cache,
							  const LOGFONTA      *logfont);
PANGO_AVAILABLE_IN_1_16
HFONT                vogue_win32_font_cache_loadw        (VogueWin32FontCache *cache,
							  const LOGFONTW      *logfont);
PANGO_AVAILABLE_IN_ALL
void                 vogue_win32_font_cache_unload       (VogueWin32FontCache *cache,
							  HFONT                hfont);

PANGO_AVAILABLE_IN_ALL
VogueFontMap        *vogue_win32_font_map_for_display    (void);
PANGO_AVAILABLE_IN_ALL
void                 vogue_win32_shutdown_display        (void);
PANGO_AVAILABLE_IN_ALL
VogueWin32FontCache *vogue_win32_font_map_get_font_cache (VogueFontMap       *font_map);

PANGO_AVAILABLE_IN_ALL
LOGFONTA            *vogue_win32_font_logfont            (VogueFont          *font);
PANGO_AVAILABLE_IN_1_16
LOGFONTW            *vogue_win32_font_logfontw           (VogueFont          *font);

PANGO_AVAILABLE_IN_1_12
VogueFontDescription *vogue_win32_font_description_from_logfont (const LOGFONTA *lfp);

PANGO_AVAILABLE_IN_1_16
VogueFontDescription *vogue_win32_font_description_from_logfontw (const LOGFONTW *lfp);

G_END_DECLS

#endif /* __PANGOWIN32_H__ */
