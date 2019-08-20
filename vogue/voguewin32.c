/* Vogue
 * voguewin32.c: Routines for handling Windows fonts
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
 * Copyright (C) 2007 Novell, Inc.
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
 * SECTION:win32-fonts
 * @short_description:Font handling and rendering on Windows
 * @title:Win32 Fonts and Rendering
 *
 * The macros and functions in this section are used to access fonts natively on
 * Win32 systems and to render text in conjunction with Win32 APIs.
 */
#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <hb.h>

#include "vogue-impl-utils.h"
#include "voguewin32.h"
#include "voguewin32-private.h"

#define MAX_FREED_FONTS 256

HDC _vogue_win32_hdc;
gboolean _vogue_win32_debug = FALSE;

static void vogue_win32_font_dispose    (GObject             *object);
static void vogue_win32_font_finalize   (GObject             *object);

static VogueFontDescription *vogue_win32_font_describe          (VogueFont        *font);
static VogueFontDescription *vogue_win32_font_describe_absolute (VogueFont        *font);
static VogueCoverage        *vogue_win32_font_get_coverage      (VogueFont        *font,
								 VogueLanguage    *lang);
static void                  vogue_win32_font_get_glyph_extents (VogueFont        *font,
								 VogueGlyph        glyph,
								 VogueRectangle   *ink_rect,
								 VogueRectangle   *logical_rect);
static VogueFontMetrics *    vogue_win32_font_get_metrics       (VogueFont        *font,
								 VogueLanguage    *lang);
static VogueFontMap *        vogue_win32_font_get_font_map      (VogueFont        *font);

static gboolean vogue_win32_font_real_select_font      (VogueFont *font,
							HDC        hdc);
static void     vogue_win32_font_real_done_font        (VogueFont *font);
static double   vogue_win32_font_real_get_metrics_factor (VogueFont *font);

static void                  vogue_win32_get_item_properties    (VogueItem        *item,
								 VogueUnderline   *uline,
								 VogueAttrColor   *uline_color,
								 gboolean         *uline_set,
								 VogueAttrColor   *fg_color,
								 gboolean         *fg_set,
								 VogueAttrColor   *bg_color,
								 gboolean         *bg_set);

static hb_font_t *           vogue_win32_font_create_hb_font    (VogueFont *font);

HFONT
_vogue_win32_font_get_hfont (VogueFont *font)
{
  VogueWin32Font *win32font = (VogueWin32Font *)font;
  VogueWin32FontCache *cache;

  if (!win32font)
    return NULL;

  if (!win32font->hfont)
    {
      cache = vogue_win32_font_map_get_font_cache (win32font->fontmap);
      if (G_UNLIKELY (!cache))
        return NULL;

      win32font->hfont = vogue_win32_font_cache_loadw (cache, &win32font->logfontw);
      if (!win32font->hfont)
	{
	  gchar *face_utf8 = g_utf16_to_utf8 (win32font->logfontw.lfFaceName,
					      -1, NULL, NULL, NULL);
	  g_warning ("Cannot load font '%s\n", face_utf8);
	  g_free (face_utf8);
	  return NULL;
	}
    }

  return win32font->hfont;
}

/**
 * vogue_win32_get_context:
 *
 * Retrieves a #VogueContext appropriate for rendering with Windows fonts.
 *
 * Return value: the new #VogueContext
 *
 * Deprecated: 1.22: Use vogue_win32_font_map_for_display() followed by
 * vogue_font_map_create_context() instead.
 **/
VogueContext *
vogue_win32_get_context (void)
{
  return vogue_font_map_create_context (vogue_win32_font_map_for_display ());
}

G_DEFINE_TYPE (VogueWin32Font, _vogue_win32_font, PANGO_TYPE_FONT)

static void
_vogue_win32_font_init (VogueWin32Font *win32font)
{
  win32font->size = -1;

  win32font->metrics_by_lang = NULL;

  win32font->glyph_info = g_hash_table_new_full (NULL, NULL, NULL, g_free);
}

/**
 * vogue_win32_get_dc:
 *
 * Obtains a handle to the Windows device context that is used by Vogue.
 *
 * Return value: A handle to the Windows device context that is used by Vogue.
 **/
HDC
vogue_win32_get_dc (void)
{
  if (g_once_init_enter (&_vogue_win32_hdc))
    {
      HDC hdc = CreateDC ("DISPLAY", NULL, NULL, NULL);

      /* Also do some generic voguewin32 initialisations... this function
       * is a suitable place for those as it is called from a couple
       * of class_init functions.
       */
#ifdef PANGO_WIN32_DEBUGGING
      if (getenv ("PANGO_WIN32_DEBUG") != NULL)
	_vogue_win32_debug = TRUE;
#endif
      g_once_init_leave (&_vogue_win32_hdc, hdc);
    }

  return _vogue_win32_hdc;
}

