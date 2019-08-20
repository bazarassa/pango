/* Vogue
 * testiter.c: Test vogue attributes
 *
 * Copyright (C) 2015 Red Hat, Inc.
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

#include <vogue/vogue.h>
#include "test-common.h"

static void
test_copy (VogueAttribute *attr)
{
  VogueAttribute *a;

  a = vogue_attribute_copy (attr);
  g_assert_true (vogue_attribute_equal (attr, a));
  vogue_attribute_destroy (a);
  vogue_attribute_destroy (attr);
}

static void
test_attributes_basic (void)
{
  VogueFontDescription *desc;
  VogueRectangle rect = { 0, 0, 10, 10 };

  test_copy (vogue_attr_language_new (vogue_language_from_string ("ja-JP")));
  test_copy (vogue_attr_family_new ("Times"));
  test_copy (vogue_attr_foreground_new (100, 200, 300));
  test_copy (vogue_attr_background_new (100, 200, 300));
  test_copy (vogue_attr_size_new (1024));
  test_copy (vogue_attr_size_new_absolute (1024));
  test_copy (vogue_attr_style_new (PANGO_STYLE_ITALIC));
  test_copy (vogue_attr_weight_new (PANGO_WEIGHT_ULTRALIGHT));
  test_copy (vogue_attr_variant_new (PANGO_VARIANT_SMALL_CAPS));
  test_copy (vogue_attr_stretch_new (PANGO_STRETCH_SEMI_EXPANDED));
  desc = vogue_font_description_from_string ("Computer Modern 12");
  test_copy (vogue_attr_font_desc_new (desc));
  vogue_font_description_free (desc);
  test_copy (vogue_attr_underline_new (PANGO_UNDERLINE_LOW));
  test_copy (vogue_attr_underline_color_new (100, 200, 300));
  test_copy (vogue_attr_strikethrough_new (TRUE));
  test_copy (vogue_attr_strikethrough_color_new (100, 200, 300));
  test_copy (vogue_attr_rise_new (256));
  test_copy (vogue_attr_scale_new (2.56));
  test_copy (vogue_attr_fallback_new (FALSE));
  test_copy (vogue_attr_letter_spacing_new (1024));
  test_copy (vogue_attr_shape_new (&rect, &rect));
  test_copy (vogue_attr_gravity_new (PANGO_GRAVITY_SOUTH));
  test_copy (vogue_attr_gravity_hint_new (PANGO_GRAVITY_HINT_STRONG));
  test_copy (vogue_attr_allow_breaks_new (FALSE));
  test_copy (vogue_attr_show_new (PANGO_SHOW_SPACES));
  test_copy (vogue_attr_insert_hyphens_new (FALSE));
}

static void
test_attributes_equal (void)
{
  VogueAttribute *attr1, *attr2, *attr3;

  /* check that vogue_attribute_equal compares values, but not ranges */
  attr1 = vogue_attr_size_new (10);
  attr2 = vogue_attr_size_new (20);
  attr3 = vogue_attr_size_new (20);
  attr3->start_index = 1;
  attr3->end_index = 2;

  g_assert_true (!vogue_attribute_equal (attr1, attr2));
  g_assert_true (vogue_attribute_equal (attr2, attr3));

  vogue_attribute_destroy (attr1);
  vogue_attribute_destroy (attr2);
  vogue_attribute_destroy (attr3);
}

static void
assert_attributes (GSList     *attrs,
                   const char *expected)
{
  GString *s;

  s = g_string_new ("");
  print_attributes (attrs, s);
  g_assert_cmpstr (s->str, ==, expected);
  g_string_free (s, FALSE);
}

static void
assert_attr_list (VogueAttrList *list,
                  const char    *expected)
{
  GSList *attrs;

  attrs = vogue_attr_list_get_attributes (list);
  assert_attributes (attrs, expected);
  g_slist_free_full (attrs, (GDestroyNotify)vogue_attribute_destroy);
}

static void
assert_attr_iterator (VogueAttrIterator *iter,
                      const char        *expected)
{
  GSList *attrs;

  attrs = vogue_attr_iterator_get_attrs (iter);
  assert_attributes (attrs, expected);
  g_slist_free_full (attrs, (GDestroyNotify)vogue_attribute_destroy);
}

static void
test_list (void)
{
  VogueAttrList *list;
  VogueAttribute *attr;

  list = vogue_attr_list_new ();

  attr = vogue_attr_size_new (10);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_size_new (20);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_size_new (30);
  vogue_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[0,-1]size=20\n"
                          "[0,-1]size=30\n");
  vogue_attr_list_unref (list);

  list = vogue_attr_list_new ();

  /* test that insertion respects start_index */
  attr = vogue_attr_size_new (10);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_size_new (20);
  attr->start_index = 10;
  attr->end_index = 20;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_size_new (30);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_size_new (40);
  attr->start_index = 10;
  attr->end_index = 40;
  vogue_attr_list_insert_before (list, attr);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[0,-1]size=30\n"
                          "[10,40]size=40\n"
                          "[10,20]size=20\n");
  vogue_attr_list_unref (list);
}

