/* Vogue
 * voguefc-private.h: Private routines and declarations for generic
 *  fontconfig operation
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

#ifndef __PANGOFC_PRIVATE_H__
#define __PANGOFC_PRIVATE_H__

#include <voguefc-fontmap-private.h>

G_BEGIN_DECLS


#ifndef FC_WEIGHT_DEMILIGHT
#define FC_WEIGHT_DEMILIGHT 55
#define FC_WEIGHT_SEMILIGHT FC_WEIGHT_DEMILIGHT
#endif


typedef struct _VogueFcMetricsInfo  VogueFcMetricsInfo;

struct _VogueFcMetricsInfo
{
  const char       *sample_str;
  VogueFontMetrics *metrics;
};


#define PANGO_SCALE_26_6 (PANGO_SCALE / (1<<6))
#define PANGO_PIXELS_26_6(d)				\
  (((d) >= 0) ?						\
   ((d) + PANGO_SCALE_26_6 / 2) / PANGO_SCALE_26_6 :	\
   ((d) - PANGO_SCALE_26_6 / 2) / PANGO_SCALE_26_6)
#define PANGO_UNITS_26_6(d)    ((d) * PANGO_SCALE_26_6)
#define PANGO_UNITS_TO_26_6(d) ((d) / PANGO_SCALE_26_6)

void _vogue_fc_font_shutdown (VogueFcFont *fcfont);

void           _vogue_fc_font_map_remove          (VogueFcFontMap *fcfontmap,
						   VogueFcFont    *fcfont);

VogueCoverage *_vogue_fc_font_map_get_coverage    (VogueFcFontMap *fcfontmap,
						   VogueFcFont    *fcfont);
VogueCoverage  *_vogue_fc_font_map_fc_to_coverage (FcCharSet      *charset);

VogueFcDecoder *_vogue_fc_font_get_decoder       (VogueFcFont    *font);
void            _vogue_fc_font_set_decoder       (VogueFcFont    *font,
						  VogueFcDecoder *decoder);

VogueFcFontKey *_vogue_fc_font_get_font_key      (VogueFcFont    *fcfont);
void            _vogue_fc_font_set_font_key      (VogueFcFont    *fcfont,
						  VogueFcFontKey *key);

_PANGO_EXTERN
void            vogue_fc_font_get_raw_extents    (VogueFcFont    *font,
						  VogueGlyph      glyph,
						  VogueRectangle *ink_rect,
						  VogueRectangle *logical_rect);

_PANGO_EXTERN
VogueFontMetrics *vogue_fc_font_create_base_metrics_for_context (VogueFcFont   *font,
								 VogueContext  *context);

G_END_DECLS

#endif /* __PANGOFC_PRIVATE_H__ */
