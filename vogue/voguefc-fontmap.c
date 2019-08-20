/* Vogue
 * voguefc-fontmap.c: Base fontmap type for fontconfig-based backends
 *
 * Copyright (C) 2000-2003 Red Hat, Inc.
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
 * SECTION:voguefc-fontmap
 * @short_description:Base fontmap class for Fontconfig-based backends
 * @title:VogueFcFontMap
 * @see_also:
 * <variablelist><varlistentry>
 * <term>#VogueFcFont</term>
 * <listitem>The base class for fonts; creating a new
 * Fontconfig-based backend involves deriving from both
 * #VogueFcFontMap and #VogueFcFont.</listitem>
 * </varlistentry></variablelist>
 *
 * VogueFcFontMap is a base class for font map implementations using the
 * Fontconfig and FreeType libraries. It is used in the
 * <link linkend="vogue-Xft-Fonts-and-Rendering">Xft</link> and
 * <link linkend="vogue-FreeType-Fonts-and-Rendering">FreeType</link>
 * backends shipped with Vogue, but can also be used when creating
 * new backends. Any backend deriving from this base class will
 * take advantage of the wide range of shapers implemented using
 * FreeType that come with Vogue.
 */
#define FONTSET_CACHE_SIZE 256

#include "config.h"
#include <math.h>

#include "vogue-context.h"
#include "vogue-font-private.h"
#include "voguefc-fontmap-private.h"
#include "voguefc-private.h"
#include "vogue-impl-utils.h"
#include "vogue-enum-types.h"
#include "vogue-coverage-private.h"
#include <hb-ft.h>


/* Overview:
 *
 * All programming is a practice in caching data.  VogueFcFontMap is the
 * major caching container of a Vogue system on a Linux desktop.  Here is
 * a short overview of how it all works.
 *
 * In short, Fontconfig search patterns are constructed and a fontset loaded
 * using them.  Here is how we achieve that:
 *
 * - All FcPattern's referenced by any object in the fontmap are uniquified
 *   and cached in the fontmap.  This both speeds lookups based on patterns
 *   faster, and saves memory.  This is handled by fontmap->priv->pattern_hash.
 *   The patterns are cached indefinitely.
 *
 * - The results of a FcFontSort() are used to populate fontsets.  However,
 *   FcFontSort() relies on the search pattern only, which includes the font
 *   size but not the full font matrix.  The fontset however depends on the
 *   matrix.  As a result, multiple fontsets may need results of the
 *   FcFontSort() on the same input pattern (think rotating text).  As such,
 *   we cache FcFontSort() results in fontmap->priv->patterns_hash which
 *   is a refcounted structure.  This level of abstraction also allows for
 *   optimizations like calling FcFontMatch() instead of FcFontSort(), and
 *   only calling FcFontSort() if any patterns other than the first match
 *   are needed.  Another possible optimization would be to call FcFontSort()
 *   without trimming, and do the trimming lazily as we go.  Only pattern sets
 *   already referenced by a fontset are cached.
 *
 * - A number of most-recently-used fontsets are cached and reused when
 *   needed.  This is achieved using fontmap->priv->fontset_hash and
 *   fontmap->priv->fontset_cache.
 *
 * - All fonts created by any of our fontsets are also cached and reused.
 *   This is what fontmap->priv->font_hash does.
 *
 * - Data that only depends on the font file and face index is cached and
 *   reused by multiple fonts.  This includes coverage and cmap cache info.
 *   This is done using fontmap->priv->font_face_data_hash.
 *
 * Upon a cache_clear() request, all caches are emptied.  All objects (fonts,
 * fontsets, faces, families) having a reference from outside will still live
 * and may reference the fontmap still, but will not be reused by the fontmap.
 *
 *
 * Todo:
 *
 * - Make VogueCoverage a GObject and subclass it as VogueFcCoverage which
 *   will directly use FcCharset. (#569622)
 *
 * - Lazy trimming of FcFontSort() results.  Requires fontconfig with
 *   FcCharSetMerge().
 */


typedef struct _VogueFcFontFaceData VogueFcFontFaceData;
typedef struct _VogueFcFace         VogueFcFace;
typedef struct _VogueFcFamily       VogueFcFamily;
typedef struct _VogueFcFindFuncInfo VogueFcFindFuncInfo;
typedef struct _VogueFcPatterns     VogueFcPatterns;
typedef struct _VogueFcFontset      VogueFcFontset;

#define PANGO_FC_TYPE_FAMILY            (vogue_fc_family_get_type ())
#define PANGO_FC_FAMILY(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FAMILY, VogueFcFamily))
#define PANGO_FC_IS_FAMILY(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FAMILY))

#define PANGO_FC_TYPE_FACE              (vogue_fc_face_get_type ())
#define PANGO_FC_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FACE, VogueFcFace))
#define PANGO_FC_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FACE))

#define PANGO_FC_TYPE_FONTSET           (vogue_fc_fontset_get_type ())
#define PANGO_FC_FONTSET(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FONTSET, VogueFcFontset))
#define PANGO_FC_IS_FONTSET(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FONTSET))

struct _VogueFcFontMapPrivate
{
  GHashTable *fontset_hash;	/* Maps VogueFcFontsetKey -> VogueFcFontset  */
  GQueue *fontset_cache;	/* Recently used fontsets */

  GHashTable *font_hash;	/* Maps VogueFcFontKey -> VogueFcFont */

  GHashTable *patterns_hash;	/* Maps FcPattern -> VogueFcPatterns */

  /* pattern_hash is used to make sure we only store one copy of
   * each identical pattern. (Speeds up lookup).
   */
  GHashTable *pattern_hash;

  GHashTable *font_face_data_hash; /* Maps font file name/id -> data */

  /* List of all families availible */
  VogueFcFamily **families;
  int n_families;		/* -1 == uninitialized */

  double dpi;

  /* Decoders */
  GSList *findfuncs;

  guint closed : 1;

  FcConfig *config;
};

struct _VogueFcFontFaceData
{
  /* Key */
  char *filename;
  int id;            /* needed to handle TTC files with multiple faces */

  /* Data */
  FcPattern *pattern;	/* Referenced pattern that owns filename */
  VogueCoverage *coverage;

  hb_face_t *hb_face;
};

struct _VogueFcFace
{
  VogueFontFace parent_instance;

  VogueFcFamily *family;
  char *style;
  FcPattern *pattern;

  guint fake : 1;
};

struct _VogueFcFamily
{
  VogueFontFamily parent_instance;

  VogueFcFontMap *fontmap;
  char *family_name;

  FcFontSet *patterns;
  VogueFcFace **faces;
  int n_faces;		/* -1 == uninitialized */

  int spacing;  /* FC_SPACING */
  gboolean variable;
};

struct _VogueFcFindFuncInfo
{
  VogueFcDecoderFindFunc findfunc;
  gpointer               user_data;
  GDestroyNotify         dnotify;
  gpointer               ddata;
};

static GType    vogue_fc_family_get_type     (void);
static GType    vogue_fc_face_get_type       (void);
static GType    vogue_fc_fontset_get_type    (void);

static void          vogue_fc_font_map_finalize      (GObject                      *object);
static VogueFont *   vogue_fc_font_map_load_font     (VogueFontMap                 *fontmap,
						       VogueContext                 *context,
						       const VogueFontDescription   *description);
static VogueFontset *vogue_fc_font_map_load_fontset  (VogueFontMap                 *fontmap,
						       VogueContext                 *context,
						       const VogueFontDescription   *desc,
						       VogueLanguage                *language);
static void          vogue_fc_font_map_list_families (VogueFontMap                 *fontmap,
						       VogueFontFamily            ***families,
						       int                          *n_families);

static double vogue_fc_font_map_get_resolution (VogueFcFontMap *fcfontmap,
						VogueContext   *context);
static VogueFont *vogue_fc_font_map_new_font   (VogueFcFontMap    *fontmap,
						VogueFcFontsetKey *fontset_key,
						FcPattern         *match);

static guint    vogue_fc_font_face_data_hash  (VogueFcFontFaceData *key);
static gboolean vogue_fc_font_face_data_equal (VogueFcFontFaceData *key1,
					       VogueFcFontFaceData *key2);

static void               vogue_fc_fontset_key_init  (VogueFcFontsetKey          *key,
						      VogueFcFontMap             *fcfontmap,
						      VogueContext               *context,
						      const VogueFontDescription *desc,
						      VogueLanguage              *language);
static VogueFcFontsetKey *vogue_fc_fontset_key_copy  (const VogueFcFontsetKey *key);
static void               vogue_fc_fontset_key_free  (VogueFcFontsetKey       *key);
static guint              vogue_fc_fontset_key_hash  (const VogueFcFontsetKey *key);
static gboolean           vogue_fc_fontset_key_equal (const VogueFcFontsetKey *key_a,
						      const VogueFcFontsetKey *key_b);

static void               vogue_fc_font_key_init     (VogueFcFontKey       *key,
						      VogueFcFontMap       *fcfontmap,
						      VogueFcFontsetKey    *fontset_key,
						      FcPattern            *pattern);
static VogueFcFontKey    *vogue_fc_font_key_copy     (const VogueFcFontKey *key);
static void               vogue_fc_font_key_free     (VogueFcFontKey       *key);
static guint              vogue_fc_font_key_hash     (const VogueFcFontKey *key);
static gboolean           vogue_fc_font_key_equal    (const VogueFcFontKey *key_a,
						      const VogueFcFontKey *key_b);

static VogueFcPatterns *vogue_fc_patterns_new   (FcPattern       *pat,
						 VogueFcFontMap  *fontmap);
static VogueFcPatterns *vogue_fc_patterns_ref   (VogueFcPatterns *pats);
static void             vogue_fc_patterns_unref (VogueFcPatterns *pats);
static FcPattern       *vogue_fc_patterns_get_pattern      (VogueFcPatterns *pats);
static FcPattern       *vogue_fc_patterns_get_font_pattern (VogueFcPatterns *pats,
							    int              i,
							    gboolean        *prepare);

static FcPattern *uniquify_pattern (VogueFcFontMap *fcfontmap,
				    FcPattern      *pattern);

gpointer get_gravity_class (void);

gpointer
get_gravity_class (void)
{
  static GEnumClass *class = NULL; /* MT-safe */

  if (g_once_init_enter (&class))
    g_once_init_leave (&class, (gpointer)g_type_class_ref (PANGO_TYPE_GRAVITY));

  return class;
}

static guint
vogue_fc_font_face_data_hash (VogueFcFontFaceData *key)
{
  return g_str_hash (key->filename) ^ key->id;
}

