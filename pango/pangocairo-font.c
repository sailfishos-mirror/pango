/* Pango
 * pangocairo-font.c: Cairo font handling
 *
 * Copyright (C) 2000-2005 Red Hat Software
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

#include "config.h"

#include <math.h>
#include <string.h>

#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pango-font-private.h"
#include "pango-font-metrics-private.h"
#include "pango-impl-utils.h"
#include "pango-hbfont-private.h"
#include "pango-hbface-private.h"
#include "pango-userfont-private.h"
#include "pango-userface-private.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include <cairo-ft.h>
#pragma GCC diagnostic pop

#include <hb-ft.h>
#include <freetype/ftmm.h>

#define PANGO_CAIRO_FONT_PRIVATE(font)		\
  ((PangoCairoFontPrivate *)			\
   (font == NULL ? NULL :			\
    G_STRUCT_MEMBER_P (font,			\
    PANGO_CAIRO_FONT_GET_IFACE(PANGO_CAIRO_FONT(font))->cf_priv_offset)))

typedef PangoCairoFontIface PangoCairoFontInterface;
G_DEFINE_INTERFACE (PangoCairoFont, pango_cairo_font, PANGO_TYPE_FONT)

static void
pango_cairo_font_default_init (PangoCairoFontIface *iface)
{
}


static PangoCairoFontPrivateScaledFontData *
_pango_cairo_font_private_scaled_font_data_create (void)
{
  return g_slice_new (PangoCairoFontPrivateScaledFontData);
}

static void
_pango_cairo_font_private_scaled_font_data_destroy (PangoCairoFontPrivateScaledFontData *data)
{
  if (data)
    {
      cairo_font_options_destroy (data->options);
      g_slice_free (PangoCairoFontPrivateScaledFontData, data);
    }
}

static FT_Library ft_library;

static cairo_user_data_key_t cairo_user_data;

static cairo_status_t
render_func (cairo_scaled_font_t  *scaled_font,
             unsigned long         glyph,
             cairo_t              *cr,
             cairo_text_extents_t *extents)
{
  cairo_font_face_t *font_face;
  PangoUserFont *font;
  hb_glyph_extents_t glyph_extents;
  hb_position_t h_advance;
  hb_position_t v_advance;
  gboolean is_color;

  font_face = cairo_scaled_font_get_font_face (scaled_font);
  font = cairo_font_face_get_user_data (font_face, &cairo_user_data);

  extents->x_bearing = 0;
  extents->y_bearing = 0;
  extents->width = 0;
  extents->height = 0;
  extents->x_advance = 0;
  extents->y_advance = 0;

  if (!font->face->glyph_info_func (font->face, 1024,
                                    (hb_codepoint_t)glyph,
                                    &glyph_extents,
                                    &h_advance, &v_advance,
                                    &is_color,
                                    font->face->user_data))
    {
      return CAIRO_STATUS_USER_FONT_ERROR;
    }

  extents->x_bearing = glyph_extents.x_bearing / (double) 1024;
  extents->y_bearing = glyph_extents.y_bearing / (double) 1024;
  extents->width = glyph_extents.width / (double) 1024;
  extents->height = glyph_extents.height / (double) 1024;
  extents->x_advance = h_advance / (double) 1024;
  extents->y_advance = v_advance / (double) 1024;

  if (!font->face->render_func (font->face, font->size,
                                (hb_codepoint_t)glyph,
                                font->face->user_data,
                                "cairo",
                                cr))
    {
      return CAIRO_STATUS_USER_FONT_ERROR;
    }

  return CAIRO_STATUS_SUCCESS;
}

static cairo_font_face_t *
create_font_face_for_user_font (PangoUserFont *font)
{
  cairo_font_face_t *cairo_face;

  cairo_face = cairo_user_font_face_create ();
  cairo_font_face_set_user_data (cairo_face, &cairo_user_data, font, NULL);
#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC
  cairo_user_font_face_set_render_color_glyph_func (cairo_face, render_func);
#else
  cairo_user_font_face_set_render_glyph_func (cairo_face, render_func);
#endif

  return cairo_face;
}

static cairo_font_face_t *
create_font_face_for_hb_font (PangoHbFont *font)
{
  hb_blob_t *blob;
  const char *blob_data;
  unsigned int blob_length;
  FT_Face ft_face;
  hb_font_t *hb_font;
  unsigned int num_coords;
  const int *coords;
  cairo_font_face_t *cairo_face;
  static const cairo_user_data_key_t key;
  FT_Error error;

  if (g_once_init_enter (&ft_library))
    {
      FT_Library library;
      FT_Init_FreeType (&library);
      g_once_init_leave (&ft_library, library);
    }

  blob = hb_face_reference_blob (hb_font_get_face (pango_font_get_hb_font (PANGO_FONT (font))));
  blob_data = hb_blob_get_data (blob, &blob_length);
  hb_blob_destroy (blob);

  if ((error = FT_New_Memory_Face (ft_library,
                                   (const FT_Byte *) blob_data,
                                   blob_length,
                                   hb_face_get_index (font->face->face),
                                   &ft_face)) != 0)
    g_error ("FT_New_Memory_Face failed: %d %s", error, FT_Error_String (error));

  hb_font = pango_font_get_hb_font (PANGO_FONT (font));
  coords = hb_font_get_var_coords_normalized (hb_font, &num_coords);
  if (num_coords > 0)
    {
      FT_Fixed *ft_coords = (FT_Fixed *) g_alloca (num_coords * sizeof (FT_Fixed));

      for (unsigned int i = 0; i < num_coords; i++)
        ft_coords[i] = coords[i] << 2;

      FT_Set_Var_Blend_Coordinates (ft_face, num_coords, ft_coords);
    }

  cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, FT_LOAD_NO_HINTING | FT_LOAD_COLOR);
  if (font->face->embolden)
    cairo_ft_font_face_set_synthesize (cairo_face, CAIRO_FT_SYNTHESIZE_BOLD);
  cairo_font_face_set_user_data (cairo_face, &key,
                                 ft_face, (cairo_destroy_func_t) FT_Done_Face);

  return cairo_face;
}

cairo_scaled_font_t *
_pango_cairo_font_private_get_scaled_font (PangoCairoFontPrivate *cf_priv)
{
  cairo_font_face_t *font_face;

  if (G_LIKELY (cf_priv->scaled_font))
    return cf_priv->scaled_font;

  /* need to create it */

  if (G_UNLIKELY (cf_priv->data == NULL))
    {
      /* we have tried to create and failed before */
      return NULL;
    }

  if (PANGO_IS_CAIRO_FONT (cf_priv->cfont))
    font_face = (* PANGO_CAIRO_FONT_GET_IFACE (cf_priv->cfont)->create_font_face) (cf_priv->cfont);
  else if (PANGO_IS_HB_FONT (cf_priv->cfont))
    font_face = create_font_face_for_hb_font (PANGO_HB_FONT (cf_priv->cfont));
  else if (PANGO_IS_USER_FONT (cf_priv->cfont))
    font_face = create_font_face_for_user_font (PANGO_USER_FONT (cf_priv->cfont));

  if (G_UNLIKELY (font_face == NULL))
    goto done;

  cf_priv->scaled_font = cairo_scaled_font_create (font_face,
						   &cf_priv->data->font_matrix,
						   &cf_priv->data->ctm,
						   cf_priv->data->options);

  cairo_font_face_destroy (font_face);