/**
 * vogue_win32_get_debug_flag:
 *
 * Returns whether debugging is turned on.
 *
 * Return value: %TRUE if debugging is turned on.
 *
 * Since: 1.2
 */
gboolean
vogue_win32_get_debug_flag (void)
{
  return _vogue_win32_debug;
}

static void
_vogue_win32_font_class_init (VogueWin32FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = vogue_win32_font_finalize;
  object_class->dispose = vogue_win32_font_dispose;

  font_class->describe = vogue_win32_font_describe;
  font_class->describe_absolute = vogue_win32_font_describe_absolute;
  font_class->get_coverage = vogue_win32_font_get_coverage;
  font_class->get_glyph_extents = vogue_win32_font_get_glyph_extents;
  font_class->get_metrics = vogue_win32_font_get_metrics;
  font_class->get_font_map = vogue_win32_font_get_font_map;
  font_class->create_hb_font = vogue_win32_font_create_hb_font;

  class->select_font = vogue_win32_font_real_select_font;
  class->done_font = vogue_win32_font_real_done_font;
  class->get_metrics_factor = vogue_win32_font_real_get_metrics_factor;

  vogue_win32_get_dc ();
}

/**
 * vogue_win32_render:
 * @hdc:     the device context
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a #VogueGlyphString onto a Windows DC
 */
void
vogue_win32_render (HDC               hdc,
		    VogueFont        *font,
		    VogueGlyphString *glyphs,
		    int               x,
		    int               y)
{
  HFONT hfont, old_hfont = NULL;
  int i, j, num_valid_glyphs;
  guint16 *glyph_indexes;
  gint *dX;
  gint this_x;
  gint start_x_offset, x_offset, next_x_offset, cur_y_offset; /* in Vogue units */

  g_return_if_fail (glyphs != NULL);

#ifdef PANGO_WIN32_DEBUGGING
  if (_vogue_win32_debug)
    {
      PING (("num_glyphs:%d", glyphs->num_glyphs));
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  g_print (" %d:%d", glyphs->glyphs[i].glyph, glyphs->glyphs[i].geometry.width);
	  if (glyphs->glyphs[i].geometry.x_offset != 0 ||
	      glyphs->glyphs[i].geometry.y_offset != 0)
	    g_print (":%d,%d", glyphs->glyphs[i].geometry.x_offset,
		     glyphs->glyphs[i].geometry.y_offset);
	}
      g_print ("\n");
    }
#endif

  if (glyphs->num_glyphs == 0)
    return;

  hfont = _vogue_win32_font_get_hfont (font);
  if (!hfont)
    return;

  old_hfont = SelectObject (hdc, hfont);

  glyph_indexes = g_new (guint16, glyphs->num_glyphs);
  dX = g_new (INT, glyphs->num_glyphs);

  /* Render glyphs using one ExtTextOutW() call for each run of glyphs
   * that have the same y offset. The big majoroty of glyphs will have
   * y offset of zero, so in general, the whole glyph string will be
   * rendered by one call to ExtTextOutW().
   *
   * In order to minimize buildup of rounding errors, we keep track of
   * where the glyphs should be rendered in Vogue units, and round
   * to pixels separately for each glyph,
   */

  i = 0;

  /* Outer loop through all glyphs in string */
  while (i < glyphs->num_glyphs)
    {
      cur_y_offset = glyphs->glyphs[i].geometry.y_offset;
      num_valid_glyphs = 0;
      x_offset = 0;
      start_x_offset = glyphs->glyphs[i].geometry.x_offset;
      this_x = PANGO_PIXELS (start_x_offset);

      /* Inner loop through glyphs with the same y offset, or code
       * point zero (just spacing).
       */
      while (i < glyphs->num_glyphs &&
	     (glyphs->glyphs[i].glyph == PANGO_GLYPH_EMPTY ||
	      cur_y_offset == glyphs->glyphs[i].geometry.y_offset))
	{
	  if (glyphs->glyphs[i].glyph == PANGO_GLYPH_EMPTY)
	    {
	      /* PANGO_GLYPH_EMPTY glyphs should not be rendered, but their
	       * indicated width (set up by VogueLayout) should be taken
	       * into account.
	       */

	      /* If the string starts with spacing, must shift the
	       * starting point for the glyphs actually rendered.  For
	       * spacing in the middle of the glyph string, add to the dX
	       * of the previous glyph to be rendered.
	       */
	      if (num_valid_glyphs == 0)
		start_x_offset += glyphs->glyphs[i].geometry.width;
	      else
		{
		  x_offset += glyphs->glyphs[i].geometry.width;
		  dX[num_valid_glyphs-1] = PANGO_PIXELS (x_offset) - this_x;
		}
	    }
	  else
	    {
	      if (glyphs->glyphs[i].glyph & PANGO_GLYPH_UNKNOWN_FLAG)
		{
		  /* Glyph index is actually the char value that doesn't
		   * have any glyph (ORed with the flag). We should really
		   * do the same that vogue_xft_real_render() does: render
		   * a box with the char value in hex inside it in a tiny
		   * font. Later. For now, use the TrueType invalid glyph
		   * at 0.
		   */
		  glyph_indexes[num_valid_glyphs] = 0;
		}
	      else
		glyph_indexes[num_valid_glyphs] = glyphs->glyphs[i].glyph;

	      x_offset += glyphs->glyphs[i].geometry.width;

	      /* If the next glyph has an X offset, take that into consideration now */
	      if (i < glyphs->num_glyphs - 1)
		next_x_offset = glyphs->glyphs[i+1].geometry.x_offset;
	      else
		next_x_offset = 0;

	      dX[num_valid_glyphs] = PANGO_PIXELS (x_offset + next_x_offset) - this_x;

	      /* Prepare for next glyph */
	      this_x += dX[num_valid_glyphs];
	      num_valid_glyphs++;
	    }
	  i++;
	}
#ifdef PANGO_WIN32_DEBUGGING
      if (_vogue_win32_debug)
	{
	  g_print ("ExtTextOutW at %d,%d deltas:",
		   x + PANGO_PIXELS (start_x_offset),
		   y + PANGO_PIXELS (cur_y_offset));
	  for (j = 0; j < num_valid_glyphs; j++)
	    g_print (" %d", dX[j]);
	  g_print ("\n");
	}
#endif

      ExtTextOutW (hdc,
		   x + PANGO_PIXELS (start_x_offset),
		   y + PANGO_PIXELS (cur_y_offset),
		   ETO_GLYPH_INDEX,
		   NULL,
		   glyph_indexes, num_valid_glyphs,
		   dX);
      x += this_x;
    }


  SelectObject (hdc, old_hfont); /* restore */
  g_free (glyph_indexes);
  g_free (dX);
}

