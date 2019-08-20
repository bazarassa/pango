/* Vogue
 * vogue-font.h: Font handling
 *
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __PANGO_FONTMAP_H__
#define __PANGO_FONTMAP_H__

#include <vogue/vogue-font.h>
#include <vogue/vogue-fontset.h>

G_BEGIN_DECLS

/**
 * PANGO_TYPE_FONT_MAP:
 *
 * The #GObject type for #VogueFontMap.
 */
/**
 * PANGO_FONT_MAP:
 * @object: a #GObject.
 *
 * Casts a #GObject to a #VogueFontMap.
 */
/**
 * PANGO_IS_FONT_MAP:
 * @object: a #GObject.
 *
 * Returns: %TRUE if @object is a #VogueFontMap.
 */
#define PANGO_TYPE_FONT_MAP              (vogue_font_map_get_type ())
#define PANGO_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT_MAP, VogueFontMap))
#define PANGO_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT_MAP))
#define PANGO_FONT_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_MAP, VogueFontMapClass))
#define PANGO_IS_FONT_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT_MAP))
#define PANGO_FONT_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_MAP, VogueFontMapClass))

typedef struct _VogueFontMapClass VogueFontMapClass;
typedef struct _VogueContext VogueContext;

/**
 * VogueFontMap:
 *
 * The #VogueFontMap represents the set of fonts available for a
 * particular rendering system. This is a virtual object with
 * implementations being specific to particular rendering systems.  To
 * create an implementation of a #VogueFontMap, the rendering-system
 * specific code should allocate a larger structure that contains a nested
 * #VogueFontMap, fill in the <structfield>klass</structfield> member of the nested #VogueFontMap with a
 * pointer to a appropriate #VogueFontMapClass, then call
 * vogue_font_map_init() on the structure.
 *
 * The #VogueFontMap structure contains one member which the implementation
 * fills in.
 */
struct _VogueFontMap
{
  GObject parent_instance;
};

/**
 * VogueFontMapClass:
 * @parent_class: parent #GObjectClass.
 * @load_font: a function to load a font with a given description. See
 * vogue_font_map_load_font().
 * @list_families: A function to list available font families. See
 * vogue_font_map_list_families().
 * @load_fontset: a function to load a fontset with a given given description
 * suitable for a particular language. See vogue_font_map_load_fontset().
 * @shape_engine_type: the type of rendering-system-dependent engines that
 * can handle fonts of this fonts loaded with this fontmap.
 * @get_serial: a function to get the serial number of the fontmap.
 * See vogue_font_map_get_serial().
 * @changed: See vogue_font_map_changed()
 *
 * The #VogueFontMapClass structure holds the virtual functions for
 * a particular #VogueFontMap implementation.
 */
struct _VogueFontMapClass
{
  GObjectClass parent_class;

  /*< public >*/

  VogueFont *   (*load_font)     (VogueFontMap               *fontmap,
                                  VogueContext               *context,
                                  const VogueFontDescription *desc);
  void          (*list_families) (VogueFontMap               *fontmap,
                                  VogueFontFamily          ***families,
                                  int                        *n_families);
  VogueFontset *(*load_fontset)  (VogueFontMap               *fontmap,
                                  VogueContext               *context,
                                  const VogueFontDescription *desc,
                                  VogueLanguage              *language);

  const char     *shape_engine_type;

  guint         (*get_serial)    (VogueFontMap               *fontmap);
  void          (*changed)       (VogueFontMap               *fontmap);

  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved1) (void);
  void (*_vogue_reserved2) (void);
};

PANGO_AVAILABLE_IN_ALL
GType         vogue_font_map_get_type       (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_1_22
VogueContext * vogue_font_map_create_context (VogueFontMap               *fontmap);
PANGO_AVAILABLE_IN_ALL
VogueFont *   vogue_font_map_load_font     (VogueFontMap                 *fontmap,
					    VogueContext                 *context,
					    const VogueFontDescription   *desc);
PANGO_AVAILABLE_IN_ALL
VogueFontset *vogue_font_map_load_fontset  (VogueFontMap                 *fontmap,
					    VogueContext                 *context,
					    const VogueFontDescription   *desc,
					    VogueLanguage                *language);
PANGO_AVAILABLE_IN_ALL
void          vogue_font_map_list_families (VogueFontMap                 *fontmap,
					    VogueFontFamily            ***families,
					    int                          *n_families);
PANGO_AVAILABLE_IN_1_32
guint         vogue_font_map_get_serial    (VogueFontMap                 *fontmap);
PANGO_AVAILABLE_IN_1_34
void          vogue_font_map_changed       (VogueFontMap                 *fontmap);


G_END_DECLS

#endif /* __PANGO_FONTMAP_H__ */
