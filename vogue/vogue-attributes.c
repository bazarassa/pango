/* Vogue
 * vogue-attributes.c: Attributed text
 *
 * Copyright (C) 2000-2002 Red Hat Software
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
 * SECTION:text-attributes
 * @short_description:Font and other attributes for annotating text
 * @title:Attributes
 *
 * Attributed text is used in a number of places in Vogue. It
 * is used as the input to the itemization process and also when
 * creating a #VogueLayout. The data types and functions in
 * this section are used to represent and manipulate sets
 * of attributes applied to a portion of text.
 */
#include "config.h"
#include <string.h>

#include "vogue-attributes.h"
#include "vogue-impl-utils.h"

struct _VogueAttrList
{
  guint ref_count;
  GSList *attributes;
  GSList *attributes_tail;
};

struct _VogueAttrIterator
{
  GSList *next_attribute;
  GList *attribute_stack;
  guint start_index;
  guint end_index;
};

static VogueAttribute *vogue_attr_color_new         (const VogueAttrClass *klass,
						     guint16               red,
						     guint16               green,
						     guint16               blue);
static VogueAttribute *vogue_attr_string_new        (const VogueAttrClass *klass,
						     const char           *str);
static VogueAttribute *vogue_attr_int_new           (const VogueAttrClass *klass,
						     int                   value);
static VogueAttribute *vogue_attr_float_new         (const VogueAttrClass *klass,
						     double                value);
static VogueAttribute *vogue_attr_size_new_internal (int                   size,
						     gboolean              absolute);


G_LOCK_DEFINE_STATIC (attr_type);
static GHashTable *name_map = NULL; /* MT-safe */

/**
 * vogue_attr_type_register:
 * @name: an identifier for the type
 *
 * Allocate a new attribute type ID.  The attribute type name can be accessed
 * later by using vogue_attr_type_get_name().
 *
 * Return value: the new type ID.
 **/
VogueAttrType
vogue_attr_type_register (const gchar *name)
{
  static guint current_type = 0x1000000; /* MT-safe */
  guint type;

  G_LOCK (attr_type);

  type = current_type++;

  if (name)
    {
      if (G_UNLIKELY (!name_map))
	name_map = g_hash_table_new (NULL, NULL);

      g_hash_table_insert (name_map, GUINT_TO_POINTER (type), (gpointer) g_intern_string (name));
    }

  G_UNLOCK (attr_type);

  return type;
}

/**
 * vogue_attr_type_get_name:
 * @type: an attribute type ID to fetch the name for
 *
 * Fetches the attribute type name passed in when registering the type using
 * vogue_attr_type_register().
 *
 * The returned value is an interned string (see g_intern_string() for what
 * that means) that should not be modified or freed.
 *
 * Return value: (nullable): the type ID name (which may be %NULL), or
 * %NULL if @type is a built-in Vogue attribute type or invalid.
 *
 * Since: 1.22
 **/
const char *
vogue_attr_type_get_name (VogueAttrType type)
{
  const char *result = NULL;

  G_LOCK (attr_type);

  if (name_map)
    result = g_hash_table_lookup (name_map, GUINT_TO_POINTER ((guint) type));

  G_UNLOCK (attr_type);

  return result;
}

/**
 * vogue_attribute_init:
 * @attr: a #VogueAttribute
 * @klass: a #VogueAttrClass
 *
 * Initializes @attr's klass to @klass,
 * it's start_index to %PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING
 * and end_index to %PANGO_ATTR_INDEX_TO_TEXT_END
 * such that the attribute applies
 * to the entire text by default.
 *
 * Since: 1.20
 **/
void
vogue_attribute_init (VogueAttribute       *attr,
		      const VogueAttrClass *klass)
{
  g_return_if_fail (attr != NULL);
  g_return_if_fail (klass != NULL);

  attr->klass = klass;
  attr->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
  attr->end_index   = PANGO_ATTR_INDEX_TO_TEXT_END;
}

/**
 * vogue_attribute_copy:
 * @attr: a #VogueAttribute
 *
 * Make a copy of an attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attribute_copy (const VogueAttribute *attr)
{
  VogueAttribute *result;

  g_return_val_if_fail (attr != NULL, NULL);

  result = attr->klass->copy (attr);
  result->start_index = attr->start_index;
  result->end_index = attr->end_index;

  return result;
}

/**
 * vogue_attribute_destroy:
 * @attr: a #VogueAttribute.
 *
 * Destroy a #VogueAttribute and free all associated memory.
 **/
void
vogue_attribute_destroy (VogueAttribute *attr)
{
  g_return_if_fail (attr != NULL);

  attr->klass->destroy (attr);
}

G_DEFINE_BOXED_TYPE (VogueAttribute, vogue_attribute,
                     vogue_attribute_copy,
                     vogue_attribute_destroy);

/**
 * vogue_attribute_equal:
 * @attr1: a #VogueAttribute
 * @attr2: another #VogueAttribute
 *
 * Compare two attributes for equality. This compares only the
 * actual value of the two attributes and not the ranges that the
 * attributes apply to.
 *
 * Return value: %TRUE if the two attributes have the same value.
 **/
gboolean
vogue_attribute_equal (const VogueAttribute *attr1,
		       const VogueAttribute *attr2)
{
  g_return_val_if_fail (attr1 != NULL, FALSE);
  g_return_val_if_fail (attr2 != NULL, FALSE);

  if (attr1->klass->type != attr2->klass->type)
    return FALSE;

  return attr1->klass->equal (attr1, attr2);
}

static VogueAttribute *
vogue_attr_string_copy (const VogueAttribute *attr)
{
  return vogue_attr_string_new (attr->klass, ((VogueAttrString *)attr)->value);
}

static void
vogue_attr_string_destroy (VogueAttribute *attr)
{
  VogueAttrString *sattr = (VogueAttrString *)attr;

  g_free (sattr->value);
  g_slice_free (VogueAttrString, sattr);
}

static gboolean
vogue_attr_string_equal (const VogueAttribute *attr1,
			 const VogueAttribute *attr2)
{
  return strcmp (((VogueAttrString *)attr1)->value, ((VogueAttrString *)attr2)->value) == 0;
}

static VogueAttribute *
vogue_attr_string_new (const VogueAttrClass *klass,
		       const char           *str)
{
  VogueAttrString *result = g_slice_new (VogueAttrString);
  vogue_attribute_init (&result->attr, klass);
  result->value = g_strdup (str);

  return (VogueAttribute *)result;
}