/**
 * vogue_win32_render_transformed:
 * @hdc:     a windows device context
 * @matrix:  (nullable): a #VogueMatrix, or %NULL to use an identity
 *           transformation
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of the start of the string (in Vogue
 *           units in user space coordinates)
 * @y:       the y position of the baseline (in Vogue units
 *           in user space coordinates)
 *
 * Renders a #VogueGlyphString onto a windows DC, possibly
 * transforming the layed-out coordinates through a transformation
 * matrix. Note that the transformation matrix for @font is not
 * changed, so to produce correct rendering results, the @font
 * must have been loaded using a #VogueContext with an identical
 * transformation matrix to that passed in to this function.
 **/
void
vogue_win32_render_transformed (HDC                hdc,
				const VogueMatrix *matrix,
				VogueFont         *font,
				VogueGlyphString  *glyphs,
				int                x,
				int                y)
{
  XFORM xForm;
  XFORM xFormPrev = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
  int   mode = GetGraphicsMode (hdc);

  if (!SetGraphicsMode (hdc, GM_ADVANCED))
    g_warning ("SetGraphicsMode() failed");
  else if (!GetWorldTransform (hdc, &xFormPrev))
    g_warning ("GetWorldTransform() failed");
  else if (matrix)
    {
      xForm.eM11 = matrix->xx;
      xForm.eM12 = matrix->yx;
      xForm.eM21 = matrix->xy;
      xForm.eM22 = matrix->yy;
      xForm.eDx = matrix->x0;
      xForm.eDy = matrix->y0;
      if (!SetWorldTransform (hdc, &xForm))
	g_warning ("GetWorldTransform() failed");
    }

  vogue_win32_render (hdc, font, glyphs, x/PANGO_SCALE, y/PANGO_SCALE);

  /* restore */
  SetWorldTransform (hdc, &xFormPrev);
  SetGraphicsMode (hdc, mode);
}