done:

  if (G_UNLIKELY (cf_priv->scaled_font == NULL || cairo_scaled_font_status (cf_priv->scaled_font) != CAIRO_STATUS_SUCCESS))
    {
      cairo_scaled_font_t *scaled_font = cf_priv->scaled_font;
      PangoFont *font = PANGO_FONT (cf_priv->cfont);
      static GQuark warned_quark = 0; /* MT-safe */
      if (!warned_quark)
	warned_quark = g_quark_from_static_string ("pangocairo-scaledfont-warned");

      if (!g_object_get_qdata (G_OBJECT (font), warned_quark))
	{
	  PangoFontDescription *desc;
	  char *s;

	  desc = pango_font_describe (font);
	  s = pango_font_description_to_string (desc);
	  pango_font_description_free (desc);

	  g_warning ("failed to create cairo %s, expect ugly output. the offending font is '%s'",
		     font_face ? "scaled font" : "font face",
		     s);

	  if (!font_face)
		g_warning ("font_face is NULL");
	  else
		g_warning ("font_face status is: %s",
			   cairo_status_to_string (cairo_font_face_status (font_face)));

	  if (!scaled_font)
		g_warning ("scaled_font is NULL");
	  else
		g_warning ("scaled_font status is: %s",
			   cairo_status_to_string (cairo_scaled_font_status (scaled_font)));

	  g_free (s);

	  g_object_set_qdata_full (G_OBJECT (font), warned_quark,
				   GINT_TO_POINTER (1), NULL);
	}
    }

  _pango_cairo_font_private_scaled_font_data_destroy (cf_priv->data);
  cf_priv->data = NULL;

  return cf_priv->scaled_font;
}

/**
 * pango_cairo_font_get_scaled_font:
 * @font: (nullable): a `PangoFont` from a `PangoCairoFontMap`
 *
 * Gets the `cairo_scaled_font_t` used by @font.
 * The scaled font can be referenced and kept using
 * cairo_scaled_font_reference().
 *
 * Return value: (transfer none) (nullable): the `cairo_scaled_font_t`
 *   used by @font
 *
 * Since: 1.18
 */
cairo_scaled_font_t *
pango_cairo_font_get_scaled_font (PangoCairoFont *cfont)
{
  PangoCairoFontPrivate *cf_priv;

  if (G_UNLIKELY (!cfont))
    return NULL;

  cf_priv = PANGO_CAIRO_FONT_PRIVATE (cfont);

  return _pango_cairo_font_private_get_scaled_font (cf_priv);
}

