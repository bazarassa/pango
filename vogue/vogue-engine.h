/* Vogue
 * vogue-engine.h: Engines for script and language specific processing
 *
 * Copyright (C) 2000,2003 Red Hat Software
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

#ifndef __PANGO_ENGINE_H__
#define __PANGO_ENGINE_H__

#include <vogue/vogue-types.h>
#include <vogue/vogue-item.h>
#include <vogue/vogue-font.h>
#include <vogue/vogue-glyph.h>
#include <vogue/vogue-script.h>

G_BEGIN_DECLS

#ifndef PANGO_DISABLE_DEPRECATED

/**
 * PANGO_RENDER_TYPE_NONE:
 *
 * A string constant defining the render type
 * for engines that are not rendering-system specific.
 *
 * Deprecated: 1.38
 */
#define PANGO_RENDER_TYPE_NONE "VogueRenderNone"

#define PANGO_TYPE_ENGINE              (vogue_engine_get_type ())
#define PANGO_ENGINE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_ENGINE, VogueEngine))
#define PANGO_IS_ENGINE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_ENGINE))
#define PANGO_ENGINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_ENGINE, VogueEngineClass))
#define PANGO_IS_ENGINE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_ENGINE))
#define PANGO_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_ENGINE, VogueEngineClass))

typedef struct _VogueEngine VogueEngine;
typedef struct _VogueEngineClass VogueEngineClass;

/**
 * VogueEngine:
 *
 * #VogueEngine is the base class for all types of language and
 * script specific engines. It has no functionality by itself.
 *
 * Deprecated: 1.38
 **/
struct _VogueEngine
{
  /*< private >*/
  GObject parent_instance;
};

/**
 * VogueEngineClass:
 *
 * Class structure for #VogueEngine
 *
 * Deprecated: 1.38
 **/
struct _VogueEngineClass
{
  /*< private >*/
  GObjectClass parent_class;
};

PANGO_DEPRECATED_IN_1_38
GType vogue_engine_get_type (void) G_GNUC_CONST;

/**
 * PANGO_ENGINE_TYPE_LANG:
 *
 * A string constant defining the engine type for language engines.
 * These engines derive from #VogueEngineLang.
 *
 * Deprecated: 1.38
 */
#define PANGO_ENGINE_TYPE_LANG "VogueEngineLang"

#define PANGO_TYPE_ENGINE_LANG              (vogue_engine_lang_get_type ())
#define PANGO_ENGINE_LANG(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_ENGINE_LANG, VogueEngineLang))
#define PANGO_IS_ENGINE_LANG(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_ENGINE_LANG))
#define PANGO_ENGINE_LANG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_ENGINE_LANG, VogueEngineLangClass))
#define PANGO_IS_ENGINE_LANG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_ENGINE_LANG))
#define PANGO_ENGINE_LANG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_ENGINE_LANG, VogueEngineLangClass))

typedef struct _VogueEngineLangClass VogueEngineLangClass;

/**
 * VogueEngineLang:
 *
 * The #VogueEngineLang class is implemented by engines that
 * customize the rendering-system independent part of the
 * Vogue pipeline for a particular script or language. For
 * instance, a custom #VogueEngineLang could be provided for
 * Thai to implement the dictionary-based word boundary
 * lookups needed for that language.
 *
 * Deprecated: 1.38
 **/
struct _VogueEngineLang
{
  /*< private >*/
  VogueEngine parent_instance;
};

/**
 * VogueEngineLangClass:
 * @script_break: (nullable): Provides a custom implementation of
 * vogue_break().  If %NULL, vogue_default_break() is used instead. If
 * not %NULL, for Vogue versions before 1.16 (module interface version
 * before 1.6.0), this was called instead of vogue_default_break(),
 * but in newer versions, vogue_default_break() is always called and
 * this is called after that to allow tailoring the breaking results.
 *
 * Class structure for #VogueEngineLang
 *
 * Deprecated: 1.38
 **/
struct _VogueEngineLangClass
{
  /*< private >*/
  VogueEngineClass parent_class;