static void
vogue_win32_font_get_glyph_extents (VogueFont      *font,
				    VogueGlyph      glyph,
				    VogueRectangle *ink_rect,
				    VogueRectangle *logical_rect)
{
  VogueWin32Font *win32font = (VogueWin32Font *)font;
  guint16 glyph_index = glyph;
  GLYPHMETRICS gm;
  TEXTMETRIC tm;
  guint32 res;
  HFONT hfont;
  MAT2 m = {{0,1}, {0,0}, {0,0}, {0,1}};
  VogueWin32GlyphInfo *info;

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->width = ink_rect->y = ink_rect->height = 0;
      if (logical_rect)
	logical_rect->x = logical_rect->width = logical_rect->y = logical_rect->height = 0;
      return;
    }

  if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    glyph_index = glyph = 0;

  info = g_hash_table_lookup (win32font->glyph_info, GUINT_TO_POINTER (glyph));

  if (!info)
    {
      HDC hdc = vogue_win32_get_dc ();

      info = g_new0 (VogueWin32GlyphInfo, 1);

      memset (&gm, 0, sizeof (gm));

      hfont = _vogue_win32_font_get_hfont (font);
      SelectObject (hdc, hfont);
      res = GetGlyphOutlineA (hdc,
			      glyph_index,
			      GGO_METRICS | GGO_GLYPH_INDEX,
			      &gm,
			      0, NULL,
			      &m);

      if (res == GDI_ERROR)
	{
	  gchar *error = g_win32_error_message (GetLastError ());
	  g_warning ("GetGlyphOutline(%04X) failed: %s\n",
		     glyph_index, error);
	  g_free (error);

	  /* Don't just return now, use the still zeroed out gm */
	}

      info->ink_rect.x = PANGO_SCALE * gm.gmptGlyphOrigin.x;
      info->ink_rect.width = PANGO_SCALE * gm.gmBlackBoxX;
      info->ink_rect.y = - PANGO_SCALE * gm.gmptGlyphOrigin.y;
      info->ink_rect.height = PANGO_SCALE * gm.gmBlackBoxY;

      GetTextMetrics (_vogue_win32_hdc, &tm);
      info->logical_rect.x = 0;
      info->logical_rect.width = PANGO_SCALE * gm.gmCellIncX;
      info->logical_rect.y = - PANGO_SCALE * tm.tmAscent;
      info->logical_rect.height = PANGO_SCALE * (tm.tmAscent + tm.tmDescent);

      g_hash_table_insert (win32font->glyph_info, GUINT_TO_POINTER(glyph), info);
    }

  if (ink_rect)
    *ink_rect = info->ink_rect;

  if (logical_rect)
    *logical_rect = info->logical_rect;
}

static int
max_glyph_width (VogueLayout *layout)
{
  int max_width = 0;
  GSList *l, *r;

  for (l = vogue_layout_get_lines_readonly (layout); l; l = l->next)
    {
      VogueLayoutLine *line = l->data;

      for (r = line->runs; r; r = r->next)
	{
	  VogueGlyphString *glyphs = ((VogueGlyphItem *)r->data)->glyphs;
	  int i;

	  for (i = 0; i < glyphs->num_glyphs; i++)
	    if (glyphs->glyphs[i].geometry.width > max_width)
	      max_width = glyphs->glyphs[i].geometry.width;
	}
    }

  return max_width;
}

static VogueFontMetrics *
vogue_win32_font_get_metrics (VogueFont     *font,
			      VogueLanguage *language)
{
  VogueWin32MetricsInfo *info = NULL; /* Quiet gcc */
  VogueWin32Font *win32font = (VogueWin32Font *)font;
  GSList *tmp_list;

  const char *sample_str = vogue_language_get_sample_string (language);

  tmp_list = win32font->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;

      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      HFONT hfont;
      VogueFontMetrics *metrics;

      info = g_new (VogueWin32MetricsInfo, 1);
      win32font->metrics_by_lang = g_slist_prepend (win32font->metrics_by_lang, info);

      info->sample_str = sample_str;
      info->metrics = metrics = vogue_font_metrics_new ();

      hfont = _vogue_win32_font_get_hfont (font);
      if (hfont != NULL)
	{
	  VogueCoverage *coverage;
	  TEXTMETRIC tm;

	  SelectObject (_vogue_win32_hdc, hfont);
	  GetTextMetrics (_vogue_win32_hdc, &tm);

	  metrics->ascent = tm.tmAscent * PANGO_SCALE;
	  metrics->descent = tm.tmDescent * PANGO_SCALE;
          metrics->height = (tm.tmHeight + tm.tmInternalLeading + tm.tmExternalLeading) * PANGO_SCALE;
	  metrics->approximate_char_width = tm.tmAveCharWidth * PANGO_SCALE;

	  coverage = vogue_win32_font_get_coverage (font, language);
	  if (vogue_coverage_get (coverage, '0') != PANGO_COVERAGE_NONE &&
	      vogue_coverage_get (coverage, '9') != PANGO_COVERAGE_NONE)
	    {
	      VogueContext *context;
	      VogueFontDescription *font_desc;
	      VogueLayout *layout;

	      /*  Get the average width of the chars in "0123456789" */
	      context = vogue_font_map_create_context (vogue_win32_font_map_for_display ());
	      vogue_context_set_language (context, language);
	      font_desc = vogue_font_describe_with_absolute_size (font);
	      vogue_context_set_font_description (context, font_desc);
	      layout = vogue_layout_new (context);
	      vogue_layout_set_text (layout, "0123456789", -1);

	      metrics->approximate_digit_width = max_glyph_width (layout);

	      vogue_font_description_free (font_desc);
	      g_object_unref (layout);
	      g_object_unref (context);
	    }
	  else
	    metrics->approximate_digit_width = metrics->approximate_char_width;

	  vogue_coverage_unref (coverage);

	  /* FIXME: Should get the real values from the TrueType font file */
	  metrics->underline_position = -2 * PANGO_SCALE;
	  metrics->underline_thickness = 1 * PANGO_SCALE;
	  metrics->strikethrough_thickness = metrics->underline_thickness;
	  /* Really really wild guess */
	  metrics->strikethrough_position = metrics->ascent / 3;
	}
    }

  return vogue_font_metrics_ref (info->metrics);
}