static gboolean
vogue_fc_font_face_data_equal (VogueFcFontFaceData *key1,
			       VogueFcFontFaceData *key2)
{
  return key1->id == key2->id &&
	 (key1 == key2 || 0 == strcmp (key1->filename, key2->filename));
}

static void
vogue_fc_font_face_data_free (VogueFcFontFaceData *data)
{
  FcPatternDestroy (data->pattern);

  if (data->coverage)
    vogue_coverage_unref (data->coverage);

  hb_face_destroy (data->hb_face);

  g_slice_free (VogueFcFontFaceData, data);
}

/* Fowler / Noll / Vo (FNV) Hash (http://www.isthe.com/chongo/tech/comp/fnv/)
 *
 * Not necessarily better than a lot of other hashes, but should be OK, and
 * well tested with binary data.
 */

#define FNV_32_PRIME ((guint32)0x01000193)
#define FNV1_32_INIT ((guint32)0x811c9dc5)

static guint32
hash_bytes_fnv (unsigned char *buffer,
		int            len,
		guint32        hval)
{
  while (len--)
    {
      hval *= FNV_32_PRIME;
      hval ^= *buffer++;
    }

  return hval;
}

static void
get_context_matrix (VogueContext *context,
		    VogueMatrix *matrix)
{
  const VogueMatrix *set_matrix;
  const VogueMatrix identity = PANGO_MATRIX_INIT;

  set_matrix = context ? vogue_context_get_matrix (context) : NULL;
  *matrix = set_matrix ? *set_matrix : identity;
  matrix->x0 = matrix->y0 = 0.;
}

static int
get_scaled_size (VogueFcFontMap             *fcfontmap,
		 VogueContext               *context,
		 const VogueFontDescription *desc)
{
  double size = vogue_font_description_get_size (desc);

  if (!vogue_font_description_get_size_is_absolute (desc))
    {
      double dpi = vogue_fc_font_map_get_resolution (fcfontmap, context);

      size = size * dpi / 72.;
    }

  return .5 + vogue_matrix_get_font_scale_factor (vogue_context_get_matrix (context)) * size;
}



struct _VogueFcFontsetKey {
  VogueFcFontMap *fontmap;
  VogueLanguage *language;
  VogueFontDescription *desc;
  VogueMatrix matrix;
  int pixelsize;
  double resolution;
  gpointer context_key;
  char *variations;
};

struct _VogueFcFontKey {
  VogueFcFontMap *fontmap;
  FcPattern *pattern;
  VogueMatrix matrix;
  gpointer context_key;
  char *variations;
};

static void
vogue_fc_fontset_key_init (VogueFcFontsetKey          *key,
			   VogueFcFontMap             *fcfontmap,
			   VogueContext               *context,
			   const VogueFontDescription *desc,
			   VogueLanguage              *language)
{
  if (!language && context)
    language = vogue_context_get_language (context);

  key->fontmap = fcfontmap;
  get_context_matrix (context, &key->matrix);
  key->pixelsize = get_scaled_size (fcfontmap, context, desc);
  key->resolution = vogue_fc_font_map_get_resolution (fcfontmap, context);
  key->language = language;
  key->variations = g_strdup (vogue_font_description_get_variations (desc));
  key->desc = vogue_font_description_copy_static (desc);
  vogue_font_description_unset_fields (key->desc, PANGO_FONT_MASK_SIZE | PANGO_FONT_MASK_VARIATIONS);

  if (context && PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get)
    key->context_key = (gpointer)PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get (fcfontmap, context);
  else
    key->context_key = NULL;
}

static gboolean
vogue_fc_fontset_key_equal (const VogueFcFontsetKey *key_a,
			    const VogueFcFontsetKey *key_b)
{
  if (key_a->language == key_b->language &&
      key_a->pixelsize == key_b->pixelsize &&
      key_a->resolution == key_b->resolution &&
      ((key_a->variations == NULL && key_b->variations == NULL) ||
       (key_a->variations && key_b->variations && (strcmp (key_a->variations, key_b->variations) == 0))) &&
      vogue_font_description_equal (key_a->desc, key_b->desc) &&
      0 == memcmp (&key_a->matrix, &key_b->matrix, 4 * sizeof (double)))
    {
      if (key_a->context_key)
	return PANGO_FC_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
										key_a->context_key,
										key_b->context_key);
      else
        return key_a->context_key == key_b->context_key;
    }
  else
    return FALSE;
}

static guint
vogue_fc_fontset_key_hash (const VogueFcFontsetKey *key)
{
    guint32 hash = FNV1_32_INIT;

    /* We do a bytewise hash on the doubles */
    hash = hash_bytes_fnv ((unsigned char *)(&key->matrix), sizeof (double) * 4, hash);
    hash = hash_bytes_fnv ((unsigned char *)(&key->resolution), sizeof (double), hash);

    hash ^= key->pixelsize;

    if (key->variations)
      hash ^= g_str_hash (key->variations);

    if (key->context_key)
      hash ^= PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap,
									    key->context_key);

    return (hash ^
	    GPOINTER_TO_UINT (key->language) ^
	    vogue_font_description_hash (key->desc));
}

static void
vogue_fc_fontset_key_free (VogueFcFontsetKey *key)
{
  vogue_font_description_free (key->desc);
  g_free (key->variations);

  if (key->context_key)
    PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap,
								  key->context_key);

  g_slice_free (VogueFcFontsetKey, key);
}

static VogueFcFontsetKey *
vogue_fc_fontset_key_copy (const VogueFcFontsetKey *old)
{
  VogueFcFontsetKey *key = g_slice_new (VogueFcFontsetKey);

  key->fontmap = old->fontmap;
  key->language = old->language;
  key->desc = vogue_font_description_copy (old->desc);
  key->matrix = old->matrix;
  key->pixelsize = old->pixelsize;
  key->resolution = old->resolution;
  key->variations = g_strdup (old->variations);

  if (old->context_key)
    key->context_key = PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap,
										     old->context_key);
  else
    key->context_key = NULL;

  return key;
}

/**
 * vogue_fc_fontset_key_get_language:
 * @key: the fontset key
 *
 * Gets the language member of @key.
 *
 * Returns: the language
 *
 * Since: 1.24
 **/
VogueLanguage *
vogue_fc_fontset_key_get_language (const VogueFcFontsetKey *key)
{
  return key->language;
}

/**
 * vogue_fc_fontset_key_get_description:
 * @key: the fontset key
 *
 * Gets the font description of @key.
 *
 * Returns: the font description, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
const VogueFontDescription *
vogue_fc_fontset_key_get_description (const VogueFcFontsetKey *key)
{
  return key->desc;
}

/**
 * vogue_fc_fontset_key_get_matrix:
 * @key: the fontset key
 *
 * Gets the matrix member of @key.
 *
 * Returns: the matrix, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
const VogueMatrix *
vogue_fc_fontset_key_get_matrix      (const VogueFcFontsetKey *key)
{
  return &key->matrix;
}

/**
 * vogue_fc_fontset_key_get_absolute_size:
 * @key: the fontset key
 *
 * Gets the absolute font size of @key in Vogue units.  This is adjusted
 * for both resolution and transformation matrix.
 *
 * Returns: the pixel size of @key.
 *
 * Since: 1.24
 **/
double
vogue_fc_fontset_key_get_absolute_size   (const VogueFcFontsetKey *key)
{
  return key->pixelsize;
}

/**
 * vogue_fc_fontset_key_get_resolution:
 * @key: the fontset key
 *
 * Gets the resolution of @key
 *
 * Returns: the resolution of @key
 *
 * Since: 1.24
 **/
double
vogue_fc_fontset_key_get_resolution  (const VogueFcFontsetKey *key)
{
  return key->resolution;
}

/**
 * vogue_fc_fontset_key_get_context_key:
 * @key: the font key
 *
 * Gets the context key member of @key.
 *
 * Returns: the context key, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
gpointer
vogue_fc_fontset_key_get_context_key (const VogueFcFontsetKey *key)
{
  return key->context_key;
}

/*
 * VogueFcFontKey
 */

static gboolean
vogue_fc_font_key_equal (const VogueFcFontKey *key_a,
			 const VogueFcFontKey *key_b)
{
  if (key_a->pattern == key_b->pattern &&
      ((key_a->variations == NULL && key_b->variations == NULL) ||
       (key_a->variations && key_b->variations && (strcmp (key_a->variations, key_b->variations) == 0))) &&
      0 == memcmp (&key_a->matrix, &key_b->matrix, 4 * sizeof (double)))
    {
      if (key_a->context_key && key_b->context_key)
	return PANGO_FC_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
										key_a->context_key,
										key_b->context_key);
      else
        return key_a->context_key == key_b->context_key;
    }
  else
    return FALSE;
}

static guint
vogue_fc_font_key_hash (const VogueFcFontKey *key)
{
    guint32 hash = FNV1_32_INIT;

    /* We do a bytewise hash on the doubles */
    hash = hash_bytes_fnv ((unsigned char *)(&key->matrix), sizeof (double) * 4, hash);

    if (key->variations)
      hash ^= g_str_hash (key->variations);

    if (key->context_key)
      hash ^= PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap,
									    key->context_key);

    return (hash ^ GPOINTER_TO_UINT (key->pattern));
}

static void
vogue_fc_font_key_free (VogueFcFontKey *key)
{
  if (key->pattern)
    FcPatternDestroy (key->pattern);

  if (key->context_key)
    PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap,
								  key->context_key);

  g_free (key->variations);

  g_slice_free (VogueFcFontKey, key);
}

static VogueFcFontKey *
vogue_fc_font_key_copy (const VogueFcFontKey *old)
{
  VogueFcFontKey *key = g_slice_new (VogueFcFontKey);

  key->fontmap = old->fontmap;
  FcPatternReference (old->pattern);
  key->pattern = old->pattern;
  key->matrix = old->matrix;
  key->variations = g_strdup (old->variations);
  if (old->context_key)
    key->context_key = PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap,
										     old->context_key);
  else
    key->context_key = NULL;

  return key;
}

static void
vogue_fc_font_key_init (VogueFcFontKey    *key,
			VogueFcFontMap    *fcfontmap,
			VogueFcFontsetKey *fontset_key,
			FcPattern         *pattern)
{
  key->fontmap = fcfontmap;
  key->pattern = pattern;
  key->matrix = *vogue_fc_fontset_key_get_matrix (fontset_key);
  key->variations = fontset_key->variations;
  key->context_key = vogue_fc_fontset_key_get_context_key (fontset_key);
}

/* Public API */

