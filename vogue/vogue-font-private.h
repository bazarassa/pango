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

#ifndef __PANGO_FONT_PRIVATE_H__
#define __PANGO_FONT_PRIVATE_H__

#include <vogue/vogue-font.h>
#include <vogue/vogue-coverage.h>
#include <vogue/vogue-types.h>

#include <glib-object.h>

G_BEGIN_DECLS

PANGO_AVAILABLE_IN_ALL
VogueFontMetrics *vogue_font_metrics_new (void);

struct _VogueFontMetrics
{
  /* <private> */
  guint ref_count;

  int ascent;
  int descent;
  int height;
  int approximate_char_width;
  int approximate_digit_width;
  int underline_position;
  int underline_thickness;
  int strikethrough_position;
  int strikethrough_thickness;
};


#define PANGO_FONT_FAMILY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_FAMILY, VogueFontFamilyClass))
#define PANGO_IS_FONT_FAMILY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT_FAMILY))
#define PANGO_FONT_FAMILY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_FAMILY, VogueFontFamilyClass))

typedef struct _VogueFontFamilyClass VogueFontFamilyClass;


/**
 * VogueFontFamily:
 *
 * The #VogueFontFamily structure is used to represent a family of related
 * font faces. The faces in a family share a common design, but differ in
 * slant, weight, width and other aspects.
 */
struct _VogueFontFamily
{
  GObject parent_instance;
};

struct _VogueFontFamilyClass
{
  GObjectClass parent_class;

  /*< public >*/

  void  (*list_faces)      (VogueFontFamily  *family,
			    VogueFontFace  ***faces,
			    int              *n_faces);
  const char * (*get_name) (VogueFontFamily  *family);
  gboolean (*is_monospace) (VogueFontFamily *family);
  gboolean (*is_variable)  (VogueFontFamily *family);

  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved2) (void);
  void (*_vogue_reserved3) (void);
};


#define PANGO_FONT_FACE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_FACE, VogueFontFaceClass))
#define PANGO_IS_FONT_FACE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT_FACE))
#define PANGO_FONT_FACE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_FACE, VogueFontFaceClass))

typedef struct _VogueFontFaceClass   VogueFontFaceClass;

/**
 * VogueFontFace:
 *
 * The #VogueFontFace structure is used to represent a group of fonts with
 * the same family, slant, weight, width, but varying sizes.
 */
struct _VogueFontFace
{
  GObject parent_instance;
};

struct _VogueFontFaceClass
{
  GObjectClass parent_class;

  /*< public >*/

  const char           * (*get_face_name)  (VogueFontFace *face);
  VogueFontDescription * (*describe)       (VogueFontFace *face);
  void                   (*list_sizes)     (VogueFontFace  *face,
					    int           **sizes,
					    int            *n_sizes);
  gboolean               (*is_synthesized) (VogueFontFace *face);

  /*< private >*/

  /* Padding for future expansion */
  void (*_vogue_reserved3) (void);
  void (*_vogue_reserved4) (void);
};


#define PANGO_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT, VogueFontClass))
#define PANGO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT))
#define PANGO_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT, VogueFontClass))

typedef struct _VogueFontClass       VogueFontClass;

struct _VogueFontClass
{
  GObjectClass parent_class;

  /*< public >*/

  VogueFontDescription *(*describe)           (VogueFont      *font);
  VogueCoverage *       (*get_coverage)       (VogueFont      *font,
					       VogueLanguage  *language);
  void                  (*get_glyph_extents)  (VogueFont      *font,
					       VogueGlyph      glyph,
					       VogueRectangle *ink_rect,
					       VogueRectangle *logical_rect);
  VogueFontMetrics *    (*get_metrics)        (VogueFont      *font,
					       VogueLanguage  *language);
  VogueFontMap *        (*get_font_map)       (VogueFont      *font);
  VogueFontDescription *(*describe_absolute)  (VogueFont      *font);
  void                  (*get_features)       (VogueFont      *font,
                                               hb_feature_t   *features,
                                               guint           len,
                                               guint          *num_features);
  hb_font_t *           (*create_hb_font)     (VogueFont      *font);
};


G_END_DECLS

#endif /* __PANGO_FONT_PRIVATE_H__ */