static VogueFontMap *
vogue_win32_font_get_font_map (VogueFont *font)
{
  VogueWin32Font *win32font = (VogueWin32Font *)font;

  return win32font->fontmap;
}

static gboolean
vogue_win32_font_real_select_font (VogueFont *font,
				   HDC        hdc)
{
  HFONT hfont = _vogue_win32_font_get_hfont (font);

  if (!hfont)
    return FALSE;

  if (!SelectObject (hdc, hfont))
    {
      g_warning ("vogue_win32_font_real_select_font: Cannot select font\n");
      return FALSE;
    }

  return TRUE;
}

static void
vogue_win32_font_real_done_font (VogueFont *font)
{
}

static double
vogue_win32_font_real_get_metrics_factor (VogueFont *font)
{
  return PANGO_SCALE;
}

/**
 * vogue_win32_font_logfont:
 * @font: a #VogueFont which must be from the Win32 backend
 *
 * Determine the LOGFONTA struct for the specified font. Note that
 * Vogue internally uses LOGFONTW structs, so if converting the UTF-16
 * face name in the LOGFONTW struct to system codepage fails, the
 * returned LOGFONTA will have an emppty face name. To get the
 * LOGFONTW of a VogueFont, use vogue_win32_font_logfontw(). It
 * is recommended to do that always even if you don't expect
 * to come across fonts with odd names.
 *
 * Return value: A newly allocated LOGFONTA struct. It must be
 * freed with g_free().
 **/
LOGFONTA *
vogue_win32_font_logfont (VogueFont *font)
{
  VogueWin32Font *win32font = (VogueWin32Font *)font;
  LOGFONTA *lfp;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), NULL);

  lfp = g_new (LOGFONTA, 1);

  *lfp = *(LOGFONTA*) &win32font->logfontw;
  if (!WideCharToMultiByte (CP_ACP, 0,
			    win32font->logfontw.lfFaceName, -1,
			    lfp->lfFaceName, G_N_ELEMENTS (lfp->lfFaceName),
			    NULL, NULL))
    lfp->lfFaceName[0] = '\0';

  return lfp;
}

/**
 * vogue_win32_font_logfontw:
 * @font: a #VogueFont which must be from the Win32 backend
 * 
 * Determine the LOGFONTW struct for the specified font.
 * 
 * Return value: A newly allocated LOGFONTW struct. It must be
 * freed with g_free().
 *
 * Since: 1.16
 **/
LOGFONTW *
vogue_win32_font_logfontw (VogueFont *font)
{
  VogueWin32Font *win32font = (VogueWin32Font *)font;
  LOGFONTW *lfp;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), NULL);

  lfp = g_new (LOGFONTW, 1);
  *lfp = win32font->logfontw;

  return lfp;
}

/**
 * vogue_win32_font_select_font:
 * @font: a #VogueFont from the Win32 backend
 * @hdc: a windows device context
 *
 * Selects the font into the specified DC and changes the mapping mode
 * and world transformation of the DC appropriately for the font.
 * You may want to surround the use of this function with calls
 * to SaveDC() and RestoreDC(). Call vogue_win32_font_done_font() when
 * you are done using the DC to release allocated resources.
 *
 * See vogue_win32_font_get_metrics_factor() for information about
 * converting from the coordinate space used by this function
 * into Vogue units.
 *
 * Return value: %TRUE if the operation succeeded.
 **/
gboolean
vogue_win32_font_select_font (VogueFont *font,
			      HDC        hdc)
{
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), FALSE);

  return PANGO_WIN32_FONT_GET_CLASS (font)->select_font (font, hdc);
}

/**
 * vogue_win32_font_done_font:
 * @font: a #VogueFont from the win32 backend
 *
 * Releases any resources allocated by vogue_win32_font_done_font()
 **/
void
vogue_win32_font_done_font (VogueFont *font)
{
  g_return_if_fail (PANGO_WIN32_IS_FONT (font));

  PANGO_WIN32_FONT_GET_CLASS (font)->done_font (font);
}

/**
 * vogue_win32_font_get_metrics_factor:
 * @font: a #VogueFont from the win32 backend
 *
 * Returns the scale factor from logical units in the coordinate
 * space used by vogue_win32_font_select_font() to Vogue units
 * in user space.
 *
 * Return value: factor to multiply logical units by to get Vogue
 *               units.
 **/
double
vogue_win32_font_get_metrics_factor (VogueFont *font)
{
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), 1.);

  return PANGO_WIN32_FONT_GET_CLASS (font)->get_metrics_factor (font);
}

