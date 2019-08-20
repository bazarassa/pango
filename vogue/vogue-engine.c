/* Vogue
 * vogue-engine.c: Engines for script and language specific processing
 *
 * Copyright (C) 2003 Red Hat Software
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
 * SECTION:engines
 * @short_description:Language-specific and rendering-system-specific processing
 * @title:Engines
 *
 * Vogue used to have a module architecture in which the language-specific
 * and render-system-specific components are provided by loadable
 * modules.
 *
 * This is no longer the case, and all the APIs related
 * to modules and engines should not be used anymore.
 *
 * Deprecated: 1.38
 */
/**
 * SECTION:vogue-engine-lang
 * @short_description:Rendering-system independent script engines
 * @title:VogueEngineLang
 * @stability:Unstable
 *
 * The <firstterm>language engines</firstterm> are rendering-system independent
 * engines that determine line, word, and character breaks for character strings.
 * These engines are used in vogue_break().
 *
 * Deprecated: 1.38
 */
/**
 * SECTION:vogue-engine-shape
 * @short_description:Rendering-system dependent script engines
 * @title:VogueEngineShape
 * @stability:Unstable
 *
 * The <firstterm>shape engines</firstterm> are rendering-system dependent
 * engines that convert character strings into glyph strings.
 * These engines are used in vogue_shape().
 *
 * Deprecated: 1.38
 */
#include "config.h"

#include "vogue-engine.h"
#include "vogue-impl-utils.h"


G_DEFINE_ABSTRACT_TYPE (VogueEngine, vogue_engine, G_TYPE_OBJECT);

static void
vogue_engine_init (VogueEngine *self)
{
}

static void
vogue_engine_class_init (VogueEngineClass *klass)
{
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_ABSTRACT_TYPE (VogueEngineLang, vogue_engine_lang, PANGO_TYPE_ENGINE);
G_GNUC_END_IGNORE_DEPRECATIONS

static void
vogue_engine_lang_init (VogueEngineLang *self)
{
}

static void
vogue_engine_lang_class_init (VogueEngineLangClass *klass)
{
}


static VogueCoverageLevel
vogue_engine_shape_real_covers (VogueEngineShape *engine G_GNUC_UNUSED,
				VogueFont        *font,
				VogueLanguage    *language,
				gunichar          wc)
{
  VogueCoverage *coverage = vogue_font_get_coverage (font, language);
  VogueCoverageLevel result = vogue_coverage_get (coverage, wc);

  vogue_coverage_unref (coverage);

  return result;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_ABSTRACT_TYPE (VogueEngineShape, vogue_engine_shape, PANGO_TYPE_ENGINE);
G_GNUC_END_IGNORE_DEPRECATIONS

static void
vogue_engine_shape_init (VogueEngineShape *klass)
{
}

static void
vogue_engine_shape_class_init (VogueEngineShapeClass *class)
{
  class->covers = vogue_engine_shape_real_covers;
}
