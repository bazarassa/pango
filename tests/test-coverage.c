/* Vogue
 * test-coverage.c: Test coverage
 *
 * Copyright (C) 2019 Red Hat, Inc.
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

static void
test_coverage_basic (void)
{
  VogueCoverage *coverage;
  int i;

  coverage = vogue_coverage_new ();

  for (i = 0; i < 100; i++)
    g_assert_cmpint (vogue_coverage_get (coverage, i), ==, PANGO_COVERAGE_NONE);

  for (i = 0; i < 100; i++)
    vogue_coverage_set (coverage, i, PANGO_COVERAGE_EXACT);

  for (i = 0; i < 100; i++)
    g_assert_cmpint (vogue_coverage_get (coverage, i), ==, PANGO_COVERAGE_EXACT);

  for (i = 0; i < 100; i++)
    vogue_coverage_set (coverage, i, PANGO_COVERAGE_NONE);

  for (i = 0; i < 100; i++)
    g_assert_cmpint (vogue_coverage_get (coverage, i), ==, PANGO_COVERAGE_NONE);

  vogue_coverage_unref (coverage);
}

static void
test_coverage_copy (void)
{
  VogueCoverage *coverage;
  VogueCoverage *coverage2;
  int i;

  coverage = vogue_coverage_new ();

  for (i = 0; i < 100; i++)
    vogue_coverage_set (coverage, i, PANGO_COVERAGE_EXACT);

  coverage2 = vogue_coverage_copy (coverage);

  for (i = 0; i < 50; i++)
    vogue_coverage_set (coverage, i, PANGO_COVERAGE_NONE);

  for (i = 0; i < 100; i++)
    g_assert_cmpint (vogue_coverage_get (coverage2, i), ==, PANGO_COVERAGE_EXACT);

  vogue_coverage_unref (coverage);
  vogue_coverage_unref (coverage2);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/coverage/basic", test_coverage_basic);
  g_test_add_func ("/coverage/copy", test_coverage_copy);

  return g_test_run ();
}
