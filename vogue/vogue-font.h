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

#ifndef __PANGO_FONT_H__
#define __PANGO_FONT_H__

#include <vogue/vogue-coverage.h>
#include <vogue/vogue-types.h>

#include <glib-object.h>
#include <hb.h>

G_BEGIN_DECLS

/**
 * VogueFontDescription:
 *
 * The #VogueFontDescription structure represents the description
 * of an ideal font. These structures are used both to list
 * what fonts are available on the system and also for specifying
 * the characteristics of a font to load.
 */
typedef struct _VogueFontDescription VogueFontDescription;
/**
 * VogueFontMetrics:
 *
 * A #VogueFontMetrics structure holds the overall metric information
 * for a font (possibly restricted to a script). The fields of this
 * structure are private to implementations of a font backend. See
 * the documentation of the corresponding getters for documentation
 * of their meaning.
 */
typedef struct _VogueFontMetrics VogueFontMetrics;

/**
 * VogueStyle:
 * @PANGO_STYLE_NORMAL: the font is upright.
 * @PANGO_STYLE_OBLIQUE: the font is slanted, but in a roman style.
 * @PANGO_STYLE_ITALIC: the font is slanted in an italic style.
 *
 * An enumeration specifying the various slant styles possible for a font.
 **/
typedef enum {
  PANGO_STYLE_NORMAL,
  PANGO_STYLE_OBLIQUE,
  PANGO_STYLE_ITALIC
} VogueStyle;

/**
 * VogueVariant:
 * @PANGO_VARIANT_NORMAL: A normal font.
 * @PANGO_VARIANT_SMALL_CAPS: A font with the lower case characters
 * replaced by smaller variants of the capital characters.
 *
 * An enumeration specifying capitalization variant of the font.
 */
typedef enum {
  PANGO_VARIANT_NORMAL,
  PANGO_VARIANT_SMALL_CAPS
} VogueVariant;

/**
 * VogueWeight:
 * @PANGO_WEIGHT_THIN: the thin weight (= 100; Since: 1.24)
 * @PANGO_WEIGHT_ULTRALIGHT: the ultralight weight (= 200)
 * @PANGO_WEIGHT_LIGHT: the light weight (= 300)
 * @PANGO_WEIGHT_SEMILIGHT: the semilight weight (= 350; Since: 1.36.7)
 * @PANGO_WEIGHT_BOOK: the book weight (= 380; Since: 1.24)
 * @PANGO_WEIGHT_NORMAL: the default weight (= 400)
 * @PANGO_WEIGHT_MEDIUM: the normal weight (= 500; Since: 1.24)
 * @PANGO_WEIGHT_SEMIBOLD: the semibold weight (= 600)
 * @PANGO_WEIGHT_BOLD: the bold weight (= 700)
 * @PANGO_WEIGHT_ULTRABOLD: the ultrabold weight (= 800)
 * @PANGO_WEIGHT_HEAVY: the heavy weight (= 900)
 * @PANGO_WEIGHT_ULTRAHEAVY: the ultraheavy weight (= 1000; Since: 1.24)
 *
 * An enumeration specifying the weight (boldness) of a font. This is a numerical
 * value ranging from 100 to 1000, but there are some predefined values:
 */
typedef enum {
  PANGO_WEIGHT_THIN = 100,
  PANGO_WEIGHT_ULTRALIGHT = 200,
  PANGO_WEIGHT_LIGHT = 300,
  PANGO_WEIGHT_SEMILIGHT = 350,
  PANGO_WEIGHT_BOOK = 380,
  PANGO_WEIGHT_NORMAL = 400,
  PANGO_WEIGHT_MEDIUM = 500,
  PANGO_WEIGHT_SEMIBOLD = 600,
  PANGO_WEIGHT_BOLD = 700,
  PANGO_WEIGHT_ULTRABOLD = 800,
  PANGO_WEIGHT_HEAVY = 900,
  PANGO_WEIGHT_ULTRAHEAVY = 1000
} VogueWeight;

/**
 * VogueStretch:
 * @PANGO_STRETCH_ULTRA_CONDENSED: ultra condensed width
 * @PANGO_STRETCH_EXTRA_CONDENSED: extra condensed width
 * @PANGO_STRETCH_CONDENSED: condensed width
 * @PANGO_STRETCH_SEMI_CONDENSED: semi condensed width
 * @PANGO_STRETCH_NORMAL: the normal width
 * @PANGO_STRETCH_SEMI_EXPANDED: semi expanded width
 * @PANGO_STRETCH_EXPANDED: expanded width
 * @PANGO_STRETCH_EXTRA_EXPANDED: extra expanded width
 * @PANGO_STRETCH_ULTRA_EXPANDED: ultra expanded width
 *
 * An enumeration specifying the width of the font relative to other designs
 * within a family.
 */