static void
test_list_change (void)
{
  VogueAttrList *list;
  VogueAttribute *attr;

  list = vogue_attr_list_new ();

  attr = vogue_attr_size_new (10);
  attr->start_index = 0;
  attr->end_index = 10;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_size_new (20);
  attr->start_index = 20;
  attr->end_index = 30;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index = 30;
  vogue_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,10]size=10\n"
                          "[0,30]weight=700\n"
                          "[20,30]size=20\n");

  /* simple insertion with vogue_attr_list_change */
  attr = vogue_attr_variant_new (PANGO_VARIANT_SMALL_CAPS);
  attr->start_index = 10;
  attr->end_index = 20;
  vogue_attr_list_change (list, attr);

  assert_attr_list (list, "[0,10]size=10\n"
                          "[0,30]weight=700\n"
                          "[10,20]variant=1\n"
                          "[20,30]size=20\n");

  /* insertion with splitting */
  attr = vogue_attr_weight_new (PANGO_WEIGHT_LIGHT);
  attr->start_index = 15;
  attr->end_index = 20;
  vogue_attr_list_change (list, attr);

  assert_attr_list (list, "[0,10]size=10\n"
                          "[0,15]weight=700\n"
                          "[10,20]variant=1\n"
                          "[15,20]weight=300\n"
                          "[20,30]size=20\n"
                          "[20,30]weight=700\n");

  /* insertion with joining */
  attr = vogue_attr_size_new (20);
  attr->start_index = 5;
  attr->end_index = 20;
  vogue_attr_list_change (list, attr);

  assert_attr_list (list, "[0,5]size=10\n"
                          "[0,15]weight=700\n"
                          "[5,30]size=20\n"
                          "[10,20]variant=1\n"
                          "[15,20]weight=300\n"
                          "[20,30]weight=700\n");

  vogue_attr_list_unref (list);
}

static void
test_list_splice (void)
{
  VogueAttrList *base;
  VogueAttrList *list;
  VogueAttrList *other;
  VogueAttribute *attr;

  base = vogue_attr_list_new ();
  attr = vogue_attr_size_new (10);
  attr->start_index = 0;
  attr->end_index = -1;
  vogue_attr_list_insert (base, attr);
  attr = vogue_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 10;
  attr->end_index = 15;
  vogue_attr_list_insert (base, attr);
  attr = vogue_attr_variant_new (PANGO_VARIANT_SMALL_CAPS);
  attr->start_index = 20;
  attr->end_index = 30;
  vogue_attr_list_insert (base, attr);

  assert_attr_list (base, "[0,-1]size=10\n"
                          "[10,15]weight=700\n"
                          "[20,30]variant=1\n");

  /* splice in an empty list */
  list = vogue_attr_list_copy (base);
  other = vogue_attr_list_new ();
  vogue_attr_list_splice (list, other, 11, 5);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[10,20]weight=700\n"
                          "[25,35]variant=1\n");

  vogue_attr_list_unref (list);
  vogue_attr_list_unref (other);

  /* splice in some attributes */
  list = vogue_attr_list_copy (base);
  other = vogue_attr_list_new ();
  attr = vogue_attr_size_new (20);
  attr->start_index = 0;
  attr->end_index = 3;
  vogue_attr_list_insert (other, attr);
  attr = vogue_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 2;
  attr->end_index = 4;
  vogue_attr_list_insert (other, attr);

  vogue_attr_list_splice (list, other, 11, 5);

  assert_attr_list (list, "[0,11]size=10\n"
                          "[10,20]weight=700\n"
                          "[11,14]size=20\n"
                          "[13,15]stretch=2\n"
                          "[14,-1]size=10\n"
                          "[25,35]variant=1\n");

  vogue_attr_list_unref (list);
  vogue_attr_list_unref (other);

  vogue_attr_list_unref (base);
}

static gboolean
never_true (VogueAttribute *attribute, gpointer user_data)
{
  return FALSE;
}

static gboolean
just_weight (VogueAttribute *attribute, gpointer user_data)
{
  if (attribute->klass->type == PANGO_ATTR_WEIGHT)
    return TRUE;
  else
    return FALSE;
}

