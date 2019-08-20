/* Vogue
 * voguefc-decoder.h: Custom encoders/decoders on a per-font basis.
 *
 * Copyright (C) 2004 Red Hat Software
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

#ifndef __PANGO_DECODER_H_
#define __PANGO_DECODER_H_

#include <vogue/voguefc-font.h>

G_BEGIN_DECLS

#define PANGO_TYPE_FC_DECODER       (vogue_fc_decoder_get_type())
#define PANGO_FC_DECODER(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FC_DECODER, VogueFcDecoder))
#define PANGO_IS_FC_DECODER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FC_DECODER))

typedef struct _VogueFcDecoder      VogueFcDecoder;
typedef struct _VogueFcDecoderClass VogueFcDecoderClass;

#define PANGO_FC_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FC_DECODER, VogueFcDecoderClass))
#define PANGO_IS_FC_DECODER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FC_DECODER))
#define PANGO_FC_DECODER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FC_DECODER, VogueFcDecoderClass))

/**
 * VogueFcDecoder:
 *
 * #VogueFcDecoder is a virtual base class that implementations will
 * inherit from.  It's the interface that is used to define a custom
 * encoding for a font.  These objects are created in your code from a
 * function callback that was originally registered with
 * vogue_fc_font_map_add_decoder_find_func().  Vogue requires
 * information about the supported charset for a font as well as the
 * individual character to glyph conversions.  Vogue gets that
 * information via the #get_charset and #get_glyph callbacks into your
 * object implementation.
 *
 * Since: 1.6
 **/
struct _VogueFcDecoder
{
  /*< private >*/
  GObject parent_instance;
};

/**
 * VogueFcDecoderClass:
 * @get_charset: This returns an #FcCharset given a #VogueFcFont that
 *  includes a list of supported characters in the font.  The
 *  #FcCharSet that is returned should be an internal reference to your
 *  code.  Vogue will not free this structure.  It is important that
 *  you make this callback fast because this callback is called
 *  separately for each character to determine Unicode coverage.
 * @get_glyph: This returns a single #VogueGlyph for a given Unicode
 *  code point.
 *
 * Class structure for #VogueFcDecoder.
 *
 * Since: 1.6
 **/
struct _VogueFcDecoderClass
{
  /*< private >*/
  GObjectClass parent_class;

  /* vtable - not signals */
  /*< public >*/
  FcCharSet  *(*get_charset) (VogueFcDecoder *decoder,
			      VogueFcFont    *fcfont);
  VogueGlyph  (*get_glyph)   (VogueFcDecoder *decoder,
			      VogueFcFont    *fcfont,
			      guint32         wc);

  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved1) (void);
  void (*_vogue_reserved2) (void);
  void (*_vogue_reserved3) (void);
  void (*_vogue_reserved4) (void);
};

PANGO_AVAILABLE_IN_1_6
GType      vogue_fc_decoder_get_type    (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_6
FcCharSet *vogue_fc_decoder_get_charset (VogueFcDecoder *decoder,
					 VogueFcFont    *fcfont);

PANGO_AVAILABLE_IN_1_6
VogueGlyph vogue_fc_decoder_get_glyph   (VogueFcDecoder *decoder,
					 VogueFcFont    *fcfont,
					 guint32         wc);

G_END_DECLS

#endif /* __PANGO_DECODER_H_ */

