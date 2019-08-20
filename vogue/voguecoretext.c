/* Vogue
 * voguecoretext.c
 *
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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
 * SECTION:coretext-fonts
 * @short_description:Font handling and rendering on OS X
 * @title:CoreText Fonts and Rendering
 *
 * The macros and functions in this section are used to access fonts natively on
 * OS X using the CoreText text rendering subsystem.
 */
#include "config.h"

#include "voguecoretext.h"
#include "voguecoretext-private.h"
#include <hb-coretext.h>

struct _VogueCoreTextFontPrivate
{
  VogueCoreTextFace *face;
  gpointer context_key;

  CTFontRef font_ref;
  VogueCoreTextFontKey *key;

  VogueCoverage *coverage;

  VogueFontMap *fontmap;
};

G_DEFINE_TYPE_WITH_PRIVATE (VogueCoreTextFont, vogue_core_text_font, PANGO_TYPE_FONT)

static void
vogue_core_text_font_finalize (GObject *object)
{
  VogueCoreTextFont *ctfont = (VogueCoreTextFont *)object;
  VogueCoreTextFontPrivate *priv = ctfont->priv;
  VogueCoreTextFontMap* fontmap = g_weak_ref_get ((GWeakRef *)&priv->fontmap);
  if (fontmap)
    {
      g_weak_ref_clear ((GWeakRef *)&priv->fontmap);
      g_object_unref (fontmap);
    }

  if (priv->coverage)
    vogue_coverage_unref (priv->coverage);

  G_OBJECT_CLASS (vogue_core_text_font_parent_class)->finalize (object);
}

static VogueFontDescription *
vogue_core_text_font_describe (VogueFont *font)
{
  VogueCoreTextFont *ctfont = (VogueCoreTextFont *)font;
  VogueCoreTextFontPrivate *priv = ctfont->priv;
  CTFontDescriptorRef ctfontdesc;

  ctfontdesc = vogue_core_text_font_key_get_ctfontdescriptor (priv->key);

  return _vogue_core_text_font_description_from_ct_font_descriptor (ctfontdesc);
}

static VogueCoverage *
ct_font_descriptor_get_coverage (CTFontDescriptorRef desc)
{
  CFCharacterSetRef charset;
  CFIndex i, length;
  CFDataRef bitmap;
  const UInt8 *ptr, *plane_ptr;
  const UInt32 plane_size = 8192;
  VogueCoverage *coverage;

  coverage = vogue_coverage_new ();

  charset = CTFontDescriptorCopyAttribute (desc, kCTFontCharacterSetAttribute);
  if (!charset)
    /* Return an empty coverage */
    return coverage;

  bitmap = CFCharacterSetCreateBitmapRepresentation (kCFAllocatorDefault,
                                                     charset);
  ptr = CFDataGetBytePtr (bitmap);

  /* First handle the BMP plane. */
  length = MIN (CFDataGetLength (bitmap), plane_size);

  /* FIXME: can and should this be done more efficiently? */
  for (i = 0; i < length; i++)
    {
      int j;

      for (j = 0; j < 8; j++)
        if ((ptr[i] & (1 << j)) == (1 << j))
          vogue_coverage_set (coverage, i * 8 + j, PANGO_COVERAGE_EXACT);
    }

  /* Next, handle the other planes. The plane number is encoded first as
   * a single byte. In the following 8192 bytes that plane's coverage bitmap
   * is stored.
   */
  plane_ptr = ptr + plane_size;
  while (plane_ptr - ptr < CFDataGetLength (bitmap))
    {
      const UInt8 plane_number = *plane_ptr;
      plane_ptr++;

      for (i = 0; i < plane_size; i++)
        {
          int j;

          for (j = 0; j < 8; j++)
            if ((plane_ptr[i] & (1 << j)) == (1 << j))
              vogue_coverage_set (coverage, (plane_number * plane_size + i) * 8 + j,
                                  PANGO_COVERAGE_EXACT);
        }

      plane_ptr += plane_size;
    }

  CFRelease (bitmap);
  CFRelease (charset);

  return coverage;
}

