/*
 * Copyright (C) 1999 Red Hat Software
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

#include <glib.h>
#include <pango2/pango-item.h>

G_BEGIN_DECLS


/* Logical attributes of a character.
 */
/**
 * Pango2LogAttr:
 * @is_line_break: if set, can break line in front of character
 * @is_mandatory_break: if set, must break line in front of character
 * @is_char_break: if set, can break here when doing character wrapping
 * @is_white: is whitespace character
 * @is_cursor_position: if set, cursor can appear in front of character.
 *   i.e. this is a grapheme boundary, or the first character in the text.
 *   This flag implements Unicode's
 *   [Grapheme Cluster Boundaries](http://www.unicode.org/reports/tr29/)
 *   semantics.
 * @is_word_start: is first character in a word
 * @is_word_end: is first non-word char after a word
 *   Note that in degenerate cases, you could have both @is_word_start
 *   and @is_word_end set for some character.
 * @is_sentence_boundary: is a sentence boundary.
 *   There are two ways to divide sentences. The first assigns all
 *   inter-sentence whitespace/control/format chars to some sentence,
 *   so all chars are in some sentence; @is_sentence_boundary denotes
 *   the boundaries there. The second way doesn't assign
 *   between-sentence spaces, etc. to any sentence, so
 *   @is_sentence_start/@is_sentence_end mark the boundaries of those sentences.
 * @is_sentence_start: is first character in a sentence
 * @is_sentence_end: is first char after a sentence.
 *   Note that in degenerate cases, you could have both @is_sentence_start
 *   and @is_sentence_end set for some character. (e.g. no space after a
 *   period, so the next sentence starts right away)
 * @backspace_deletes_character: if set, backspace deletes one character
 *   rather than the entire grapheme cluster. This field is only meaningful
 *   on grapheme boundaries (where @is_cursor_position is set). In some languages,
 *   the full grapheme (e.g. letter + diacritics) is considered a unit, while in
 *   others, each decomposed character in the grapheme is a unit. In the default
 *   implementation of [func@default_break], this bit is set on all grapheme
 *   boundaries except those following Latin, Cyrillic or Greek base characters.
 * @is_expandable_space: is a whitespace character that can possibly be
 *   expanded for justification purposes.
 * @is_word_boundary: is a word boundary, as defined by UAX#29.
 *   More specifically, means that this is not a position in the middle of a word.
 *   For example, both sides of a punctuation mark are considered word boundaries.
 *   This flag is particularly useful when selecting text word-by-word. This flag
 *   implements Unicode's [Word Boundaries](http://www.unicode.org/reports/tr29/)
 *   semantics.
 * @break_inserts_hyphen: when breaking lines before this char, insert a hyphen.
 * @break_removes_preceding: when breaking lines before this char, remove the
 *   preceding char.
 *
 * The `Pango2LogAttr` structure stores information about the attributes of a
 * single character.
 */
struct _Pango2LogAttr
{
  guint is_line_break               : 1;
  guint is_mandatory_break          : 1;
  guint is_char_break               : 1;
  guint is_white                    : 1;
  guint is_cursor_position          : 1;
  guint is_word_start               : 1;
  guint is_word_end                 : 1;
  guint is_sentence_boundary        : 1;
  guint is_sentence_start           : 1;
  guint is_sentence_end             : 1;
  guint backspace_deletes_character : 1;
  guint is_expandable_space         : 1;
  guint is_word_boundary            : 1;
  guint break_inserts_hyphen        : 1;
  guint break_removes_preceding     : 1;

  guint reserved                    : 17;
};

PANGO2_AVAILABLE_IN_ALL
void                    pango2_get_log_attrs     (const char     *text,
                                                  int             length,
                                                  Pango2AttrList *attr_list,
                                                  int             level,
                                                  Pango2Language *language,
                                                  Pango2LogAttr  *attrs,
                                                  int             attrs_len);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_default_break     (const char     *text,
                                                  int             length,
                                                  Pango2LogAttr  *attrs,
                                                  int             attrs_len);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_tailor_break      (const char     *text,
                                                  int             length,
                                                  Pango2Analysis *analysis,
                                                  int             offset,
                                                  Pango2LogAttr  *attrs,
                                                  int             attrs_len);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_break        (const char     *text,
                                                  int             length,
                                                  Pango2AttrList *attr_list,
                                                  int             offset,
                                                  Pango2LogAttr  *attrs,
                                                  int             attrs_len);

G_END_DECLS