/**
 * vogue_attr_family_new:
 * @family: the family or comma separated list of families
 *
 * Create a new font family attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_family_new (const char *family)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_FAMILY,
    vogue_attr_string_copy,
    vogue_attr_string_destroy,
    vogue_attr_string_equal
  };

  g_return_val_if_fail (family != NULL, NULL);

  return vogue_attr_string_new (&klass, family);
}

static VogueAttribute *
vogue_attr_language_copy (const VogueAttribute *attr)
{
  return vogue_attr_language_new (((VogueAttrLanguage *)attr)->value);
}

static void
vogue_attr_language_destroy (VogueAttribute *attr)
{
  VogueAttrLanguage *lattr = (VogueAttrLanguage *)attr;

  g_slice_free (VogueAttrLanguage, lattr);
}

static gboolean
vogue_attr_language_equal (const VogueAttribute *attr1,
			   const VogueAttribute *attr2)
{
  return ((VogueAttrLanguage *)attr1)->value == ((VogueAttrLanguage *)attr2)->value;
}

/**
 * vogue_attr_language_new:
 * @language: language tag
 *
 * Create a new language tag attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_language_new (VogueLanguage *language)
{
  VogueAttrLanguage *result;

  static const VogueAttrClass klass = {
    PANGO_ATTR_LANGUAGE,
    vogue_attr_language_copy,
    vogue_attr_language_destroy,
    vogue_attr_language_equal
  };

  result = g_slice_new (VogueAttrLanguage);
  vogue_attribute_init (&result->attr, &klass);
  result->value = language;

  return (VogueAttribute *)result;
}

static VogueAttribute *
vogue_attr_color_copy (const VogueAttribute *attr)
{
  const VogueAttrColor *color_attr = (VogueAttrColor *)attr;

  return vogue_attr_color_new (attr->klass,
			       color_attr->color.red,
			       color_attr->color.green,
			       color_attr->color.blue);
}

static void
vogue_attr_color_destroy (VogueAttribute *attr)
{
  VogueAttrColor *cattr = (VogueAttrColor *)attr;

  g_slice_free (VogueAttrColor, cattr);
}

static gboolean
vogue_attr_color_equal (const VogueAttribute *attr1,
			const VogueAttribute *attr2)
{
  const VogueAttrColor *color_attr1 = (const VogueAttrColor *)attr1;
  const VogueAttrColor *color_attr2 = (const VogueAttrColor *)attr2;

  return (color_attr1->color.red == color_attr2->color.red &&
	  color_attr1->color.blue == color_attr2->color.blue &&
	  color_attr1->color.green == color_attr2->color.green);
}

static VogueAttribute *
vogue_attr_color_new (const VogueAttrClass *klass,
		      guint16               red,
		      guint16               green,
		      guint16               blue)
{
  VogueAttrColor *result = g_slice_new (VogueAttrColor);
  vogue_attribute_init (&result->attr, klass);
  result->color.red = red;
  result->color.green = green;
  result->color.blue = blue;

  return (VogueAttribute *)result;
}

/**
 * vogue_attr_foreground_new:
 * @red: the red value (ranging from 0 to 65535)
 * @green: the green value
 * @blue: the blue value
 *
 * Create a new foreground color attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_foreground_new (guint16 red,
			   guint16 green,
			   guint16 blue)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_FOREGROUND,
    vogue_attr_color_copy,
    vogue_attr_color_destroy,
    vogue_attr_color_equal
  };

  return vogue_attr_color_new (&klass, red, green, blue);
}

/**
 * vogue_attr_background_new:
 * @red: the red value (ranging from 0 to 65535)
 * @green: the green value
 * @blue: the blue value
 *
 * Create a new background color attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_background_new (guint16 red,
			   guint16 green,
			   guint16 blue)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_BACKGROUND,
    vogue_attr_color_copy,
    vogue_attr_color_destroy,
    vogue_attr_color_equal
  };

  return vogue_attr_color_new (&klass, red, green, blue);
}

static VogueAttribute *
vogue_attr_int_copy (const VogueAttribute *attr)
{
  const VogueAttrInt *int_attr = (VogueAttrInt *)attr;

  return vogue_attr_int_new (attr->klass, int_attr->value);
}

static void
vogue_attr_int_destroy (VogueAttribute *attr)
{
  VogueAttrInt *iattr = (VogueAttrInt *)attr;

  g_slice_free (VogueAttrInt, iattr);
}

static gboolean
vogue_attr_int_equal (const VogueAttribute *attr1,
		      const VogueAttribute *attr2)
{
  const VogueAttrInt *int_attr1 = (const VogueAttrInt *)attr1;
  const VogueAttrInt *int_attr2 = (const VogueAttrInt *)attr2;

  return (int_attr1->value == int_attr2->value);
}

static VogueAttribute *
vogue_attr_int_new (const VogueAttrClass *klass,
		    int                   value)
{
  VogueAttrInt *result = g_slice_new (VogueAttrInt);
  vogue_attribute_init (&result->attr, klass);
  result->value = value;

  return (VogueAttribute *)result;
}

static VogueAttribute *
vogue_attr_float_copy (const VogueAttribute *attr)
{
  const VogueAttrFloat *float_attr = (VogueAttrFloat *)attr;

  return vogue_attr_float_new (attr->klass, float_attr->value);
}

static void
vogue_attr_float_destroy (VogueAttribute *attr)
{
  VogueAttrFloat *fattr = (VogueAttrFloat *)attr;

  g_slice_free (VogueAttrFloat, fattr);
}

static gboolean
vogue_attr_float_equal (const VogueAttribute *attr1,
			const VogueAttribute *attr2)
{
  const VogueAttrFloat *float_attr1 = (const VogueAttrFloat *)attr1;
  const VogueAttrFloat *float_attr2 = (const VogueAttrFloat *)attr2;

  return (float_attr1->value == float_attr2->value);
}

static VogueAttribute*
vogue_attr_float_new  (const VogueAttrClass *klass,
		       double                value)
{
  VogueAttrFloat *result = g_slice_new (VogueAttrFloat);
  vogue_attribute_init (&result->attr, klass);
  result->value = value;

  return (VogueAttribute *)result;
}

static VogueAttribute *
vogue_attr_size_copy (const VogueAttribute *attr)
{
  const VogueAttrSize *size_attr = (VogueAttrSize *)attr;

  if (attr->klass->type == PANGO_ATTR_ABSOLUTE_SIZE)
    return vogue_attr_size_new_absolute (size_attr->size);
  else
    return vogue_attr_size_new (size_attr->size);
}

static void
vogue_attr_size_destroy (VogueAttribute *attr)
{
  VogueAttrSize *sattr = (VogueAttrSize *)attr;

  g_slice_free (VogueAttrSize, sattr);
}

static gboolean
vogue_attr_size_equal (const VogueAttribute *attr1,
		       const VogueAttribute *attr2)
{
  const VogueAttrSize *size_attr1 = (const VogueAttrSize *)attr1;
  const VogueAttrSize *size_attr2 = (const VogueAttrSize *)attr2;

  return size_attr1->size == size_attr2->size;
}

static VogueAttribute *
vogue_attr_size_new_internal (int size,
			      gboolean absolute)
{
  VogueAttrSize *result;

  static const VogueAttrClass klass = {
    PANGO_ATTR_SIZE,
    vogue_attr_size_copy,
    vogue_attr_size_destroy,
    vogue_attr_size_equal
  };
  static const VogueAttrClass absolute_klass = {
    PANGO_ATTR_ABSOLUTE_SIZE,
    vogue_attr_size_copy,
    vogue_attr_size_destroy,
    vogue_attr_size_equal
  };

  result = g_slice_new (VogueAttrSize);
  vogue_attribute_init (&result->attr, absolute ? &absolute_klass : &klass);
  result->size = size;
  result->absolute = absolute;

  return (VogueAttribute *)result;
}

/**
 * vogue_attr_size_new:
 * @size: the font size, in %PANGO_SCALEths of a point.
 *
 * Create a new font-size attribute in fractional points.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_size_new (int size)
{
  return vogue_attr_size_new_internal (size, FALSE);
}

/**
 * vogue_attr_size_new_absolute:
 * @size: the font size, in %PANGO_SCALEths of a device unit.
 *
 * Create a new font-size attribute in device units.
 *
 * Return value: the newly allocated #VogueAttribute, which should be
 *               freed with vogue_attribute_destroy().
 *
 * Since: 1.8
 **/