static void
test_list_filter (void)
{
  VogueAttrList *list;
  VogueAttrList *out;
  VogueAttribute *attr;

  list = vogue_attr_list_new ();
  attr = vogue_attr_size_new (10);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 20;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 20;
  vogue_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[10,20]stretch=2\n"
                          "[20,-1]weight=700\n");

  out = vogue_attr_list_filter (list, never_true, NULL);
  g_assert_null (out);

  out = vogue_attr_list_filter (list, just_weight, NULL);
  g_assert_nonnull (out);

  assert_attr_list (list, "[0,-1]size=10\n"
                          "[10,20]stretch=2\n");
  assert_attr_list (out, "[20,-1]weight=700\n");

  vogue_attr_list_unref (list);
  vogue_attr_list_unref (out);
}

static void
test_iter (void)
{
  VogueAttrList *list;
  VogueAttribute *attr;
  VogueAttrIterator *iter;
  VogueAttrIterator *copy;
  gint start, end;

  list = vogue_attr_list_new ();
  attr = vogue_attr_size_new (10);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 30;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 20;
  vogue_attr_list_insert (list, attr);

  iter = vogue_attr_list_get_iterator (list);
  copy = vogue_attr_iterator_copy (iter);
  vogue_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 10);
  g_assert_true (vogue_attr_iterator_next (iter));
  vogue_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 10);
  g_assert_cmpint (end, ==, 20);
  g_assert_true (vogue_attr_iterator_next (iter));
  vogue_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 20);
  g_assert_cmpint (end, ==, 30);
  g_assert_true (vogue_attr_iterator_next (iter));
  vogue_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, 30);
  g_assert_cmpint (end, ==, G_MAXINT);
  g_assert_true (vogue_attr_iterator_next (iter));
  vogue_attr_iterator_range (iter, &start, &end);
  g_assert_cmpint (start, ==, G_MAXINT);
  g_assert_cmpint (end, ==, G_MAXINT);
  g_assert_true (!vogue_attr_iterator_next (iter));

  vogue_attr_iterator_destroy (iter);

  vogue_attr_iterator_range (copy, &start, &end);
  g_assert_cmpint (start, ==, 0);
  g_assert_cmpint (end, ==, 10);
  vogue_attr_iterator_destroy (copy);

  vogue_attr_list_unref (list);
}

static void
test_iter_get (void)
{
  VogueAttrList *list;
  VogueAttribute *attr;
  VogueAttrIterator *iter;

  list = vogue_attr_list_new ();
  attr = vogue_attr_size_new (10);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 30;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 20;
  vogue_attr_list_insert (list, attr);

  iter = vogue_attr_list_get_iterator (list);
  vogue_attr_iterator_next (iter);
  attr = vogue_attr_iterator_get (iter, PANGO_ATTR_SIZE);
  g_assert_nonnull (attr);
  g_assert_cmpuint (attr->start_index, ==, 0);
  g_assert_cmpuint (attr->end_index, ==, G_MAXUINT);
  attr = vogue_attr_iterator_get (iter, PANGO_ATTR_STRETCH);
  g_assert_nonnull (attr);
  g_assert_cmpuint (attr->start_index, ==, 10);
  g_assert_cmpuint (attr->end_index, ==, 30);
  attr = vogue_attr_iterator_get (iter, PANGO_ATTR_WEIGHT);
  g_assert_null (attr);
  attr = vogue_attr_iterator_get (iter, PANGO_ATTR_GRAVITY);
  g_assert_null (attr);

  vogue_attr_iterator_destroy (iter);
  vogue_attr_list_unref (list);
}

static void
test_iter_get_font (void)
{
  VogueAttrList *list;
  VogueAttribute *attr;
  VogueAttrIterator *iter;
  VogueFontDescription *desc;
  VogueFontDescription *desc2;
  VogueLanguage *lang;
  GSList *attrs;

  list = vogue_attr_list_new ();
  attr = vogue_attr_size_new (10 * PANGO_SCALE);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_family_new ("Times");
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 30;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_language_new (vogue_language_from_string ("ja-JP"));
  attr->start_index = 10;
  attr->end_index = 20;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_rise_new (100);
  attr->start_index = 20;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_fallback_new (FALSE);
  attr->start_index = 20;
  vogue_attr_list_insert (list, attr);

  iter = vogue_attr_list_get_iterator (list);
  desc = vogue_font_description_new ();
  vogue_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = vogue_font_description_from_string ("Times 10");
  g_assert_true (vogue_font_description_equal (desc, desc2));
  g_assert_null (lang);
  g_assert_null (attrs);
  vogue_font_description_free (desc);
  vogue_font_description_free (desc2);

  vogue_attr_iterator_next (iter);
  desc = vogue_font_description_new ();
  vogue_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = vogue_font_description_from_string ("Times Condensed 10");
  g_assert_true (vogue_font_description_equal (desc, desc2));
  g_assert_nonnull (lang);
  g_assert_cmpstr (vogue_language_to_string (lang), ==, "ja-jp");
  g_assert_null (attrs);
  vogue_font_description_free (desc);
  vogue_font_description_free (desc2);

  vogue_attr_iterator_next (iter);
  desc = vogue_font_description_new ();
  vogue_attr_iterator_get_font (iter, desc, &lang, &attrs);
  desc2 = vogue_font_description_from_string ("Times Condensed 10");
  g_assert_true (vogue_font_description_equal (desc, desc2));
  g_assert_null (lang);
  assert_attributes (attrs, "[20,-1]rise=100\n"
                            "[20,-1]fallback=0\n");
  g_slist_free_full (attrs, (GDestroyNotify)vogue_attribute_destroy);

  vogue_font_description_free (desc);
  vogue_font_description_free (desc2);

  vogue_attr_iterator_destroy (iter);
  vogue_attr_list_unref (list);
}