cairo_scaled_font_t *
_pango_font_get_scaled_font (PangoFont *font)
{
  PangoCairoFontPrivate *cf_priv;

  cf_priv = _pango_font_get_cairo_font_private (font);

  if (G_UNLIKELY (!cf_priv))
    return NULL;

  return _pango_cairo_font_private_get_scaled_font (cf_priv);
}

/**
 * _pango_cairo_font_install:
 * @font: a `PangoCairoFont`
 * @cr: a #cairo_t
 *
 * Makes @font the current font for rendering in the specified
 * Cairo context.
 *
 * Return value: %TRUE if font was installed successfully, %FALSE otherwise.
 */
gboolean
_pango_cairo_font_install (PangoFont *font,
			   cairo_t   *cr)
{
  cairo_scaled_font_t *scaled_font;

  scaled_font = _pango_font_get_scaled_font (font);

  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return FALSE;

  cairo_set_scaled_font (cr, scaled_font);

  return TRUE;
}

static int
max_glyph_width (PangoLayout *layout)
{
  PangoLines *lines;
  PangoLine **l;
  int max_width = 0;

  lines = pango_layout_get_lines (layout);
  l = pango_lines_get_lines (lines);

  for (int i = 0; i < pango_lines_get_line_count (lines); i++)
    {
      PangoRun **runs = pango_line_get_runs (l[i]);
      int n_runs = pango_line_get_run_count (l[i]);

      for (int j = 0; j < n_runs; j++)
        {
          PangoGlyphString *glyphs = pango_run_get_glyphs (runs[j]);

          for (int i = 0; i < glyphs->num_glyphs; i++)
            {
              if (glyphs->glyphs[i].geometry.width > max_width)
                max_width = glyphs->glyphs[i].geometry.width;
            }
        }
    }

  return max_width;
}

typedef struct _PangoCairoFontMetricsInfo
{
  const char       *sample_str;
  PangoFontMetrics *metrics;
} PangoCairoFontMetricsInfo;

PangoFontMetrics *
_pango_cairo_font_get_metrics (PangoFont     *font,
			       PangoLanguage *language)
{
  PangoCairoFont *cfont = (PangoCairoFont *) font;
  PangoCairoFontPrivate *cf_priv = PANGO_CAIRO_FONT_PRIVATE (font);
  PangoCairoFontMetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;
  static int in_get_metrics;

  const char *sample_str = pango_language_get_sample_string (language);

  tmp_list = cf_priv->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;

      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      PangoFontMap *fontmap;
      PangoContext *context;
      cairo_font_options_t *font_options;
      PangoLayout *layout;
      PangoRectangle extents;
      PangoFontDescription *desc;
      cairo_scaled_font_t *scaled_font;
      cairo_matrix_t cairo_matrix;
      PangoMatrix pango_matrix;
      PangoMatrix identity = PANGO_MATRIX_INIT;
      glong sample_str_width;

      int height, shift;

      /* XXX this is racy.  need a ref'ing getter... */
      fontmap = pango_font_get_font_map (font);
      if (!fontmap)
        return pango_font_metrics_new ();
      fontmap = g_object_ref (fontmap);

      info = g_slice_new0 (PangoCairoFontMetricsInfo);

      cf_priv->metrics_by_lang = g_slist_prepend (cf_priv->metrics_by_lang, info);

      info->sample_str = sample_str;

      scaled_font = _pango_cairo_font_private_get_scaled_font (cf_priv);

      context = pango_font_map_create_context (fontmap);
      pango_context_set_language (context, language);

      font_options = cairo_font_options_create ();
      cairo_scaled_font_get_font_options (scaled_font, font_options);
      pango_cairo_context_set_font_options (context, font_options);
      cairo_font_options_destroy (font_options);

      info->metrics = (* PANGO_CAIRO_FONT_GET_IFACE (font)->create_base_metrics_for_context) (cfont, context);

      /* We now need to adjust the base metrics for ctm */
      cairo_scaled_font_get_ctm (scaled_font, &cairo_matrix);
      pango_matrix.xx = cairo_matrix.xx;
      pango_matrix.yx = cairo_matrix.yx;
      pango_matrix.xy = cairo_matrix.xy;
      pango_matrix.yy = cairo_matrix.yy;
      pango_matrix.x0 = 0;
      pango_matrix.y0 = 0;
      if (G_UNLIKELY (0 != memcmp (&identity, &pango_matrix, 4 * sizeof (double))))
        {
	  double xscale = pango_matrix_get_font_scale_factor (&pango_matrix);
	  if (xscale) xscale = 1 / xscale;

	  info->metrics->ascent *= xscale;
	  info->metrics->descent *= xscale;
	  info->metrics->height *= xscale;
	  info->metrics->underline_position *= xscale;
	  info->metrics->underline_thickness *= xscale;
	  info->metrics->strikethrough_position *= xscale;
	  info->metrics->strikethrough_thickness *= xscale;
	}

      /* Set the matrix on the context so we don't have to adjust the derived
       * metrics. */
      pango_context_set_matrix (context, &pango_matrix);

      /* Ugly. We need to prevent recursion when we call into
       * PangoLayout to determine approximate char width.
       */
      if (!in_get_metrics)
        {
          in_get_metrics = 1;

          /* Update approximate_*_width now */
          layout = pango_layout_new (context);
          desc = pango_font_describe_with_absolute_size (font);
          pango_layout_set_font_description (layout, desc);
          pango_font_description_free (desc);

          pango_layout_set_text (layout, sample_str, -1);
          pango_lines_get_extents (pango_layout_get_lines (layout), NULL, &extents);

          sample_str_width = pango_utf8_strwidth (sample_str);
          g_assert (sample_str_width > 0);
          info->metrics->approximate_char_width = extents.width / sample_str_width;

          pango_layout_set_text (layout, "0123456789", -1);
          info->metrics->approximate_digit_width = max_glyph_width (layout);

          g_object_unref (layout);
          in_get_metrics = 0;
        }

      /* We may actually reuse ascent/descent we got from cairo here.  that's
       * in cf_priv->font_extents.
       */
      height = info->metrics->ascent + info->metrics->descent;
      switch (cf_priv->gravity)
	{
	  default:
	  case PANGO_GRAVITY_AUTO:
	  case PANGO_GRAVITY_SOUTH:
	    break;
	  case PANGO_GRAVITY_NORTH:
	    info->metrics->ascent = info->metrics->descent;
	    break;
	  case PANGO_GRAVITY_EAST:
	  case PANGO_GRAVITY_WEST:
	    {
	      int ascent = height / 2;
	      if (cf_priv->is_hinted)
	        ascent = PANGO_UNITS_ROUND (ascent);
	      info->metrics->ascent = ascent;
	    }
	}
      shift = (height - info->metrics->ascent) - info->metrics->descent;
      info->metrics->descent += shift;
      info->metrics->underline_position -= shift;
      info->metrics->strikethrough_position -= shift;
      info->metrics->ascent = height - info->metrics->descent;

      g_object_unref (context);
      g_object_unref (fontmap);
    }

  return pango_font_metrics_ref (info->metrics);
}