VogueAttribute *
vogue_attr_size_new_absolute (int size)
{
  return vogue_attr_size_new_internal (size, TRUE);
}

/**
 * vogue_attr_style_new:
 * @style: the slant style
 *
 * Create a new font slant style attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_style_new (VogueStyle style)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_STYLE,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)style);
}

/**
 * vogue_attr_weight_new:
 * @weight: the weight
 *
 * Create a new font weight attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_weight_new (VogueWeight weight)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_WEIGHT,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)weight);
}

/**
 * vogue_attr_variant_new:
 * @variant: the variant
 *
 * Create a new font variant attribute (normal or small caps)
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_variant_new (VogueVariant variant)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_VARIANT,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)variant);
}

/**
 * vogue_attr_stretch_new:
 * @stretch: the stretch
 *
 * Create a new font stretch attribute
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_stretch_new (VogueStretch  stretch)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_STRETCH,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)stretch);
}

static VogueAttribute *
vogue_attr_font_desc_copy (const VogueAttribute *attr)
{
  const VogueAttrFontDesc *desc_attr = (const VogueAttrFontDesc *)attr;

  return vogue_attr_font_desc_new (desc_attr->desc);
}

static void
vogue_attr_font_desc_destroy (VogueAttribute *attr)
{
  VogueAttrFontDesc *desc_attr = (VogueAttrFontDesc *)attr;

  vogue_font_description_free (desc_attr->desc);
  g_slice_free (VogueAttrFontDesc, desc_attr);
}

static gboolean
vogue_attr_font_desc_equal (const VogueAttribute *attr1,
			    const VogueAttribute *attr2)
{
  const VogueAttrFontDesc *desc_attr1 = (const VogueAttrFontDesc *)attr1;
  const VogueAttrFontDesc *desc_attr2 = (const VogueAttrFontDesc *)attr2;

  return vogue_font_description_get_set_fields (desc_attr1->desc) ==
         vogue_font_description_get_set_fields (desc_attr2->desc) &&
	 vogue_font_description_equal (desc_attr1->desc, desc_attr2->desc);
}

/**
 * vogue_attr_font_desc_new:
 * @desc: the font description
 *
 * Create a new font description attribute. This attribute
 * allows setting family, style, weight, variant, stretch,
 * and size simultaneously.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_font_desc_new (const VogueFontDescription *desc)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_FONT_DESC,
    vogue_attr_font_desc_copy,
    vogue_attr_font_desc_destroy,
    vogue_attr_font_desc_equal
  };

  VogueAttrFontDesc *result = g_slice_new (VogueAttrFontDesc);
  vogue_attribute_init (&result->attr, &klass);
  result->desc = vogue_font_description_copy (desc);

  return (VogueAttribute *)result;
}


/**
 * vogue_attr_underline_new:
 * @underline: the underline style.
 *
 * Create a new underline-style attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_underline_new (VogueUnderline underline)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_UNDERLINE,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)underline);
}

/**
 * vogue_attr_underline_color_new:
 * @red: the red value (ranging from 0 to 65535)
 * @green: the green value
 * @blue: the blue value
 *
 * Create a new underline color attribute. This attribute
 * modifies the color of underlines. If not set, underlines
 * will use the foreground color.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.8
 **/
VogueAttribute *
vogue_attr_underline_color_new (guint16 red,
				guint16 green,
				guint16 blue)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_UNDERLINE_COLOR,
    vogue_attr_color_copy,
    vogue_attr_color_destroy,
    vogue_attr_color_equal
  };

  return vogue_attr_color_new (&klass, red, green, blue);
}

/**
 * vogue_attr_strikethrough_new:
 * @strikethrough: %TRUE if the text should be struck-through.
 *
 * Create a new strike-through attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_strikethrough_new (gboolean strikethrough)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_STRIKETHROUGH,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)strikethrough);
}

/**
 * vogue_attr_strikethrough_color_new:
 * @red: the red value (ranging from 0 to 65535)
 * @green: the green value
 * @blue: the blue value
 *
 * Create a new strikethrough color attribute. This attribute
 * modifies the color of strikethrough lines. If not set, strikethrough
 * lines will use the foreground color.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.8
 **/
VogueAttribute *
vogue_attr_strikethrough_color_new (guint16 red,
				    guint16 green,
				    guint16 blue)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_STRIKETHROUGH_COLOR,
    vogue_attr_color_copy,
    vogue_attr_color_destroy,
    vogue_attr_color_equal
  };

  return vogue_attr_color_new (&klass, red, green, blue);
}

/**
 * vogue_attr_rise_new:
 * @rise: the amount that the text should be displaced vertically,
 *        in Vogue units. Positive values displace the text upwards.
 *
 * Create a new baseline displacement attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_rise_new (int rise)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_RISE,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)rise);
}

/**
 * vogue_attr_scale_new:
 * @scale_factor: factor to scale the font
 *
 * Create a new font size scale attribute. The base font for the
 * affected text will have its size multiplied by @scale_factor.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute*
vogue_attr_scale_new (double scale_factor)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_SCALE,
    vogue_attr_float_copy,
    vogue_attr_float_destroy,
    vogue_attr_float_equal
  };

  return vogue_attr_float_new (&klass, scale_factor);
}

/**
 * vogue_attr_fallback_new:
 * @enable_fallback: %TRUE if we should fall back on other fonts
 *                   for characters the active font is missing.
 *
 * Create a new font fallback attribute.
 *
 * If fallback is disabled, characters will only be used from the
 * closest matching font on the system. No fallback will be done to
 * other fonts on the system that might contain the characters in the
 * text.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.4
 **/
VogueAttribute *
vogue_attr_fallback_new (gboolean enable_fallback)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_FALLBACK,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal,
  };

  return vogue_attr_int_new (&klass, (int)enable_fallback);
}

/**
 * vogue_attr_letter_spacing_new:
 * @letter_spacing: amount of extra space to add between graphemes
 *   of the text, in Vogue units.
 *
 * Create a new letter-spacing attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.6
 **/
VogueAttribute *
vogue_attr_letter_spacing_new (int letter_spacing)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_LETTER_SPACING,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, letter_spacing);
}

static VogueAttribute *
vogue_attr_shape_copy (const VogueAttribute *attr)
{
  const VogueAttrShape *shape_attr = (VogueAttrShape *)attr;
  gpointer data;

  if (shape_attr->copy_func)
    data = shape_attr->copy_func (shape_attr->data);
  else
    data = shape_attr->data;

  return vogue_attr_shape_new_with_data (&shape_attr->ink_rect, &shape_attr->logical_rect,
					 data, shape_attr->copy_func, shape_attr->destroy_func);
}

static void
vogue_attr_shape_destroy (VogueAttribute *attr)
{
  VogueAttrShape *shape_attr = (VogueAttrShape *)attr;

  if (shape_attr->destroy_func)
    shape_attr->destroy_func (shape_attr->data);

  g_slice_free (VogueAttrShape, shape_attr);
}