static void
test_iter_get_attrs (void)
{
  VogueAttrList *list;
  VogueAttribute *attr;
  VogueAttrIterator *iter;

  list = vogue_attr_list_new ();
  attr = vogue_attr_size_new (10 * PANGO_SCALE);
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_family_new ("Times");
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 10;
  attr->end_index = 30;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_language_new (vogue_language_from_string ("ja-JP"));
  attr->start_index = 10;
  attr->end_index = 20;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_rise_new (100);
  attr->start_index = 20;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_fallback_new (FALSE);
  attr->start_index = 20;
  vogue_attr_list_insert (list, attr);

  iter = vogue_attr_list_get_iterator (list);
  assert_attr_iterator (iter, "[0,-1]size=10240\n"
                              "[0,-1]family=Times\n");

  vogue_attr_iterator_next (iter);
  assert_attr_iterator (iter, "[0,-1]size=10240\n"
                              "[0,-1]family=Times\n"
                              "[10,30]stretch=2\n"
                              "[10,20]language=ja-jp\n");

  vogue_attr_iterator_next (iter);
  assert_attr_iterator (iter, "[0,-1]size=10240\n"
                              "[0,-1]family=Times\n"
                              "[10,30]stretch=2\n"
                              "[20,-1]rise=100\n"
                              "[20,-1]fallback=0\n");

  vogue_attr_iterator_next (iter);
  assert_attr_iterator (iter, "[0,-1]size=10240\n"
                              "[0,-1]family=Times\n"
                              "[20,-1]rise=100\n"
                              "[20,-1]fallback=0\n");

  vogue_attr_iterator_next (iter);
  g_assert_null (vogue_attr_iterator_get_attrs (iter));

  vogue_attr_iterator_destroy (iter);
  vogue_attr_list_unref (list);
}

static void
test_list_update (void)
{
  VogueAttrList *list;
  VogueAttribute *attr;

  list = vogue_attr_list_new ();
  attr = vogue_attr_size_new (10 * PANGO_SCALE);
  attr->start_index = 10;
  attr->end_index = 11;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_rise_new (100);
  attr->start_index = 0;
  attr->end_index = 200;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_family_new ("Times");
  attr->start_index = 5;
  attr->end_index = 15;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_fallback_new (FALSE);
  attr->start_index = 11;
  attr->end_index = 100;
  vogue_attr_list_insert (list, attr);
  attr = vogue_attr_stretch_new (PANGO_STRETCH_CONDENSED);
  attr->start_index = 30;
  attr->end_index = 60;
  vogue_attr_list_insert (list, attr);

  assert_attr_list (list, "[0,200]rise=100\n"
                          "[5,15]family=Times\n"
                          "[10,11]size=10240\n"
                          "[11,100]fallback=0\n"
                          "[30,60]stretch=2\n");

  vogue_attr_list_update (list, 8, 10, 20);

  assert_attr_list (list, "[0,210]rise=100\n"
                          "[5,8]family=Times\n"
                          "[28,110]fallback=0\n"
                          "[40,70]stretch=2\n");

  vogue_attr_list_unref (list);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/attributes/basic", test_attributes_basic);
  g_test_add_func ("/attributes/equal", test_attributes_equal);
  g_test_add_func ("/attributes/list/basic", test_list);
  g_test_add_func ("/attributes/list/change", test_list_change);
  g_test_add_func ("/attributes/list/splice", test_list_splice);
  g_test_add_func ("/attributes/list/filter", test_list_filter);
  g_test_add_func ("/attributes/list/update", test_list_update);
  g_test_add_func ("/attributes/iter/basic", test_iter);
  g_test_add_func ("/attributes/iter/get", test_iter_get);
  g_test_add_func ("/attributes/iter/get_font", test_iter_get_font);
  g_test_add_func ("/attributes/iter/get_attrs", test_iter_get_attrs);

  return g_test_run ();
}