typedef enum {
  PANGO_STRETCH_ULTRA_CONDENSED,
  PANGO_STRETCH_EXTRA_CONDENSED,
  PANGO_STRETCH_CONDENSED,
  PANGO_STRETCH_SEMI_CONDENSED,
  PANGO_STRETCH_NORMAL,
  PANGO_STRETCH_SEMI_EXPANDED,
  PANGO_STRETCH_EXPANDED,
  PANGO_STRETCH_EXTRA_EXPANDED,
  PANGO_STRETCH_ULTRA_EXPANDED
} VogueStretch;

/**
 * VogueFontMask:
 * @PANGO_FONT_MASK_FAMILY: the font family is specified.
 * @PANGO_FONT_MASK_STYLE: the font style is specified.
 * @PANGO_FONT_MASK_VARIANT: the font variant is specified.
 * @PANGO_FONT_MASK_WEIGHT: the font weight is specified.
 * @PANGO_FONT_MASK_STRETCH: the font stretch is specified.
 * @PANGO_FONT_MASK_SIZE: the font size is specified.
 * @PANGO_FONT_MASK_GRAVITY: the font gravity is specified (Since: 1.16.)
 * @PANGO_FONT_MASK_VARIATIONS: OpenType font variations are specified (Since: 1.42)
 *
 * The bits in a #VogueFontMask correspond to fields in a
 * #VogueFontDescription that have been set.
 */
typedef enum {
  PANGO_FONT_MASK_FAMILY  = 1 << 0,
  PANGO_FONT_MASK_STYLE   = 1 << 1,
  PANGO_FONT_MASK_VARIANT = 1 << 2,
  PANGO_FONT_MASK_WEIGHT  = 1 << 3,
  PANGO_FONT_MASK_STRETCH = 1 << 4,
  PANGO_FONT_MASK_SIZE    = 1 << 5,
  PANGO_FONT_MASK_GRAVITY = 1 << 6,
  PANGO_FONT_MASK_VARIATIONS = 1 << 7,
} VogueFontMask;

/* CSS scale factors (1.2 factor between each size) */
/**
 * PANGO_SCALE_XX_SMALL:
 *
 * The scale factor for three shrinking steps (1 / (1.2 * 1.2 * 1.2)).
 */
/**
 * PANGO_SCALE_X_SMALL:
 *
 * The scale factor for two shrinking steps (1 / (1.2 * 1.2)).
 */
/**
 * PANGO_SCALE_SMALL:
 *
 * The scale factor for one shrinking step (1 / 1.2).
 */
/**
 * PANGO_SCALE_MEDIUM:
 *
 * The scale factor for normal size (1.0).
 */
/**
 * PANGO_SCALE_LARGE:
 *
 * The scale factor for one magnification step (1.2).
 */
/**
 * PANGO_SCALE_X_LARGE:
 *
 * The scale factor for two magnification steps (1.2 * 1.2).
 */
/**
 * PANGO_SCALE_XX_LARGE:
 *
 * The scale factor for three magnification steps (1.2 * 1.2 * 1.2).
 */
#define PANGO_SCALE_XX_SMALL ((double)0.5787037037037)
#define PANGO_SCALE_X_SMALL  ((double)0.6944444444444)
#define PANGO_SCALE_SMALL    ((double)0.8333333333333)
#define PANGO_SCALE_MEDIUM   ((double)1.0)
#define PANGO_SCALE_LARGE    ((double)1.2)
#define PANGO_SCALE_X_LARGE  ((double)1.44)
#define PANGO_SCALE_XX_LARGE ((double)1.728)

/*
 * VogueFontDescription
 */

/**
 * PANGO_TYPE_FONT_DESCRIPTION:
 *
 * The #GObject type for #VogueFontDescription.
 */
#define PANGO_TYPE_FONT_DESCRIPTION (vogue_font_description_get_type ())