  /*< public >*/
  void (*script_break) (VogueEngineLang *engine,
			const char    *text,
			int            len,
			VogueAnalysis *analysis,
			VogueLogAttr  *attrs,
			int            attrs_len);
};

PANGO_DEPRECATED_IN_1_38
GType vogue_engine_lang_get_type (void) G_GNUC_CONST;

/**
 * PANGO_ENGINE_TYPE_SHAPE:
 *
 * A string constant defining the engine type for shaping engines.
 * These engines derive from #VogueEngineShape.
 *
 * Deprecated: 1.38
 */
#define PANGO_ENGINE_TYPE_SHAPE "VogueEngineShape"

#define PANGO_TYPE_ENGINE_SHAPE              (vogue_engine_shape_get_type ())
#define PANGO_ENGINE_SHAPE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_ENGINE_SHAPE, VogueEngineShape))
#define PANGO_IS_ENGINE_SHAPE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_ENGINE_SHAPE))
#define PANGO_ENGINE_SHAPE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_ENGINE_SHAPE, VogueEngine_ShapeClass))
#define PANGO_IS_ENGINE_SHAPE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_ENGINE_SHAPE))
#define PANGO_ENGINE_SHAPE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_ENGINE_SHAPE, VogueEngineShapeClass))

typedef struct _VogueEngineShapeClass VogueEngineShapeClass;

/**
 * VogueEngineShape:
 *
 * The #VogueEngineShape class is implemented by engines that
 * customize the rendering-system dependent part of the
 * Vogue pipeline for a particular script or language.
 * A #VogueEngineShape implementation is then specific to both
 * a particular rendering system or group of rendering systems
 * and to a particular script. For instance, there is one
 * #VogueEngineShape implementation to handle shaping Arabic
 * for Fontconfig-based backends.
 *
 * Deprecated: 1.38
 **/
struct _VogueEngineShape
{
  VogueEngine parent_instance;
};

/**
 * VogueEngineShapeClass:
 * @script_shape: Given a font, a piece of text, and a #VogueAnalysis
 *   structure, converts characters to glyphs and positions the
 *   resulting glyphs. The results are stored in the #VogueGlyphString
 *   that is passed in. (The implementation should resize it
 *   appropriately using vogue_glyph_string_set_size()). All fields
 *   of the @log_clusters and @glyphs array must be filled in, with
 *   the exception that Vogue will automatically generate
 *   <literal>glyphs->glyphs[i].attr.is_cluster_start</literal>
 *   using the @log_clusters array. Each input character must occur in one
 *   of the output logical clusters;
 *   if no rendering is desired for a character, this may involve
 *   inserting glyphs with the #VogueGlyph ID #PANGO_GLYPH_EMPTY, which
 *   is guaranteed never to render. If the shaping fails for any reason,
 *   the shaper should return with an empty (zero-size) glyph string.
 *   If the shaper has not set the size on the glyph string yet, simply
 *   returning signals the failure too.
 * @covers: Returns the characters that this engine can cover
 *   with a given font for a given language. If not overridden, the default
 *   implementation simply returns the coverage information for the
 *   font itself unmodified.
 *
 * Class structure for #VogueEngineShape
 *
 * Deprecated: 1.38
 **/
struct _VogueEngineShapeClass
{
  /*< private >*/
  VogueEngineClass parent_class;

  /*< public >*/
  void (*script_shape) (VogueEngineShape    *engine,
			VogueFont           *font,
			const char          *item_text,
			unsigned int         item_length,
			const VogueAnalysis *analysis,
			VogueGlyphString    *glyphs,
			const char          *paragraph_text,
			unsigned int         paragraph_length);
  VogueCoverageLevel (*covers)   (VogueEngineShape *engine,
				  VogueFont        *font,
				  VogueLanguage    *language,
				  gunichar          wc);
};

PANGO_DEPRECATED_IN_1_38
GType vogue_engine_shape_get_type (void) G_GNUC_CONST;

typedef struct _VogueEngineInfo VogueEngineInfo;
typedef struct _VogueEngineScriptInfo VogueEngineScriptInfo;

