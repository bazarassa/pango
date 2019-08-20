/* Vogue
 * vogue-matrix.h: Matrix manipulation routines
 *
 * Copyright (C) 2002, 2006 Red Hat Software
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

#ifndef __PANGO_MATRIX_H__
#define __PANGO_MATRIX_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _VogueMatrix    VogueMatrix;

/**
 * VogueMatrix:
 * @xx: 1st component of the transformation matrix
 * @xy: 2nd component of the transformation matrix
 * @yx: 3rd component of the transformation matrix
 * @yy: 4th component of the transformation matrix
 * @x0: x translation
 * @y0: y translation
 *
 * A structure specifying a transformation between user-space
 * coordinates and device coordinates. The transformation
 * is given by
 *
 * <programlisting>
 * x_device = x_user * matrix->xx + y_user * matrix->xy + matrix->x0;
 * y_device = x_user * matrix->yx + y_user * matrix->yy + matrix->y0;
 * </programlisting>
 *
 * Since: 1.6
 **/
struct _VogueMatrix
{
  double xx;
  double xy;
  double yx;
  double yy;
  double x0;
  double y0;
};

/**
 * PANGO_TYPE_MATRIX:
 *
 * The GObject type for #VogueMatrix
 **/
#define PANGO_TYPE_MATRIX (vogue_matrix_get_type ())

/**
 * PANGO_MATRIX_INIT:
 *
 * Constant that can be used to initialize a VogueMatrix to
 * the identity transform.
 *
 * <informalexample><programlisting>
 * VogueMatrix matrix = PANGO_MATRIX_INIT;
 * vogue_matrix_rotate (&amp;matrix, 45.);
 * </programlisting></informalexample>
 *
 * Since: 1.6
 **/
#define PANGO_MATRIX_INIT { 1., 0., 0., 1., 0., 0. }

/* for VogueRectangle */
#include <vogue/vogue-types.h>

PANGO_AVAILABLE_IN_1_6
GType vogue_matrix_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_1_6
VogueMatrix *vogue_matrix_copy   (const VogueMatrix *matrix);
PANGO_AVAILABLE_IN_1_6
void         vogue_matrix_free   (VogueMatrix *matrix);

PANGO_AVAILABLE_IN_1_6
void vogue_matrix_translate (VogueMatrix *matrix,
			     double       tx,
			     double       ty);
PANGO_AVAILABLE_IN_1_6
void vogue_matrix_scale     (VogueMatrix *matrix,
			     double       scale_x,
			     double       scale_y);
PANGO_AVAILABLE_IN_1_6
void vogue_matrix_rotate    (VogueMatrix *matrix,
			     double       degrees);
PANGO_AVAILABLE_IN_1_6
void vogue_matrix_concat    (VogueMatrix       *matrix,
			     const VogueMatrix *new_matrix);
PANGO_AVAILABLE_IN_1_16
void vogue_matrix_transform_point    (const VogueMatrix *matrix,
				      double            *x,
				      double            *y);
PANGO_AVAILABLE_IN_1_16
void vogue_matrix_transform_distance (const VogueMatrix *matrix,
				      double            *dx,
				      double            *dy);
PANGO_AVAILABLE_IN_1_16
void vogue_matrix_transform_rectangle (const VogueMatrix *matrix,
				       VogueRectangle    *rect);
PANGO_AVAILABLE_IN_1_16
void vogue_matrix_transform_pixel_rectangle (const VogueMatrix *matrix,
					     VogueRectangle    *rect);
PANGO_AVAILABLE_IN_1_12
double vogue_matrix_get_font_scale_factor (const VogueMatrix *matrix) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_38
void vogue_matrix_get_font_scale_factors (const VogueMatrix *matrix,
					  double *xscale, double *yscale);


G_END_DECLS

#endif /* __PANGO_MATRIX_H__ */
