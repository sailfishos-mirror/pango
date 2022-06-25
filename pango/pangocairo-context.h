/*
 * Copyright (C) 1999, 2004 Red Hat, Inc.
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

#include <pango/pango.h>
#include <cairo.h>

G_BEGIN_DECLS

PANGO2_AVAILABLE_IN_ALL
Pango2Context *         pango2_cairo_create_context              (cairo_t                        *cr);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_cairo_update_context              (cairo_t                        *cr,
                                                                  Pango2Context                  *context);
PANGO2_AVAILABLE_IN_ALL
Pango2Layout *          pango2_cairo_create_layout               (cairo_t                        *cr);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_cairo_update_layout               (cairo_t                        *cr,
                                                                  Pango2Layout                   *layout);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_cairo_context_set_font_options    (Pango2Context                  *context,
                                                                  const cairo_font_options_t     *options);
PANGO2_AVAILABLE_IN_ALL
const cairo_font_options_t *
                        pango2_cairo_context_get_font_options    (Pango2Context                  *context);

G_END_DECLS