static gboolean
vogue_attr_shape_equal (const VogueAttribute *attr1,
			const VogueAttribute *attr2)
{
  const VogueAttrShape *shape_attr1 = (const VogueAttrShape *)attr1;
  const VogueAttrShape *shape_attr2 = (const VogueAttrShape *)attr2;

  return (shape_attr1->logical_rect.x == shape_attr2->logical_rect.x &&
	  shape_attr1->logical_rect.y == shape_attr2->logical_rect.y &&
	  shape_attr1->logical_rect.width == shape_attr2->logical_rect.width &&
	  shape_attr1->logical_rect.height == shape_attr2->logical_rect.height &&
	  shape_attr1->ink_rect.x == shape_attr2->ink_rect.x &&
	  shape_attr1->ink_rect.y == shape_attr2->ink_rect.y &&
	  shape_attr1->ink_rect.width == shape_attr2->ink_rect.width &&
	  shape_attr1->ink_rect.height == shape_attr2->ink_rect.height &&
	  shape_attr1->data == shape_attr2->data);
}

/**
 * vogue_attr_shape_new_with_data:
 * @ink_rect:     ink rectangle to assign to each character
 * @logical_rect: logical rectangle to assign to each character
 * @data:         user data pointer
 * @copy_func: (allow-none): function to copy @data when the
 *                attribute is copied. If %NULL, @data is simply
 *                copied as a pointer.
 * @destroy_func: (allow-none): function to free @data when the
 *                attribute is freed, or %NULL
 *
 * Like vogue_attr_shape_new(), but a user data pointer is also
 * provided; this pointer can be accessed when later
 * rendering the glyph.
 *
 * Return value: the newly allocated #VogueAttribute, which should be
 *               freed with vogue_attribute_destroy().
 *
 * Since: 1.8
 **/
VogueAttribute *
vogue_attr_shape_new_with_data (const VogueRectangle  *ink_rect,
				const VogueRectangle  *logical_rect,
				gpointer               data,
				VogueAttrDataCopyFunc  copy_func,
				GDestroyNotify         destroy_func)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_SHAPE,
    vogue_attr_shape_copy,
    vogue_attr_shape_destroy,
    vogue_attr_shape_equal
  };

  VogueAttrShape *result;

  g_return_val_if_fail (ink_rect != NULL, NULL);
  g_return_val_if_fail (logical_rect != NULL, NULL);

  result = g_slice_new (VogueAttrShape);
  vogue_attribute_init (&result->attr, &klass);
  result->ink_rect = *ink_rect;
  result->logical_rect = *logical_rect;
  result->data = data;
  result->copy_func = copy_func;
  result->destroy_func =  destroy_func;

  return (VogueAttribute *)result;
}

/**
 * vogue_attr_shape_new:
 * @ink_rect:     ink rectangle to assign to each character
 * @logical_rect: logical rectangle to assign to each character
 *
 * Create a new shape attribute. A shape is used to impose a
 * particular ink and logical rectangle on the result of shaping a
 * particular glyph. This might be used, for instance, for
 * embedding a picture or a widget inside a #VogueLayout.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 **/
VogueAttribute *
vogue_attr_shape_new (const VogueRectangle *ink_rect,
		      const VogueRectangle *logical_rect)
{
  g_return_val_if_fail (ink_rect != NULL, NULL);
  g_return_val_if_fail (logical_rect != NULL, NULL);

  return vogue_attr_shape_new_with_data (ink_rect, logical_rect,
					 NULL, NULL, NULL);
}

/**
 * vogue_attr_gravity_new:
 * @gravity: the gravity value; should not be %PANGO_GRAVITY_AUTO.
 *
 * Create a new gravity attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.16
 **/
VogueAttribute *
vogue_attr_gravity_new (VogueGravity gravity)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_GRAVITY,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  g_return_val_if_fail (gravity != PANGO_GRAVITY_AUTO, NULL);

  return vogue_attr_int_new (&klass, (int)gravity);
}

/**
 * vogue_attr_gravity_hint_new:
 * @hint: the gravity hint value.
 *
 * Create a new gravity hint attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.16
 **/
VogueAttribute *
vogue_attr_gravity_hint_new (VogueGravityHint hint)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_GRAVITY_HINT,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)hint);
}

/**
 * vogue_attr_font_features_new:
 * @features: a string with OpenType font features, in CSS syntax
 *
 * Create a new font features tag attribute.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.38
 **/
VogueAttribute *
vogue_attr_font_features_new (const gchar *features)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_FONT_FEATURES,
    vogue_attr_string_copy,
    vogue_attr_string_destroy,
    vogue_attr_string_equal
  };

  g_return_val_if_fail (features != NULL, NULL);

  return vogue_attr_string_new (&klass, features);
}

/**
 * vogue_attr_foreground_alpha_new:
 * @alpha: the alpha value, between 1 and 65536
 *
 * Create a new foreground alpha attribute.
 *
 * Return value: (transfer full): the new allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.38
 */
VogueAttribute *
vogue_attr_foreground_alpha_new (guint16 alpha)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_FOREGROUND_ALPHA,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)alpha);
}

/**
 * vogue_attr_background_alpha_new:
 * @alpha: the alpha value, between 1 and 65536
 *
 * Create a new background alpha attribute.
 *
 * Return value: (transfer full): the new allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.38
 */
VogueAttribute *
vogue_attr_background_alpha_new (guint16 alpha)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_BACKGROUND_ALPHA,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal
  };

  return vogue_attr_int_new (&klass, (int)alpha);
}

/**
 * vogue_attr_allow_breaks_new:
 * @allow_breaks: %TRUE if we line breaks are allowed
 *
 * Create a new allow-breaks attribute.
 *
 * If breaks are disabled, the range will be kept in a
 * single run, as far as possible.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy()
 *
 * Since: 1.44
 */
VogueAttribute *
vogue_attr_allow_breaks_new (gboolean allow_breaks)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_ALLOW_BREAKS,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal,
  };

  return vogue_attr_int_new (&klass, (int)allow_breaks);
}

/**
 * vogue_attr_insert_hyphens_new:
 * @insert_hyphens: %TRUE if hyphens should be inserted
 *
 * Create a new insert-hyphens attribute.
 *
 * Vogue will insert hyphens when breaking lines in the middle
 * of a word. This attribute can be used to suppress the hyphen.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy()
 *
 * Since: 1.44
 */
VogueAttribute *
vogue_attr_insert_hyphens_new (gboolean insert_hyphens)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_INSERT_HYPHENS,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal,
  };

  return vogue_attr_int_new (&klass, (int)insert_hyphens);
}

/**
 * vogue_attr_show_new:
 * @flags: #VogueShowFlags to apply
 *
 * Create a new attribute that influences how invisible
 * characters are rendered.
 *
 * Return value: (transfer full): the newly allocated #VogueAttribute,
 *               which should be freed with vogue_attribute_destroy().
 *
 * Since: 1.44
 **/
VogueAttribute *
vogue_attr_show_new (VogueShowFlags flags)
{
  static const VogueAttrClass klass = {
    PANGO_ATTR_SHOW,
    vogue_attr_int_copy,
    vogue_attr_int_destroy,
    vogue_attr_int_equal,
  };

  return vogue_attr_int_new (&klass, (int)flags);
}

/*
 * Attribute List
 */

G_DEFINE_BOXED_TYPE (VogueAttrList, vogue_attr_list,
                     vogue_attr_list_copy,
                     vogue_attr_list_unref);

/**
 * vogue_attr_list_new:
 *
 * Create a new empty attribute list with a reference count of one.
 *
 * Return value: (transfer full): the newly allocated #VogueAttrList,
 *               which should be freed with vogue_attr_list_unref().
 **/
VogueAttrList *
vogue_attr_list_new (void)
{
  VogueAttrList *list = g_slice_new (VogueAttrList);

  list->ref_count = 1;
  list->attributes = NULL;
  list->attributes_tail = NULL;

  return list;
}

