/* Vogue
 * vogue-language.h: Language handling routines
 *
 * Copyright (C) 1999 Red Hat Software
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

#ifndef __PANGO_LANGUAGE_H__
#define __PANGO_LANGUAGE_H__

#include <glib.h>
#include <glib-object.h>

#include <vogue/vogue-version-macros.h>

G_BEGIN_DECLS

typedef struct _VogueLanguage VogueLanguage;

/**
 * PANGO_TYPE_LANGUAGE:
 *
 * The #GObject type for #VogueLanguage.
 */
#define PANGO_TYPE_LANGUAGE (vogue_language_get_type ())

PANGO_AVAILABLE_IN_ALL
GType          vogue_language_get_type    (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
VogueLanguage *vogue_language_from_string (const char *language);

PANGO_AVAILABLE_IN_ALL
const char    *vogue_language_to_string   (VogueLanguage *language) G_GNUC_CONST;
/* For back compat.  Will have to keep indefinitely. */
#define vogue_language_to_string(language) ((const char *)language)

PANGO_AVAILABLE_IN_ALL
const char    *vogue_language_get_sample_string (VogueLanguage *language) G_GNUC_CONST;
PANGO_AVAILABLE_IN_1_16
VogueLanguage *vogue_language_get_default (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
gboolean      vogue_language_matches  (VogueLanguage *language,
				       const char *range_list) G_GNUC_PURE;

#include <vogue/vogue-script.h>

PANGO_AVAILABLE_IN_1_4
gboolean		    vogue_language_includes_script (VogueLanguage *language,
							    VogueScript    script) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_22
const VogueScript          *vogue_language_get_scripts	   (VogueLanguage *language,
							    int           *num_scripts);

G_END_DECLS

#endif /* __PANGO_LANGUAGE_H__ */