/**
 * vogue_fc_font_key_get_pattern:
 * @key: the font key
 *
 * Gets the fontconfig pattern member of @key.
 *
 * Returns: the pattern, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
const FcPattern *
vogue_fc_font_key_get_pattern (const VogueFcFontKey *key)
{
  return key->pattern;
}

/**
 * vogue_fc_font_key_get_matrix:
 * @key: the font key
 *
 * Gets the matrix member of @key.
 *
 * Returns: the matrix, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
const VogueMatrix *
vogue_fc_font_key_get_matrix (const VogueFcFontKey *key)
{
  return &key->matrix;
}

/**
 * vogue_fc_font_key_get_context_key:
 * @key: the font key
 *
 * Gets the context key member of @key.
 *
 * Returns: the context key, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
gpointer
vogue_fc_font_key_get_context_key (const VogueFcFontKey *key)
{
  return key->context_key;
}

const char *
vogue_fc_font_key_get_variations (const VogueFcFontKey *key)
{
  return key->variations;
}

/*
 * VogueFcPatterns
 */

struct _VogueFcPatterns {
  guint ref_count;

  VogueFcFontMap *fontmap;

  FcPattern *pattern;
  FcPattern *match;
  FcFontSet *fontset;
};

static VogueFcPatterns *
vogue_fc_patterns_new (FcPattern *pat, VogueFcFontMap *fontmap)
{
  VogueFcPatterns *pats;

  pat = uniquify_pattern (fontmap, pat);
  pats = g_hash_table_lookup (fontmap->priv->patterns_hash, pat);
  if (pats)
    return vogue_fc_patterns_ref (pats);

  pats = g_slice_new0 (VogueFcPatterns);

  pats->fontmap = fontmap;

  pats->ref_count = 1;
  FcPatternReference (pat);
  pats->pattern = pat;

  g_hash_table_insert (fontmap->priv->patterns_hash,
		       pats->pattern, pats);

  return pats;
}

static VogueFcPatterns *
vogue_fc_patterns_ref (VogueFcPatterns *pats)
{
  g_return_val_if_fail (pats->ref_count > 0, NULL);

  pats->ref_count++;

  return pats;
}

static void
vogue_fc_patterns_unref (VogueFcPatterns *pats)
{
  g_return_if_fail (pats->ref_count > 0);

  pats->ref_count--;

  if (pats->ref_count)
    return;

  /* Only remove from fontmap hash if we are in it.  This is not necessarily
   * the case after a cache_clear() call. */
  if (pats->fontmap->priv->patterns_hash &&
      pats == g_hash_table_lookup (pats->fontmap->priv->patterns_hash, pats->pattern))
    g_hash_table_remove (pats->fontmap->priv->patterns_hash,
			 pats->pattern);

  if (pats->pattern)
    FcPatternDestroy (pats->pattern);

  if (pats->match)
    FcPatternDestroy (pats->match);

  if (pats->fontset)
    FcFontSetDestroy (pats->fontset);

  g_slice_free (VogueFcPatterns, pats);
}

static FcPattern *
vogue_fc_patterns_get_pattern (VogueFcPatterns *pats)
{
  return pats->pattern;
}

static gboolean
vogue_fc_is_supported_font_format (const char *fontformat)
{
  /* harfbuzz supports only SFNT fonts. */
  /* FIXME: "CFF" is used for both CFF in OpenType and bare CFF files, but
   * HarfBuzz does not support the later and FontConfig does not seem
   * to have a way to tell them apart.
   */
  if (g_ascii_strcasecmp (fontformat, "TrueType") == 0 ||
      g_ascii_strcasecmp (fontformat, "CFF") == 0)
    return TRUE;
  return FALSE;
}

static FcFontSet *
filter_fontset_by_format (FcFontSet *fontset)
{
  FcFontSet *result;
  int i;

  result = FcFontSetCreate ();

  for (i = 0; i < fontset->nfont; i++)
    {
      FcResult res;
      const char *s;

      res = FcPatternGetString (fontset->fonts[i], FC_FONTFORMAT, 0, (FcChar8 **)(void*)&s);
      g_assert (res == FcResultMatch);
      if (vogue_fc_is_supported_font_format (s))
        FcFontSetAdd (result, FcPatternDuplicate (fontset->fonts[i]));
    }

  return result;
}

static FcPattern *
vogue_fc_patterns_get_font_pattern (VogueFcPatterns *pats, int i, gboolean *prepare)
{
  if (i == 0)
    {
      FcResult result;
      if (!pats->match && !pats->fontset)
	pats->match = FcFontMatch (pats->fontmap->priv->config, pats->pattern, &result);

      if (pats->match)
	{
	  *prepare = FALSE;
	  return pats->match;
	}
    }
  else
    {
      if (!pats->fontset)
        {
	  FcResult result;
          FcFontSet *fontset;
          FcFontSet *filtered;

	  fontset = FcFontSort (pats->fontmap->priv->config, pats->pattern, FcFalse, NULL, &result);
          filtered = filter_fontset_by_format (fontset);
          FcFontSetDestroy (fontset);

          pats->fontset = FcFontSetSort (pats->fontmap->priv->config, &filtered, 1, pats->pattern, FcTrue, NULL, &result);

          FcFontSetDestroy (filtered);

	  if (pats->match)
	    {
	      FcPatternDestroy (pats->match);
	      pats->match = NULL;
	    }
	}
    }

  *prepare = TRUE;
  if (pats->fontset && i < pats->fontset->nfont)
    return pats->fontset->fonts[i];
  else
    return NULL;
}


/*
 * VogueFcFontset
 */

static void              vogue_fc_fontset_finalize     (GObject                 *object);
static VogueLanguage *   vogue_fc_fontset_get_language (VogueFontset            *fontset);
static  VogueFont *      vogue_fc_fontset_get_font     (VogueFontset            *fontset,
							guint                    wc);
static void              vogue_fc_fontset_foreach      (VogueFontset            *fontset,
							VogueFontsetForeachFunc  func,
							gpointer                 data);

struct _VogueFcFontset
{
  VogueFontset parent_instance;

  VogueFcFontsetKey *key;

  VogueFcPatterns *patterns;
  int patterns_i;

  GPtrArray *fonts;
  GPtrArray *coverages;

  GList *cache_link;
};

typedef VogueFontsetClass VogueFcFontsetClass;

G_DEFINE_TYPE (VogueFcFontset, vogue_fc_fontset, PANGO_TYPE_FONTSET)

static VogueFcFontset *
vogue_fc_fontset_new (VogueFcFontsetKey *key,
		      VogueFcPatterns   *patterns)
{
  VogueFcFontset *fontset;

  fontset = g_object_new (PANGO_FC_TYPE_FONTSET, NULL);

  fontset->key = vogue_fc_fontset_key_copy (key);
  fontset->patterns = vogue_fc_patterns_ref (patterns);

  return fontset;
}

static VogueFcFontsetKey *
vogue_fc_fontset_get_key (VogueFcFontset *fontset)
{
  return fontset->key;
}

static VogueFont *
vogue_fc_fontset_load_next_font (VogueFcFontset *fontset)
{
  FcPattern *pattern, *font_pattern;
  VogueFont *font;
  gboolean prepare;

  pattern = vogue_fc_patterns_get_pattern (fontset->patterns);
  font_pattern = vogue_fc_patterns_get_font_pattern (fontset->patterns,
						     fontset->patterns_i++,
						     &prepare);
  if (G_UNLIKELY (!font_pattern))
    return NULL;

  if (prepare)
    {
      font_pattern = FcFontRenderPrepare (NULL, pattern, font_pattern);

      if (G_UNLIKELY (!font_pattern))
	return NULL;
    }

  font = vogue_fc_font_map_new_font (fontset->key->fontmap,
                                     fontset->key,
                                     font_pattern);

  if (prepare)
    FcPatternDestroy (font_pattern);

  return font;
}

static VogueFont *
vogue_fc_fontset_get_font_at (VogueFcFontset *fontset,
			      unsigned int    i)
{
  while (i >= fontset->fonts->len)
    {
      VogueFont *font = vogue_fc_fontset_load_next_font (fontset);
      g_ptr_array_add (fontset->fonts, font);
      g_ptr_array_add (fontset->coverages, NULL);
      if (!font)
        return NULL;
    }

  return g_ptr_array_index (fontset->fonts, i);
}

static void
vogue_fc_fontset_class_init (VogueFcFontsetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontsetClass *fontset_class = PANGO_FONTSET_CLASS (class);

  object_class->finalize = vogue_fc_fontset_finalize;

  fontset_class->get_font = vogue_fc_fontset_get_font;
  fontset_class->get_language = vogue_fc_fontset_get_language;
  fontset_class->foreach = vogue_fc_fontset_foreach;
}

static void
vogue_fc_fontset_init (VogueFcFontset *fontset)
{
  fontset->fonts = g_ptr_array_new ();
  fontset->coverages = g_ptr_array_new ();
}

static void
vogue_fc_fontset_finalize (GObject *object)
{
  VogueFcFontset *fontset = PANGO_FC_FONTSET (object);
  unsigned int i;

  for (i = 0; i < fontset->fonts->len; i++)
  {
    VogueFont *font = g_ptr_array_index(fontset->fonts, i);
    if (font)
      g_object_unref (font);
  }
  g_ptr_array_free (fontset->fonts, TRUE);

  for (i = 0; i < fontset->coverages->len; i++)
    {
      VogueCoverage *coverage = g_ptr_array_index (fontset->coverages, i);
      if (coverage)
	vogue_coverage_unref (coverage);
    }
  g_ptr_array_free (fontset->coverages, TRUE);

  if (fontset->key)
    vogue_fc_fontset_key_free (fontset->key);

  if (fontset->patterns)
    vogue_fc_patterns_unref (fontset->patterns);

  G_OBJECT_CLASS (vogue_fc_fontset_parent_class)->finalize (object);
}

static VogueLanguage *
vogue_fc_fontset_get_language (VogueFontset  *fontset)
{
  VogueFcFontset *fcfontset = PANGO_FC_FONTSET (fontset);

  return vogue_fc_fontset_key_get_language (vogue_fc_fontset_get_key (fcfontset));
}

static VogueFont *
vogue_fc_fontset_get_font (VogueFontset  *fontset,
			   guint          wc)
{
  VogueFcFontset *fcfontset = PANGO_FC_FONTSET (fontset);
  VogueCoverageLevel best_level = PANGO_COVERAGE_NONE;
  VogueCoverageLevel level;
  VogueFont *font;
  VogueCoverage *coverage;
  int result = -1;
  unsigned int i;

  for (i = 0;
       vogue_fc_fontset_get_font_at (fcfontset, i);
       i++)
    {
      coverage = g_ptr_array_index (fcfontset->coverages, i);

      if (coverage == NULL)
	{
	  font = g_ptr_array_index (fcfontset->fonts, i);

	  coverage = vogue_font_get_coverage (font, fcfontset->key->language);
	  g_ptr_array_index (fcfontset->coverages, i) = coverage;
	}

      level = vogue_coverage_get (coverage, wc);

      if (result == -1 || level > best_level)
	{
	  result = i;
	  best_level = level;
	  if (level == PANGO_COVERAGE_EXACT)
	    break;
	}
    }

  if (G_UNLIKELY (result == -1))
    return NULL;

  font = g_ptr_array_index (fcfontset->fonts, result);
  return g_object_ref (font);
}