static PangoCairoFontHexBoxInfo *
_pango_cairo_font_private_get_hex_box_info (PangoCairoFontPrivate *cf_priv)
{
  const char hexdigits[] = "0123456789ABCDEF";
  char c[2] = {0, 0};
  PangoFont *mini_font;
  PangoCairoFontHexBoxInfo *hbi;

  /* for metrics hinting */
  double scale_x = 1., scale_x_inv = 1., scale_y = 1., scale_y_inv = 1.;
  gboolean is_hinted;

  int i;
  int rows;
  double pad;
  double width = 0;
  double height = 0;
  cairo_font_options_t *font_options;
  cairo_font_extents_t font_extents;
  double size, mini_size;
  PangoFontDescription *desc;
  cairo_scaled_font_t *scaled_font, *scaled_mini_font;
  PangoMatrix pango_ctm, pango_font_matrix;
  cairo_matrix_t cairo_ctm, cairo_font_matrix;
  /*PangoGravity gravity;*/

  if (!cf_priv)
    return NULL;

  if (cf_priv->hbi)
    return cf_priv->hbi;

  scaled_font = _pango_cairo_font_private_get_scaled_font (cf_priv);
  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return NULL;

  is_hinted = cf_priv->is_hinted;

  font_options = cairo_font_options_create ();
  desc = pango_font_describe_with_absolute_size ((PangoFont *)cf_priv->cfont);
  /*gravity = pango_font_description_get_gravity (desc);*/

  cairo_scaled_font_get_ctm (scaled_font, &cairo_ctm);
  cairo_scaled_font_get_font_matrix (scaled_font, &cairo_font_matrix);
  cairo_scaled_font_get_font_options (scaled_font, font_options);
  /* I started adding support for vertical hexboxes here, but it's too much
   * work.  Easier to do with cairo user fonts and vertical writing mode
   * support in cairo.
   */
  /*cairo_matrix_rotate (&cairo_ctm, pango_gravity_to_rotation (gravity));*/
  pango_ctm.xx = cairo_ctm.xx;
  pango_ctm.yx = cairo_ctm.yx;
  pango_ctm.xy = cairo_ctm.xy;
  pango_ctm.yy = cairo_ctm.yy;
  pango_ctm.x0 = cairo_ctm.x0;
  pango_ctm.y0 = cairo_ctm.y0;
  pango_font_matrix.xx = cairo_font_matrix.xx;
  pango_font_matrix.yx = cairo_font_matrix.yx;
  pango_font_matrix.xy = cairo_font_matrix.xy;
  pango_font_matrix.yy = cairo_font_matrix.yy;
  pango_font_matrix.x0 = cairo_font_matrix.x0;
  pango_font_matrix.y0 = cairo_font_matrix.y0;

  size = pango_matrix_get_font_scale_factor (&pango_font_matrix) /
	 pango_matrix_get_font_scale_factor (&pango_ctm);

  if (is_hinted)
    {
      /* prepare for some hinting */
      double x, y;

      x = 1.; y = 0.;
      cairo_matrix_transform_distance (&cairo_ctm, &x, &y);
      scale_x = sqrt (x*x + y*y);
      scale_x_inv = 1 / scale_x;

      x = 0.; y = 1.;
      cairo_matrix_transform_distance (&cairo_ctm, &x, &y);
      scale_y = sqrt (x*x + y*y);
      scale_y_inv = 1 / scale_y;
    }

/* we hint to the nearest device units */
#define HINT(value, scale, scale_inv) (ceil ((value-1e-5) * scale) * scale_inv)
#define HINT_X(value) HINT ((value), scale_x, scale_x_inv)
#define HINT_Y(value) HINT ((value), scale_y, scale_y_inv)

  /* create mini_font description */
  {
    PangoFontMap *fontmap;
    PangoContext *context;

    /* XXX this is racy.  need a ref'ing getter... */
    fontmap = pango_font_get_font_map ((PangoFont *)cf_priv->cfont);
    if (!fontmap)
      return NULL;
    fontmap = g_object_ref (fontmap);

    /* we inherit most font properties for the mini font.  just
     * change family and size.  means, you get bold hex digits
     * in the hexbox for a bold font.
     */

    /* We should rotate the box, not glyphs */
    pango_font_description_unset_fields (desc, PANGO_FONT_MASK_GRAVITY);

    pango_font_description_set_family_static (desc, "monospace");

    rows = 2;
    mini_size = size / 2.2;
    if (is_hinted)
      {
	mini_size = HINT_Y (mini_size);

	if (mini_size < 6.0)
	  {
	    rows = 1;
	    mini_size = MIN (MAX (size - 1, 0), 6.0);
	  }
      }

    pango_font_description_set_absolute_size (desc, pango_units_from_double (mini_size));

    /* load mini_font */

    context = pango_font_map_create_context (fontmap);

    pango_context_set_matrix (context, &pango_ctm);
    pango_context_set_language (context, pango_script_get_sample_language (G_UNICODE_SCRIPT_LATIN));
    pango_cairo_context_set_font_options (context, font_options);
    mini_font = pango_font_map_load_font (fontmap, context, desc);

    g_object_unref (context);
    g_object_unref (fontmap);
  }

  pango_font_description_free (desc);
  cairo_font_options_destroy (font_options);

  scaled_mini_font = _pango_font_get_scaled_font (mini_font);
  if (G_UNLIKELY (scaled_mini_font == NULL || cairo_scaled_font_status (scaled_mini_font) != CAIRO_STATUS_SUCCESS))
    return NULL;

  for (i = 0 ; i < 16 ; i++)
    {
      cairo_text_extents_t extents;

      c[0] = hexdigits[i];
      cairo_scaled_font_text_extents (scaled_mini_font, c, &extents);
      width = MAX (width, extents.width);
      height = MAX (height, extents.height);
    }

  cairo_scaled_font_extents (scaled_font, &font_extents);
  if (font_extents.ascent + font_extents.descent <= 0)
    {
      font_extents.ascent = PANGO_UNKNOWN_GLYPH_HEIGHT;
      font_extents.descent = 0;
    }

  pad = (font_extents.ascent + font_extents.descent) / 43;
  pad = MIN (pad, mini_size);

  hbi = g_slice_new (PangoCairoFontHexBoxInfo);
  hbi->font = mini_font;
  hbi->rows = rows;

  hbi->digit_width  = width;
  hbi->digit_height = height;

  hbi->pad_x = pad;
  hbi->pad_y = pad;

  if (is_hinted)
    {
      hbi->digit_width  = HINT_X (hbi->digit_width);
      hbi->digit_height = HINT_Y (hbi->digit_height);
      hbi->pad_x = HINT_X (hbi->pad_x);
      hbi->pad_y = HINT_Y (hbi->pad_y);
    }

  hbi->line_width = MIN (hbi->pad_x, hbi->pad_y);

  hbi->box_height = 3 * hbi->pad_y + rows * (hbi->pad_y + hbi->digit_height);

  if (rows == 1 || hbi->box_height <= font_extents.ascent)
    {
      hbi->box_descent = 2 * hbi->pad_y;
    }
  else if (hbi->box_height <= font_extents.ascent + font_extents.descent - 2 * hbi->pad_y)
    {
      hbi->box_descent = 2 * hbi->pad_y + hbi->box_height - font_extents.ascent;
    }
  else
    {
      hbi->box_descent = font_extents.descent * hbi->box_height /
			 (font_extents.ascent + font_extents.descent);
    }
  if (is_hinted)
    {
       hbi->box_descent = HINT_Y (hbi->box_descent);
    }

  cf_priv->hbi = hbi;
  return hbi;
}