static void
vogue_win32_fontmap_cache_add (VogueFontMap   *fontmap,
			       VogueWin32Font *win32font)
{
  VogueWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  if (win32fontmap->freed_fonts->length == MAX_FREED_FONTS)
    {
      VogueWin32Font *old_font = g_queue_pop_tail (win32fontmap->freed_fonts);
      g_object_unref (old_font);
    }

  g_object_ref (win32font);
  g_queue_push_head (win32fontmap->freed_fonts, win32font);
  win32font->in_cache = TRUE;
}

static void
vogue_win32_font_dispose (GObject *object)
{
  VogueWin32Font *win32font = PANGO_WIN32_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!win32font->in_cache && win32font->fontmap)
    vogue_win32_fontmap_cache_add (win32font->fontmap, win32font);

  G_OBJECT_CLASS (_vogue_win32_font_parent_class)->dispose (object);
}

static void
free_metrics_info (VogueWin32MetricsInfo *info)
{
  vogue_font_metrics_unref (info->metrics);
  g_free (info);
}

static void
vogue_win32_font_entry_remove (VogueWin32Face *face,
			       VogueFont      *font)
{
  face->cached_fonts = g_slist_remove (face->cached_fonts, font);
}

static void
vogue_win32_font_finalize (GObject *object)
{
  VogueWin32Font *win32font = (VogueWin32Font *)object;
  VogueWin32FontCache *cache = vogue_win32_font_map_get_font_cache (win32font->fontmap);
  VogueWin32Font *fontmap;

  if (G_UNLIKELY (!cache))
    return;

  if (win32font->hfont != NULL)
    vogue_win32_font_cache_unload (cache, win32font->hfont);

  g_slist_foreach (win32font->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (win32font->metrics_by_lang);

  if (win32font->win32face)
    vogue_win32_font_entry_remove (win32font->win32face, PANGO_FONT (win32font));

  g_hash_table_destroy (win32font->glyph_info);

  fontmap = g_weak_ref_get ((GWeakRef *) &win32font->fontmap);
  if (fontmap)
  {
    g_object_remove_weak_pointer (G_OBJECT (win32font->fontmap), (gpointer *) (gpointer) &win32font->fontmap);
    g_object_unref (fontmap);
  }

  G_OBJECT_CLASS (_vogue_win32_font_parent_class)->finalize (object);
}

static VogueFontDescription *
vogue_win32_font_describe (VogueFont *font)
{
  VogueFontDescription *desc;
  VogueWin32Font *win32font = PANGO_WIN32_FONT (font);

  desc = vogue_font_description_copy (win32font->win32face->description);
  vogue_font_description_set_size (desc, win32font->size / (PANGO_SCALE / PANGO_WIN32_FONT_MAP (win32font->fontmap)->resolution));

  return desc;
}

static VogueFontDescription *
vogue_win32_font_describe_absolute (VogueFont *font)
{
  VogueFontDescription *desc;
  VogueWin32Font *win32font = PANGO_WIN32_FONT (font);

  desc = vogue_font_description_copy (win32font->win32face->description);
  vogue_font_description_set_absolute_size (desc, win32font->size);

  return desc;
}

static VogueCoverage *
vogue_win32_font_get_coverage (VogueFont     *font,
			       VogueLanguage *lang G_GNUC_UNUSED)
{
  VogueWin32Face *win32face = ((VogueWin32Font *)font)->win32face;

  if (!win32face->coverage)
    {
      VogueCoverage *coverage = vogue_coverage_new ();
      hb_font_t *hb_font = vogue_font_get_hb_font (font);
      hb_face_t *hb_face = hb_font_get_face (hb_font);
      hb_set_t *chars = hb_set_create ();
      hb_codepoint_t ch = HB_SET_VALUE_INVALID;

      hb_face_collect_unicodes (hb_face, chars);
      while (hb_set_next(chars, &ch))
        vogue_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);

      win32face->coverage = vogue_coverage_ref (coverage);
    }

  return vogue_coverage_ref (win32face->coverage);
}

/* Utility functions */

/**
 * vogue_win32_get_unknown_glyph:
 * @font: a #VogueFont
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Use PANGO_GET_UNKNOWN_GLYPH() instead.
 *
 * Return value: a glyph index into @font
 **/
VogueGlyph
vogue_win32_get_unknown_glyph (VogueFont *font,
			       gunichar   wc)
{
  return PANGO_GET_UNKNOWN_GLYPH (wc);
}

/**
 * vogue_win32_render_layout_line:
 * @hdc:       DC to use for drawing
 * @line:      a #VogueLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #VogueLayoutLine onto a device context. For underlining to
 * work property the text alignment of the DC should have TA_BASELINE
 * and TA_LEFT.
 */