/**
 * vogue_attr_list_ref:
 * @list: (nullable): a #VogueAttrList, may be %NULL
 *
 * Increase the reference count of the given attribute list by one.
 *
 * Return value: The attribute list passed in
 *
 * Since: 1.10
 **/
VogueAttrList *
vogue_attr_list_ref (VogueAttrList *list)
{
  if (list == NULL)
    return NULL;

  g_atomic_int_inc ((int *) &list->ref_count);

  return list;
}

/**
 * vogue_attr_list_unref:
 * @list: (nullable): a #VogueAttrList, may be %NULL
 *
 * Decrease the reference count of the given attribute list by one.
 * If the result is zero, free the attribute list and the attributes
 * it contains.
 **/
void
vogue_attr_list_unref (VogueAttrList *list)
{
  GSList *tmp_list;

  if (list == NULL)
    return;

  g_return_if_fail (list->ref_count > 0);

  if (g_atomic_int_dec_and_test ((int *) &list->ref_count))
    {
      tmp_list = list->attributes;
      while (tmp_list)
	{
	  VogueAttribute *attr = tmp_list->data;
	  tmp_list = tmp_list->next;

	  attr->klass->destroy (attr);
	}

      g_slist_free (list->attributes);

      g_slice_free (VogueAttrList, list);
    }
}

/**
 * vogue_attr_list_copy:
 * @list: (nullable): a #VogueAttrList, may be %NULL
 *
 * Copy @list and return an identical new list.
 *
 * Return value: (nullable): the newly allocated #VogueAttrList, with a
 *               reference count of one, which should
 *               be freed with vogue_attr_list_unref().
 *               Returns %NULL if @list was %NULL.
 **/
VogueAttrList *
vogue_attr_list_copy (VogueAttrList *list)
{
  VogueAttrList *new;
  GSList *iter;
  GSList *new_attrs;

  if (list == NULL)
    return NULL;

  new = vogue_attr_list_new ();

  iter = list->attributes;
  new_attrs = NULL;
  while (iter != NULL)
    {
      new_attrs = g_slist_prepend (new_attrs,
				   vogue_attribute_copy (iter->data));

      iter = g_slist_next (iter);
    }

  /* we're going to reverse the nodes, so head becomes tail */
  new->attributes_tail = new_attrs;
  new->attributes = g_slist_reverse (new_attrs);

  return new;
}

static void
vogue_attr_list_insert_internal (VogueAttrList  *list,
				 VogueAttribute *attr,
				 gboolean        before)
{
  GSList *tmp_list, *prev, *link;
  guint start_index = attr->start_index;

  if (!list->attributes)
    {
      list->attributes = g_slist_prepend (NULL, attr);
      list->attributes_tail = list->attributes;
    }
  else if (((VogueAttribute *)list->attributes_tail->data)->start_index < start_index ||
	   (!before && ((VogueAttribute *)list->attributes_tail->data)->start_index == start_index))
    {
      list->attributes_tail = g_slist_append (list->attributes_tail, attr);
      list->attributes_tail = list->attributes_tail->next;
      g_assert (list->attributes_tail);
    }
  else
    {
      prev = NULL;
      tmp_list = list->attributes;
      while (1)
	{
	  VogueAttribute *tmp_attr = tmp_list->data;

	  if (tmp_attr->start_index > start_index ||
	      (before && tmp_attr->start_index == start_index))
	    {
	      link = g_slist_alloc ();
	      link->next = tmp_list;
	      link->data = attr;

	      if (prev)
		prev->next = link;
	      else
		list->attributes = link;

	      break;
	    }

	  prev = tmp_list;
	  tmp_list = tmp_list->next;
	}
    }
}

/**
 * vogue_attr_list_insert:
 * @list: a #VogueAttrList
 * @attr: (transfer full): the attribute to insert. Ownership of this
 *        value is assumed by the list.
 *
 * Insert the given attribute into the #VogueAttrList. It will
 * be inserted after all other attributes with a matching
 * @start_index.
 **/
void
vogue_attr_list_insert (VogueAttrList  *list,
			VogueAttribute *attr)
{
  g_return_if_fail (list != NULL);
  g_return_if_fail (attr != NULL);

  vogue_attr_list_insert_internal (list, attr, FALSE);
}

/**
 * vogue_attr_list_insert_before:
 * @list: a #VogueAttrList
 * @attr: (transfer full): the attribute to insert. Ownership of this
 *        value is assumed by the list.
 *
 * Insert the given attribute into the #VogueAttrList. It will
 * be inserted before all other attributes with a matching
 * @start_index.
 **/
void
vogue_attr_list_insert_before (VogueAttrList  *list,
			       VogueAttribute *attr)
{
  g_return_if_fail (list != NULL);
  g_return_if_fail (attr != NULL);

  vogue_attr_list_insert_internal (list, attr, TRUE);
}

/**
 * vogue_attr_list_change:
 * @list: a #VogueAttrList
 * @attr: (transfer full): the attribute to insert. Ownership of this
 *        value is assumed by the list.
 *
 * Insert the given attribute into the #VogueAttrList. It will
 * replace any attributes of the same type on that segment
 * and be merged with any adjoining attributes that are identical.
 *
 * This function is slower than vogue_attr_list_insert() for
 * creating a attribute list in order (potentially much slower
 * for large lists). However, vogue_attr_list_insert() is not
 * suitable for continually changing a set of attributes
 * since it never removes or combines existing attributes.
 **/
