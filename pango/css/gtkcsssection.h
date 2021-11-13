/* GTK - The GIMP Toolkit
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GTK_CSS_SECTION_H__
#define __GTK_CSS_SECTION_H__

#include <gio/gio.h>
#include <pango/css/gtkcsslocation.h>

G_BEGIN_DECLS

#define GTK_TYPE_CSS_SECTION         (gtk_css_section_get_type ())

/**
 * GtkCssSection:
 *
 * Defines a part of a CSS document.
 *
 * Because sections are nested into one another, you can use
 * gtk_css_section_get_parent() to get the containing region.
 */
typedef struct _GtkCssSection GtkCssSection;

GType              gtk_css_section_get_type            (void) G_GNUC_CONST;

GtkCssSection *    gtk_css_section_new                 (GFile                *file,
                                                        const GtkCssLocation *start,
                                                        const GtkCssLocation *end);
GtkCssSection *    gtk_css_section_ref                 (GtkCssSection        *section);
void               gtk_css_section_unref               (GtkCssSection        *section);

void               gtk_css_section_print               (const GtkCssSection  *section,
                                                        GString              *string);
char *             gtk_css_section_to_string           (const GtkCssSection  *section);

GtkCssSection *    gtk_css_section_get_parent          (const GtkCssSection  *section);
GFile *            gtk_css_section_get_file            (const GtkCssSection  *section);
const GtkCssLocation *
                   gtk_css_section_get_start_location  (const GtkCssSection  *section);
const GtkCssLocation *
                   gtk_css_section_get_end_location    (const GtkCssSection  *section);

G_END_DECLS

#endif /* __GTK_CSS_SECTION_H__ */
