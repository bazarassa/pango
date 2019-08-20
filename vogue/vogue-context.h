/* Vogue
 * vogue-context.h: Rendering contexts
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

#ifndef __PANGO_CONTEXT_H__
#define __PANGO_CONTEXT_H__

#include <vogue/vogue-font.h>
#include <vogue/vogue-fontmap.h>
#include <vogue/vogue-attributes.h>
#include <vogue/vogue-direction.h>

G_BEGIN_DECLS

/* Sort of like a GC - application set information about how
 * to handle scripts
 */

/* VogueContext typedefed in vogue-fontmap.h */
typedef struct _VogueContextClass VogueContextClass;

#define PANGO_TYPE_CONTEXT              (vogue_context_get_type ())
#define PANGO_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CONTEXT, VogueContext))
#define PANGO_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CONTEXT, VogueContextClass))
#define PANGO_IS_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CONTEXT))
#define PANGO_IS_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_CONTEXT))
#define PANGO_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CONTEXT, VogueContextClass))


/* The VogueContext and VogueContextClass structs are private; if you
 * need to create a subclass of these, file a bug.
 */

PANGO_AVAILABLE_IN_ALL
GType         vogue_context_get_type      (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
VogueContext *vogue_context_new           (void);
PANGO_AVAILABLE_IN_1_32
void          vogue_context_changed       (VogueContext                 *context);
PANGO_AVAILABLE_IN_ALL
void          vogue_context_set_font_map  (VogueContext                 *context,
					   VogueFontMap                 *font_map);
PANGO_AVAILABLE_IN_1_6
VogueFontMap *vogue_context_get_font_map  (VogueContext                 *context);
PANGO_AVAILABLE_IN_1_32
guint         vogue_context_get_serial    (VogueContext                 *context);
PANGO_AVAILABLE_IN_ALL
void          vogue_context_list_families (VogueContext                 *context,
					   VogueFontFamily            ***families,
					   int                          *n_families);
PANGO_AVAILABLE_IN_ALL
VogueFont *   vogue_context_load_font     (VogueContext                 *context,
					   const VogueFontDescription   *desc);
PANGO_AVAILABLE_IN_ALL
VogueFontset *vogue_context_load_fontset  (VogueContext                 *context,
					   const VogueFontDescription   *desc,
					   VogueLanguage                *language);

PANGO_AVAILABLE_IN_ALL
VogueFontMetrics *vogue_context_get_metrics   (VogueContext                 *context,
					       const VogueFontDescription   *desc,
					       VogueLanguage                *language);

PANGO_AVAILABLE_IN_ALL
void                      vogue_context_set_font_description (VogueContext               *context,
							      const VogueFontDescription *desc);
PANGO_AVAILABLE_IN_ALL
VogueFontDescription *    vogue_context_get_font_description (VogueContext               *context);
PANGO_AVAILABLE_IN_ALL
VogueLanguage            *vogue_context_get_language         (VogueContext               *context);
PANGO_AVAILABLE_IN_ALL
void                      vogue_context_set_language         (VogueContext               *context,
							      VogueLanguage              *language);
PANGO_AVAILABLE_IN_ALL
void                      vogue_context_set_base_dir         (VogueContext               *context,
							      VogueDirection              direction);
PANGO_AVAILABLE_IN_ALL
VogueDirection            vogue_context_get_base_dir         (VogueContext               *context);
PANGO_AVAILABLE_IN_1_16
void                      vogue_context_set_base_gravity     (VogueContext               *context,
							      VogueGravity                gravity);
PANGO_AVAILABLE_IN_1_16
VogueGravity              vogue_context_get_base_gravity     (VogueContext               *context);
PANGO_AVAILABLE_IN_1_16
VogueGravity              vogue_context_get_gravity          (VogueContext               *context);
PANGO_AVAILABLE_IN_1_16
void                      vogue_context_set_gravity_hint     (VogueContext               *context,
							      VogueGravityHint            hint);
PANGO_AVAILABLE_IN_1_16
VogueGravityHint          vogue_context_get_gravity_hint     (VogueContext               *context);

PANGO_AVAILABLE_IN_1_6
void                      vogue_context_set_matrix           (VogueContext      *context,
						              const VogueMatrix *matrix);
PANGO_AVAILABLE_IN_1_6
const VogueMatrix *       vogue_context_get_matrix           (VogueContext      *context);

PANGO_AVAILABLE_IN_1_44
void                      vogue_context_set_round_glyph_positions (VogueContext *context,
                                                                   gboolean      round_positions);
PANGO_AVAILABLE_IN_1_44
gboolean                  vogue_context_get_round_glyph_positions (VogueContext *context);


/* Break a string of Unicode characters into segments with
 * consistent shaping/language engine and bidrectional level.
 * Returns a #GList of #VogueItem's
 */
PANGO_AVAILABLE_IN_ALL
GList *vogue_itemize                (VogueContext      *context,
				     const char        *text,
				     int                start_index,
				     int                length,
				     VogueAttrList     *attrs,
				     VogueAttrIterator *cached_iter);
PANGO_AVAILABLE_IN_1_4
GList *vogue_itemize_with_base_dir  (VogueContext      *context,
				     VogueDirection     base_dir,
				     const char        *text,
				     int                start_index,
				     int                length,
				     VogueAttrList     *attrs,
				     VogueAttrIterator *cached_iter);

G_END_DECLS

#endif /* __PANGO_CONTEXT_H__ */