void
vogue_attr_list_change (VogueAttrList  *list,
			VogueAttribute *attr)
{
  GSList *tmp_list, *prev, *link;
  guint start_index = attr->start_index;
  guint end_index = attr->end_index;

  g_return_if_fail (list != NULL);

  if (start_index == end_index)	/* empty, nothing to do */
    {
      vogue_attribute_destroy (attr);
      return;
    }

  tmp_list = list->attributes;
  prev = NULL;
  while (1)
    {
      VogueAttribute *tmp_attr;

      if (!tmp_list ||
	  ((VogueAttribute *)tmp_list->data)->start_index > start_index)
	{
	  /* We need to insert a new attribute
	   */
	  link = g_slist_alloc ();
	  link->next = tmp_list;
	  link->data = attr;

	  if (prev)
	    prev->next = link;
	  else
	    list->attributes = link;

	  if (!tmp_list)
	    list->attributes_tail = link;

	  prev = link;
	  tmp_list = prev->next;
	  break;
	}

      tmp_attr = tmp_list->data;

      if (tmp_attr->klass->type == attr->klass->type &&
	  tmp_attr->end_index >= start_index)
	{
	  /* We overlap with an existing attribute */
	  if (vogue_attribute_equal (tmp_attr, attr))
	    {
	      /* We can merge the new attribute with this attribute
	       */
	      if (tmp_attr->end_index >= end_index)
		{
		  /* We are totally overlapping the previous attribute.
		   * No action is needed.
		   */
		  vogue_attribute_destroy (attr);
		  return;
		}
	      tmp_attr->end_index = end_index;
	      vogue_attribute_destroy (attr);

	      attr = tmp_attr;

	      prev = tmp_list;
	      tmp_list = tmp_list->next;

	      break;
	    }
	  else
	    {
	      /* Split, truncate, or remove the old attribute
	       */
	      if (tmp_attr->end_index > attr->end_index)
		{
		  VogueAttribute *end_attr = vogue_attribute_copy (tmp_attr);

		  end_attr->start_index = attr->end_index;
		  vogue_attr_list_insert (list, end_attr);
		}

	      if (tmp_attr->start_index == attr->start_index)
		{
		  vogue_attribute_destroy (tmp_attr);
		  tmp_list->data = attr;

		  prev = tmp_list;
		  tmp_list = tmp_list->next;
		  break;
		}
	      else
		{
		  tmp_attr->end_index = attr->start_index;
		}
	    }
	}
      prev = tmp_list;
      tmp_list = tmp_list->next;
    }
  /* At this point, prev points to the list node with attr in it,
   * tmp_list points to prev->next.
   */

  g_assert (prev->data == attr);
  g_assert (prev->next == tmp_list);

  /* We now have the range inserted into the list one way or the
   * other. Fix up the remainder
   */
  while (tmp_list)
    {
      VogueAttribute *tmp_attr = tmp_list->data;

      if (tmp_attr->start_index > end_index)
	break;
      else if (tmp_attr->klass->type == attr->klass->type)
	{
	  if (tmp_attr->end_index <= attr->end_index ||
	      vogue_attribute_equal (tmp_attr, attr))
	    {
	      /* We can merge the new attribute with this attribute.
	       */
	      attr->end_index = MAX (end_index, tmp_attr->end_index);

	      vogue_attribute_destroy (tmp_attr);
	      prev->next = tmp_list->next;

	      if (!prev->next)
		list->attributes_tail = prev;

	      g_slist_free_1 (tmp_list);
	      tmp_list = prev->next;

	      continue;
	    }
	  else
	    {
	      /* Trim the start of this attribute that it begins at the end
	       * of the new attribute. This may involve moving
	       * it in the list to maintain the required non-decreasing
	       * order of start indices
	       */
	      GSList *tmp_list2;
	      GSList *prev2;

	      tmp_attr->start_index = attr->end_index;

	      tmp_list2 = tmp_list->next;
	      prev2 = tmp_list;

	      while (tmp_list2)
		{
		  VogueAttribute *tmp_attr2 = tmp_list2->data;

		  if (tmp_attr2->start_index >= tmp_attr->start_index)
		    break;

		  prev2 = tmp_list2;
		  tmp_list2 = tmp_list2->next;
		}

	      /* Now remove and insert before tmp_list2. We'll
	       * hit this attribute again later, but that's harmless.
	       */
	      if (prev2 != tmp_list)
		{
		  GSList *old_next = tmp_list->next;

		  prev->next = old_next;
		  prev2->next = tmp_list;
		  tmp_list->next = tmp_list2;

		  if (!tmp_list->next)
		    list->attributes_tail = tmp_list;

		  tmp_list = old_next;

		  continue;
		}
	    }
	}

      prev = tmp_list;
      tmp_list = tmp_list->next;
    }
}

/**
 * vogue_attr_list_update:
 * @list: a #VogueAttrList
 * @pos: the position of the change
 * @remove: the number of removed bytes
 * @add: the number of added bytes
 *
 * Update indices of attributes in @list for
 * a change in the text they refer to.
 *
 * The change that this function applies is
 * removing @remove bytes at position @pos
 * and inserting @add bytes instead.
 *
 * Attributes that fall entirely in the
 * (@pos, @pos + @remove) range are removed.
 *
 * Attributes that start or end inside the
 * (@pos, @pos + @remove) range are shortened to
 * reflect the removal.
 *
 * Attributes start and end positions are updated
 * if they are behind @pos + @remove.
 *
 * Since: 1.44
 */
void
vogue_attr_list_update (VogueAttrList *list,
                        int             pos,
                        int             remove,
                        int             add)
{
  GSList *l, *prev, *next;

   prev = NULL;
   l = list->attributes;
   while (l)
    {
      VogueAttribute *attr = l->data;
      next = l->next;

      if (attr->start_index >= pos &&
          attr->end_index < pos + remove)
        {
          vogue_attribute_destroy (attr);
          if (prev == NULL)
            list->attributes = next;
          else
            prev->next = next;

          g_slist_free_1 (l);
        }
      else
        {
          prev = l;

          if (attr->start_index >= pos &&
              attr->start_index < pos + remove)
            {
              attr->start_index = pos + add;
            }
          else if (attr->start_index >= pos + remove)
            {
              attr->start_index += add - remove;
            }

          if (attr->end_index >= pos &&
              attr->end_index < pos + remove)
            {
              attr->end_index = pos;
            }
          else if (attr->end_index >= pos + remove)
            {
              attr->end_index += add - remove;
            }
        }

      l = next;
    }
}

/**
 * vogue_attr_list_splice:
 * @list: a #VogueAttrList
 * @other: another #VogueAttrList
 * @pos: the position in @list at which to insert @other
 * @len: the length of the spliced segment. (Note that this
 *       must be specified since the attributes in @other
 *       may only be present at some subsection of this range)
 *
 * This function opens up a hole in @list, fills it in with attributes from
 * the left, and then merges @other on top of the hole.
 *
 * This operation is equivalent to stretching every attribute
 * that applies at position @pos in @list by an amount @len,
 * and then calling vogue_attr_list_change() with a copy
 * of each attribute in @other in sequence (offset in position by @pos).
 *
 * This operation proves useful for, for instance, inserting
 * a pre-edit string in the middle of an edit buffer.
 **/
void
vogue_attr_list_splice (VogueAttrList *list,
			VogueAttrList *other,
			gint           pos,
			gint           len)
{
  GSList *tmp_list;
  guint upos, ulen;

  g_return_if_fail (list != NULL);
  g_return_if_fail (other != NULL);
  g_return_if_fail (pos >= 0);
  g_return_if_fail (len >= 0);

  upos = (guint)pos;
  ulen = (guint)len;

/* This definition only works when a and b are unsigned; overflow
 * isn't defined in the C standard for signed integers
 */
#define CLAMP_ADD(a,b) (((a) + (b) < (a)) ? G_MAXUINT : (a) + (b))

  tmp_list = list->attributes;
  while (tmp_list)
    {
      VogueAttribute *attr = tmp_list->data;

      if (attr->start_index <= upos)
	{
	  if (attr->end_index > upos)
	    attr->end_index = CLAMP_ADD (attr->end_index, ulen);
	}
      else
	{
	  /* This could result in a zero length attribute if it
	   * gets squashed up against G_MAXUINT, but deleting such
	   * an element could (in theory) suprise the caller, so
	   * we don't delete it.
	   */
	  attr->start_index = CLAMP_ADD (attr->start_index, ulen);
	  attr->end_index = CLAMP_ADD (attr->end_index, ulen);
	}

      tmp_list = tmp_list->next;
    }

  tmp_list = other->attributes;
  while (tmp_list)
    {
      VogueAttribute *attr = vogue_attribute_copy (tmp_list->data);
      attr->start_index = CLAMP_ADD (attr->start_index, upos);
      attr->end_index = CLAMP_ADD (attr->end_index, upos);

      /* Same as above, the attribute could be squashed to zero-length; here
       * vogue_attr_list_change() will take care of deleting it.
       */
      vogue_attr_list_change (list, attr);

      tmp_list = tmp_list->next;
    }
#undef CLAMP_ADD
}

/**
 * vogue_attr_list_get_attributes:
 * @list: a #VogueAttrList
 *
 * Gets a list of all attributes in @list.
 *
 * Return value: (element-type Vogue.Attribute) (transfer full):
 *   a list of all attributes in @list. To free this value, call
 *   vogue_attribute_destroy() on each value and g_slist_free()
 *   on the list.
 *
 * Since: 1.44
 */