static void
_pango_cairo_font_hex_box_info_destroy (PangoCairoFontHexBoxInfo *hbi)
{
  if (hbi)
    {
      g_object_unref (hbi->font);
      g_slice_free (PangoCairoFontHexBoxInfo, hbi);
    }
}

static void
free_cairo_font_private (gpointer data)
{
  PangoCairoFontPrivate *cf_priv = data;
  _pango_cairo_font_private_finalize (cf_priv);
  g_free (data);
}

PangoCairoFontPrivate *
_pango_font_get_cairo_font_private (PangoFont *font)
{
  PangoCairoFontPrivate *cf_priv;

  if (PANGO_IS_CAIRO_FONT (font))
    return PANGO_CAIRO_FONT_PRIVATE (font);

  cf_priv = g_object_get_data (G_OBJECT (font), "pango-hb-font-cairo_private");
  if (!cf_priv)
    {
      CommonFont *cf = (CommonFont *)font;
      cairo_font_options_t *font_options;
      cairo_matrix_t font_matrix;
      double x_scale, y_scale;
      int size;

      cairo_matrix_init (&font_matrix, 1., 0., 0., 1., 0., 0.);
      x_scale = y_scale = 1;

      if (PANGO_IS_HB_FONT (font))
        {
          PangoHbFont *hbfont = PANGO_HB_FONT (font);
          if (hbfont->face->matrix)
            cairo_matrix_init (&font_matrix,
                               hbfont->face->matrix->xx,
                               - hbfont->face->matrix->yx,
                               - hbfont->face->matrix->xy,
                               hbfont->face->matrix->yy,
                               0., 0.);

          x_scale = hbfont->face->x_scale;
          y_scale = hbfont->face->y_scale;
        }

      size = cf->size * cf->dpi / 72.;

      cairo_matrix_scale (&font_matrix,
                          x_scale * size / (double)PANGO_SCALE,
                          y_scale * size / (double)PANGO_SCALE);

      font_options = cairo_font_options_create ();
      cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_NONE);
      cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_OFF);

      cf_priv = g_new0 (PangoCairoFontPrivate, 1);
      _pango_cairo_font_private_initialize (cf_priv,
                                            (PangoCairoFont *)font,
                                            cf->gravity,
                                            font_options,
                                            &cf->matrix,
                                            &font_matrix);

      cairo_font_options_destroy (font_options);

      g_object_set_data_full (G_OBJECT (font), "pango-hb-font-cairo_private",
                              cf_priv, free_cairo_font_private);
    }

  return cf_priv;
}

