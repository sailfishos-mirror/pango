/* Pango
 * test-color.c: Test program for pango_color_parse()
 *
 * Copyright (C) 2002 Matthias Clasen
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

#include <glib.h>
#include <pango/pango.h>

typedef struct _ColorSpec {
  const char *spec;
  gboolean valid;
  guint16 red;
  guint16 green;
  guint16 blue;
  guint16 alpha;
} ColorSpec;

static void
test_one_color (ColorSpec *spec)
{
  PangoColor color;
  gboolean accepted;

  accepted = pango_color_parse (&color, spec->spec);

  if (!spec->valid)
    {
      g_assert_false (accepted);
    }
  else
    {
      g_assert_true (accepted);
      g_assert_cmpuint (color.red, ==, spec->red);
      g_assert_cmpuint (color.green, ==, spec->green);
      g_assert_cmpuint (color.blue, ==, spec->blue);
      g_assert_cmpuint (color.alpha, ==, spec->alpha);
    }
}

ColorSpec specs [] = {
  { "#abc",          1, 0xaaaa, 0xbbbb, 0xcccc, 0xffff },
  { "#aabbcc",       1, 0xaaaa, 0xbbbb, 0xcccc, 0xffff },
  { "#aaabbbccc",    1, 0xaaaa, 0xbbbb, 0xcccc, 0xffff },
  { "#100100100",    1, 0x1001, 0x1001, 0x1001, 0xffff },
  { "#aaaabbbbcccc", 1, 0xaaaa, 0xbbbb, 0xcccc, 0xffff },
  { "#fff",          1, 0xffff, 0xffff, 0xffff, 0xffff },
  { "#ffffff",       1, 0xffff, 0xffff, 0xffff, 0xffff },
  { "#fffffffff",    1, 0xffff, 0xffff, 0xffff, 0xffff },
  { "#ffffffffffff", 1, 0xffff, 0xffff, 0xffff, 0xffff },
  { "#000",          1, 0x0000, 0x0000, 0x0000, 0xffff },
  { "#000000",       1, 0x0000, 0x0000, 0x0000, 0xffff },
  { "#000000000",    1, 0x0000, 0x0000, 0x0000, 0xffff },
  { "#000000000000", 1, 0x0000, 0x0000, 0x0000, 0xffff },
  { "#AAAABBBBCCCC", 1, 0xaaaa, 0xbbbb, 0xcccc, 0xffff },
  { "#aa bb cc ",    0, 0, 0, 0, 0 },
  { "#aa bb ccc",    0, 0, 0, 0, 0 },
  { "#ab",           0, 0, 0, 0, 0 },
  { "#aabb",         1, 0xaaaa, 0xaaaa, 0xbbbb, 0xbbbb },
  { "#aaabb",        0, 0, 0, 0, 0 },
  { "aaabb",         0, 0, 0, 0, 0 },
  { "",              0, 0, 0, 0, 0 },
  { "#",             0, 0, 0, 0, 0 },
  { "##fff",         0, 0, 0, 0, 0 },
  { "#0000ff+",      0, 0, 0, 0, 0 },
  { "#0000f+",       0, 0, 0, 0, 0 },
  { "#0x00x10x2",    0, 0, 0, 0, 0 },
  { "#abcd",         1, 0xaaaa, 0xbbbb, 0xcccc, 0xdddd },
  { "#aabbccdd",     1, 0xaaaa, 0xbbbb, 0xcccc, 0xdddd },
  { "#aaaabbbbccccdddd", 1, 0xaaaa, 0xbbbb, 0xcccc, 0xdddd },
  { NULL,            0, 0, 0, 0, 0 }
};

static void
test_color (void)
{
  ColorSpec *spec;

  for (spec = specs; spec->spec; spec++)
    test_one_color (spec);
}

static void
test_color_copy (void)
{
  PangoColor orig = { 0, 200, 5000, 666 };
  PangoColor *copy;

  copy = pango_color_copy (&orig);

  g_assert_cmpint (orig.red, ==, copy->red);
  g_assert_cmpint (orig.green, ==, copy->green);
  g_assert_cmpint (orig.blue, ==, copy->blue);
  g_assert_cmpint (orig.alpha, ==, copy->alpha);

  pango_color_free (copy);
}

static void
test_color_serialize (void)
{
  PangoColor orig = { 0, 200, 5000, 666 };
  char *string;

  string = pango_color_to_string (&orig);

  g_assert_cmpstr (string, ==, "#000000c81388029a");

  g_free (string);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/color/parse", test_color);
  g_test_add_func ("/color/copy", test_color_copy);
  g_test_add_func ("/color/serialize", test_color_serialize);

  return g_test_run ();
}