static void
vogue_fc_fontset_foreach (VogueFontset           *fontset,
			  VogueFontsetForeachFunc func,
			  gpointer                data)
{
  VogueFcFontset *fcfontset = PANGO_FC_FONTSET (fontset);
  VogueFont *font;
  unsigned int i;

  for (i = 0;
       (font = vogue_fc_fontset_get_font_at (fcfontset, i));
       i++)
    {
      if ((*func) (fontset, font, data))
	return;
    }
}


/*
 * VogueFcFontMap
 */

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (VogueFcFontMap, vogue_fc_font_map, PANGO_TYPE_FONT_MAP,
                                  G_ADD_PRIVATE (VogueFcFontMap))

static void
vogue_fc_font_map_init (VogueFcFontMap *fcfontmap)
{
  VogueFcFontMapPrivate *priv;

  priv = fcfontmap->priv = vogue_fc_font_map_get_instance_private (fcfontmap);

  priv->n_families = -1;

  priv->font_hash = g_hash_table_new ((GHashFunc)vogue_fc_font_key_hash,
				      (GEqualFunc)vogue_fc_font_key_equal);

  priv->fontset_hash = g_hash_table_new_full ((GHashFunc)vogue_fc_fontset_key_hash,
					      (GEqualFunc)vogue_fc_fontset_key_equal,
					      NULL,
					      (GDestroyNotify)g_object_unref);
  priv->fontset_cache = g_queue_new ();

  priv->patterns_hash = g_hash_table_new (NULL, NULL);

  priv->pattern_hash = g_hash_table_new_full ((GHashFunc) FcPatternHash,
					      (GEqualFunc) FcPatternEqual,
					      (GDestroyNotify) FcPatternDestroy,
					      NULL);

  priv->font_face_data_hash = g_hash_table_new_full ((GHashFunc)vogue_fc_font_face_data_hash,
						     (GEqualFunc)vogue_fc_font_face_data_equal,
						     (GDestroyNotify)vogue_fc_font_face_data_free,
						     NULL);
  priv->dpi = -1;
}

static void
vogue_fc_font_map_fini (VogueFcFontMap *fcfontmap)
{
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  int i;

  g_queue_free (priv->fontset_cache);
  priv->fontset_cache = NULL;

  g_hash_table_destroy (priv->fontset_hash);
  priv->fontset_hash = NULL;

  g_hash_table_destroy (priv->patterns_hash);
  priv->patterns_hash = NULL;

  g_hash_table_destroy (priv->font_hash);
  priv->font_hash = NULL;

  g_hash_table_destroy (priv->font_face_data_hash);
  priv->font_face_data_hash = NULL;

  g_hash_table_destroy (priv->pattern_hash);
  priv->pattern_hash = NULL;

  for (i = 0; i < priv->n_families; i++)
    g_object_unref (priv->families[i]);
  g_free (priv->families);
  priv->n_families = -1;
  priv->families = NULL;
}

static void
vogue_fc_font_map_class_init (VogueFcFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);

  object_class->finalize = vogue_fc_font_map_finalize;
  fontmap_class->load_font = vogue_fc_font_map_load_font;
  fontmap_class->load_fontset = vogue_fc_font_map_load_fontset;
  fontmap_class->list_families = vogue_fc_font_map_list_families;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_FC;
}


/**
 * vogue_fc_font_map_add_decoder_find_func:
 * @fcfontmap: The #VogueFcFontMap to add this method to.
 * @findfunc: The #VogueFcDecoderFindFunc callback function
 * @user_data: User data.
 * @dnotify: A #GDestroyNotify callback that will be called when the
 *  fontmap is finalized and the decoder is released.
 *
 * This function saves a callback method in the #VogueFcFontMap that
 * will be called whenever new fonts are created.  If the
 * function returns a #VogueFcDecoder, that decoder will be used to
 * determine both coverage via a #FcCharSet and a one-to-one mapping of
 * characters to glyphs.  This will allow applications to have
 * application-specific encodings for various fonts.
 *
 * Since: 1.6
 **/
void
vogue_fc_font_map_add_decoder_find_func (VogueFcFontMap        *fcfontmap,
					 VogueFcDecoderFindFunc findfunc,
					 gpointer               user_data,
					 GDestroyNotify         dnotify)
{
  VogueFcFontMapPrivate *priv;
  VogueFcFindFuncInfo *info;

  g_return_if_fail (PANGO_IS_FC_FONT_MAP (fcfontmap));

  priv = fcfontmap->priv;

  info = g_slice_new (VogueFcFindFuncInfo);

  info->findfunc = findfunc;
  info->user_data = user_data;
  info->dnotify = dnotify;

  priv->findfuncs = g_slist_append (priv->findfuncs, info);
}

/**
 * vogue_fc_font_map_find_decoder:
 * @fcfontmap: The #VogueFcFontMap to use.
 * @pattern: The #FcPattern to find the decoder for.
 *
 * Finds the decoder to use for @pattern.  Decoders can be added to
 * a font map using vogue_fc_font_map_add_decoder_find_func().
 *
 * Returns: (transfer full) (nullable): a newly created #VogueFcDecoder
 *   object or %NULL if no decoder is set for @pattern.
 *
 * Since: 1.26
 **/