void
vogue_win32_render_layout_line (HDC              hdc,
				VogueLayoutLine *line,
				int              x,
				int              y)
{
  GSList *tmp_list = line->runs;
  VogueRectangle overall_rect;
  VogueRectangle logical_rect;
  VogueRectangle ink_rect;
  int oldbkmode = SetBkMode (hdc, TRANSPARENT);

  int x_off = 0;

  vogue_layout_line_get_extents (line,NULL, &overall_rect);

  while (tmp_list)
    {
      COLORREF oldfg = 0;
      HPEN uline_pen, old_pen;
      POINT points[2];
      VogueUnderline uline = PANGO_UNDERLINE_NONE;
      VogueLayoutRun *run = tmp_list->data;
      VogueAttrColor fg_color, bg_color, uline_color;
      gboolean fg_set, bg_set, uline_set;

      tmp_list = tmp_list->next;

      vogue_win32_get_item_properties (run->item, &uline, &uline_color, &uline_set, &fg_color, &fg_set, &bg_color, &bg_set);
      if (!uline_set)
	uline_color = fg_color;

      if (uline == PANGO_UNDERLINE_NONE)
	vogue_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	vogue_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      if (bg_set)
	{
	  COLORREF bg_col = RGB ((bg_color.color.red) >> 8,
				 (bg_color.color.green) >> 8,
				 (bg_color.color.blue) >> 8);
	  HBRUSH bg_brush = CreateSolidBrush (bg_col);
	  HBRUSH old_brush = SelectObject (hdc, bg_brush);
	  old_pen = SelectObject (hdc, GetStockObject (NULL_PEN));
	  Rectangle (hdc, x + PANGO_PIXELS (x_off + logical_rect.x),
			  y + PANGO_PIXELS (overall_rect.y),
			  1 + x + PANGO_PIXELS (x_off + logical_rect.x + logical_rect.width),
			  1 + y + PANGO_PIXELS (overall_rect.y + overall_rect.height));
	  SelectObject (hdc, old_brush);
	  DeleteObject (bg_brush);
	  SelectObject (hdc, old_pen);
	}

      if (fg_set)
	{
	  COLORREF fg_col = RGB ((fg_color.color.red) >> 8,
				 (fg_color.color.green) >> 8,
				 (fg_color.color.blue) >> 8);
	  oldfg = SetTextColor (hdc, fg_col);
	}

      vogue_win32_render (hdc, run->item->analysis.font, run->glyphs,
			  x + PANGO_PIXELS (x_off), y);

      if (fg_set)
	SetTextColor (hdc, oldfg);

      if (uline != PANGO_UNDERLINE_NONE)
	{
	  COLORREF uline_col = RGB ((uline_color.color.red) >> 8,
				    (uline_color.color.green) >> 8,
				    (uline_color.color.blue) >> 8);
	  uline_pen = CreatePen (PS_SOLID, 1, uline_col);
	  old_pen = SelectObject (hdc, uline_pen);
	}

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + 4;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  points[0].y = points[1].y = y + 2;
	  Polyline (hdc, points, 2);
	  break;
	case PANGO_UNDERLINE_SINGLE:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + 2;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  break;
	case PANGO_UNDERLINE_ERROR:
	  {
	    int point_x;
	    int counter = 0;
	    int end_x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);

	    for (point_x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
		 point_x <= end_x;
		 point_x += 2)
	    {
	      points[0].x = point_x;
	      points[1].x = MAX (point_x + 1, end_x);

	      if (counter)
		points[0].y = points[1].y = y + 2;
	      else
		points[0].y = points[1].y = y + 3;

	      Polyline (hdc, points, 2);
	      counter = (counter + 1) % 2;
	    }
	  }
	  break;
	case PANGO_UNDERLINE_LOW:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + PANGO_PIXELS (ink_rect.y + ink_rect.height) + 2;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  break;
	}

      if (uline != PANGO_UNDERLINE_NONE)
	{
	  SelectObject (hdc, old_pen);
	  DeleteObject (uline_pen);
	}

      x_off += logical_rect.width;
    }

    SetBkMode (hdc, oldbkmode);
}

/**
 * vogue_win32_render_layout:
 * @hdc:       HDC to use for drawing
 * @layout:    a #VogueLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #VogueLayoutLine onto an X drawable
 */