static VogueCoverage *
vogue_core_text_font_get_coverage (VogueFont     *font,
                                   VogueLanguage *language G_GNUC_UNUSED)
{
  VogueCoreTextFont *ctfont = (VogueCoreTextFont *)font;
  VogueCoreTextFontPrivate *priv = ctfont->priv;

  if (!priv->coverage)
    {
      CTFontDescriptorRef ctfontdesc;

      ctfontdesc = vogue_core_text_font_key_get_ctfontdescriptor (priv->key);

      priv->coverage = ct_font_descriptor_get_coverage (ctfontdesc);
    }

  return vogue_coverage_ref (priv->coverage);
}

static VogueFontMap *
vogue_core_text_font_get_font_map (VogueFont *font)
{
  VogueCoreTextFont *ctfont = (VogueCoreTextFont *)font;
  /* FIXME: Not thread safe! */
  return ctfont->priv->fontmap;
}

static hb_font_t *
vogue_core_text_font_create_hb_font (VogueFont *font)
{
  VogueCoreTextFont *ctfont = (VogueCoreTextFont *)font;

  if (ctfont->priv->font_ref)
    {
      hb_font_t *hb_font;
      int size;

      size = vogue_core_text_font_key_get_size (ctfont->priv->key);
      hb_font = hb_coretext_font_create (ctfont->priv->font_ref);
      hb_font_set_scale (hb_font, size, size);

      return hb_font;
    }

  return hb_font_get_empty ();
}

static void
vogue_core_text_font_init (VogueCoreTextFont *ctfont)
{
  ctfont->priv = vogue_core_text_font_get_instance_private (ctfont);
}

static void
vogue_core_text_font_class_init (VogueCoreTextFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = vogue_core_text_font_finalize;

  font_class->describe = vogue_core_text_font_describe;
  /* font_class->describe_absolute is left virtual for VogueCairoCoreTextFont. */
  font_class->get_coverage = vogue_core_text_font_get_coverage;
  font_class->get_font_map = vogue_core_text_font_get_font_map;
  font_class->create_hb_font = vogue_core_text_font_create_hb_font;
}

void
_vogue_core_text_font_set_font_map (VogueCoreTextFont    *font,
                                    VogueCoreTextFontMap *fontmap)
{
  VogueCoreTextFontPrivate *priv = font->priv;

  g_return_if_fail (priv->fontmap == NULL);
  g_weak_ref_set((GWeakRef *) &priv->fontmap, fontmap);
}

void
_vogue_core_text_font_set_face (VogueCoreTextFont *ctfont,
                                VogueCoreTextFace *ctface)
{
  VogueCoreTextFontPrivate *priv = ctfont->priv;

  priv->face = ctface;
}

VogueCoreTextFace *
_vogue_core_text_font_get_face (VogueCoreTextFont *font)
{
  VogueCoreTextFontPrivate *priv = font->priv;

  return priv->face;
}

gpointer
_vogue_core_text_font_get_context_key (VogueCoreTextFont *font)
{
  VogueCoreTextFontPrivate *priv = font->priv;

  return priv->context_key;
}

void
_vogue_core_text_font_set_context_key (VogueCoreTextFont *font,
                                       gpointer        context_key)
{
  VogueCoreTextFontPrivate *priv = font->priv;

  priv->context_key = context_key;
}

void
_vogue_core_text_font_set_font_key (VogueCoreTextFont    *font,
                                    VogueCoreTextFontKey *key)
{
  VogueCoreTextFontPrivate *priv = font->priv;

  priv->key = key;

  if (priv->coverage)
    {
      vogue_coverage_unref (priv->coverage);
      priv->coverage = NULL;
    }
}

void
_vogue_core_text_font_set_ctfont (VogueCoreTextFont *font,
                                  CTFontRef          font_ref)
{
  VogueCoreTextFontPrivate *priv = font->priv;

  priv->font_ref = font_ref;
}

/**
 * vogue_core_text_font_get_ctfont:
 * @font: A #VogueCoreTextFont
 *
 * Returns the CTFontRef of a font.
 *
 * Return value: the CTFontRef associated to @font.
 *
 * Since: 1.24
 */
CTFontRef
vogue_core_text_font_get_ctfont (VogueCoreTextFont *font)
{
  VogueCoreTextFontPrivate *priv = font->priv;

  return priv->font_ref;
}