VogueFcDecoder *
vogue_fc_font_map_find_decoder  (VogueFcFontMap *fcfontmap,
				 FcPattern      *pattern)
{
  GSList *l;

  g_return_val_if_fail (PANGO_IS_FC_FONT_MAP (fcfontmap), NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  for (l = fcfontmap->priv->findfuncs; l && l->data; l = l->next)
    {
      VogueFcFindFuncInfo *info = l->data;
      VogueFcDecoder *decoder;

      decoder = info->findfunc (pattern, info->user_data);
      if (decoder)
	return decoder;
    }

  return NULL;
}

static void
vogue_fc_font_map_finalize (GObject *object)
{
  VogueFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (object);

  vogue_fc_font_map_shutdown (fcfontmap);

  G_OBJECT_CLASS (vogue_fc_font_map_parent_class)->finalize (object);
}

/* Add a mapping from key to fcfont */
static void
vogue_fc_font_map_add (VogueFcFontMap *fcfontmap,
		       VogueFcFontKey *key,
		       VogueFcFont    *fcfont)
{
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  VogueFcFontKey *key_copy;

  key_copy = vogue_fc_font_key_copy (key);
  _vogue_fc_font_set_font_key (fcfont, key_copy);
  g_hash_table_insert (priv->font_hash, key_copy, fcfont);
}

/* Remove mapping from fcfont->key to fcfont */
/* Closely related to shutdown_font() */
void
_vogue_fc_font_map_remove (VogueFcFontMap *fcfontmap,
			   VogueFcFont    *fcfont)
{
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  VogueFcFontKey *key;

  key = _vogue_fc_font_get_font_key (fcfont);
  if (key)
    {
      /* Only remove from fontmap hash if we are in it.  This is not necessarily
       * the case after a cache_clear() call. */
      if (priv->font_hash &&
	  fcfont == g_hash_table_lookup (priv->font_hash, key))
        {
	  g_hash_table_remove (priv->font_hash, key);
	}
      _vogue_fc_font_set_font_key (fcfont, NULL);
      vogue_fc_font_key_free (key);
    }
}

static VogueFcFamily *
create_family (VogueFcFontMap *fcfontmap,
	       const char     *family_name,
	       int             spacing)
{
  VogueFcFamily *family = g_object_new (PANGO_FC_TYPE_FAMILY, NULL);
  family->fontmap = fcfontmap;
  family->family_name = g_strdup (family_name);
  family->spacing = spacing;
  family->variable = FALSE;
  family->patterns = FcFontSetCreate ();

  return family;
}

static gboolean
is_alias_family (const char *family_name)
{
  switch (family_name[0])
    {
    case 'c':
    case 'C':
      return (g_ascii_strcasecmp (family_name, "cursive") == 0);
    case 'f':
    case 'F':
      return (g_ascii_strcasecmp (family_name, "fantasy") == 0);
    case 'm':
    case 'M':
      return (g_ascii_strcasecmp (family_name, "monospace") == 0);
    case 's':
    case 'S':
      return (g_ascii_strcasecmp (family_name, "sans") == 0 ||
	      g_ascii_strcasecmp (family_name, "serif") == 0 ||
	      g_ascii_strcasecmp (family_name, "system-ui") == 0);
    }

  return FALSE;
}

static void
vogue_fc_font_map_list_families (VogueFontMap      *fontmap,
				 VogueFontFamily ***families,
				 int               *n_families)
{
  VogueFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (fontmap);
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  FcFontSet *fontset;
  int i;
  int count;

  if (priv->closed)
    {
      if (families)
	*families = NULL;
      if (n_families)
	*n_families = 0;

      return;
    }

  if (priv->n_families < 0)
    {
      FcObjectSet *os = FcObjectSetBuild (FC_FAMILY, FC_SPACING, FC_STYLE, FC_WEIGHT, FC_WIDTH, FC_SLANT,
#ifdef FC_VARIABLE
                                          FC_VARIABLE,
#endif
                                          FC_FONTFORMAT,
                                          NULL);
      FcPattern *pat = FcPatternCreate ();
      GHashTable *temp_family_hash;

      fontset = FcFontList (priv->config, pat, os);

      FcPatternDestroy (pat);
      FcObjectSetDestroy (os);

      priv->families = g_new (VogueFcFamily *, fontset->nfont + 4); /* 4 standard aliases */
      temp_family_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

      count = 0;
      for (i = 0; i < fontset->nfont; i++)
	{
	  char *s;
	  FcResult res;
	  int spacing;
          int variable;
	  VogueFcFamily *temp_family;

	  res = FcPatternGetString (fontset->fonts[i], FC_FONTFORMAT, 0, (FcChar8 **)(void*)&s);
	  g_assert (res == FcResultMatch);
          if (!vogue_fc_is_supported_font_format (s))
            continue;

	  res = FcPatternGetString (fontset->fonts[i], FC_FAMILY, 0, (FcChar8 **)(void*)&s);
	  g_assert (res == FcResultMatch);

	  temp_family = g_hash_table_lookup (temp_family_hash, s);
	  if (!is_alias_family (s) && !temp_family)
	    {
	      res = FcPatternGetInteger (fontset->fonts[i], FC_SPACING, 0, &spacing);
	      g_assert (res == FcResultMatch || res == FcResultNoMatch);
	      if (res == FcResultNoMatch)
		spacing = FC_PROPORTIONAL;

	      temp_family = create_family (fcfontmap, s, spacing);
	      g_hash_table_insert (temp_family_hash, g_strdup (s), temp_family);
	      priv->families[count++] = temp_family;
	    }

	  if (temp_family)
	    {
              variable = FALSE;
#ifdef FC_VARIABLE
              res = FcPatternGetBool (fontset->fonts[i], FC_VARIABLE, 0, &variable);
#endif
              if (variable)
                temp_family->variable = TRUE;

	      FcPatternReference (fontset->fonts[i]);
	      FcFontSetAdd (temp_family->patterns, fontset->fonts[i]);
	    }
	}

      FcFontSetDestroy (fontset);
      g_hash_table_destroy (temp_family_hash);

      priv->families[count++] = create_family (fcfontmap, "Sans", FC_PROPORTIONAL);
      priv->families[count++] = create_family (fcfontmap, "Serif", FC_PROPORTIONAL);
      priv->families[count++] = create_family (fcfontmap, "Monospace", FC_MONO);
      priv->families[count++] = create_family (fcfontmap, "System-ui", FC_PROPORTIONAL);

      priv->n_families = count;
    }

  if (n_families)
    *n_families = priv->n_families;

  if (families)
    *families = g_memdup (priv->families, priv->n_families * sizeof (VogueFontFamily *));
}

static double
vogue_fc_convert_weight_to_fc (VogueWeight vogue_weight)
{
#ifdef HAVE_FCWEIGHTFROMOPENTYPEDOUBLE
  return FcWeightFromOpenTypeDouble (vogue_weight);
#else
  return FcWeightFromOpenType (vogue_weight);
#endif
}

static int
vogue_fc_convert_slant_to_fc (VogueStyle vogue_style)
{
  switch (vogue_style)
    {
    case PANGO_STYLE_NORMAL:
      return FC_SLANT_ROMAN;
    case PANGO_STYLE_ITALIC:
      return FC_SLANT_ITALIC;
    case PANGO_STYLE_OBLIQUE:
      return FC_SLANT_OBLIQUE;
    default:
      return FC_SLANT_ROMAN;
    }
}

static int
vogue_fc_convert_width_to_fc (VogueStretch vogue_stretch)
{
  switch (vogue_stretch)
    {
    case PANGO_STRETCH_NORMAL:
      return FC_WIDTH_NORMAL;
    case PANGO_STRETCH_ULTRA_CONDENSED:
      return FC_WIDTH_ULTRACONDENSED;
    case PANGO_STRETCH_EXTRA_CONDENSED:
      return FC_WIDTH_EXTRACONDENSED;
    case PANGO_STRETCH_CONDENSED:
      return FC_WIDTH_CONDENSED;
    case PANGO_STRETCH_SEMI_CONDENSED:
      return FC_WIDTH_SEMICONDENSED;
    case PANGO_STRETCH_SEMI_EXPANDED:
      return FC_WIDTH_SEMIEXPANDED;
    case PANGO_STRETCH_EXPANDED:
      return FC_WIDTH_EXPANDED;
    case PANGO_STRETCH_EXTRA_EXPANDED:
      return FC_WIDTH_EXTRAEXPANDED;
    case PANGO_STRETCH_ULTRA_EXPANDED:
      return FC_WIDTH_ULTRAEXPANDED;
    default:
      return FC_WIDTH_NORMAL;
    }
}

static FcPattern *
vogue_fc_make_pattern (const  VogueFontDescription *description,
		       VogueLanguage               *language,
		       int                          pixel_size,
		       double                       dpi,
                       const char                  *variations)
{
  FcPattern *pattern;
  const char *prgname;
  int slant;
  double weight;
  VogueGravity gravity;
  FcBool vertical;
  char **families;
  int i;
  int width;

  prgname = g_get_prgname ();
  slant = vogue_fc_convert_slant_to_fc (vogue_font_description_get_style (description));
  weight = vogue_fc_convert_weight_to_fc (vogue_font_description_get_weight (description));
  width = vogue_fc_convert_width_to_fc (vogue_font_description_get_stretch (description));

  gravity = vogue_font_description_get_gravity (description);
  vertical = PANGO_GRAVITY_IS_VERTICAL (gravity) ? FcTrue : FcFalse;

  /* The reason for passing in FC_SIZE as well as FC_PIXEL_SIZE is
   * to work around a bug in libgnomeprint where it doesn't look
   * for FC_PIXEL_SIZE. See http://bugzilla.gnome.org/show_bug.cgi?id=169020
   *
   * Putting FC_SIZE in here slightly reduces the efficiency
   * of caching of patterns and fonts when working with multiple different
   * dpi values.
   */
  pattern = FcPatternBuild (NULL,
			    PANGO_FC_VERSION, FcTypeInteger, vogue_version(),
			    FC_WEIGHT, FcTypeDouble, weight,
			    FC_SLANT,  FcTypeInteger, slant,
			    FC_WIDTH,  FcTypeInteger, width,
			    FC_VERTICAL_LAYOUT,  FcTypeBool, vertical,
#ifdef FC_VARIABLE
			    FC_VARIABLE,  FcTypeBool, FcDontCare,
#endif
			    FC_DPI, FcTypeDouble, dpi,
			    FC_SIZE,  FcTypeDouble,  pixel_size * (72. / 1024. / dpi),
			    FC_PIXEL_SIZE,  FcTypeDouble,  pixel_size / 1024.,
			    NULL);

  if (variations)
    FcPatternAddString (pattern, PANGO_FC_FONT_VARIATIONS, (FcChar8*) variations);

  if (vogue_font_description_get_family (description))
    {
      families = g_strsplit (vogue_font_description_get_family (description), ",", -1);

      for (i = 0; families[i]; i++)
	FcPatternAddString (pattern, FC_FAMILY, (FcChar8*) families[i]);

      g_strfreev (families);
    }

  if (language)
    FcPatternAddString (pattern, FC_LANG, (FcChar8 *) vogue_language_to_string (language));

  if (gravity != PANGO_GRAVITY_SOUTH)
    {
      GEnumValue *value = g_enum_get_value (get_gravity_class (), gravity);
      FcPatternAddString (pattern, PANGO_FC_GRAVITY, (FcChar8*) value->value_nick);
    }

  if (prgname)
    FcPatternAddString (pattern, PANGO_FC_PRGNAME, (FcChar8*) prgname);

  return pattern;
}

static FcPattern *
uniquify_pattern (VogueFcFontMap *fcfontmap,
		  FcPattern      *pattern)
{
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *old_pattern;

  old_pattern = g_hash_table_lookup (priv->pattern_hash, pattern);
  if (old_pattern)
    {
      return old_pattern;
    }
  else
    {
      FcPatternReference (pattern);
      g_hash_table_insert (priv->pattern_hash, pattern, pattern);
      return pattern;
    }
}

static VogueFont *
vogue_fc_font_map_new_font (VogueFcFontMap    *fcfontmap,
			    VogueFcFontsetKey *fontset_key,
			    FcPattern         *match)
{
  VogueFcFontMapClass *class;
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *pattern;
  VogueFcFont *fcfont;
  VogueFcFontKey key;

  if (priv->closed)
    return NULL;

  match = uniquify_pattern (fcfontmap, match);

  vogue_fc_font_key_init (&key, fcfontmap, fontset_key, match);

  fcfont = g_hash_table_lookup (priv->font_hash, &key);
  if (fcfont)
    return g_object_ref (PANGO_FONT (fcfont));

  class = PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap);

  if (class->create_font)
    {
      fcfont = class->create_font (fcfontmap, &key);
    }
  else
    {
      const VogueMatrix *vogue_matrix = vogue_fc_fontset_key_get_matrix (fontset_key);
      FcMatrix fc_matrix, *fc_matrix_val;
      int i;

      /* Fontconfig has the Y axis pointing up, Vogue, down.
       */
      fc_matrix.xx = vogue_matrix->xx;
      fc_matrix.xy = - vogue_matrix->xy;
      fc_matrix.yx = - vogue_matrix->yx;
      fc_matrix.yy = vogue_matrix->yy;

      pattern = FcPatternDuplicate (match);

      for (i = 0; FcPatternGetMatrix (pattern, FC_MATRIX, i, &fc_matrix_val) == FcResultMatch; i++)
	FcMatrixMultiply (&fc_matrix, &fc_matrix, fc_matrix_val);

      FcPatternDel (pattern, FC_MATRIX);
      FcPatternAddMatrix (pattern, FC_MATRIX, &fc_matrix);

      fcfont = class->new_font (fcfontmap, uniquify_pattern (fcfontmap, pattern));

      FcPatternDestroy (pattern);
    }

  if (!fcfont)
    return NULL;

  fcfont->matrix = key.matrix;
  /* In case the backend didn't set the fontmap */
  if (!fcfont->fontmap)
    g_object_set (fcfont,
		  "fontmap", fcfontmap,
		  NULL);

  /* cache it on fontmap */
  vogue_fc_font_map_add (fcfontmap, &key, fcfont);

  return (VogueFont *)fcfont;
}

static void
vogue_fc_default_substitute (VogueFcFontMap    *fontmap,
			     VogueFcFontsetKey *fontsetkey,
			     FcPattern         *pattern)
{
  if (PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->fontset_key_substitute)
    PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->fontset_key_substitute (fontmap, fontsetkey, pattern);
  else if (PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->default_substitute)
    PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->default_substitute (fontmap, pattern);
}

static double
vogue_fc_font_map_get_resolution (VogueFcFontMap *fcfontmap,
				  VogueContext   *context)
{
  if (PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->get_resolution)
    return PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->get_resolution (fcfontmap, context);

  if (fcfontmap->priv->dpi < 0)
    {
      FcResult result = FcResultNoMatch;
      FcPattern *tmp = FcPatternBuild (NULL,
				       FC_FAMILY, FcTypeString, "Sans",
				       FC_SIZE,   FcTypeDouble, 10.,
				       NULL);
      if (tmp)
	{
	  vogue_fc_default_substitute (fcfontmap, NULL, tmp);
	  result = FcPatternGetDouble (tmp, FC_DPI, 0, &fcfontmap->priv->dpi);
	  FcPatternDestroy (tmp);
	}

      if (result != FcResultMatch)
	{
	  g_warning ("Error getting DPI from fontconfig, using 72.0");
	  fcfontmap->priv->dpi = 72.0;
	}
    }

  return fcfontmap->priv->dpi;
}

