/* Vogue
 * vogue-fontset.h: Font set handling
 *
 * Copyright (C) 2001 Red Hat Software
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

#ifndef __PANGO_FONTSET_PRIVATE_H__
#define __PANGO_FONTSET_PRIVATE_H__

#include <vogue/vogue-types.h>
#include <vogue/vogue-fontset.h>
#include <vogue/vogue-coverage.h>

#include <glib-object.h>

G_BEGIN_DECLS


typedef struct _VogueFontsetClass   VogueFontsetClass;

#define PANGO_FONTSET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONTSET, VogueFontsetClass))
#define PANGO_IS_FONTSET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONTSET))
#define PANGO_FONTSET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONTSET, VogueFontsetClass))

/**
 * VogueFontset:
 *
 * A #VogueFontset represents a set of #VogueFont to use
 * when rendering text. It is the result of resolving a
 * #VogueFontDescription against a particular #VogueContext.
 * It has operations for finding the component font for
 * a particular Unicode character, and for finding a composite
 * set of metrics for the entire fontset.
 */
struct _VogueFontset
{
  GObject parent_instance;
};

/**
 * VogueFontsetClass:
 * @parent_class: parent #GObjectClass.
 * @get_font: a function to get the font in the fontset that contains the
 * best glyph for the given Unicode character; see vogue_fontset_get_font().
 * @get_metrics: a function to get overall metric information for the fonts
 * in the fontset; see vogue_fontset_get_metrics().
 * @get_language: a function to get the language of the fontset.
 * @foreach: a function to loop over the fonts in the fontset. See
 * vogue_fontset_foreach().
 *
 * The #VogueFontsetClass structure holds the virtual functions for
 * a particular #VogueFontset implementation.
 */
struct _VogueFontsetClass
{
  GObjectClass parent_class;

  /*< public >*/

  VogueFont *       (*get_font)     (VogueFontset     *fontset,
				     guint             wc);

  VogueFontMetrics *(*get_metrics)  (VogueFontset     *fontset);
  VogueLanguage *   (*get_language) (VogueFontset     *fontset);
  void              (*foreach)      (VogueFontset           *fontset,
				     VogueFontsetForeachFunc func,
				     gpointer                data);

  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved1) (void);
  void (*_vogue_reserved2) (void);
  void (*_vogue_reserved3) (void);
  void (*_vogue_reserved4) (void);
};

/*
 * VogueFontsetSimple
 */

/**
 * PANGO_TYPE_FONTSET_SIMPLE:
 *
 * The #GObject type for #VogueFontsetSimple.
 */
/**
 * VogueFontsetSimple:
 *
 * #VogueFontsetSimple is a implementation of the abstract
 * #VogueFontset base class in terms of an array of fonts,
 * which the creator provides when constructing the
 * #VogueFontsetSimple.
 */
#define PANGO_TYPE_FONTSET_SIMPLE       (vogue_fontset_simple_get_type ())
#define PANGO_FONTSET_SIMPLE(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONTSET_SIMPLE, VogueFontsetSimple))
#define PANGO_IS_FONTSET_SIMPLE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONTSET_SIMPLE))

typedef struct _VogueFontsetSimple  VogueFontsetSimple;
typedef struct _VogueFontsetSimpleClass  VogueFontsetSimpleClass;

PANGO_AVAILABLE_IN_ALL
GType vogue_fontset_simple_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
VogueFontsetSimple * vogue_fontset_simple_new    (VogueLanguage      *language);
PANGO_AVAILABLE_IN_ALL
void                 vogue_fontset_simple_append (VogueFontsetSimple *fontset,
						  VogueFont          *font);
PANGO_AVAILABLE_IN_ALL
int                  vogue_fontset_simple_size   (VogueFontsetSimple *fontset);


G_END_DECLS

#endif /* __PANGO_FONTSET_PRIVATE_H__ */