void
vogue_win32_render_layout (HDC          hdc,
			   VogueLayout *layout,
			   int          x,
			   int          y)
{
  VogueLayoutIter *iter;

  g_return_if_fail (hdc != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  iter = vogue_layout_get_iter (layout);

  do
    {
      VogueRectangle   logical_rect;
      VogueLayoutLine *line;
      int              baseline;

      line = vogue_layout_iter_get_line_readonly (iter);

      vogue_layout_iter_get_line_extents (iter, NULL, &logical_rect);
      baseline = vogue_layout_iter_get_baseline (iter);

      vogue_win32_render_layout_line (hdc,
				      line,
				      x + PANGO_PIXELS (logical_rect.x),
				      y + PANGO_PIXELS (baseline));
    }
  while (vogue_layout_iter_next_line (iter));

  vogue_layout_iter_free (iter);
}

/* This utility function is duplicated here and in vogue-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
vogue_win32_get_item_properties (VogueItem      *item,
				 VogueUnderline *uline,
				 VogueAttrColor *uline_color,
				 gboolean       *uline_set,
				 VogueAttrColor *fg_color,
				 gboolean       *fg_set,
				 VogueAttrColor *bg_color,
				 gboolean       *bg_set)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  if (fg_set)
    *fg_set = FALSE;

  if (bg_set)
    *bg_set = FALSE;

  while (tmp_list)
    {
      VogueAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((VogueAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_UNDERLINE_COLOR:
	  if (uline_color)
	    *uline_color = *((VogueAttrColor *)attr);
	  if (uline_set)
	    *uline_set = TRUE;

	  break;

	case PANGO_ATTR_FOREGROUND:
	  if (fg_color)
	    *fg_color = *((VogueAttrColor *)attr);
	  if (fg_set)
	    *fg_set = TRUE;

	  break;

	case PANGO_ATTR_BACKGROUND:
	  if (bg_color)
	    *bg_color = *((VogueAttrColor *)attr);
	  if (bg_set)
	    *bg_set = TRUE;

	  break;

	default:
	  break;
	}
      tmp_list = tmp_list->next;
    }
}

/**
 * vogue_win32_font_get_glyph_index:
 * @font: a #VogueFont.
 * @wc: a Unicode character.
 *
 * Obtains the index of the glyph for @wc in @font, or 0, if not
 * covered.
 *
 * Return value: the glyph index for @wc.
 **/
gint
vogue_win32_font_get_glyph_index (VogueFont *font,
				  gunichar   wc)
{
  hb_font_t *hb_font = vogue_font_get_hb_font (font);
  hb_codepoint_t glyph = 0;

  hb_font_get_nominal_glyph (hb_font, wc, &glyph);

  return glyph;
}

/*
 * Swap HarfBuzz-style tags to tags that GetFontData() understands,
 * adapted from https://github.com/harfbuzz/harfbuzz/pull/1832,
 * by Ebrahim Byagowi.
 */

static inline guint16 hb_gdi_uint16_swap (const guint16 v)
{ return (v >> 8) | (v << 8); }
static inline guint32 hb_gdi_uint32_swap (const guint32 v)
{ return (hb_gdi_uint16_swap (v) << 16) | hb_gdi_uint16_swap (v >> 16); }

/*
 * Adapted from https://www.mail-archive.com/harfbuzz@lists.freedesktop.org/msg06538.html
 * by Konstantin Ritt.
 */
static hb_blob_t *
hfont_reference_table (hb_face_t *face, hb_tag_t tag, void *user_data)
{
  HDC hdc;
  HFONT hfont, old_hfont;
  gchar *buf = NULL;
  DWORD size;

  /* We have a common DC for our VogueWin32Font, so let's just use it */
  hdc = vogue_win32_get_dc ();
  hfont = (HFONT) user_data;

  /* we want to restore things, just to be safe */
  old_hfont = SelectObject (hdc, hfont);
  if (old_hfont == NULL)
    {
      g_warning ("SelectObject() for the VogueWin32Font failed!");
      return hb_blob_get_empty ();
    }

  size = GetFontData (hdc, hb_gdi_uint32_swap (tag), 0, NULL, 0);

  /*
   * not all tags support retrieving the sizes, so don't warn,
   * just return hb_blob_get_empty()
   */
  if (size == GDI_ERROR)
    {
      SelectObject (hdc, old_hfont);
      return hb_blob_get_empty ();
    }

  buf = g_malloc (size * sizeof (gchar));

  /* This should be quite unlikely to fail if size was not GDI_ERROR */
  if (GetFontData (hdc, hb_gdi_uint32_swap (tag), 0, buf, size) == GDI_ERROR)
    size = 0;

  SelectObject (hdc, old_hfont);
  return hb_blob_create (buf, size, HB_MEMORY_MODE_READONLY, buf, g_free);
}

static hb_font_t *
vogue_win32_font_create_hb_font (VogueFont *font)
{
  VogueWin32Font *win32font = (VogueWin32Font *)font;
  HFONT hfont;
  hb_face_t *face = NULL;
  hb_font_t *hb_font = NULL;

  g_return_val_if_fail (font != NULL, NULL);

  hfont = _vogue_win32_font_get_hfont (font);

  /* We are *not* allowed to destroy the HFONT here ! */
  face = hb_face_create_for_tables (hfont_reference_table, (void *)hfont, NULL);

  hb_font = hb_font_create (face);
  hb_font_set_scale (hb_font, win32font->size, win32font->size);
  hb_face_destroy (face);

  return hb_font;
}