static FcPattern *
vogue_fc_fontset_key_make_pattern (VogueFcFontsetKey *key)
{
  return vogue_fc_make_pattern (key->desc,
				key->language,
				key->pixelsize,
				key->resolution,
                                key->variations);
}

static VogueFcPatterns *
vogue_fc_font_map_get_patterns (VogueFontMap      *fontmap,
				VogueFcFontsetKey *key)
{
  VogueFcFontMap *fcfontmap = (VogueFcFontMap *)fontmap;
  VogueFcPatterns *patterns;
  FcPattern *pattern;

  pattern = vogue_fc_fontset_key_make_pattern (key);
  vogue_fc_default_substitute (fcfontmap, key, pattern);

  patterns = vogue_fc_patterns_new (pattern, fcfontmap);

  FcPatternDestroy (pattern);

  return patterns;
}

static gboolean
get_first_font (VogueFontset  *fontset G_GNUC_UNUSED,
		VogueFont     *font,
		gpointer       data)
{
  *(VogueFont **)data = font;

  return TRUE;
}

static VogueFont *
vogue_fc_font_map_load_font (VogueFontMap               *fontmap,
			     VogueContext               *context,
			     const VogueFontDescription *description)
{
  VogueLanguage *language;
  VogueFontset *fontset;
  VogueFont *font = NULL;

  if (context)
    language = vogue_context_get_language (context);
  else
    language = NULL;

  fontset = vogue_font_map_load_fontset (fontmap, context, description, language);

  if (fontset)
    {
      vogue_fontset_foreach (fontset, get_first_font, &font);

      if (font)
	g_object_ref (font);

      g_object_unref (fontset);
    }

  return font;
}

static void
vogue_fc_fontset_cache (VogueFcFontset *fontset,
			VogueFcFontMap *fcfontmap)
{
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  GQueue *cache = priv->fontset_cache;

  if (fontset->cache_link)
    {
      if (fontset->cache_link == cache->head)
        return;

      /* Already in cache, move to head
       */
      if (fontset->cache_link == cache->tail)
	cache->tail = fontset->cache_link->prev;

      cache->head = g_list_remove_link (cache->head, fontset->cache_link);
      cache->length--;
    }
  else
    {
      /* Add to cache initially
       */
#if 1
      if (cache->length == FONTSET_CACHE_SIZE)
	{
	  VogueFcFontset *tmp_fontset = g_queue_pop_tail (cache);
	  tmp_fontset->cache_link = NULL;
	  g_hash_table_remove (priv->fontset_hash, tmp_fontset->key);
	}
#endif

      fontset->cache_link = g_list_prepend (NULL, fontset);
    }

  g_queue_push_head_link (cache, fontset->cache_link);
}

static VogueFontset *
vogue_fc_font_map_load_fontset (VogueFontMap                 *fontmap,
				VogueContext                 *context,
				const VogueFontDescription   *desc,
				VogueLanguage                *language)
{
  VogueFcFontMap *fcfontmap = (VogueFcFontMap *)fontmap;
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  VogueFcFontset *fontset;
  VogueFcFontsetKey key;

  vogue_fc_fontset_key_init (&key, fcfontmap, context, desc, language);

  fontset = g_hash_table_lookup (priv->fontset_hash, &key);

  if (G_UNLIKELY (!fontset))
    {
      VogueFcPatterns *patterns = vogue_fc_font_map_get_patterns (fontmap, &key);

      if (!patterns)
	return NULL;

      fontset = vogue_fc_fontset_new (&key, patterns);
      g_hash_table_insert (priv->fontset_hash, vogue_fc_fontset_get_key (fontset), fontset);

      vogue_fc_patterns_unref (patterns);
    }

  vogue_fc_fontset_cache (fontset, fcfontmap);

  vogue_font_description_free (key.desc);
  g_free (key.variations);

  return g_object_ref (PANGO_FONTSET (fontset));
}

/**
 * vogue_fc_font_map_cache_clear:
 * @fcfontmap: a #VogueFcFontMap
 *
 * Clear all cached information and fontsets for this font map;
 * this should be called whenever there is a change in the
 * output of the default_substitute() virtual function of the
 * font map, or if fontconfig has been reinitialized to new
 * configuration.
 *
 * Since: 1.4
 **/
void
vogue_fc_font_map_cache_clear (VogueFcFontMap *fcfontmap)
{
  if (G_UNLIKELY (fcfontmap->priv->closed))
    return;

  vogue_fc_font_map_fini (fcfontmap);
  vogue_fc_font_map_init (fcfontmap);

  vogue_font_map_changed (PANGO_FONT_MAP (fcfontmap));
}

/**
 * vogue_fc_font_map_config_changed:
 * @fcfontmap: a #VogueFcFontMap
 *
 * Informs font map that the fontconfig configuration (ie, FcConfig object)
 * used by this font map has changed.  This currently calls
 * vogue_fc_font_map_cache_clear() which ensures that list of fonts, etc
 * will be regenerated using the updated configuration.
 *
 * Since: 1.38
 **/
void
vogue_fc_font_map_config_changed (VogueFcFontMap *fcfontmap)
{
  vogue_fc_font_map_cache_clear (fcfontmap);
}

/**
 * vogue_fc_font_map_set_config:
 * @fcfontmap: a #VogueFcFontMap
 * @fcconfig: (nullable): a #FcConfig, or %NULL
 *
 * Set the FcConfig for this font map to use.  The default value
 * is %NULL, which causes Fontconfig to use its global "current config".
 * You can create a new FcConfig object and use this API to attach it
 * to a font map.
 *
 * This is particularly useful for example, if you want to use application
 * fonts with Vogue.  For that, you would create a fresh FcConfig, add your
 * app fonts to it, and attach it to a new Vogue font map.
 *
 * If @fcconfig is different from the previous config attached to the font map,
 * vogue_fc_font_map_config_changed() is called.
 *
 * This function acquires a reference to the FcConfig object; the caller
 * does NOT need to retain a reference.
 *
 * Since: 1.38
 **/
void
vogue_fc_font_map_set_config (VogueFcFontMap *fcfontmap,
			      FcConfig       *fcconfig)
{
  FcConfig *oldconfig;

  g_return_if_fail (PANGO_IS_FC_FONT_MAP (fcfontmap));

  oldconfig = fcfontmap->priv->config;

  if (fcconfig)
    FcConfigReference (fcconfig);

  fcfontmap->priv->config = fcconfig;

  if (oldconfig != fcconfig)
    vogue_fc_font_map_config_changed (fcfontmap);

  if (oldconfig)
    FcConfigDestroy (oldconfig);
}

/**
 * vogue_fc_font_map_get_config:
 * @fcfontmap: a #VogueFcFontMap
 *
 * Fetches FcConfig attached to a font map.  See vogue_fc_font_map_set_config().
 *
 * Returns: (nullable): the #FcConfig object attached to @fcfontmap, which
 *          might be %NULL.
 *
 * Since: 1.38
 **/
FcConfig *
vogue_fc_font_map_get_config (VogueFcFontMap *fcfontmap)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT_MAP (fcfontmap), NULL);

  return fcfontmap->priv->config;
}

static VogueFcFontFaceData *
vogue_fc_font_map_get_font_face_data (VogueFcFontMap *fcfontmap,
				      FcPattern      *font_pattern)
{
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  VogueFcFontFaceData key;
  VogueFcFontFaceData *data;

  if (FcPatternGetString (font_pattern, FC_FILE, 0, (FcChar8 **)(void*)&key.filename) != FcResultMatch)
    return NULL;

  if (FcPatternGetInteger (font_pattern, FC_INDEX, 0, &key.id) != FcResultMatch)
    return NULL;

  data = g_hash_table_lookup (priv->font_face_data_hash, &key);
  if (G_LIKELY (data))
    return data;

  data = g_slice_new0 (VogueFcFontFaceData);
  data->filename = key.filename;
  data->id = key.id;

  data->pattern = font_pattern;
  FcPatternReference (data->pattern);

  g_hash_table_insert (priv->font_face_data_hash, data, data);

  return data;
}

typedef struct {
  VogueCoverage parent_instance;

  FcCharSet *charset;
} VogueFcCoverage;

typedef struct {
  VogueCoverageClass parent_class;
} VogueFcCoverageClass;

GType vogue_fc_coverage_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (VogueFcCoverage, vogue_fc_coverage, PANGO_TYPE_COVERAGE)

static void
vogue_fc_coverage_init (VogueFcCoverage *coverage)
{
}

static VogueCoverageLevel
vogue_fc_coverage_real_get (VogueCoverage *coverage,
                            int            index)
{
  VogueFcCoverage *fc_coverage = (VogueFcCoverage*)coverage;

  return FcCharSetHasChar (fc_coverage->charset, index)
         ? PANGO_COVERAGE_EXACT
         : PANGO_COVERAGE_NONE;
}

static void
vogue_fc_coverage_real_set (VogueCoverage *coverage,
                            int            index,
                            VogueCoverageLevel level)
{
  VogueFcCoverage *fc_coverage = (VogueFcCoverage*)coverage;

  if (level == PANGO_COVERAGE_NONE)
    FcCharSetDelChar (fc_coverage->charset, index);
  else
    FcCharSetAddChar (fc_coverage->charset, index);
}

static VogueCoverage *
vogue_fc_coverage_real_copy (VogueCoverage *coverage)
{
  VogueFcCoverage *fc_coverage = (VogueFcCoverage*)coverage;
  VogueFcCoverage *copy;

  copy = g_object_new (vogue_fc_coverage_get_type (), NULL);
  copy->charset = FcCharSetCopy (fc_coverage->charset);

  return (VogueCoverage *)copy;
}

static void
vogue_fc_coverage_finalize (GObject *object)
{
  VogueFcCoverage *fc_coverage = (VogueFcCoverage*)object;

  FcCharSetDestroy (fc_coverage->charset);

  G_OBJECT_CLASS (vogue_fc_coverage_parent_class)->finalize (object);
}

static void
vogue_fc_coverage_class_init (VogueFcCoverageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  VogueCoverageClass *coverage_class = PANGO_COVERAGE_CLASS (class);

  object_class->finalize = vogue_fc_coverage_finalize;

  coverage_class->get = vogue_fc_coverage_real_get;
  coverage_class->set = vogue_fc_coverage_real_set;
  coverage_class->copy = vogue_fc_coverage_real_copy;
}

