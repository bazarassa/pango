/* Vogue
 * voguefc-decoder.c: Custom font encoder/decoders
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

/**
 * SECTION:voguefc-decoder
 * @short_description:Custom font encoding handling
 * @title:VogueFcDecoder
 *
 * VogueFcDecoder represents a decoder that an application provides
 * for handling a font that is encoded in a custom way.
 */
#include "config.h"
#include "voguefc-decoder.h"

G_DEFINE_ABSTRACT_TYPE (VogueFcDecoder, vogue_fc_decoder, G_TYPE_OBJECT)

static void
vogue_fc_decoder_init (VogueFcDecoder *decoder G_GNUC_UNUSED)
{
}

static void
vogue_fc_decoder_class_init (VogueFcDecoderClass *klass G_GNUC_UNUSED)
{
}

/**
 * vogue_fc_decoder_get_charset:
 * @decoder: a #VogueFcDecoder
 * @fcfont: the #VogueFcFont to query.
 *
 * Generates an #FcCharSet of supported characters for the fcfont
 * given.  The returned #FcCharSet will be a reference to an
 * internal value stored by the #VogueFcDecoder and must not
 * be modified or freed.
 *
 * Returns: (transfer none): the #FcCharset for @fcfont; must not
 *   be modified or freed.
 *
 * Since: 1.6
 **/
FcCharSet *
vogue_fc_decoder_get_charset (VogueFcDecoder *decoder,
			      VogueFcFont    *fcfont)
{
  g_return_val_if_fail (PANGO_IS_FC_DECODER (decoder), NULL);

  return PANGO_FC_DECODER_GET_CLASS (decoder)->get_charset (decoder, fcfont);
}

/**
 * vogue_fc_decoder_get_glyph:
 * @decoder: a #VogueFcDecoder
 * @fcfont: a #VogueFcFont to query.
 * @wc: the Unicode code point to convert to a single #VogueGlyph.
 *
 * Generates a #VogueGlyph for the given Unicode point using the
 * custom decoder. For complex scripts where there can be multiple
 * glyphs for a single character, the decoder will return whatever
 * glyph is most convenient for it. (Usually whatever glyph is directly
 * in the fonts character map table.)
 *
 * Return value: the glyph index, or 0 if the glyph isn't
 * covered by the font.
 *
 * Since: 1.6
 **/
VogueGlyph
vogue_fc_decoder_get_glyph (VogueFcDecoder *decoder,
			    VogueFcFont    *fcfont,
			    guint32         wc)
{
  g_return_val_if_fail (PANGO_IS_FC_DECODER (decoder), 0);

  return PANGO_FC_DECODER_GET_CLASS (decoder)->get_glyph (decoder, fcfont, wc);
}