/**
 * VogueEngineScriptInfo:
 * @script: a #VogueScript. The value %PANGO_SCRIPT_COMMON has
 * the special meaning here of "all scripts"
 * @langs: a semicolon separated list of languages that this
 * engine handles for this script. This may be empty,
 * in which case the engine is saying that it is a
 * fallback choice for all languages for this range,
 * but should not be used if another engine
 * indicates that it is specific for the language for
 * a given code point. An entry in this list of "*"
 * indicates that this engine is specific to all
 * languages for this range.
 *
 * The #VogueEngineScriptInfo structure contains
 * information about how the shaper covers a particular script.
 *
 * Deprecated: 1.38
 */
struct _VogueEngineScriptInfo
{
  VogueScript script;
  const gchar *langs;
};

/**
 * VogueEngineInfo:
 * @id: a unique string ID for the engine.
 * @engine_type: a string identifying the engine type.
 * @render_type: a string identifying the render type.
 * @scripts: array of scripts this engine supports.
 * @n_scripts: number of items in @scripts.
 *
 * The #VogueEngineInfo structure contains information about a particular
 * engine. It contains the following fields:
 *
 * Deprecated: 1.38
 */
struct _VogueEngineInfo
{
  const gchar *id;
  const gchar *engine_type;
  const gchar *render_type;
  VogueEngineScriptInfo *scripts;
  gint n_scripts;
};

/* We should to ignore these unprefixed symbols when going through
 * this header with the introspection scanner
 */
#ifndef __GI_SCANNER__

/**
 * script_engine_list: (skip)
 * @engines: location to store a pointer to an array of engines.
 * @n_engines: location to store the number of elements in @engines.
 *
 * Do not use.
 *
 * Deprecated: 1.38
 **/
PANGO_DEPRECATED_IN_1_38
void script_engine_list (VogueEngineInfo **engines,
			 int              *n_engines);

/**
 * script_engine_init: (skip)
 * @module: a #GTypeModule structure used to associate any
 *  GObject types created in this module with the module.
 *
 * Do not use.
 *
 * Deprecated: 1.38
 **/
PANGO_DEPRECATED_IN_1_38
void script_engine_init (GTypeModule *module);


/**
 * script_engine_exit: (skip)
 *
 * Do not use.
 *
 * Deprecated: 1.38
 **/
PANGO_DEPRECATED_IN_1_38
void script_engine_exit (void);

/**
 * script_engine_create: (skip)
 * @id: the ID of an engine as reported by script_engine_list.
 *
 * Do not use.
 *
 * Deprecated: 1.38
 **/
PANGO_DEPRECATED_IN_1_38
VogueEngine *script_engine_create (const char *id);

/* Utility macro used by PANGO_ENGINE_LANG_DEFINE_TYPE and
 * PANGO_ENGINE_LANG_DEFINE_TYPE
 */
#define PANGO_ENGINE_DEFINE_TYPE(name, prefix, class_init, instance_init, parent_type) \
static GType prefix ## _type;						  \
static void								  \
prefix ## _register_type (GTypeModule *module)				  \
{									  \
  const GTypeInfo object_info =						  \
    {									  \
      sizeof (name ## Class),						  \
      (GBaseInitFunc) NULL,						  \
      (GBaseFinalizeFunc) NULL,						  \
      (GClassInitFunc) class_init,					  \
      (GClassFinalizeFunc) NULL,					  \
      NULL,          /* class_data */					  \
      sizeof (name),							  \
      0,             /* n_prelocs */					  \
      (GInstanceInitFunc) instance_init,				  \
      NULL           /* value_table */					  \
    };									  \
									  \
  prefix ## _type =  g_type_module_register_type (module, parent_type,	  \
						  # name,		  \
						  &object_info, 0);	  \
}