VogueCoverage *
_vogue_fc_font_map_get_coverage (VogueFcFontMap *fcfontmap,
				 VogueFcFont    *fcfont)
{
  VogueFcFontFaceData *data;
  FcCharSet *charset;

  data = vogue_fc_font_map_get_font_face_data (fcfontmap, fcfont->font_pattern);
  if (G_UNLIKELY (!data))
    return NULL;

  if (G_UNLIKELY (data->coverage == NULL))
    {
      /*
       * Pull the coverage out of the pattern, this
       * doesn't require loading the font
       */
      if (FcPatternGetCharSet (fcfont->font_pattern, FC_CHARSET, 0, &charset) != FcResultMatch)
        return NULL;

      data->coverage = _vogue_fc_font_map_fc_to_coverage (charset);
    }

  return vogue_coverage_ref (data->coverage);
}

/**
 * _vogue_fc_font_map_fc_to_coverage:
 * @charset: #FcCharSet to convert to a #VogueCoverage object.
 *
 * Convert the given #FcCharSet into a new #VogueCoverage object.  The
 * caller is responsible for freeing the newly created object.
 *
 * Since: 1.6
 **/
VogueCoverage  *
_vogue_fc_font_map_fc_to_coverage (FcCharSet *charset)
{
  VogueFcCoverage *coverage;

  coverage = g_object_new (vogue_fc_coverage_get_type (), NULL);
  coverage->charset = FcCharSetCopy (charset);

  return (VogueCoverage *)coverage;
}

/**
 * vogue_fc_font_map_create_context:
 * @fcfontmap: a #VogueFcFontMap
 *
 * Creates a new context for this fontmap. This function is intended
 * only for backend implementations deriving from #VogueFcFontMap;
 * it is possible that a backend will store additional information
 * needed for correct operation on the #VogueContext after calling
 * this function.
 *
 * Return value: a new #VogueContext
 *
 * Since: 1.4
 *
 * Deprecated: 1.22: Use vogue_font_map_create_context() instead.
 **/
VogueContext *
vogue_fc_font_map_create_context (VogueFcFontMap *fcfontmap)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT_MAP (fcfontmap), NULL);

  return vogue_font_map_create_context (PANGO_FONT_MAP (fcfontmap));
}

static void
shutdown_font (gpointer        key,
	       VogueFcFont    *fcfont,
	       VogueFcFontMap *fcfontmap)
{
  _vogue_fc_font_shutdown (fcfont);

  _vogue_fc_font_set_font_key (fcfont, NULL);
  vogue_fc_font_key_free (key);
}

/**
 * vogue_fc_font_map_shutdown:
 * @fcfontmap: a #VogueFcFontMap
 *
 * Clears all cached information for the fontmap and marks
 * all fonts open for the fontmap as dead. (See the shutdown()
 * virtual function of #VogueFcFont.) This function might be used
 * by a backend when the underlying windowing system for the font
 * map exits. This function is only intended to be called
 * only for backend implementations deriving from #VogueFcFontMap.
 *
 * Since: 1.4
 **/
void
vogue_fc_font_map_shutdown (VogueFcFontMap *fcfontmap)
{
  VogueFcFontMapPrivate *priv = fcfontmap->priv;
  int i;

  if (priv->closed)
    return;

  g_hash_table_foreach (priv->font_hash, (GHFunc) shutdown_font, fcfontmap);
  for (i = 0; i < priv->n_families; i++)
    priv->families[i]->fontmap = NULL;

  vogue_fc_font_map_fini (fcfontmap);

  while (priv->findfuncs)
    {
      VogueFcFindFuncInfo *info;
      info = priv->findfuncs->data;
      if (info->dnotify)
	info->dnotify (info->user_data);

      g_slice_free (VogueFcFindFuncInfo, info);
      priv->findfuncs = g_slist_delete_link (priv->findfuncs, priv->findfuncs);
    }

  priv->closed = TRUE;
}

static VogueWeight
vogue_fc_convert_weight_to_vogue (double fc_weight)
{
#ifdef HAVE_FCWEIGHTFROMOPENTYPEDOUBLE
  return FcWeightToOpenTypeDouble (fc_weight);
#else
  return FcWeightToOpenType (fc_weight);
#endif
}

static VogueStyle
vogue_fc_convert_slant_to_vogue (int fc_style)
{
  switch (fc_style)
    {
    case FC_SLANT_ROMAN:
      return PANGO_STYLE_NORMAL;
    case FC_SLANT_ITALIC:
      return PANGO_STYLE_ITALIC;
    case FC_SLANT_OBLIQUE:
      return PANGO_STYLE_OBLIQUE;
    default:
      return PANGO_STYLE_NORMAL;
    }
}

static VogueStretch
vogue_fc_convert_width_to_vogue (int fc_stretch)
{
  switch (fc_stretch)
    {
    case FC_WIDTH_NORMAL:
      return PANGO_STRETCH_NORMAL;
    case FC_WIDTH_ULTRACONDENSED:
      return PANGO_STRETCH_ULTRA_CONDENSED;
    case FC_WIDTH_EXTRACONDENSED:
      return PANGO_STRETCH_EXTRA_CONDENSED;
    case FC_WIDTH_CONDENSED:
      return PANGO_STRETCH_CONDENSED;
    case FC_WIDTH_SEMICONDENSED:
      return PANGO_STRETCH_SEMI_CONDENSED;
    case FC_WIDTH_SEMIEXPANDED:
      return PANGO_STRETCH_SEMI_EXPANDED;
    case FC_WIDTH_EXPANDED:
      return PANGO_STRETCH_EXPANDED;
    case FC_WIDTH_EXTRAEXPANDED:
      return PANGO_STRETCH_EXTRA_EXPANDED;
    case FC_WIDTH_ULTRAEXPANDED:
      return PANGO_STRETCH_ULTRA_EXPANDED;
    default:
      return PANGO_STRETCH_NORMAL;
    }
}

/**
 * vogue_fc_font_description_from_pattern:
 * @pattern: a #FcPattern
 * @include_size: if %TRUE, the pattern will include the size from
 *   the @pattern; otherwise the resulting pattern will be unsized.
 *   (only %FC_SIZE is examined, not %FC_PIXEL_SIZE)
 *
 * Creates a #VogueFontDescription that matches the specified
 * Fontconfig pattern as closely as possible. Many possible Fontconfig
 * pattern values, such as %FC_RASTERIZER or %FC_DPI, don't make sense in
 * the context of #VogueFontDescription, so will be ignored.
 *
 * Return value: a new #VogueFontDescription. Free with
 *  vogue_font_description_free().
 *
 * Since: 1.4
 **/
VogueFontDescription *
vogue_fc_font_description_from_pattern (FcPattern *pattern, gboolean include_size)
{
  VogueFontDescription *desc;
  VogueStyle style;
  VogueWeight weight;
  VogueStretch stretch;
  double size;
  VogueGravity gravity;

  FcChar8 *s;
  int i;
  double d;
  FcResult res;

  desc = vogue_font_description_new ();

  res = FcPatternGetString (pattern, FC_FAMILY, 0, (FcChar8 **) &s);
  g_assert (res == FcResultMatch);

  vogue_font_description_set_family (desc, (gchar *)s);

  if (FcPatternGetInteger (pattern, FC_SLANT, 0, &i) == FcResultMatch)
    style = vogue_fc_convert_slant_to_vogue (i);
  else
    style = PANGO_STYLE_NORMAL;

  vogue_font_description_set_style (desc, style);

  if (FcPatternGetDouble (pattern, FC_WEIGHT, 0, &d) == FcResultMatch)
    weight = vogue_fc_convert_weight_to_vogue (d);
  else
    weight = PANGO_WEIGHT_NORMAL;

  vogue_font_description_set_weight (desc, weight);

  if (FcPatternGetInteger (pattern, FC_WIDTH, 0, &i) == FcResultMatch)
    stretch = vogue_fc_convert_width_to_vogue (i);
  else
    stretch = PANGO_STRETCH_NORMAL;

  vogue_font_description_set_stretch (desc, stretch);

  vogue_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);

  if (include_size && FcPatternGetDouble (pattern, FC_SIZE, 0, &size) == FcResultMatch)
    vogue_font_description_set_size (desc, size * PANGO_SCALE);

  /* gravity is a bit different.  we don't want to set it if it was not set on
   * the pattern */
  if (FcPatternGetString (pattern, PANGO_FC_GRAVITY, 0, (FcChar8 **)&s) == FcResultMatch)
    {
      GEnumValue *value = g_enum_get_value_by_nick (get_gravity_class (), (char *)s);
      gravity = value->value;

      vogue_font_description_set_gravity (desc, gravity);
    }

  if (include_size && FcPatternGetString (pattern, PANGO_FC_FONT_VARIATIONS, 0, (FcChar8 **)&s) == FcResultMatch)
    {
      if (s && *s)
        vogue_font_description_set_variations (desc, (char *)s);
    }

  return desc;
}

/*
 * VogueFcFace
 */

typedef VogueFontFaceClass VogueFcFaceClass;

G_DEFINE_TYPE (VogueFcFace, vogue_fc_face, PANGO_TYPE_FONT_FACE)