PANGO_AVAILABLE_IN_ALL
GType                 vogue_font_description_get_type    (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
VogueFontDescription *vogue_font_description_new         (void);
PANGO_AVAILABLE_IN_ALL
VogueFontDescription *vogue_font_description_copy        (const VogueFontDescription  *desc);
PANGO_AVAILABLE_IN_ALL
VogueFontDescription *vogue_font_description_copy_static (const VogueFontDescription  *desc);
PANGO_AVAILABLE_IN_ALL
guint                 vogue_font_description_hash        (const VogueFontDescription  *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
gboolean              vogue_font_description_equal       (const VogueFontDescription  *desc1,
							  const VogueFontDescription  *desc2) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                  vogue_font_description_free        (VogueFontDescription        *desc);
PANGO_AVAILABLE_IN_ALL
void                  vogue_font_descriptions_free       (VogueFontDescription       **descs,
							  int                          n_descs);

PANGO_AVAILABLE_IN_ALL
void                 vogue_font_description_set_family        (VogueFontDescription *desc,
							       const char           *family);
PANGO_AVAILABLE_IN_ALL
void                 vogue_font_description_set_family_static (VogueFontDescription *desc,
							       const char           *family);
PANGO_AVAILABLE_IN_ALL
const char          *vogue_font_description_get_family        (const VogueFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                 vogue_font_description_set_style         (VogueFontDescription *desc,
							       VogueStyle            style);
PANGO_AVAILABLE_IN_ALL
VogueStyle           vogue_font_description_get_style         (const VogueFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                 vogue_font_description_set_variant       (VogueFontDescription *desc,
							       VogueVariant          variant);
PANGO_AVAILABLE_IN_ALL
VogueVariant         vogue_font_description_get_variant       (const VogueFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                 vogue_font_description_set_weight        (VogueFontDescription *desc,
							       VogueWeight           weight);
PANGO_AVAILABLE_IN_ALL
VogueWeight          vogue_font_description_get_weight        (const VogueFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                 vogue_font_description_set_stretch       (VogueFontDescription *desc,
							       VogueStretch          stretch);
PANGO_AVAILABLE_IN_ALL
VogueStretch         vogue_font_description_get_stretch       (const VogueFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                 vogue_font_description_set_size          (VogueFontDescription *desc,
							       gint                  size);
PANGO_AVAILABLE_IN_ALL
gint                 vogue_font_description_get_size          (const VogueFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_8
void                 vogue_font_description_set_absolute_size (VogueFontDescription *desc,
							       double                size);
PANGO_AVAILABLE_IN_1_8
gboolean             vogue_font_description_get_size_is_absolute (const VogueFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_16
void                 vogue_font_description_set_gravity       (VogueFontDescription *desc,
							       VogueGravity          gravity);
PANGO_AVAILABLE_IN_1_16
VogueGravity         vogue_font_description_get_gravity       (const VogueFontDescription *desc) G_GNUC_PURE;

PANGO_AVAILABLE_IN_1_42
void                 vogue_font_description_set_variations_static (VogueFontDescription       *desc,
                                                                   const char                 *variations);
PANGO_AVAILABLE_IN_1_42
void                 vogue_font_description_set_variations    (VogueFontDescription       *desc,
                                                               const char                 *variations);
PANGO_AVAILABLE_IN_1_42
const char          *vogue_font_description_get_variations    (const VogueFontDescription *desc) G_GNUC_PURE;

PANGO_AVAILABLE_IN_ALL
VogueFontMask vogue_font_description_get_set_fields (const VogueFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void          vogue_font_description_unset_fields   (VogueFontDescription       *desc,
						     VogueFontMask               to_unset);

PANGO_AVAILABLE_IN_ALL
void vogue_font_description_merge        (VogueFontDescription       *desc,
					  const VogueFontDescription *desc_to_merge,
					  gboolean                    replace_existing);
PANGO_AVAILABLE_IN_ALL
void vogue_font_description_merge_static (VogueFontDescription       *desc,
					  const VogueFontDescription *desc_to_merge,
					  gboolean                    replace_existing);

PANGO_AVAILABLE_IN_ALL
gboolean vogue_font_description_better_match (const VogueFontDescription *desc,
					      const VogueFontDescription *old_match,
					      const VogueFontDescription *new_match) G_GNUC_PURE;

PANGO_AVAILABLE_IN_ALL
VogueFontDescription *vogue_font_description_from_string (const char                  *str);
PANGO_AVAILABLE_IN_ALL
char *                vogue_font_description_to_string   (const VogueFontDescription  *desc);
PANGO_AVAILABLE_IN_ALL
char *                vogue_font_description_to_filename (const VogueFontDescription  *desc);

/*
 * VogueFontMetrics
 */

/**
 * PANGO_TYPE_FONT_METRICS:
 *
 * The #GObject type for #VogueFontMetrics.
 */
#define PANGO_TYPE_FONT_METRICS  (vogue_font_metrics_get_type ())
PANGO_AVAILABLE_IN_ALL
GType             vogue_font_metrics_get_type                    (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
VogueFontMetrics *vogue_font_metrics_ref                         (VogueFontMetrics *metrics);
PANGO_AVAILABLE_IN_ALL
void              vogue_font_metrics_unref                       (VogueFontMetrics *metrics);
PANGO_AVAILABLE_IN_ALL
int               vogue_font_metrics_get_ascent                  (VogueFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               vogue_font_metrics_get_descent                 (VogueFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_44
int               vogue_font_metrics_get_height                  (VogueFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               vogue_font_metrics_get_approximate_char_width  (VogueFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               vogue_font_metrics_get_approximate_digit_width (VogueFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_6
int               vogue_font_metrics_get_underline_position      (VogueFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_6
int               vogue_font_metrics_get_underline_thickness     (VogueFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_6
int               vogue_font_metrics_get_strikethrough_position  (VogueFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_6
int               vogue_font_metrics_get_strikethrough_thickness (VogueFontMetrics *metrics) G_GNUC_PURE;


/*
 * VogueFontFamily
 */

/**
 * PANGO_TYPE_FONT_FAMILY:
 *
 * The #GObject type for #VogueFontFamily.
 */
/**
 * PANGO_FONT_FAMILY:
 * @object: a #GObject.
 *
 * Casts a #GObject to a #VogueFontFamily.
 */
/**
 * PANGO_IS_FONT_FAMILY:
 * @object: a #GObject.
 *
 * Returns: %TRUE if @object is a #VogueFontFamily.
 */
#define PANGO_TYPE_FONT_FAMILY              (vogue_font_family_get_type ())
#define PANGO_FONT_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT_FAMILY, VogueFontFamily))
#define PANGO_IS_FONT_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT_FAMILY))

typedef struct _VogueFontFamily      VogueFontFamily;
typedef struct _VogueFontFace        VogueFontFace;

PANGO_AVAILABLE_IN_ALL
GType      vogue_font_family_get_type       (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
void                 vogue_font_family_list_faces (VogueFontFamily  *family,
						   VogueFontFace  ***faces,
						   int              *n_faces);
PANGO_AVAILABLE_IN_ALL
const char *vogue_font_family_get_name   (VogueFontFamily  *family) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_4
gboolean   vogue_font_family_is_monospace         (VogueFontFamily  *family) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_44
gboolean   vogue_font_family_is_variable          (VogueFontFamily  *family) G_GNUC_PURE;


/*
 * VogueFontFace
 */

/**
 * PANGO_TYPE_FONT_FACE:
 *
 * The #GObject type for #VogueFontFace.
 */
/**
 * PANGO_FONT_FACE:
 * @object: a #GObject.
 *
 * Casts a #GObject to a #VogueFontFace.
 */
/**
 * PANGO_IS_FONT_FACE:
 * @object: a #GObject.
 *
 * Returns: %TRUE if @object is a #VogueFontFace.
 */
#define PANGO_TYPE_FONT_FACE              (vogue_font_face_get_type ())
#define PANGO_FONT_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT_FACE, VogueFontFace))
#define PANGO_IS_FONT_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT_FACE))

PANGO_AVAILABLE_IN_ALL
GType      vogue_font_face_get_type       (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
VogueFontDescription *vogue_font_face_describe       (VogueFontFace  *face);
PANGO_AVAILABLE_IN_ALL
const char           *vogue_font_face_get_face_name  (VogueFontFace  *face) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_4
void                  vogue_font_face_list_sizes     (VogueFontFace  *face,
						      int           **sizes,
						      int            *n_sizes);
PANGO_AVAILABLE_IN_1_18
gboolean              vogue_font_face_is_synthesized (VogueFontFace  *face) G_GNUC_PURE;


/*
 * VogueFont
 */

/**
 * PANGO_TYPE_FONT:
 *
 * The #GObject type for #VogueFont.
 */
/**
 * PANGO_FONT:
 * @object: a #GObject.
 *
 * Casts a #GObject to a #VogueFont.
 */
/**
 * PANGO_IS_FONT:
 * @object: a #GObject.
 *
 * Returns: %TRUE if @object is a #VogueFont.
 */
#define PANGO_TYPE_FONT              (vogue_font_get_type ())
#define PANGO_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT, VogueFont))
#define PANGO_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT))

#ifndef PANGO_DISABLE_DEPRECATED

/**
 * VogueFont:
 *
 * The #VogueFont structure is used to represent
 * a font in a rendering-system-independent matter.
 * To create an implementation of a #VogueFont,
 * the rendering-system specific code should allocate
 * a larger structure that contains a nested
 * #VogueFont, fill in the <structfield>klass</structfield> member of
 * the nested #VogueFont with a pointer to
 * a appropriate #VogueFontClass, then call
 * vogue_font_init() on the structure.
 *
 * The #VogueFont structure contains one member
 * which the implementation fills in.
 */
struct _VogueFont
{
  GObject parent_instance;
};

#endif /* PANGO_DISABLE_DEPRECATED */

PANGO_AVAILABLE_IN_ALL
GType                 vogue_font_get_type          (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
VogueFontDescription *vogue_font_describe          (VogueFont        *font);
PANGO_AVAILABLE_IN_1_14
VogueFontDescription *vogue_font_describe_with_absolute_size (VogueFont        *font);
PANGO_AVAILABLE_IN_ALL
VogueCoverage *       vogue_font_get_coverage      (VogueFont        *font,
						    VogueLanguage    *language);
PANGO_DEPRECATED_IN_1_44
VogueEngineShape *    vogue_font_find_shaper       (VogueFont        *font,
						    VogueLanguage    *language,
						    guint32           ch);
PANGO_AVAILABLE_IN_ALL
VogueFontMetrics *    vogue_font_get_metrics       (VogueFont        *font,
						    VogueLanguage    *language);
PANGO_AVAILABLE_IN_ALL
void                  vogue_font_get_glyph_extents (VogueFont        *font,
						    VogueGlyph        glyph,
						    VogueRectangle   *ink_rect,
						    VogueRectangle   *logical_rect);
PANGO_AVAILABLE_IN_1_10
VogueFontMap         *vogue_font_get_font_map      (VogueFont        *font);

PANGO_AVAILABLE_IN_1_44
gboolean              vogue_font_has_char          (VogueFont        *font,
                                                    gunichar          wc);
PANGO_AVAILABLE_IN_1_44
void                  vogue_font_get_features      (VogueFont        *font,
                                                    hb_feature_t     *features,
                                                    guint             len,
                                                    guint            *num_features);
PANGO_AVAILABLE_IN_1_44
hb_font_t *           vogue_font_get_hb_font       (VogueFont        *font);


/**
 * PANGO_GLYPH_EMPTY:
 *
 * The %PANGO_GLYPH_EMPTY macro represents a #VogueGlyph value that has a
 *  special meaning, which is a zero-width empty glyph.  This is useful for
 * example in shaper modules, to use as the glyph for various zero-width
 * Unicode characters (those passing vogue_is_zero_width()).
 */
/**
 * PANGO_GLYPH_INVALID_INPUT:
 *
 * The %PANGO_GLYPH_INVALID_INPUT macro represents a #VogueGlyph value that has a
 * special meaning of invalid input.  #VogueLayout produces one such glyph
 * per invalid input UTF-8 byte and such a glyph is rendered as a crossed
 * box.
 *
 * Note that this value is defined such that it has the %PANGO_GLYPH_UNKNOWN_FLAG
 * on.
 *
 * Since: 1.20
 */
/**
 * PANGO_GLYPH_UNKNOWN_FLAG:
 *
 * The %PANGO_GLYPH_UNKNOWN_FLAG macro is a flag value that can be added to
 * a #gunichar value of a valid Unicode character, to produce a #VogueGlyph
 * value, representing an unknown-character glyph for the respective #gunichar.
 */
/**
 * PANGO_GET_UNKNOWN_GLYPH:
 * @wc: a Unicode character
 *
 * The way this unknown glyphs are rendered is backend specific.  For example,
 * a box with the hexadecimal Unicode code-point of the character written in it
 * is what is done in the most common backends.
 *
 * Returns: a #VogueGlyph value that means no glyph was found for @wc.
 */
#define PANGO_GLYPH_EMPTY           ((VogueGlyph)0x0FFFFFFF)
#define PANGO_GLYPH_INVALID_INPUT   ((VogueGlyph)0xFFFFFFFF)
#define PANGO_GLYPH_UNKNOWN_FLAG    ((VogueGlyph)0x10000000)
#define PANGO_GET_UNKNOWN_GLYPH(wc) ((VogueGlyph)(wc)|PANGO_GLYPH_UNKNOWN_FLAG)

#ifndef PANGO_DISABLE_DEPRECATED
#define PANGO_UNKNOWN_GLYPH_WIDTH  10
#define PANGO_UNKNOWN_GLYPH_HEIGHT 14
#endif

G_END_DECLS

#endif /* __PANGO_FONT_H__ */
