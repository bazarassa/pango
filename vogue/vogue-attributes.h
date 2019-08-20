/* Vogue
 * vogue-attributes.h: Attributed text
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

#ifndef __PANGO_ATTRIBUTES_H__
#define __PANGO_ATTRIBUTES_H__

#include <vogue/vogue-font.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* VogueColor */

typedef struct _VogueColor VogueColor;

/**
 * VogueColor:
 * @red: value of red component
 * @green: value of green component
 * @blue: value of blue component
 *
 * The #VogueColor structure is used to
 * represent a color in an uncalibrated RGB color-space.
 */
struct _VogueColor
{
  guint16 red;
  guint16 green;
  guint16 blue;
};

/**
 * PANGO_TYPE_COLOR:
 *
 * The #GObject type for #VogueColor.
 */
#define PANGO_TYPE_COLOR vogue_color_get_type ()
PANGO_AVAILABLE_IN_ALL
GType       vogue_color_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
GType       vogue_attribute_get_type    (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
VogueColor *vogue_color_copy     (const VogueColor *src);
PANGO_AVAILABLE_IN_ALL
void        vogue_color_free     (VogueColor       *color);
PANGO_AVAILABLE_IN_ALL
gboolean    vogue_color_parse    (VogueColor       *color,
				  const char       *spec);
PANGO_AVAILABLE_IN_1_16
gchar      *vogue_color_to_string(const VogueColor *color);


/* Attributes */

typedef struct _VogueAttribute    VogueAttribute;
typedef struct _VogueAttrClass    VogueAttrClass;

typedef struct _VogueAttrString   VogueAttrString;
typedef struct _VogueAttrLanguage VogueAttrLanguage;
typedef struct _VogueAttrInt      VogueAttrInt;
typedef struct _VogueAttrSize     VogueAttrSize;
typedef struct _VogueAttrFloat    VogueAttrFloat;
typedef struct _VogueAttrColor    VogueAttrColor;
typedef struct _VogueAttrFontDesc VogueAttrFontDesc;
typedef struct _VogueAttrShape    VogueAttrShape;
typedef struct _VogueAttrFontFeatures VogueAttrFontFeatures;

/**
 * PANGO_TYPE_ATTR_LIST:
 *
 * The #GObject type for #VogueAttrList.
 */
#define PANGO_TYPE_ATTR_LIST vogue_attr_list_get_type ()
/**
 * VogueAttrIterator:
 *
 * The #VogueAttrIterator structure is used to represent an
 * iterator through a #VogueAttrList. A new iterator is created
 * with vogue_attr_list_get_iterator(). Once the iterator
 * is created, it can be advanced through the style changes
 * in the text using vogue_attr_iterator_next(). At each
 * style change, the range of the current style segment and the
 * attributes currently in effect can be queried.
 */
/**
 * VogueAttrList:
 *
 * The #VogueAttrList structure represents a list of attributes
 * that apply to a section of text. The attributes are, in general,
 * allowed to overlap in an arbitrary fashion, however, if the
 * attributes are manipulated only through vogue_attr_list_change(),
 * the overlap between properties will meet stricter criteria.
 *
 * Since the #VogueAttrList structure is stored as a linear list,
 * it is not suitable for storing attributes for large amounts
 * of text. In general, you should not use a single #VogueAttrList
 * for more than one paragraph of text.
 */
typedef struct _VogueAttrList     VogueAttrList;
typedef struct _VogueAttrIterator VogueAttrIterator;

/**
 * VogueAttrType:
 * @PANGO_ATTR_INVALID: does not happen
 * @PANGO_ATTR_LANGUAGE: language (#VogueAttrLanguage)
 * @PANGO_ATTR_FAMILY: font family name list (#VogueAttrString)
 * @PANGO_ATTR_STYLE: font slant style (#VogueAttrInt)
 * @PANGO_ATTR_WEIGHT: font weight (#VogueAttrInt)
 * @PANGO_ATTR_VARIANT: font variant (normal or small caps) (#VogueAttrInt)
 * @PANGO_ATTR_STRETCH: font stretch (#VogueAttrInt)
 * @PANGO_ATTR_SIZE: font size in points scaled by %PANGO_SCALE (#VogueAttrInt)
 * @PANGO_ATTR_FONT_DESC: font description (#VogueAttrFontDesc)
 * @PANGO_ATTR_FOREGROUND: foreground color (#VogueAttrColor)
 * @PANGO_ATTR_BACKGROUND: background color (#VogueAttrColor)
 * @PANGO_ATTR_UNDERLINE: whether the text has an underline (#VogueAttrInt)
 * @PANGO_ATTR_STRIKETHROUGH: whether the text is struck-through (#VogueAttrInt)
 * @PANGO_ATTR_RISE: baseline displacement (#VogueAttrInt)
 * @PANGO_ATTR_SHAPE: shape (#VogueAttrShape)
 * @PANGO_ATTR_SCALE: font size scale factor (#VogueAttrFloat)
 * @PANGO_ATTR_FALLBACK: whether fallback is enabled (#VogueAttrInt)
 * @PANGO_ATTR_LETTER_SPACING: letter spacing (#VogueAttrInt)
 * @PANGO_ATTR_UNDERLINE_COLOR: underline color (#VogueAttrColor)
 * @PANGO_ATTR_STRIKETHROUGH_COLOR: strikethrough color (#VogueAttrColor)
 * @PANGO_ATTR_ABSOLUTE_SIZE: font size in pixels scaled by %PANGO_SCALE (#VogueAttrInt)
 * @PANGO_ATTR_GRAVITY: base text gravity (#VogueAttrInt)
 * @PANGO_ATTR_GRAVITY_HINT: gravity hint (#VogueAttrInt)
 * @PANGO_ATTR_FONT_FEATURES: OpenType font features (#VogueAttrString). Since 1.38
 * @PANGO_ATTR_FOREGROUND_ALPHA: foreground alpha (#VogueAttrInt). Since 1.38
 * @PANGO_ATTR_BACKGROUND_ALPHA: background alpha (#VogueAttrInt). Since 1.38
 * @PANGO_ATTR_ALLOW_BREAKS: whether breaks are allowed (#VogueAttrInt). Since 1.44
 * @PANGO_ATTR_SHOW: how to render invisible characters (#VogueAttrInt). Since 1.44
 * @PANGO_ATTR_INSERT_HYPHENS: whether to insert hyphens at intra-word line breaks (#VogueAttrInt). Since 1.44
 *
 * The #VogueAttrType
 * distinguishes between different types of attributes. Along with the
 * predefined values, it is possible to allocate additional values
 * for custom attributes using vogue_attr_type_register(). The predefined
 * values are given below. The type of structure used to store the
 * attribute is listed in parentheses after the description.
 */
typedef enum
{
  PANGO_ATTR_INVALID,           /* 0 is an invalid attribute type */
  PANGO_ATTR_LANGUAGE,		/* VogueAttrLanguage */
  PANGO_ATTR_FAMILY,		/* VogueAttrString */
  PANGO_ATTR_STYLE,		/* VogueAttrInt */
  PANGO_ATTR_WEIGHT,		/* VogueAttrInt */
  PANGO_ATTR_VARIANT,		/* VogueAttrInt */
  PANGO_ATTR_STRETCH,		/* VogueAttrInt */
  PANGO_ATTR_SIZE,		/* VogueAttrSize */
  PANGO_ATTR_FONT_DESC,		/* VogueAttrFontDesc */
  PANGO_ATTR_FOREGROUND,	/* VogueAttrColor */
  PANGO_ATTR_BACKGROUND,	/* VogueAttrColor */
  PANGO_ATTR_UNDERLINE,		/* VogueAttrInt */
  PANGO_ATTR_STRIKETHROUGH,	/* VogueAttrInt */
  PANGO_ATTR_RISE,		/* VogueAttrInt */
  PANGO_ATTR_SHAPE,		/* VogueAttrShape */
  PANGO_ATTR_SCALE,             /* VogueAttrFloat */
  PANGO_ATTR_FALLBACK,          /* VogueAttrInt */
  PANGO_ATTR_LETTER_SPACING,    /* VogueAttrInt */
  PANGO_ATTR_UNDERLINE_COLOR,	/* VogueAttrColor */
  PANGO_ATTR_STRIKETHROUGH_COLOR,/* VogueAttrColor */
  PANGO_ATTR_ABSOLUTE_SIZE,	/* VogueAttrSize */
  PANGO_ATTR_GRAVITY,		/* VogueAttrInt */
  PANGO_ATTR_GRAVITY_HINT,	/* VogueAttrInt */
  PANGO_ATTR_FONT_FEATURES,	/* VogueAttrString */
  PANGO_ATTR_FOREGROUND_ALPHA,	/* VogueAttrInt */
  PANGO_ATTR_BACKGROUND_ALPHA,	/* VogueAttrInt */
  PANGO_ATTR_ALLOW_BREAKS,	/* VogueAttrInt */
  PANGO_ATTR_SHOW,		/* VogueAttrInt */
  PANGO_ATTR_INSERT_HYPHENS,	/* VogueAttrInt */
} VogueAttrType;

/**
 * VogueUnderline:
 * @PANGO_UNDERLINE_NONE: no underline should be drawn
 * @PANGO_UNDERLINE_SINGLE: a single underline should be drawn
 * @PANGO_UNDERLINE_DOUBLE: a double underline should be drawn
 * @PANGO_UNDERLINE_LOW: a single underline should be drawn at a
 *     position beneath the ink extents of the text being
 *     underlined. This should be used only for underlining
 *     single characters, such as for keyboard accelerators.
 *     %PANGO_UNDERLINE_SINGLE should be used for extended
 *     portions of text.
 * @PANGO_UNDERLINE_ERROR: a wavy underline should be drawn below.
 *     This underline is typically used to indicate an error such
 *     as a possible mispelling; in some cases a contrasting color
 *     may automatically be used. This type of underlining is
 *     available since Vogue 1.4.
 *
 * The #VogueUnderline enumeration is used to specify
 * whether text should be underlined, and if so, the type
 * of underlining.
 */
typedef enum {
  PANGO_UNDERLINE_NONE,
  PANGO_UNDERLINE_SINGLE,
  PANGO_UNDERLINE_DOUBLE,
  PANGO_UNDERLINE_LOW,
  PANGO_UNDERLINE_ERROR
} VogueUnderline;

/**
 * PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING:
 *
 * This value can be used to set the start_index member of a #VogueAttribute
 * such that the attribute covers from the beginning of the text.
 *
 * Since: 1.24
 */
/**
 * PANGO_ATTR_INDEX_TO_TEXT_END:
 *
 * This value can be used to set the end_index member of a #VogueAttribute
 * such that the attribute covers to the end of the text.
 *
 * Since: 1.24
 */
#define PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING	0
#define PANGO_ATTR_INDEX_TO_TEXT_END		G_MAXUINT

/**
 * VogueAttribute:
 * @klass: the class structure holding information about the type of the attribute
 * @start_index: the start index of the range (in bytes).
 * @end_index: end index of the range (in bytes). The character at this index
 * is not included in the range.
 *
 * The #VogueAttribute structure represents the common portions of all
 * attributes. Particular types of attributes include this structure
 * as their initial portion. The common portion of the attribute holds
 * the range to which the value in the type-specific part of the attribute
 * applies and should be initialized using vogue_attribute_init().
 * By default an attribute will have an all-inclusive range of [0,%G_MAXUINT].
 */
struct _VogueAttribute
{
  const VogueAttrClass *klass;
  guint start_index;	/* in bytes */
  guint end_index;	/* in bytes. The character at this index is not included */
};

/**
 * VogueAttrFilterFunc:
 * @attribute: a Vogue attribute
 * @user_data: user data passed to the function
 *
 * Type of a function filtering a list of attributes.
 *
 * Return value: %TRUE if the attribute should be selected for
 * filtering, %FALSE otherwise.
 **/
typedef gboolean (*VogueAttrFilterFunc) (VogueAttribute *attribute,
					 gpointer        user_data);

/**
 * VogueAttrDataCopyFunc:
 * @user_data: user data to copy
 *
 * Type of a function that can duplicate user data for an attribute.
 *
 * Return value: new copy of @user_data.
 **/
typedef gpointer (*VogueAttrDataCopyFunc) (gconstpointer user_data);

/**
 * VogueAttrClass:
 * @type: the type ID for this attribute
 * @copy: function to duplicate an attribute of this type (see vogue_attribute_copy())
 * @destroy: function to free an attribute of this type (see vogue_attribute_destroy())
 * @equal: function to check two attributes of this type for equality (see vogue_attribute_equal())
 *
 * The #VogueAttrClass structure stores the type and operations for
 * a particular type of attribute. The functions in this structure should
 * not be called directly. Instead, one should use the wrapper functions
 * provided for #VogueAttribute.
 */
struct _VogueAttrClass
{
  /*< public >*/
  VogueAttrType type;
  VogueAttribute * (*copy) (const VogueAttribute *attr);
  void             (*destroy) (VogueAttribute *attr);
  gboolean         (*equal) (const VogueAttribute *attr1, const VogueAttribute *attr2);
};

/**
 * VogueAttrString:
 * @attr: the common portion of the attribute
 * @value: the string which is the value of the attribute
 *
 * The #VogueAttrString structure is used to represent attributes with
 * a string value.
 */
struct _VogueAttrString
{
  VogueAttribute attr;
  char *value;
};
/**
 * VogueAttrLanguage:
 * @attr: the common portion of the attribute
 * @value: the #VogueLanguage which is the value of the attribute
 *
 * The #VogueAttrLanguage structure is used to represent attributes that
 * are languages.
 */
struct _VogueAttrLanguage
{
  VogueAttribute attr;
  VogueLanguage *value;
};
/**
 * VogueAttrInt:
 * @attr: the common portion of the attribute
 * @value: the value of the attribute
 *
 * The #VogueAttrInt structure is used to represent attributes with
 * an integer or enumeration value.
 */
struct _VogueAttrInt
{
  VogueAttribute attr;
  int value;
};
/**
 * VogueAttrFloat:
 * @attr: the common portion of the attribute
 * @value: the value of the attribute
 *
 * The #VogueAttrFloat structure is used to represent attributes with
 * a float or double value.
 */
struct _VogueAttrFloat
{
  VogueAttribute attr;
  double value;
};
/**
 * VogueAttrColor:
 * @attr: the common portion of the attribute
 * @color: the #VogueColor which is the value of the attribute
 *
 * The #VogueAttrColor structure is used to represent attributes that
 * are colors.
 */
struct _VogueAttrColor
{
  VogueAttribute attr;
  VogueColor color;
};

/**
 * VogueAttrSize:
 * @attr: the common portion of the attribute
 * @size: size of font, in units of 1/%PANGO_SCALE of a point (for
 * %PANGO_ATTR_SIZE) or of a device uni (for %PANGO_ATTR_ABSOLUTE_SIZE)
 * @absolute: whether the font size is in device units or points.
 * This field is only present for compatibility with Vogue-1.8.0
 * (%PANGO_ATTR_ABSOLUTE_SIZE was added in 1.8.1); and always will
 * be %FALSE for %PANGO_ATTR_SIZE and %TRUE for %PANGO_ATTR_ABSOLUTE_SIZE.
 *
 * The #VogueAttrSize structure is used to represent attributes which
 * set font size.
 */
struct _VogueAttrSize
{
  VogueAttribute attr;
  int size;
  guint absolute : 1;
};

/**
 * VogueAttrShape:
 * @attr: the common portion of the attribute
 * @ink_rect: the ink rectangle to restrict to
 * @logical_rect: the logical rectangle to restrict to
 * @data: user data set (see vogue_attr_shape_new_with_data())
 * @copy_func: copy function for the user data
 * @destroy_func: destroy function for the user data
 *
 * The #VogueAttrShape structure is used to represent attributes which
 * impose shape restrictions.
 */
struct _VogueAttrShape
{
  VogueAttribute attr;
  VogueRectangle ink_rect;
  VogueRectangle logical_rect;

  gpointer              data;
  VogueAttrDataCopyFunc copy_func;
  GDestroyNotify        destroy_func;
};

/**
 * VogueAttrFontDesc:
 * @attr: the common portion of the attribute
 * @desc: the font description which is the value of this attribute
 *
 * The #VogueAttrFontDesc structure is used to store an attribute that
 * sets all aspects of the font description at once.
 */
struct _VogueAttrFontDesc
{
  VogueAttribute attr;
  VogueFontDescription *desc;
};

/**
 * VogueAttrFontFeatures:
 * @attr: the common portion of the attribute
 * @features: the featues, as a string in CSS syntax
 *
 * The #VogueAttrFontFeatures structure is used to represent OpenType
 * font features as an attribute.
 *
 * Since: 1.38
 */
struct _VogueAttrFontFeatures
{
  VogueAttribute attr;
  gchar *features;
};

PANGO_AVAILABLE_IN_ALL
VogueAttrType         vogue_attr_type_register (const gchar        *name);
PANGO_AVAILABLE_IN_1_22
const char *          vogue_attr_type_get_name (VogueAttrType       type) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_20
void             vogue_attribute_init        (VogueAttribute       *attr,
					      const VogueAttrClass *klass);
PANGO_AVAILABLE_IN_ALL
VogueAttribute * vogue_attribute_copy        (const VogueAttribute *attr);
PANGO_AVAILABLE_IN_ALL
void             vogue_attribute_destroy     (VogueAttribute       *attr);
PANGO_AVAILABLE_IN_ALL
gboolean         vogue_attribute_equal       (const VogueAttribute *attr1,
					      const VogueAttribute *attr2) G_GNUC_PURE;

PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_language_new      (VogueLanguage              *language);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_family_new        (const char                 *family);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_foreground_new    (guint16                     red,
					      guint16                     green,
					      guint16                     blue);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_background_new    (guint16                     red,
					      guint16                     green,
					      guint16                     blue);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_size_new          (int                         size);
PANGO_AVAILABLE_IN_1_8
VogueAttribute *vogue_attr_size_new_absolute (int                         size);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_style_new         (VogueStyle                  style);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_weight_new        (VogueWeight                 weight);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_variant_new       (VogueVariant                variant);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_stretch_new       (VogueStretch                stretch);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_font_desc_new     (const VogueFontDescription *desc);

PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_underline_new           (VogueUnderline underline);
PANGO_AVAILABLE_IN_1_8
VogueAttribute *vogue_attr_underline_color_new     (guint16        red,
						    guint16        green,
						    guint16        blue);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_strikethrough_new       (gboolean       strikethrough);
PANGO_AVAILABLE_IN_1_8
VogueAttribute *vogue_attr_strikethrough_color_new (guint16        red,
						    guint16        green,
						    guint16        blue);

PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_rise_new          (int                         rise);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_scale_new         (double                      scale_factor);
PANGO_AVAILABLE_IN_1_4
VogueAttribute *vogue_attr_fallback_new      (gboolean                    enable_fallback);
PANGO_AVAILABLE_IN_1_6
VogueAttribute *vogue_attr_letter_spacing_new (int                        letter_spacing);

PANGO_AVAILABLE_IN_ALL
VogueAttribute *vogue_attr_shape_new           (const VogueRectangle       *ink_rect,
						const VogueRectangle       *logical_rect);
PANGO_AVAILABLE_IN_1_8
VogueAttribute *vogue_attr_shape_new_with_data (const VogueRectangle       *ink_rect,
						const VogueRectangle       *logical_rect,
						gpointer                    data,
						VogueAttrDataCopyFunc       copy_func,
						GDestroyNotify              destroy_func);

PANGO_AVAILABLE_IN_1_16
VogueAttribute *vogue_attr_gravity_new      (VogueGravity     gravity);
PANGO_AVAILABLE_IN_1_16
VogueAttribute *vogue_attr_gravity_hint_new (VogueGravityHint hint);
PANGO_AVAILABLE_IN_1_38
VogueAttribute *vogue_attr_font_features_new (const gchar *features);
PANGO_AVAILABLE_IN_1_38
VogueAttribute *vogue_attr_foreground_alpha_new (guint16 alpha);
PANGO_AVAILABLE_IN_1_38
VogueAttribute *vogue_attr_background_alpha_new (guint16 alpha);
PANGO_AVAILABLE_IN_1_44
VogueAttribute *vogue_attr_allow_breaks_new     (gboolean allow_breaks);
PANGO_AVAILABLE_IN_1_44
VogueAttribute *vogue_attr_insert_hyphens_new   (gboolean insert_hyphens);

/**
 * VogueShowFlags:
 * @PANGO_SHOW_NONE: No special treatment for invisible characters
 * @PANGO_SHOW_SPACES: Render spaces, tabs and newlines visibly
 * @PANGO_SHOW_LINE_BREAKS: Render line breaks visibly
 * @PANGO_SHOW_IGNORABLES: Render default-ignorable Unicode
 *      characters visibly
 *
 * These flags affect how Vogue treats characters that are normally
 * not visible in the output.
 */
typedef enum {
  PANGO_SHOW_NONE        = 0,
  PANGO_SHOW_SPACES      = 1 << 0,
  PANGO_SHOW_LINE_BREAKS = 1 << 1,
  PANGO_SHOW_IGNORABLES  = 1 << 2
} VogueShowFlags;

PANGO_AVAILABLE_IN_1_44
VogueAttribute *vogue_attr_show_new              (VogueShowFlags flags);

PANGO_AVAILABLE_IN_ALL
GType              vogue_attr_list_get_type      (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
VogueAttrList *    vogue_attr_list_new           (void);
PANGO_AVAILABLE_IN_1_10
VogueAttrList *    vogue_attr_list_ref           (VogueAttrList  *list);
PANGO_AVAILABLE_IN_ALL
void               vogue_attr_list_unref         (VogueAttrList  *list);
PANGO_AVAILABLE_IN_ALL
VogueAttrList *    vogue_attr_list_copy          (VogueAttrList  *list);
PANGO_AVAILABLE_IN_ALL
void               vogue_attr_list_insert        (VogueAttrList  *list,
						  VogueAttribute *attr);
PANGO_AVAILABLE_IN_ALL
void               vogue_attr_list_insert_before (VogueAttrList  *list,
						  VogueAttribute *attr);
PANGO_AVAILABLE_IN_ALL
void               vogue_attr_list_change        (VogueAttrList  *list,
						  VogueAttribute *attr);
PANGO_AVAILABLE_IN_ALL
void               vogue_attr_list_splice        (VogueAttrList  *list,
						  VogueAttrList  *other,
						  gint            pos,
						  gint            len);
PANGO_AVAILABLE_IN_1_44
void               vogue_attr_list_update        (VogueAttrList  *list,
                                                  int             pos,
                                                  int             remove,
                                                  int             add);

PANGO_AVAILABLE_IN_1_2
VogueAttrList *vogue_attr_list_filter (VogueAttrList       *list,
				       VogueAttrFilterFunc  func,
				       gpointer             data);

PANGO_AVAILABLE_IN_1_44
GSList        *vogue_attr_list_get_attributes    (VogueAttrList *list);

PANGO_AVAILABLE_IN_1_44
GType              vogue_attr_iterator_get_type  (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
VogueAttrIterator *vogue_attr_list_get_iterator  (VogueAttrList  *list);

PANGO_AVAILABLE_IN_ALL
void               vogue_attr_iterator_range    (VogueAttrIterator     *iterator,
						 gint                  *start,
						 gint                  *end);
PANGO_AVAILABLE_IN_ALL
gboolean           vogue_attr_iterator_next     (VogueAttrIterator     *iterator);
PANGO_AVAILABLE_IN_ALL
VogueAttrIterator *vogue_attr_iterator_copy     (VogueAttrIterator     *iterator);
PANGO_AVAILABLE_IN_ALL
void               vogue_attr_iterator_destroy  (VogueAttrIterator     *iterator);
PANGO_AVAILABLE_IN_ALL
VogueAttribute *   vogue_attr_iterator_get      (VogueAttrIterator     *iterator,
						 VogueAttrType          type);
PANGO_AVAILABLE_IN_ALL
void               vogue_attr_iterator_get_font (VogueAttrIterator     *iterator,
						 VogueFontDescription  *desc,
						 VogueLanguage        **language,
						 GSList               **extra_attrs);
PANGO_AVAILABLE_IN_1_2
GSList *          vogue_attr_iterator_get_attrs (VogueAttrIterator     *iterator);


PANGO_AVAILABLE_IN_ALL
gboolean vogue_parse_markup (const char                 *markup_text,
			     int                         length,
			     gunichar                    accel_marker,
			     VogueAttrList             **attr_list,
			     char                      **text,
			     gunichar                   *accel_char,
			     GError                    **error);

PANGO_AVAILABLE_IN_1_32
GMarkupParseContext * vogue_markup_parser_new (gunichar               accel_marker);
PANGO_AVAILABLE_IN_1_32
gboolean              vogue_markup_parser_finish (GMarkupParseContext   *context,
                                                  VogueAttrList        **attr_list,
                                                  char                 **text,
                                                  gunichar              *accel_char,
                                                  GError               **error);

G_END_DECLS

#endif /* __PANGO_ATTRIBUTES_H__ */