static VogueFontDescription *
make_alias_description (VogueFcFamily *fcfamily,
			gboolean        bold,
			gboolean        italic)
{
  VogueFontDescription *desc = vogue_font_description_new ();

  vogue_font_description_set_family (desc, fcfamily->family_name);
  vogue_font_description_set_style (desc, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  vogue_font_description_set_weight (desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);

  return desc;
}

static VogueFontDescription *
vogue_fc_face_describe (VogueFontFace *face)
{
  VogueFcFace *fcface = PANGO_FC_FACE (face);
  VogueFcFamily *fcfamily = fcface->family;
  VogueFontDescription *desc = NULL;

  if (G_UNLIKELY (!fcfamily))
    return vogue_font_description_new ();

  if (fcface->fake)
    {
      if (strcmp (fcface->style, "Regular") == 0)
	return make_alias_description (fcfamily, FALSE, FALSE);
      else if (strcmp (fcface->style, "Bold") == 0)
	return make_alias_description (fcfamily, TRUE, FALSE);
      else if (strcmp (fcface->style, "Italic") == 0)
	return make_alias_description (fcfamily, FALSE, TRUE);
      else			/* Bold Italic */
	return make_alias_description (fcfamily, TRUE, TRUE);
    }

  g_assert (fcface->pattern);
  desc = vogue_fc_font_description_from_pattern (fcface->pattern, FALSE);

  return desc;
}

static const char *
vogue_fc_face_get_face_name (VogueFontFace *face)
{
  VogueFcFace *fcface = PANGO_FC_FACE (face);

  return fcface->style;
}

static int
compare_ints (gconstpointer ap,
	      gconstpointer bp)
{
  int a = *(int *)ap;
  int b = *(int *)bp;

  if (a == b)
    return 0;
  else if (a > b)
    return 1;
  else
    return -1;
}

static void
vogue_fc_face_list_sizes (VogueFontFace  *face,
			  int           **sizes,
			  int            *n_sizes)
{
  VogueFcFace *fcface = PANGO_FC_FACE (face);
  FcPattern *pattern;
  FcFontSet *fontset;
  FcObjectSet *objectset;

  *sizes = NULL;
  *n_sizes = 0;
  if (G_UNLIKELY (!fcface->family || !fcface->family->fontmap))
    return;

  pattern = FcPatternCreate ();
  FcPatternAddString (pattern, FC_FAMILY, (FcChar8*)(void*)fcface->family->family_name);
  FcPatternAddString (pattern, FC_STYLE, (FcChar8*)(void*)fcface->style);

  objectset = FcObjectSetCreate ();
  FcObjectSetAdd (objectset, FC_PIXEL_SIZE);

  fontset = FcFontList (NULL, pattern, objectset);

  if (fontset)
    {
      GArray *size_array;
      double size, dpi = -1.0;
      int i, size_i, j;

      size_array = g_array_new (FALSE, FALSE, sizeof (int));

      for (i = 0; i < fontset->nfont; i++)
	{
	  for (j = 0;
	       FcPatternGetDouble (fontset->fonts[i], FC_PIXEL_SIZE, j, &size) == FcResultMatch;
	       j++)
	    {
	      if (dpi < 0)
		dpi = vogue_fc_font_map_get_resolution (fcface->family->fontmap, NULL);

	      size_i = (int) (PANGO_SCALE * size * 72.0 / dpi);
	      g_array_append_val (size_array, size_i);
	    }
	}

      g_array_sort (size_array, compare_ints);

      if (size_array->len == 0)
	{
	  *n_sizes = 0;
	  if (sizes)
	    *sizes = NULL;
	  g_array_free (size_array, TRUE);
	}
      else
	{
	  *n_sizes = size_array->len;
	  if (sizes)
	    {
	      *sizes = (int *) size_array->data;
	      g_array_free (size_array, FALSE);
	    }
	  else
	    g_array_free (size_array, TRUE);
	}

      FcFontSetDestroy (fontset);
    }
  else
    {
      *n_sizes = 0;
      if (sizes)
	*sizes = NULL;
    }

  FcPatternDestroy (pattern);
  FcObjectSetDestroy (objectset);
}

static gboolean
vogue_fc_face_is_synthesized (VogueFontFace *face)
{
  VogueFcFace *fcface = PANGO_FC_FACE (face);

  return fcface->fake;
}

static void
vogue_fc_face_finalize (GObject *object)
{
  VogueFcFace *fcface = PANGO_FC_FACE (object);

  g_free (fcface->style);
  FcPatternDestroy (fcface->pattern);

  G_OBJECT_CLASS (vogue_fc_face_parent_class)->finalize (object);
}

static void
vogue_fc_face_init (VogueFcFace *self)
{
}

static void
vogue_fc_face_class_init (VogueFcFaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = vogue_fc_face_finalize;

  class->describe = vogue_fc_face_describe;
  class->get_face_name = vogue_fc_face_get_face_name;
  class->list_sizes = vogue_fc_face_list_sizes;
  class->is_synthesized = vogue_fc_face_is_synthesized;
}


/*
 * VogueFcFamily
 */

typedef VogueFontFamilyClass VogueFcFamilyClass;

G_DEFINE_TYPE (VogueFcFamily, vogue_fc_family, PANGO_TYPE_FONT_FAMILY)

static VogueFcFace *
create_face (VogueFcFamily *fcfamily,
	     const char    *style,
	     FcPattern     *pattern,
	     gboolean       fake)
{
  VogueFcFace *face = g_object_new (PANGO_FC_TYPE_FACE, NULL);
  face->style = g_strdup (style);
  if (pattern)
    FcPatternReference (pattern);
  face->pattern = pattern;
  face->family = fcfamily;
  face->fake = fake;

  return face;
}

static void
vogue_fc_family_list_faces (VogueFontFamily  *family,
			    VogueFontFace  ***faces,
			    int              *n_faces)
{
  VogueFcFamily *fcfamily = PANGO_FC_FAMILY (family);
  VogueFcFontMap *fcfontmap = fcfamily->fontmap;
  VogueFcFontMapPrivate *priv;

  *faces = NULL;
  *n_faces = 0;
  if (G_UNLIKELY (!fcfontmap))
    return;

  priv = fcfontmap->priv;

  if (fcfamily->n_faces < 0)
    {
      FcFontSet *fontset;
      int i;

      if (is_alias_family (fcfamily->family_name) || priv->closed)
	{
	  fcfamily->n_faces = 4;
	  fcfamily->faces = g_new (VogueFcFace *, fcfamily->n_faces);

	  i = 0;
	  fcfamily->faces[i++] = create_face (fcfamily, "Regular", NULL, TRUE);
	  fcfamily->faces[i++] = create_face (fcfamily, "Bold", NULL, TRUE);
	  fcfamily->faces[i++] = create_face (fcfamily, "Italic", NULL, TRUE);
	  fcfamily->faces[i++] = create_face (fcfamily, "Bold Italic", NULL, TRUE);
	}
      else
	{
	  enum {
	    REGULAR,
	    ITALIC,
	    BOLD,
	    BOLD_ITALIC
	  };
	  /* Regular, Italic, Bold, Bold Italic */
	  gboolean has_face [4] = { FALSE, FALSE, FALSE, FALSE };
	  VogueFcFace **faces;
	  gint num = 0;

	  fontset = fcfamily->patterns;

	  /* at most we have 3 additional artifical faces */
	  faces = g_new (VogueFcFace *, fontset->nfont + 3);

	  for (i = 0; i < fontset->nfont; i++)
	    {
	      const char *style, *font_style = NULL;
	      int weight, slant;

	      if (FcPatternGetInteger(fontset->fonts[i], FC_WEIGHT, 0, &weight) != FcResultMatch)
		weight = FC_WEIGHT_MEDIUM;

	      if (FcPatternGetInteger(fontset->fonts[i], FC_SLANT, 0, &slant) != FcResultMatch)
		slant = FC_SLANT_ROMAN;

#ifdef FC_VARIABLE
              {
                gboolean variable;
                if (FcPatternGetBool(fontset->fonts[i], FC_VARIABLE, 0, &variable) != FcResultMatch)
                  variable = FALSE;
                if (variable) /* skip the variable face */
                  continue;
              }
#endif

	      if (FcPatternGetString (fontset->fonts[i], FC_STYLE, 0, (FcChar8 **)(void*)&font_style) != FcResultMatch)
		font_style = NULL;

	      if (weight <= FC_WEIGHT_MEDIUM)
		{
		  if (slant == FC_SLANT_ROMAN)
		    {
		      has_face[REGULAR] = TRUE;
		      style = "Regular";
		    }
		  else
		    {
		      has_face[ITALIC] = TRUE;
		      style = "Italic";
		    }
		}
	      else
		{
		  if (slant == FC_SLANT_ROMAN)
		    {
		      has_face[BOLD] = TRUE;
		      style = "Bold";
		    }
		  else
		    {
		      has_face[BOLD_ITALIC] = TRUE;
		      style = "Bold Italic";
		    }
		}

	      if (!font_style)
		font_style = style;
	      faces[num++] = create_face (fcfamily, font_style, fontset->fonts[i], FALSE);
	    }

	  if (has_face[REGULAR])
	    {
	      if (!has_face[ITALIC])
		faces[num++] = create_face (fcfamily, "Italic", NULL, TRUE);
	      if (!has_face[BOLD])
		faces[num++] = create_face (fcfamily, "Bold", NULL, TRUE);

	    }
	  if ((has_face[REGULAR] || has_face[ITALIC] || has_face[BOLD]) && !has_face[BOLD_ITALIC])
	    faces[num++] = create_face (fcfamily, "Bold Italic", NULL, TRUE);

	  faces = g_renew (VogueFcFace *, faces, num);

	  fcfamily->n_faces = num;
	  fcfamily->faces = faces;
	}
    }

  if (n_faces)
    *n_faces = fcfamily->n_faces;

  if (faces)
    *faces = g_memdup (fcfamily->faces, fcfamily->n_faces * sizeof (VogueFontFace *));
}

static const char *
vogue_fc_family_get_name (VogueFontFamily  *family)
{
  VogueFcFamily *fcfamily = PANGO_FC_FAMILY (family);

  return fcfamily->family_name;
}

static gboolean
vogue_fc_family_is_monospace (VogueFontFamily *family)
{
  VogueFcFamily *fcfamily = PANGO_FC_FAMILY (family);

  return fcfamily->spacing == FC_MONO ||
	 fcfamily->spacing == FC_DUAL ||
	 fcfamily->spacing == FC_CHARCELL;
}

static gboolean
vogue_fc_family_is_variable (VogueFontFamily *family)
{
  VogueFcFamily *fcfamily = PANGO_FC_FAMILY (family);

  return fcfamily->variable;
}

static void
vogue_fc_family_finalize (GObject *object)
{
  int i;
  VogueFcFamily *fcfamily = PANGO_FC_FAMILY (object);

  g_free (fcfamily->family_name);

  for (i = 0; i < fcfamily->n_faces; i++)
    {
      fcfamily->faces[i]->family = NULL;
      g_object_unref (fcfamily->faces[i]);
    }
  FcFontSetDestroy (fcfamily->patterns);
  g_free (fcfamily->faces);

  G_OBJECT_CLASS (vogue_fc_family_parent_class)->finalize (object);
}

static void
vogue_fc_family_class_init (VogueFcFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = vogue_fc_family_finalize;

  class->list_faces = vogue_fc_family_list_faces;
  class->get_name = vogue_fc_family_get_name;
  class->is_monospace = vogue_fc_family_is_monospace;
  class->is_variable = vogue_fc_family_is_variable;
}

static void
vogue_fc_family_init (VogueFcFamily *fcfamily)
{
  fcfamily->n_faces = -1;
}

hb_face_t *
vogue_fc_font_map_get_hb_face (VogueFcFontMap *fcfontmap,
                               VogueFcFont    *fcfont)
{
  VogueFcFontFaceData *data;

  data = vogue_fc_font_map_get_font_face_data (fcfontmap, fcfont->font_pattern);

  if (!data->hb_face)
    {
      hb_blob_t *blob;

      if (!hb_version_atleast (2, 0, 0))
        g_error ("Harfbuzz version too old (%s)\n", hb_version_string ());

      blob = hb_blob_create_from_file (data->filename);
      data->hb_face = hb_face_create (blob, data->id);
      hb_blob_destroy (blob);
    }

  return data->hb_face;
}
