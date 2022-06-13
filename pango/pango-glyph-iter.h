/*
 * Copyright (C) 2002 Red Hat Software
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <pango/pango-glyph-item.h>

G_BEGIN_DECLS

/**
 * PangoGlyphItemIter:
 *
 * A `PangoGlyphItemIter` is an iterator over the clusters in a
 * `PangoGlyphItem`.
 *
 * The *forward direction* of the iterator is the logical direction of text.
 * That is, with increasing @start_index and @start_char values. If @glyph_item
 * is right-to-left (that is, if `glyph_item->item->analysis.level` is odd),
 * then @start_glyph decreases as the iterator moves forward.  Moreover,
 * in right-to-left cases, @start_glyph is greater than @end_glyph.
 *
 * An iterator should be initialized using either
 * [method@Pango.GlyphItemIter.init_start] or
 * [method@Pango.GlyphItemIter.init_end], for forward and backward iteration
 * respectively, and walked over using any desired mixture of
 * [method@Pango.GlyphItemIter.next_cluster] and
 * [method@Pango.GlyphItemIter.prev_cluster].
 *
 * A common idiom for doing a forward iteration over the clusters is:
 *
 * ```
 * PangoGlyphItemIter cluster_iter;
 * gboolean have_cluster;
 *
 * for (have_cluster = pango_glyph_item_iter_init_start (&cluster_iter,
 *                                                       glyph_item, text);
 *      have_cluster;
 *      have_cluster = pango_glyph_item_iter_next_cluster (&cluster_iter))
 * {
 *   ...
 * }
 * ```
 *
 * Note that @text is the start of the text for layout, which is then
 * indexed by `glyph_item->item->offset` to get to the text of @glyph_item.
 * The @start_index and @end_index values can directly index into @text. The
 * @start_glyph, @end_glyph, @start_char, and @end_char values however are
 * zero-based for the @glyph_item.  For each cluster, the item pointed at by
 * the start variables is included in the cluster while the one pointed at by
 * end variables is not.
 *
 * None of the members of a `PangoGlyphItemIter` should be modified manually.
 */
typedef struct _PangoGlyphItemIter PangoGlyphItemIter;

struct _PangoGlyphItemIter
{
  PangoGlyphItem *glyph_item;
  const char *text;

  int start_glyph;
  int start_index;
  int start_char;

  int end_glyph;
  int end_index;
  int end_char;
};

#define PANGO_TYPE_GLYPH_ITEM_ITER (pango_glyph_item_iter_get_type ())

PANGO_AVAILABLE_IN_ALL
GType                   pango_glyph_item_iter_get_type     (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
PangoGlyphItemIter *    pango_glyph_item_iter_copy         (PangoGlyphItemIter *orig);
PANGO_AVAILABLE_IN_ALL
void                    pango_glyph_item_iter_free         (PangoGlyphItemIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_glyph_item_iter_init_start   (PangoGlyphItemIter *iter,
                                                            PangoGlyphItem     *glyph_item,
                                                            const char         *text);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_glyph_item_iter_init_end     (PangoGlyphItemIter *iter,
                                                            PangoGlyphItem     *glyph_item,
                                                             const char         *text);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_glyph_item_iter_next_cluster (PangoGlyphItemIter *iter);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_glyph_item_iter_prev_cluster (PangoGlyphItemIter *iter);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoGlyphItemIter, pango_glyph_item_iter_free)

G_END_DECLS
