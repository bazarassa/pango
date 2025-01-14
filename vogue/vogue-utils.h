/* Vogue
 * vogue-utils.c: Utilities for internal functions and modules
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

#ifndef __PANGO_UTILS_H__
#define __PANGO_UTILS_H__

#include <stdio.h>
#include <glib.h>
#include <vogue/vogue-font.h>

G_BEGIN_DECLS

PANGO_DEPRECATED
char **  vogue_split_file_list (const char *str);

PANGO_DEPRECATED
char    *vogue_trim_string     (const char *str);
PANGO_DEPRECATED
gint     vogue_read_line      (FILE        *stream,
			       GString     *str);
PANGO_DEPRECATED
gboolean vogue_skip_space     (const char **pos);
PANGO_DEPRECATED
gboolean vogue_scan_word      (const char **pos,
			       GString     *out);
PANGO_DEPRECATED
gboolean vogue_scan_string    (const char **pos,
			       GString     *out);
PANGO_DEPRECATED
gboolean vogue_scan_int       (const char **pos,
			       int         *out);

PANGO_DEPRECATED
gboolean vogue_parse_enum     (GType       type,
			       const char *str,
			       int        *value,
			       gboolean    warn,
			       char      **possible_values);

/* Functions for parsing textual representations
 * of VogueFontDescription fields. They return TRUE if the input string
 * contains a valid value, which then has been assigned to the corresponding
 * field in the VogueFontDescription. If the warn parameter is TRUE,
 * a warning is printed (with g_warning) if the string does not
 * contain a valid value.
 */
PANGO_AVAILABLE_IN_ALL
gboolean vogue_parse_style   (const char   *str,
			      VogueStyle   *style,
			      gboolean      warn);
PANGO_AVAILABLE_IN_ALL
gboolean vogue_parse_variant (const char   *str,
			      VogueVariant *variant,
			      gboolean      warn);
PANGO_AVAILABLE_IN_ALL
gboolean vogue_parse_weight  (const char   *str,
			      VogueWeight  *weight,
			      gboolean      warn);
PANGO_AVAILABLE_IN_ALL
gboolean vogue_parse_stretch (const char   *str,
			      VogueStretch *stretch,
			      gboolean      warn);


/* Hint line position and thickness.
 */
PANGO_AVAILABLE_IN_1_12
void vogue_quantize_line_geometry (int *thickness,
				   int *position);

/* A routine from fribidi that we either wrap or provide ourselves.
 */
PANGO_AVAILABLE_IN_1_4
guint8 * vogue_log2vis_get_embedding_levels (const gchar    *text,
					     int             length,
					     VogueDirection *pbase_dir);

/* Unicode characters that are zero-width and should not be rendered
 * normally.
 */
PANGO_AVAILABLE_IN_1_10
gboolean vogue_is_zero_width (gunichar ch) G_GNUC_CONST;

/* Vogue version checking */

/* Encode a Vogue version as an integer */
/**
 * PANGO_VERSION_ENCODE:
 * @major: the major component of the version number
 * @minor: the minor component of the version number
 * @micro: the micro component of the version number
 *
 * This macro encodes the given Vogue version into an integer.  The numbers
 * returned by %PANGO_VERSION and vogue_version() are encoded using this macro.
 * Two encoded version numbers can be compared as integers.
 */
#define PANGO_VERSION_ENCODE(major, minor, micro) (     \
	  ((major) * 10000)                             \
	+ ((minor) *   100)                             \
	+ ((micro) *     1))

/* Encoded version of Vogue at compile-time */
/**
 * PANGO_VERSION:
 *
 * The version of Vogue available at compile-time, encoded using PANGO_VERSION_ENCODE().
 */
/**
 * PANGO_VERSION_STRING:
 *
 * A string literal containing the version of Vogue available at compile-time.
 */
/**
 * PANGO_VERSION_MAJOR:
 *
 * The major component of the version of Vogue available at compile-time.
 */
/**
 * PANGO_VERSION_MINOR:
 *
 * The minor component of the version of Vogue available at compile-time.
 */
/**
 * PANGO_VERSION_MICRO:
 *
 * The micro component of the version of Vogue available at compile-time.
 */
#define PANGO_VERSION PANGO_VERSION_ENCODE(     \
	PANGO_VERSION_MAJOR,                    \
	PANGO_VERSION_MINOR,                    \
	PANGO_VERSION_MICRO)

/* Check that compile-time Vogue is as new as required */
/**
 * PANGO_VERSION_CHECK:
 * @major: the major component of the version number
 * @minor: the minor component of the version number
 * @micro: the micro component of the version number
 *
 * Checks that the version of Vogue available at compile-time is not older than
 * the provided version number.
 */
#define PANGO_VERSION_CHECK(major,minor,micro)    \
	(PANGO_VERSION >= PANGO_VERSION_ENCODE(major,minor,micro))


/* Return encoded version of Vogue at run-time */
PANGO_AVAILABLE_IN_1_16
int vogue_version (void) G_GNUC_CONST;

/* Return run-time Vogue version as an string */
PANGO_AVAILABLE_IN_1_16
const char * vogue_version_string (void) G_GNUC_CONST;

/* Check that run-time Vogue is as new as required */
PANGO_AVAILABLE_IN_1_16
const char * vogue_version_check (int required_major,
                                  int required_minor,
                                  int required_micro) G_GNUC_CONST;

G_END_DECLS

#endif /* __PANGO_UTILS_H__ */