PangoCairoFontHexBoxInfo *
_pango_cairo_font_get_hex_box_info (PangoFont *font)
{
  PangoCairoFontPrivate *cf_priv = _pango_font_get_cairo_font_private (font);

  return _pango_cairo_font_private_get_hex_box_info (cf_priv);
}

void
_pango_cairo_font_private_initialize (PangoCairoFontPrivate      *cf_priv,
				      PangoCairoFont             *cfont,
				      PangoGravity                gravity,
				      const cairo_font_options_t *font_options,
				      const PangoMatrix          *pango_ctm,
				      const cairo_matrix_t       *font_matrix)
{
  cairo_matrix_t gravity_matrix;

  cf_priv->cfont = cfont;
  cf_priv->gravity = gravity != PANGO_GRAVITY_AUTO ? gravity : PANGO_GRAVITY_SOUTH;

  cf_priv->data = _pango_cairo_font_private_scaled_font_data_create (); 

  /* first apply gravity rotation, then font_matrix, such that
   * vertical italic text comes out "correct".  we don't do anything
   * like baseline adjustment etc though.  should be specially
   * handled when we support italic correction. */
  cairo_matrix_init_rotate(&gravity_matrix,
			   pango_gravity_to_rotation (cf_priv->gravity));
  cairo_matrix_multiply (&cf_priv->data->font_matrix,
			 font_matrix,
			 &gravity_matrix);

  if (pango_ctm)
    cairo_matrix_init (&cf_priv->data->ctm,
		       pango_ctm->xx,
		       pango_ctm->yx,
		       pango_ctm->xy,
		       pango_ctm->yy,
		       0., 0.);
  else
    cairo_matrix_init_identity (&cf_priv->data->ctm);

  cf_priv->data->options = cairo_font_options_copy (font_options);
  cf_priv->is_hinted = cairo_font_options_get_hint_metrics (font_options) != CAIRO_HINT_METRICS_OFF;

  cf_priv->scaled_font = NULL;
  cf_priv->hbi = NULL;
  cf_priv->glyph_extents_cache = NULL;
  cf_priv->metrics_by_lang = NULL;
}

static void
free_metrics_info (PangoCairoFontMetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_slice_free (PangoCairoFontMetricsInfo, info);
}