GSList *
vogue_attr_list_get_attributes (VogueAttrList *list)
{
  g_return_val_if_fail (list != NULL, NULL);

  return g_slist_copy_deep (list->attributes, (GCopyFunc)vogue_attribute_copy, NULL);
}

G_DEFINE_BOXED_TYPE (VogueAttrIterator,
                     vogue_attr_iterator,
                     vogue_attr_iterator_copy,
                     vogue_attr_iterator_destroy)

/**
 * vogue_attr_list_get_iterator:
 * @list: a #VogueAttrList
 *
 * Create a iterator initialized to the beginning of the list.
 * @list must not be modified until this iterator is freed.
 *
 * Return value: (transfer full): the newly allocated #VogueAttrIterator, which should
 *               be freed with vogue_attr_iterator_destroy().
 **/
VogueAttrIterator *
vogue_attr_list_get_iterator (VogueAttrList  *list)
{
  VogueAttrIterator *iterator;

  g_return_val_if_fail (list != NULL, NULL);

  iterator = g_slice_new (VogueAttrIterator);
  iterator->next_attribute = list->attributes;
  iterator->attribute_stack = NULL;

  iterator->start_index = 0;
  iterator->end_index = 0;

  if (!vogue_attr_iterator_next (iterator))
    iterator->end_index = G_MAXUINT;

  return iterator;
}

/**
 * vogue_attr_iterator_range:
 * @iterator: a #VogueAttrIterator
 * @start: (out): location to store the start of the range
 * @end: (out): location to store the end of the range
 *
 * Get the range of the current segment. Note that the
 * stored return values are signed, not unsigned like
 * the values in #VogueAttribute. To deal with this API
 * oversight, stored return values that wouldn't fit into
 * a signed integer are clamped to %G_MAXINT.
 **/
void
vogue_attr_iterator_range (VogueAttrIterator *iterator,
			   gint              *start,
			   gint              *end)
{
  g_return_if_fail (iterator != NULL);

  if (start)
    *start = MIN (iterator->start_index, G_MAXINT);
  if (end)
    *end = MIN (iterator->end_index, G_MAXINT);
}

/**
 * vogue_attr_iterator_next:
 * @iterator: a #VogueAttrIterator
 *
 * Advance the iterator until the next change of style.
 *
 * Return value: %FALSE if the iterator is at the end of the list, otherwise %TRUE
 **/
gboolean
vogue_attr_iterator_next (VogueAttrIterator *iterator)
{
  GList *tmp_list;

  g_return_val_if_fail (iterator != NULL, FALSE);

  if (!iterator->next_attribute && !iterator->attribute_stack)
    return FALSE;

  iterator->start_index = iterator->end_index;
  iterator->end_index = G_MAXUINT;

  tmp_list = iterator->attribute_stack;
  while (tmp_list)
    {
      GList *next = tmp_list->next;
      VogueAttribute *attr = tmp_list->data;

      if (attr->end_index == iterator->start_index)
	{
	  iterator->attribute_stack = g_list_remove_link (iterator->attribute_stack, tmp_list);
	  g_list_free_1 (tmp_list);
	}
      else
	{
	  iterator->end_index = MIN (iterator->end_index, attr->end_index);
	}

      tmp_list = next;
    }

  while (iterator->next_attribute &&
	 ((VogueAttribute *)iterator->next_attribute->data)->start_index == iterator->start_index)
    {
      if (((VogueAttribute *)iterator->next_attribute->data)->end_index > iterator->start_index)
	{
	  iterator->attribute_stack = g_list_prepend (iterator->attribute_stack, iterator->next_attribute->data);
	  iterator->end_index = MIN (iterator->end_index, ((VogueAttribute *)iterator->next_attribute->data)->end_index);
	}
      iterator->next_attribute = iterator->next_attribute->next;
    }

  if (iterator->next_attribute)
    iterator->end_index = MIN (iterator->end_index, ((VogueAttribute *)iterator->next_attribute->data)->start_index);

  return TRUE;
}

/**
 * vogue_attr_iterator_copy:
 * @iterator: a #VogueAttrIterator.
 *
 * Copy a #VogueAttrIterator
 *
 * Return value: (transfer full): the newly allocated
 *               #VogueAttrIterator, which should be freed with
 *               vogue_attr_iterator_destroy().
 **/
VogueAttrIterator *
vogue_attr_iterator_copy (VogueAttrIterator *iterator)
{
  VogueAttrIterator *copy;

  g_return_val_if_fail (iterator != NULL, NULL);

  copy = g_slice_new (VogueAttrIterator);

  *copy = *iterator;

  copy->attribute_stack = g_list_copy (iterator->attribute_stack);

  return copy;
}

/**
 * vogue_attr_iterator_destroy:
 * @iterator: a #VogueAttrIterator.
 *
 * Destroy a #VogueAttrIterator and free all associated memory.
 **/
void
vogue_attr_iterator_destroy (VogueAttrIterator *iterator)
{
  g_return_if_fail (iterator != NULL);

  g_list_free (iterator->attribute_stack);
  g_slice_free (VogueAttrIterator, iterator);
}

/**
 * vogue_attr_iterator_get:
 * @iterator: a #VogueAttrIterator
 * @type: the type of attribute to find.
 *
 * Find the current attribute of a particular type at the iterator
 * location. When multiple attributes of the same type overlap,
 * the attribute whose range starts closest to the current location
 * is used.
 *
 * Return value: (nullable): the current attribute of the given type,
 *               or %NULL if no attribute of that type applies to the
 *               current location.
 **/
