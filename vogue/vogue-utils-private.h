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

#ifndef __PANGO_UTILS_PRIVATE_H__
#define __PANGO_UTILS_PRIVATE_H__

#include <stdio.h>
#include <glib.h>
#include <vogue/vogue-font.h>
#include <vogue/vogue-utils.h>

G_BEGIN_DECLS

PANGO_DEPRECATED_IN_1_38
char *   vogue_config_key_get_system (const char *key);
PANGO_DEPRECATED_IN_1_38
char *   vogue_config_key_get (const char  *key);
PANGO_DEPRECATED_IN_1_32
void     vogue_lookup_aliases (const char   *fontname,
			       char       ***families,
			       int          *n_families);

/* On Unix, return the name of the "vogue" subdirectory of SYSCONFDIR
 * (which is set at compile time). On Win32, return the Vogue
 * installation directory (which is set at installation time, and
 * stored in the registry). The returned string should not be
 * g_free'd.
 */
PANGO_DEPRECATED
const char *   vogue_get_sysconf_subdirectory (void) G_GNUC_PURE;

/* Ditto for LIBDIR/vogue. On Win32, use the same Vogue
 * installation directory. This returned string should not be
 * g_free'd either.
 */
PANGO_DEPRECATED
const char *   vogue_get_lib_subdirectory (void) G_GNUC_PURE;

G_END_DECLS

#endif /* __PANGO_UTILS_PRIATE_H__ */
