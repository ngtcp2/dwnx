/*
 * dwnx
 *
 * Copyright (c) 2026 dwnx contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef DWNX_MACRO_H
#define DWNX_MACRO_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <stddef.h>

#include <dwnx/dwnx.h>

#define dwnx_struct_of(ptr, type, member)                                      \
  ((type *)(void *)((char *)(ptr) - offsetof(type, member)))

/*
 * dwnx_arraylen returns the number of elements in array |A|.
 */
#define dwnx_arraylen(A) (sizeof(A) / sizeof(A[0]))

/*
 * dwnx_strlen_lit returns the length of string literal |S|.  This
 * macro assumes |S| is NULL-terminated string literal.  It must not
 * be used with pointers.
 */
#define dwnx_strlen_lit(S) (sizeof(S) - 1)

#define dwnx_max_def(SUFFIX, T)                                                \
  static inline T dwnx_max_##SUFFIX(T a, T b) { return a < b ? b : a; }

dwnx_max_def(long_long_int, long long int)
dwnx_max_def(long_int, long int)
dwnx_max_def(int, int)
dwnx_max_def(short_int, short int)
dwnx_max_def(signed_char, signed char)
dwnx_max_def(char, char)
dwnx_max_def(unsigned_long_long_int, unsigned long long int)
dwnx_max_def(unsigned_long_int, unsigned long int)
dwnx_max_def(unsigned_int, unsigned int)
dwnx_max_def(unsigned_short_int, unsigned short int)
dwnx_max_def(unsigned_char, unsigned char)

#define dwnx_max(A, B)                                                         \
  _Generic((A),                                                                \
    long long int: dwnx_max_long_long_int,                                     \
    long int: dwnx_max_long_int,                                               \
    int: _Generic((B),                                                         \
      long long int: dwnx_max_long_long_int,                                   \
      long int: dwnx_max_long_int,                                             \
      int: dwnx_max_int,                                                       \
      short int: dwnx_max_short_int,                                           \
      signed char: dwnx_max_signed_char,                                       \
      char: dwnx_max_char,                                                     \
      unsigned long long int: dwnx_max_unsigned_long_long_int,                 \
      unsigned long int: dwnx_max_unsigned_long_int,                           \
      unsigned int: dwnx_max_unsigned_int,                                     \
      unsigned short int: dwnx_max_unsigned_short_int,                         \
      unsigned char: dwnx_max_unsigned_char),                                  \
    short int: dwnx_max_short_int,                                             \
    signed char: dwnx_max_signed_char,                                         \
    char: dwnx_max_char,                                                       \
    unsigned long long int: dwnx_max_unsigned_long_long_int,                   \
    unsigned long int: dwnx_max_unsigned_long_int,                             \
    unsigned int: dwnx_max_unsigned_int,                                       \
    unsigned short int: dwnx_max_unsigned_short_int,                           \
    unsigned char: dwnx_max_unsigned_char)((A), (B))

#define dwnx_min_def(SUFFIX, T)                                                \
  static inline T dwnx_min_##SUFFIX(T a, T b) { return a < b ? a : b; }

dwnx_min_def(long_long_int, long long int)
dwnx_min_def(long_int, long int)
dwnx_min_def(int, int)
dwnx_min_def(short_int, short int)
dwnx_min_def(signed_char, signed char)
dwnx_min_def(char, char)
dwnx_min_def(unsigned_long_long_int, unsigned long long int)
dwnx_min_def(unsigned_long_int, unsigned long int)
dwnx_min_def(unsigned_int, unsigned int)
dwnx_min_def(unsigned_short_int, unsigned short int)
dwnx_min_def(unsigned_char, unsigned char)

#define dwnx_min(A, B)                                                         \
  _Generic((A),                                                                \
    long long int: dwnx_min_long_long_int,                                     \
    long int: dwnx_min_long_int,                                               \
    int: _Generic((B),                                                         \
      long long int: dwnx_min_long_long_int,                                   \
      long int: dwnx_min_long_int,                                             \
      int: dwnx_min_int,                                                       \
      short int: dwnx_min_short_int,                                           \
      signed char: dwnx_min_signed_char,                                       \
      char: dwnx_min_char,                                                     \
      unsigned long long int: dwnx_min_unsigned_long_long_int,                 \
      unsigned long int: dwnx_min_unsigned_long_int,                           \
      unsigned int: dwnx_min_unsigned_int,                                     \
      unsigned short int: dwnx_min_unsigned_short_int,                         \
      unsigned char: dwnx_min_unsigned_char),                                  \
    short int: dwnx_min_short_int,                                             \
    signed char: dwnx_min_signed_char,                                         \
    char: dwnx_min_char,                                                       \
    unsigned long long int: dwnx_min_unsigned_long_long_int,                   \
    unsigned long int: dwnx_min_unsigned_long_int,                             \
    unsigned int: dwnx_min_unsigned_int,                                       \
    unsigned short int: dwnx_min_unsigned_short_int,                           \
    unsigned char: dwnx_min_unsigned_char)((A), (B))

#endif /* !defined(DWNX_MACRO_H) */