VogueAttribute *
vogue_attr_iterator_get (VogueAttrIterator *iterator,
			 VogueAttrType      type)
{
  GList *tmp_list;

  g_return_val_if_fail (iterator != NULL, NULL);

  tmp_list = iterator->attribute_stack;
  while (tmp_list)
    {
      VogueAttribute *attr = tmp_list->data;

      if (attr->klass->type == type)
	return attr;

      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * vogue_attr_iterator_get_font:
 * @iterator: a #VogueAttrIterator
 * @desc: a #VogueFontDescription to fill in with the current values.
 *        The family name in this structure will be set using
 *        vogue_font_description_set_family_static() using values from
 *        an attribute in the #VogueAttrList associated with the iterator,
 *        so if you plan to keep it around, you must call:
 *        <literal>vogue_font_description_set_family (desc, vogue_font_description_get_family (desc))</literal>.
 * @language: (allow-none): if non-%NULL, location to store language tag for item, or %NULL
 *            if none is found.
 * @extra_attrs: (allow-none) (element-type Vogue.Attribute) (transfer full): if non-%NULL,
 *           location in which to store a list of non-font
 *           attributes at the the current position; only the highest priority
 *           value of each attribute will be added to this list. In order
 *           to free this value, you must call vogue_attribute_destroy() on
 *           each member.
 *
 * Get the font and other attributes at the current iterator position.
 **/
void
vogue_attr_iterator_get_font (VogueAttrIterator     *iterator,
			      VogueFontDescription  *desc,
			      VogueLanguage        **language,
			      GSList               **extra_attrs)
{
  GList *tmp_list1;
  GSList *tmp_list2;

  VogueFontMask mask = 0;
  gboolean have_language = FALSE;
  gdouble scale = 0;
  gboolean have_scale = FALSE;

  g_return_if_fail (iterator != NULL);
  g_return_if_fail (desc != NULL);

  if (language)
    *language = NULL;

  if (extra_attrs)
    *extra_attrs = NULL;

  tmp_list1 = iterator->attribute_stack;
  while (tmp_list1)
    {
      VogueAttribute *attr = tmp_list1->data;
      tmp_list1 = tmp_list1->next;

      switch ((int) attr->klass->type)
	{
	case PANGO_ATTR_FONT_DESC:
	  {
	    VogueFontMask new_mask = vogue_font_description_get_set_fields (((VogueAttrFontDesc *)attr)->desc) & ~mask;
	    mask |= new_mask;
	    vogue_font_description_unset_fields (desc, new_mask);
	    vogue_font_description_merge_static (desc, ((VogueAttrFontDesc *)attr)->desc, FALSE);

	    break;
	  }
	case PANGO_ATTR_FAMILY:
	  if (!(mask & PANGO_FONT_MASK_FAMILY))
	    {
	      mask |= PANGO_FONT_MASK_FAMILY;
	      vogue_font_description_set_family (desc, ((VogueAttrString *)attr)->value);
	    }
	  break;
	case PANGO_ATTR_STYLE:
	  if (!(mask & PANGO_FONT_MASK_STYLE))
	    {
	      mask |= PANGO_FONT_MASK_STYLE;
	      vogue_font_description_set_style (desc, ((VogueAttrInt *)attr)->value);
	    }
	  break;
	case PANGO_ATTR_VARIANT:
	  if (!(mask & PANGO_FONT_MASK_VARIANT))
	    {
	      mask |= PANGO_FONT_MASK_VARIANT;
	      vogue_font_description_set_variant (desc, ((VogueAttrInt *)attr)->value);
	    }
	  break;
	case PANGO_ATTR_WEIGHT:
	  if (!(mask & PANGO_FONT_MASK_WEIGHT))
	    {
	      mask |= PANGO_FONT_MASK_WEIGHT;
	      vogue_font_description_set_weight (desc, ((VogueAttrInt *)attr)->value);
	    }
	  break;
	case PANGO_ATTR_STRETCH:
	  if (!(mask & PANGO_FONT_MASK_STRETCH))
	    {
	      mask |= PANGO_FONT_MASK_STRETCH;
	      vogue_font_description_set_stretch (desc, ((VogueAttrInt *)attr)->value);
	    }
	  break;
	case PANGO_ATTR_SIZE:
	  if (!(mask & PANGO_FONT_MASK_SIZE))
	    {
	      mask |= PANGO_FONT_MASK_SIZE;
	      vogue_font_description_set_size (desc, ((VogueAttrSize *)attr)->size);
	    }
	  break;
	case PANGO_ATTR_ABSOLUTE_SIZE:
	  if (!(mask & PANGO_FONT_MASK_SIZE))
	    {
	      mask |= PANGO_FONT_MASK_SIZE;
	      vogue_font_description_set_absolute_size (desc, ((VogueAttrSize *)attr)->size);
	    }
	  break;
	case PANGO_ATTR_SCALE:
	  if (!have_scale)
	    {
	      have_scale = TRUE;
	      scale = ((VogueAttrFloat *)attr)->value;
	    }
	  break;
	case PANGO_ATTR_LANGUAGE:
	  if (language)
	    {
	      if (!have_language)
		{
		  have_language = TRUE;
		  *language = ((VogueAttrLanguage *)attr)->value;
		}
	    }
	  break;
	default:
	  if (extra_attrs)
	    {
	      gboolean found = FALSE;

	      tmp_list2 = *extra_attrs;
	      /* Hack: special-case FONT_FEATURES.  We don't want them to
	       * override each other, so we never merge them.  This should
	       * be fixed when we implement attr-merging. */
	      if (attr->klass->type != PANGO_ATTR_FONT_FEATURES)
		while (tmp_list2)
		  {
		    VogueAttribute *old_attr = tmp_list2->data;
		    if (attr->klass->type == old_attr->klass->type)
		      {
			found = TRUE;
			break;
		      }

		    tmp_list2 = tmp_list2->next;
		  }

	      if (!found)
		*extra_attrs = g_slist_prepend (*extra_attrs, vogue_attribute_copy (attr));
	    }
	}
    }

  if (have_scale)
    {
      if (vogue_font_description_get_size_is_absolute (desc))
        vogue_font_description_set_absolute_size (desc, scale * vogue_font_description_get_size (desc));
      else
        vogue_font_description_set_size (desc, scale * vogue_font_description_get_size (desc));
    }
}

/**
 * vogue_attr_list_filter:
 * @list: a #VogueAttrList
 * @func: (scope call) (closure data): callback function; returns %TRUE
 *        if an attribute should be filtered out.
 * @data: (closure): Data to be passed to @func
 *
 * Given a #VogueAttrList and callback function, removes any elements
 * of @list for which @func returns %TRUE and inserts them into
 * a new list.
 *
 * Return value: (transfer full) (nullable): the new #VogueAttrList or
 *  %NULL if no attributes of the given types were found.
 *
 * Since: 1.2
 **/
VogueAttrList *
vogue_attr_list_filter (VogueAttrList       *list,
			VogueAttrFilterFunc  func,
			gpointer             data)

{
  VogueAttrList *new = NULL;
  GSList *tmp_list;
  GSList *prev;

  g_return_val_if_fail (list != NULL, NULL);

  tmp_list = list->attributes;
  prev = NULL;
  while (tmp_list)
    {
      GSList *next = tmp_list->next;
      VogueAttribute *tmp_attr = tmp_list->data;

      if ((*func) (tmp_attr, data))
	{
	  if (!tmp_list->next)
	    list->attributes_tail = prev;

	  if (prev)
	    prev->next = tmp_list->next;
	  else
	    list->attributes = tmp_list->next;

	  tmp_list->next = NULL;

	  if (!new)
	    {
	      new = vogue_attr_list_new ();
	      new->attributes = new->attributes_tail = tmp_list;
	    }
	  else
	    {
	      new->attributes_tail->next = tmp_list;
	      new->attributes_tail = tmp_list;
	    }

	  goto next_attr;
	}

      prev = tmp_list;

    next_attr:
      tmp_list = next;
    }

  return new;
}

/**
 * vogue_attr_iterator_get_attrs:
 * @iterator: a #VogueAttrIterator
 *
 * Gets a list of all attributes at the current position of the
 * iterator.
 *
 * Return value: (element-type Vogue.Attribute) (transfer full): a list of
 *   all attributes for the current range.
 *   To free this value, call vogue_attribute_destroy() on
 *   each value and g_slist_free() on the list.
 *
 * Since: 1.2
 **/
GSList *
vogue_attr_iterator_get_attrs (VogueAttrIterator *iterator)
{
  GSList *attrs = NULL;
  GList *tmp_list;

  for (tmp_list = iterator->attribute_stack; tmp_list; tmp_list = tmp_list->next)
    {
      VogueAttribute *attr = tmp_list->data;
      GSList *tmp_list2;
      gboolean found = FALSE;

      for (tmp_list2 = attrs; tmp_list2; tmp_list2 = tmp_list2->next)
	{
	  VogueAttribute *old_attr = tmp_list2->data;
	  if (attr->klass->type == old_attr->klass->type)
	    {
	      found = TRUE;
	      break;
	    }
	}

      if (!found)
	attrs = g_slist_prepend (attrs, vogue_attribute_copy (attr));
    }

  return attrs;
}
