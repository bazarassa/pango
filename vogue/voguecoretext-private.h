/* Vogue
 * voguecoretext-private.h:
 *
 * Copyright (C) 2003 Red Hat Software
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

#ifndef __PANGOCORETEXT_PRIVATE_H__
#define __PANGOCORETEXT_PRIVATE_H__

#include "voguecoretext.h"
#include "vogue-font-private.h"
#include "vogue-fontmap-private.h"
#include "vogue-fontset-private.h"

G_BEGIN_DECLS

/**
 * PANGO_RENDER_TYPE_CORE_TEXT:
 *
 * A string constant identifying the CoreText renderer. The associated quark (see
 * g_quark_from_string()) is used to identify the renderer in vogue_find_map().
 */
#define PANGO_RENDER_TYPE_CORE_TEXT "VogueRenderCoreText"

#define PANGO_CORE_TEXT_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CORE_TEXT_FONT, VogueCoreTextFontClass))
#define PANGO_IS_CORE_TEXT_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_CORE_TEXT_FONT))
#define PANGO_CORE_TEXT_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CORE_TEXT_FONT, VogueCoreTextFontClass))

typedef struct _VogueCoreTextFontPrivate  VogueCoreTextFontPrivate;

struct _VogueCoreTextFont
{
  VogueFont parent_instance;
  VogueCoreTextFontPrivate *priv;
};

struct _VogueCoreTextFontClass
{
  VogueFontClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved1) (void);
  void (*_vogue_reserved2) (void);
  void (*_vogue_reserved3) (void);
  void (*_vogue_reserved4) (void);
};

PANGO_AVAILABLE_IN_1_24
CTFontRef  vogue_core_text_font_get_ctfont  (VogueCoreTextFont *font);


#define PANGO_TYPE_CORE_TEXT_FONT_MAP             (vogue_core_text_font_map_get_type ())
#define PANGO_CORE_TEXT_FONT_MAP(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CORE_TEXT_FONT_MAP, VogueCoreTextFontMap))
#define PANGO_CORE_TEXT_IS_FONT_MAP(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CORE_TEXT_FONT_MAP))
#define PANGO_CORE_TEXT_FONT_MAP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CORE_TEXT_FONT_MAP, VogueCoreTextFontMapClass))
#define PANGO_IS_CORE_TEXT_FONT_MAP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_CORE_TEXT_FONT_MAP))
#define PANGO_CORE_TEXT_FONT_MAP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CORE_TEXT_FONT_MAP, VogueCoreTextFontMapClass))


typedef struct _VogueCoreTextFamily       VogueCoreTextFamily;
typedef struct _VogueCoreTextFace         VogueCoreTextFace;

typedef struct _VogueCoreTextFontMap      VogueCoreTextFontMap;
typedef struct _VogueCoreTextFontMapClass VogueCoreTextFontMapClass;

typedef struct _VogueCoreTextFontsetKey   VogueCoreTextFontsetKey;
typedef struct _VogueCoreTextFontKey      VogueCoreTextFontKey;

struct _VogueCoreTextFontMap
{
  VogueFontMap parent_instance;

  guint serial;
  GHashTable *fontset_hash;
  GHashTable *font_hash;

  GHashTable *families;
};

struct _VogueCoreTextFontMapClass
{
  VogueFontMapClass parent_class;

  gconstpointer (*context_key_get)   (VogueCoreTextFontMap   *ctfontmap,
                                      VogueContext           *context);
  gpointer     (*context_key_copy)   (VogueCoreTextFontMap   *ctfontmap,
                                      gconstpointer           key);
  void         (*context_key_free)   (VogueCoreTextFontMap   *ctfontmap,
                                      gpointer                key);
  guint32      (*context_key_hash)   (VogueCoreTextFontMap   *ctfontmap,
                                      gconstpointer           key);
  gboolean     (*context_key_equal)  (VogueCoreTextFontMap   *ctfontmap,
                                      gconstpointer           key_a,
                                      gconstpointer           key_b);

  VogueCoreTextFont * (* create_font)   (VogueCoreTextFontMap       *fontmap,
                                         VogueCoreTextFontKey       *key);

  double              (* get_resolution) (VogueCoreTextFontMap      *fontmap,
                                          VogueContext              *context);
};


_PANGO_EXTERN
GType                 vogue_core_text_font_map_get_type          (void) G_GNUC_CONST;

void                  _vogue_core_text_font_set_font_map         (VogueCoreTextFont    *afont,
                                                                  VogueCoreTextFontMap *fontmap);
void                  _vogue_core_text_font_set_face             (VogueCoreTextFont    *afont, 
                                                                  VogueCoreTextFace    *aface);
VogueCoreTextFace *   _vogue_core_text_font_get_face             (VogueCoreTextFont    *font);
gpointer              _vogue_core_text_font_get_context_key      (VogueCoreTextFont    *afont);
void                  _vogue_core_text_font_set_context_key      (VogueCoreTextFont    *afont,
                                                                  gpointer           context_key);
void                  _vogue_core_text_font_set_font_key         (VogueCoreTextFont    *font,
                                                                  VogueCoreTextFontKey *key);
void                  _vogue_core_text_font_set_ctfont           (VogueCoreTextFont    *font,
                                                                  CTFontRef         font_ref);

VogueFontDescription *_vogue_core_text_font_description_from_ct_font_descriptor (CTFontDescriptorRef desc);

_PANGO_EXTERN
int                   vogue_core_text_font_key_get_size             (const VogueCoreTextFontKey *key);
_PANGO_EXTERN
int                   vogue_core_text_font_key_get_size    (const VogueCoreTextFontKey *key);
_PANGO_EXTERN
double                vogue_core_text_font_key_get_resolution       (const VogueCoreTextFontKey *key);
_PANGO_EXTERN
gboolean              vogue_core_text_font_key_get_synthetic_italic (const VogueCoreTextFontKey *key);
_PANGO_EXTERN
gpointer              vogue_core_text_font_key_get_context_key      (const VogueCoreTextFontKey *key);
_PANGO_EXTERN
const VogueMatrix    *vogue_core_text_font_key_get_matrix           (const VogueCoreTextFontKey *key);
_PANGO_EXTERN
VogueGravity          vogue_core_text_font_key_get_gravity          (const VogueCoreTextFontKey *key);
_PANGO_EXTERN
CTFontDescriptorRef   vogue_core_text_font_key_get_ctfontdescriptor (const VogueCoreTextFontKey *key);

G_END_DECLS

#endif /* __PANGOCORETEXT_PRIVATE_H__ */
