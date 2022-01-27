/*
 * Copyright 2022 Red Hat, Inc.
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

#include "pango-userfont.h"
#include "pango-userface.h"
#include "pango-font-private.h"


struct _PangoUserFont
{
  PangoFont parent_instance;

  int size; /* point size, scaled by PANGO_SCALE */
  float dpi;
  PangoGravity gravity;
  PangoMatrix matrix;

  /* up to here shared with PangoHbFont */

  PangoUserFace *face;
};