/**
 * PANGO_ENGINE_LANG_DEFINE_TYPE:
 * @name: Name of the the type to register (for example:, <literal>ArabicEngineFc</literal>
 * @prefix: Prefix for symbols that will be defined (for example:, <literal>arabic_engine_fc</literal>
 * @class_init: (nullable): Class initialization function for the new type, or %NULL
 * @instance_init: (nullable): Instance initialization function for the new type, or %NULL
 *
 * Outputs the necessary code for GObject type registration for a
 * #VogueEngineLang class defined in a module. Two static symbols
 * are defined.
 *
 * <programlisting>
 *  static GType <replaceable>prefix</replaceable>_type;
 *  static void <replaceable>prefix</replaceable>_register_type (GTypeModule module);
 * </programlisting>
 *
 * The <function><replaceable>prefix</replaceable>_register_type()</function>
 * function should be called in your script_engine_init() function for
 * each type that your module implements, and then your script_engine_create()
 * function can create instances of the object as follows:
 *
 * <informalexample><programlisting>
 *  VogueEngine *engine = g_object_new (<replaceable>prefix</replaceable>_type, NULL);
 * </programlisting></informalexample>
 *
 * Deprecated: 1.38
 **/
#define PANGO_ENGINE_LANG_DEFINE_TYPE(name, prefix, class_init, instance_init)	\
  PANGO_ENGINE_DEFINE_TYPE (name, prefix,				\
			    class_init, instance_init,			\
			    PANGO_TYPE_ENGINE_LANG)

/**
 * PANGO_ENGINE_SHAPE_DEFINE_TYPE:
 * @name: Name of the the type to register (for example:, <literal>ArabicEngineFc</literal>
 * @prefix: Prefix for symbols that will be defined (for example:, <literal>arabic_engine_fc</literal>
 * @class_init: (nullable): Class initialization function for the new type, or %NULL
 * @instance_init: (nullable): Instance initialization function for the new type, or %NULL
 *
 * Outputs the necessary code for GObject type registration for a
 * #VogueEngineShape class defined in a module. Two static symbols
 * are defined.
 *
 * <programlisting>
 *  static GType <replaceable>prefix</replaceable>_type;
 *  static void <replaceable>prefix</replaceable>_register_type (GTypeModule module);
 * </programlisting>
 *
 * The <function><replaceable>prefix</replaceable>_register_type()</function>
 * function should be called in your script_engine_init() function for
 * each type that your module implements, and then your script_engine_create()
 * function can create instances of the object as follows:
 *
 * <informalexample><programlisting>
 *  VogueEngine *engine = g_object_new (<replaceable>prefix</replaceable>_type, NULL);
 * </programlisting></informalexample>
 *
 * Deprecated: 1.38
 **/
#define PANGO_ENGINE_SHAPE_DEFINE_TYPE(name, prefix, class_init, instance_init)	\
  PANGO_ENGINE_DEFINE_TYPE (name, prefix,				\
			    class_init, instance_init,			\
			    PANGO_TYPE_ENGINE_SHAPE)

/* Macro used for possibly builtin Vogue modules. Not useful
 * for externally build modules. If we are compiling a module standalone,
 * then we name the entry points script_engine_list, etc. But if we
 * are compiling it for inclusion directly in Vogue, then we need them to
 * to have distinct names for this module, so we prepend a prefix.
 *
 * The two intermediate macros are to deal with details of the C
 * preprocessor; token pasting tokens must be function arguments,
 * and macro substitution isn't used on function arguments that
 * are used for token pasting.
 */
#ifdef PANGO_MODULE_PREFIX
#define PANGO_MODULE_ENTRY(func) _PANGO_MODULE_ENTRY2(PANGO_MODULE_PREFIX,func)
#define _PANGO_MODULE_ENTRY2(prefix,func) _PANGO_MODULE_ENTRY3(prefix,func)
#define _PANGO_MODULE_ENTRY3(prefix,func) prefix##_script_engine_##func
#else
#define PANGO_MODULE_ENTRY(func) script_engine_##func
#endif

#endif /* PANGO_DISABLE_DEPRECATED */

#endif /* __GI_SCANNER__ */

G_END_DECLS

#endif /* __PANGO_ENGINE_H__ */