void
_pango_cairo_font_private_finalize (PangoCairoFontPrivate *cf_priv)
{
  _pango_cairo_font_private_scaled_font_data_destroy (cf_priv->data);

  if (cf_priv->scaled_font)
    cairo_scaled_font_destroy (cf_priv->scaled_font);
  cf_priv->scaled_font = NULL;

  _pango_cairo_font_hex_box_info_destroy (cf_priv->hbi);
  cf_priv->hbi = NULL;

  if (cf_priv->glyph_extents_cache)
    g_free (cf_priv->glyph_extents_cache);
  cf_priv->glyph_extents_cache = NULL;

  g_slist_foreach (cf_priv->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (cf_priv->metrics_by_lang);
  cf_priv->metrics_by_lang = NULL;
}

gboolean
_pango_cairo_font_private_is_metrics_hinted (PangoCairoFontPrivate *cf_priv)
{
  return cf_priv->is_hinted;
}


static void
get_space_extents (PangoCairoFontPrivate *cf_priv,
                   PangoRectangle        *ink_rect,
                   PangoRectangle        *logical_rect)
{
  /* See https://docs.microsoft.com/en-us/typography/develop/character-design-standards/whitespace */

  int width = pango_font_get_absolute_size (PANGO_FONT (cf_priv->cfont)) / 4;

  if (ink_rect)
    {
      ink_rect->x = ink_rect->y = ink_rect->height = 0;
      ink_rect->width = width;
    }
  if (logical_rect)
    {
      *logical_rect = cf_priv->font_extents;
      logical_rect->width = width;
    }
}

static void
_pango_cairo_font_private_get_glyph_extents_missing (PangoCairoFontPrivate *cf_priv,
						     PangoGlyph             glyph,
						     PangoRectangle        *ink_rect,
						     PangoRectangle        *logical_rect)
{
  PangoCairoFontHexBoxInfo *hbi;
  gunichar ch;
  gint rows, cols;

  ch = glyph & ~PANGO_GLYPH_UNKNOWN_FLAG;

  if (ch == 0x20 || ch == 0x2423)
    {
      get_space_extents (cf_priv, ink_rect, logical_rect);
      return;
    }

  hbi = _pango_cairo_font_private_get_hex_box_info (cf_priv);
  if (!hbi)
    {
      pango_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  if (G_UNLIKELY (glyph == PANGO_GLYPH_INVALID_INPUT || ch > 0x10FFFF))
    {
      rows = hbi->rows;
      cols = 1;
    }
  else if (pango_get_ignorable_size (ch, &rows, &cols))
    {
      /* We special-case ignorables when rendering hex boxes */
    }
  else
    {
      rows = hbi->rows;
      cols = ((glyph & ~PANGO_GLYPH_UNKNOWN_FLAG) > 0xffff ? 6 : 4) / rows;
    }

  if (ink_rect)
    {
      ink_rect->x = PANGO_SCALE * hbi->pad_x;
      ink_rect->y = PANGO_SCALE * (hbi->box_descent - hbi->box_height);
      ink_rect->width = PANGO_SCALE * (3 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      ink_rect->height = PANGO_SCALE * hbi->box_height;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = PANGO_SCALE * (hbi->box_descent - (hbi->box_height + hbi->pad_y));
      logical_rect->width = PANGO_SCALE * (5 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      logical_rect->height = PANGO_SCALE * (hbi->box_height + 2 * hbi->pad_y);
    }
}

#define GLYPH_CACHE_NUM_ENTRIES 256 /* should be power of two */
#define GLYPH_CACHE_MASK (GLYPH_CACHE_NUM_ENTRIES - 1)
/* An entry in the fixed-size cache for the glyph->extents mapping.
 * The cache is indexed by the lower N bits of the glyph (see
 * GLYPH_CACHE_NUM_ENTRIES).  For scripts with few glyphs,
 * this should provide pretty much instant lookups.
 */
struct _PangoCairoFontGlyphExtentsCacheEntry
{
  PangoGlyph     glyph;
  int            width;
  PangoRectangle ink_rect;
};

static gboolean
_pango_cairo_font_private_glyph_extents_cache_init (PangoCairoFontPrivate *cf_priv)
{
  hb_font_extents_t extents;

  hb_font_get_h_extents (pango_font_get_hb_font (PANGO_FONT (cf_priv->cfont)),
                         &extents);

  cf_priv->font_extents.x = 0;
  cf_priv->font_extents.width = 0;
  cf_priv->font_extents.height = extents.ascender - extents.descender;

  switch (cf_priv->gravity)
    {
      default:
      case PANGO_GRAVITY_AUTO:
      case PANGO_GRAVITY_SOUTH:
        cf_priv->font_extents.y = - extents.ascender;
        break;
      case PANGO_GRAVITY_NORTH:
        cf_priv->font_extents.y = extents.descender;
        break;
      case PANGO_GRAVITY_EAST:
      case PANGO_GRAVITY_WEST:
        {
          int ascent = cf_priv->font_extents.height / 2;
          if (cf_priv->is_hinted)
            ascent = PANGO_UNITS_ROUND (ascent);
          cf_priv->font_extents.y = - ascent;
        }
       break;
    }

  if (cf_priv->is_hinted)
    {
      if (cf_priv->font_extents.y < 0)
        cf_priv->font_extents.y = PANGO_UNITS_FLOOR (cf_priv->font_extents.y);
      else
        cf_priv->font_extents.y = PANGO_UNITS_CEIL (cf_priv->font_extents.y);
      if (cf_priv->font_extents.height < 0)
        cf_priv->font_extents.height = PANGO_UNITS_FLOOR (extents.ascender) - PANGO_UNITS_CEIL (extents.descender);
      else
        cf_priv->font_extents.height = PANGO_UNITS_CEIL (extents.ascender) - PANGO_UNITS_FLOOR (extents.descender);
    }

  if (PANGO_GRAVITY_IS_IMPROPER (cf_priv->gravity))
    {
      cf_priv->font_extents.y = - cf_priv->font_extents.y;
      cf_priv->font_extents.height = - cf_priv->font_extents.height;
    }

  if (!cf_priv->glyph_extents_cache)
    {
      cf_priv->glyph_extents_cache = g_new0 (PangoCairoFontGlyphExtentsCacheEntry, GLYPH_CACHE_NUM_ENTRIES);
      /* Make sure all cache entries are invalid initially */
      cf_priv->glyph_extents_cache[0].glyph = 1; /* glyph 1 cannot happen in bucket 0 */
    }

  return TRUE;
}


/* Fills in the glyph extents cache entry
 */
static void
compute_glyph_extents (PangoCairoFontPrivate  *cf_priv,
		       PangoGlyph              glyph,
		       PangoCairoFontGlyphExtentsCacheEntry *entry)
{
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;

  cairo_glyph.index = glyph;
  cairo_glyph.x = 0;
  cairo_glyph.y = 0;

  cairo_scaled_font_glyph_extents (_pango_cairo_font_private_get_scaled_font (cf_priv),
				   &cairo_glyph, 1, &extents);

  entry->glyph = glyph;
  if (PANGO_GRAVITY_IS_VERTICAL (cf_priv->gravity))
    entry->width = pango_units_from_double (extents.y_advance);
  else
    entry->width = pango_units_from_double (extents.x_advance);
  entry->ink_rect.x = pango_units_from_double (extents.x_bearing);
  entry->ink_rect.y = pango_units_from_double (extents.y_bearing);
  entry->ink_rect.width = pango_units_from_double (extents.width);
  entry->ink_rect.height = pango_units_from_double (extents.height);
}

static PangoCairoFontGlyphExtentsCacheEntry *
_pango_cairo_font_private_get_glyph_extents_cache_entry (PangoCairoFontPrivate  *cf_priv,
							 PangoGlyph              glyph)
{
  PangoCairoFontGlyphExtentsCacheEntry *entry;
  guint idx;

  idx = glyph & GLYPH_CACHE_MASK;
  entry = cf_priv->glyph_extents_cache + idx;

  if (entry->glyph != glyph)
    {
      compute_glyph_extents (cf_priv, glyph, entry);
    }

  return entry;
}

void
_pango_cairo_font_private_get_glyph_extents (PangoCairoFontPrivate *cf_priv,
					     PangoGlyph             glyph,
					     PangoRectangle        *ink_rect,
					     PangoRectangle        *logical_rect)
{
  PangoCairoFontGlyphExtentsCacheEntry *entry;

  if (!cf_priv ||
      (cf_priv->glyph_extents_cache == NULL &&
       !_pango_cairo_font_private_glyph_extents_cache_init (cf_priv)))
    {
      /* Get generic unknown-glyph extents. */
      pango_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->y = ink_rect->width = ink_rect->height = 0;
      if (logical_rect)
	*logical_rect = cf_priv->font_extents;
      return;
    }
  else if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
      _pango_cairo_font_private_get_glyph_extents_missing (cf_priv, glyph, ink_rect, logical_rect);
      return;
    }

  entry = _pango_cairo_font_private_get_glyph_extents_cache_entry (cf_priv, glyph);

  if (ink_rect)
    *ink_rect = entry->ink_rect;
  if (logical_rect)
    {
      *logical_rect = cf_priv->font_extents;
      switch (cf_priv->gravity)
        {
        case PANGO_GRAVITY_SOUTH:
          logical_rect->width = entry->width;
          break;
        case PANGO_GRAVITY_EAST:
          logical_rect->width = cf_priv->font_extents.height;
          logical_rect->x = - logical_rect->width;
          break;
        case PANGO_GRAVITY_NORTH:
          logical_rect->width = entry->width;
          break;
        case PANGO_GRAVITY_WEST:
          logical_rect->width = - cf_priv->font_extents.height;
          logical_rect->x = - logical_rect->width;
          break;
        case PANGO_GRAVITY_AUTO:
        default:
          g_assert_not_reached ();
        }
    }
}
